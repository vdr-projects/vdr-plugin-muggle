##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2000, Rolf Brugger
#
##################################################

package gdserv;

use lib '/home/andi/muggle/bin';
use gdio;
use myhash;		# incremental hash calculation
use gddb;		# db modification routines
use gdparams;		# global variables, constants and parameters
use gdgentools;		# general tools
use gdupdate;
use gdsoundcard;

use DBI;
use strict;

use FileHandle;	    # to set auto flush on pipe
use File::Basename;


### Init constants
my $shrtFieldLen = 25;

my $pl_list = 0;	# play list
my $rp_list = 1;	# rip list
my $co_list = 2;	# compression list
my $cr_list = 3;	# cd recording list


# player ouput constants
my $audio0 = 0;
my $audio1 = 1;
my $speaker = 10;
my $monitor = 11;

# Player Types
my $soundcard0   = 0;   # main sound output of first soundcard
my $soundcard1   = 1;   # alternative sound output on second s9undcard (not supported) 
my $mp3streamer  = 20;  # mp3 streamer (barix exstreamer)

# Audio out channel
my $audchannel0 = 0;
my $audchannel1 = 1;

# Currently, there is only audio channel 0 supported, and it can be mapped to 
# either $soundcard0  or  $mp3streamer












### Global variables
my $dbhost;	# IP of database host (better use var in gdparams!?)
my $dbh;
my $playerid;
#my $playertype; replaced by audiochannel

my $rippipe_open=0;

my $random_result_table;	#gobal variable that holds random result list
my $random_result_type;		#gobal variable: what's in the table? 


BEGIN{
}

############################################################

# Initialization
sub init_server{
  my $logtarget;
  ($dbhost, $logtarget) = @_;

  ### Open database connection
  if($logtarget==2){ #stdout
    print ("### Open database connection\n");
  }

  #print "dbhost is: $dbhost\n";
  $dbh = DBI->connect("DBI:mysql:GiantDisc:".$dbhost, "music", undef)
    or die "unable to connect to GiantDisc db";


  if (gdupdate::db_check_update_132($dbh)){
    print "\n  Update to database structure version 1.32 required\n";
    print "  please run command 'gdupdatedb.pl'\n\n";
    exit;
  }

  ### fix records (bug in recording routines prior to version 0.92)
  #gdupdate::fix_leading_slash_bug($dbh);

  ### UPDATE DATABASE to new versions
#  gdupdate::db_update_094($dbh);
#  gdupdate::db_update_095($dbh);
#  gdupdate::db_update_096($dbh);
#  gdupdate::db_update_097($dbh);
#  gdupdate::db_update_111($dbh);
#  gdupdate::db_update_112($dbh);
#  gdupdate::db_update_114($dbh);
#  gdupdate::db_update_131($dbh);
}


sub init_player{
# Checks, if a player record with the same ipaddr and uichannel exists.
# If it doesn't exist, a new record is created.
# It returns the new generated player-ID
#
# 
#
  my ($ipaddr, $uichannel, $logtarget, $cdripper, 
      $mp3encoder, $cdromdev, $cdrwdev) = @_;
  $playerid = 0; #default
  my $sth = $dbh->prepare( 
  	 "SELECT * FROM player "
  	."WHERE ipaddr='$ipaddr' AND uichannel='$uichannel'" );
  $sth->execute;

  my $row;
  if($row = $sth->fetchrow_hashref){
    # matching player record found
    $playerid = $row->{id};
  }
  else{
    # get highest id in db
    my $sth2 = $dbh->prepare( 
    	 "SELECT * FROM player ORDER BY id DESC LIMIT 1" );
    $sth2->execute;
    if($row = $sth2->fetchrow_hashref){
      # matching player record found
      $playerid = $row->{id}+1;
    }
    #else playerid has default value 0
    $sth2->finish;
    
    ### Create player record
    my $retval = $dbh->do("INSERT INTO player SET "
    	."ipaddr    ='$ipaddr', "
    	."uichannel ='$uichannel', "
    	."id        = $playerid, "
    	."logtarget = $logtarget, "
    	."cdripper  ='$cdripper', "
    	."mp3encoder='$mp3encoder', "
    	."cdromdev  ='$cdromdev', "
    	."cdrwdev   ='$cdrwdev'");
  }
  $sth->finish;

  ### delete all tracklists (riplist, burnlist and comprlist) of this player
  $dbh->do("DELETE FROM tracklistitem WHERE playerid=$playerid AND listtype=$rp_list"); #riplist
  $dbh->do("DELETE FROM tracklistitem WHERE playerid=$playerid AND listtype=$co_list"); #compressionlist
  $dbh->do("DELETE FROM tracklistitem WHERE playerid=$playerid AND listtype=$cr_list"); #cdrecordlist
  
  return $playerid;
}


sub init_playerstate{
# 
# global var: playerid

  my ($playerhost, $snddevice, $playerapp, $ptlogger, $playertype);
  my ($mp3playerparams, $oggplayerparams);
  ($playerhost, $playertype, $snddevice, $playerapp, $mp3playerparams, $oggplayerparams, $ptlogger) = @_;
  my $playerparams = join '\t', $mp3playerparams, $oggplayerparams;
  my $audiochannel = $audchannel0;
  gdgentools::pll_new_playstate($dbh, $playerid, $audiochannel,
  	$playertype, $snddevice, $playerapp, $playerparams, $ptlogger);

  ### create message queue for this player-id/audio-channel
  gdgentools::plmsg_newqueue($playerid, $audiochannel);
  
  ### init soundcard driver
  gdsoundcard::sndc_init($playertype, $playerhost, $audiochannel);
}



sub open_rippipe{
  ### Open recording pipe
  open(RIPPIPE, "| gdripcd.pl $dbhost $playerid $audchannel0")
    or die "can't fork RIP-PIPE\n";
  autoflush RIPPIPE 1;
  $rippipe_open=1;
}

sub close_rippipe{
  ### close recording pipe
  close(RIPPIPE);
  $rippipe_open=0;
}

############################################################
###                    TOOL ROUTINES                     ###
############################################################

############################################################

sub truncstr{
### truncstr (string, length, noellipsis)
  # If the string is longer than 'length' characters, it is truncated and 
  # '..' is appended => max length is 'length'+2
  if (length($_[0]) > $_[1]){
    $_[0] = substr ($_[0], 0, $_[1]);
    if(!$_[2]){ # add ellipsis
      $_[0] .= "..";
    }
  }
}


############################################################
###                 DB CREATION & UPDATE                 ###
############################################################

############################################################
### Creates/updates an album record 
sub replace_album_record{ 
  my ($artist,$title,$cddbid) = @_;

  my $base = gdparams::gdbase();

  if(length($artist)==0){$artist="-";};
  if(length($title)==0) {$title="-";};
  if(length($cddbid)==0){print("ERROR: no CDDB-ID defined!\n");};
  my $sqlcmd = 
           "REPLACE INTO album "
          ."(artist,title,cddbid,modified) "
          ."VALUES "
          ."(".$dbh->quote($artist)
          .",".$dbh->quote($title)
          .",'$cddbid', CURDATE()) ";

  #print("$sqlcmd \n");
  $dbh->do($sqlcmd);
}


############################################################
### 
sub record_cd_track{ 
  my ($artist,$title,$genre1,$genre2,$year,
      $lang,$type,$rating,$length,$source,$sourceid,
      $tracknb,$audiofile,$condition,$voladjust,      #$lengthfrm,$startfrm,$bpm,
      $created,$id,$bitrate
      ) = @_;

  my $base = gdparams::gdbase();

  ### Get an audio directory with enough space left (1GB)
  my $ripdir=gdgentools::get_ripdirectory($gdparams::minfreehdspace); #1000 MBytes should be available

  my $trid;
  ### create a new track record
  if($ripdir ne ""){
    my ($audiofmt, $audioparam) = gdgentools::bitrate_str_to_format_param($bitrate);  # split "mp3 128"
    $trid = gddb::insert_track_record($dbh,$artist,$title,$genre1,$genre2,$year,
               $lang,$type,$rating,$length,$source,$sourceid,
               $tracknb,"tr0x".$sourceid."-".$tracknb.".".$audiofmt,
               $condition,$voladjust,0,0,0,       #$lengthfrm,$startfrm,$bpm,
               $bitrate,
               $created,$id);

    unlink glob("$base/??/tr0x".$sourceid."-".$tracknb.".".$audiofmt); # delete old audio file
    ### send rip command to pipe
    if(!$rippipe_open){open_rippipe();}
    print ("printing to RIPPIPE: tr=$tracknb id=$trid "
	      ."br=$bitrate len=$length "
              ."dir=$base/$ripdir "
	      ."file=tr0x$sourceid art=$artist tit=$title\n");
    gdgentools::tracklist_append_list($dbh, $playerid, $rp_list, $trid);
    ### action is "rip"
    print (RIPPIPE "rip\t$tracknb\t$trid\t$bitrate\t$length\t"
              ."$base/$ripdir\ttr0x$sourceid\t$artist\t$title\n");
  }
  else{
    print("Not enough space left on disc \n");
  }
}


############################################################
### 
sub import_audio_track{ 
# creates a new record and moves the specified audio-file to a
# music directory which has enough space left

  my ($artist,$title,$genre1,$genre2,$year,
      $lang,$type,$rating,$length,$source,$sourceid,
      $tracknb,$audiofile,$condition,$voladjust,#$lengthfrm,$startfrm,$bpm,
      $created,$id,$bitrate
      ) = @_;

  my $base = gdparams::gdbase();

  ### Get an audio directory with enough space left (1GB)
  my $ripdir=gdgentools::get_ripdirectory($gdparams::minfreehdspace);

  my $trid;
  ### create a new track record
  if($ripdir ne ""){
    ### put audio-file in music directory

    my $origaudiofile = readlink "$base/inbox/$audiofile";

    if(!$rippipe_open){open_rippipe();}

    ### action is "move"
    #format: "move, sourcefile, targetfile, linkname
    print ("printing to RIPPIPE: move\t$origaudiofile\t$base/$ripdir/$audiofile\t$base/inbox/$audiofile\n");
    print (RIPPIPE "move\t$origaudiofile\t$base/$ripdir/$audiofile\t$base/inbox/$audiofile\n");

    ### create track record
    $trid = gddb::insert_track_record($dbh,$artist,$title,$genre1,$genre2,$year,
               $lang,$type,$rating,$length,$source,$sourceid,
               $tracknb,$audiofile,$condition,$voladjust,
               0,0,0, #$lengthfrm,$startfrm,$bpm,
               $bitrate,
               $created,$id);
    
  }
  else{
    print("Not enough space left on disc \n");
  }
}



############################################################
### 
# these variables are st by the routine, that sends track details to the
# client. The value in there gives a hint, if the frequencies should be recalculated
my ($trk_last_id, $trk_last_lang, $trk_last_genre1, $trk_last_genre2); 

sub update_track_record{ 
  my ($artist,$title,$genre1,$genre2,$year,
      $lang,$type,$rating,$length,$source,$sourceid,
      $tracknb,$audiofile,$condition,$voladjust,#$lengthfrm,$startfrm,$bpm,
      $created,$id,$bitrate
      ) = @_;

  my ($trid, $audiofpath);
  
  ### always recalculate bitrate and track length
  $audiofpath = gdgentools::get_full_audiofile_path($audiofile);
  if(gdgentools::is_mp3stream($audiofile) || (length($audiofpath)>0)){
    if(length($audiofpath)>0){ ## real mp3 file exists
      $bitrate = gdgentools::get_bitrate_str("$audiofpath");
      $length  = gdgentools::audiofile_lengthsec("$audiofpath");
    }
    else{
      $length = 0;
    }
    
    #print("UPDATING: $artist,$title,$genre1,$genre2,$year,"
    #             ."$lang,$type,$rating,$length,$source,$sourceid,"
    #             ."$tracknb,$audiofile,$condition,$voladjust,$created,$id\n");
    $trid = gddb::insert_track_record($dbh,$artist,$title,$genre1,$genre2,$year,
                 $lang,$type,$rating,$length,$source,$sourceid,
                 $tracknb,$audiofile,$condition,$voladjust,
                 0,0,0,#$lengthfrm,$startfrm,$bpm,
                 $bitrate,
                 $created,$id);
                 
    if (   $id != $trk_last_id
        || $lang   ne $trk_last_lang){   ### language changed?
      system("killall -q gdtablefreq.pl");
      system("nice gdtablefreq.pl --dbhost $dbhost --trklanguage&");
#      system("nice gdtablefreq.pl --dbhost $dbhost --trklanguage --verbose&");
    }
    if (   $id != $trk_last_id
        || $genre1 ne $trk_last_genre1
        || $genre2 ne $trk_last_genre2){ ### genre changed?
      # recalculate genre frequencies
      system("killall -q gdtablefreq.pl");
      if (length($sourceid)>=8){
        system("nice gdtablefreq.pl --dbhost $dbhost --trkgenre --albgenre $sourceid&");
#        system("nice gdtablefreq.pl --dbhost $dbhost --trkgenre --albgenre $sourceid --verbose&");
      }
      else{
        system("nice gdtablefreq.pl --dbhost $dbhost --trkgenre&");
#        system("nice gdtablefreq.pl --dbhost $dbhost --trkgenre --verbose&");
      }
    }
  }
  else{
    print("Warning: can't update record (id=$id) because the associated mp3file does not exist, and no mp3 streaming url is specified\n");
  }
}



sub recalc_table_frequencies{
  system("killall -q gdtablefreq.pl");
  system("nice gdtablefreq.pl --dbhost $dbhost --trkgenre --albgenre&");
}

############################################################
###                        QUERIES                       ###
############################################################


############################################################
###
sub do_tr_query{ # returns a query handle
  
  my $where = gddb::track_where_clause(@_);
  my $order = gddb::track_order_clause(@_);

  #print("WHERE clause: $where      $order\n");
  my $sth = $dbh->prepare( "SELECT * FROM tracks WHERE $where $order" );

  my $rv = $sth->execute;
  print("$rv records found\n");
  return $sth;
}

############################################################
###
sub do_album_query{ # does an album query and returns a query handle
    ### 
# EXAMPLE:
# SELECT album.* FROM tracks JOIN album 
# WHERE album.cddbid=tracks.sourceid AND tracks.genre1='mdh' 
#   AND album.title like "%de%" 
# GROUP BY album.cddbid;

#  my ($artist,$title,$genre1,$genre2,$yearfrom,$yearto,
#      $lang,$type,$rating) = @_;

  my $tmpcmd;

  my $where = gddb::album_where_clause(@_);
  my $order = gddb::album_order_clause(@_);

#print "alb-order: $order\n\n";
  if (length($order)<5){
    $order = "ORDER BY album.artist";
  }

  my $sth = $dbh->prepare(
#          " SELECT album.* FROM tracks JOIN album "
          " SELECT album.artist AS artist, album.title AS title, "
          ."album.cddbid AS cddbid, album.genre AS genre, tracks.year AS year "
          ."FROM tracks JOIN album "
          ."WHERE album.cddbid=tracks.sourceid AND $where "
          ."GROUP BY album.cddbid $order"
	);

  $sth->execute;
  return $sth;
}

############################################################
###
sub do_playlist_query{ # does a playlist query and returns a query handle
    ### 
  my ($author,$title,$genre1,$genre2,$yearfrom,$yearto,
      $lang,$type,$rating) = @_;

  my $where=" 1 ";	# true AND ...

  ### Artist
  $where .= gddb::field_where_clause("author",$author);

  ### Title
  $where .= gddb::field_where_clause("title",$title);

  my $sth = $dbh->prepare(
          'SELECT * FROM playlist '
          ."WHERE $where"
	);

  $sth->execute;
  return $sth;
}

############################################################
###
sub get_track{ # returns a query handle
  my ($trackid) = @_;

  my $sth = $dbh->prepare(
          'SELECT * FROM tracks '
          ."WHERE id = $trackid"
	);

  $sth->execute;
  return $sth;
}

############################################################
###
sub do_tracks_of_album_query{ # returns a query handle
  my ($albumid) = @_;
  my $sth = $dbh->prepare(
          'SELECT * FROM tracks '
          ."WHERE sourceid = \"$albumid\" ORDER BY tracknb"
	);

  $sth->execute;
  return $sth;
}

############################################################
###
sub do_tracks_of_playlist_query{ # returns a query handle
  my ($playlistid) = @_;
  my $sth = $dbh->prepare(
          "SELECT * FROM playlistitem "
          ."WHERE playlist = $playlistid ORDER BY tracknumber"
	);

  $sth->execute;
  return $sth;
}


############################################################
###                       LISTINGS                       ###
############################################################

############################################################
### send tracks
sub send_tracks_v1{ # send a record set to the serial line
                    # NEW VERSION
  my ($sth) = @_;
  my ($row);
  my $stop=0;
  while($row = $sth->fetchrow_hashref){
    $stop = send_track($row);
    last if ($stop);
  }
  if (!$stop){ 
    gdio::putline ("");}	# End of result
  $sth->finish;
}
sub send_track{
  my ($row)= @_;
  my ($stop);
    truncstr($row->{artist}, $shrtFieldLen);
    truncstr($row->{title}, $shrtFieldLen);
    print ("$row->{id}\t");
    print ("$row->{artist}\t");
    print ("$row->{title}\t");
    print ("$row->{length}\t");
    print ("$row->{genre1}\t");
   #print ("$row->{genre2}\t");
    print ("$row->{year}\t");
    print ("$row->{type}\t");
    print ("$row->{rating}\n");
    $stop = gdio::putline("$row->{id}\t$row->{artist}\t$row->{title}\t"
		 ."$row->{length}\t$row->{genre1}\t"
		 ."$row->{year}\t$row->{type}\t$row->{rating}");
   return $stop
}
#sub send_tracks_v0{ # send a record set to the serial line
#                    # OLD VERSION: will be obsolete when all 
#                    # routines are ported to 'send_tracks'
#  my ($sth) = @_;
#  my ($row);
#  my $stop=0;
#  while($row = $sth->fetchrow_hashref){
#    truncstr($row->{artist}, $shrtFieldLen);
#    truncstr($row->{title}, $shrtFieldLen);
#    print ("$row->{id}\t");
#    print ("$row->{artist}\t");
#    print ("$row->{title}\t");
#    print ("$row->{length}\n");
#    $stop = gdio::putline("$row->{id}\t$row->{artist}\t$row->{title}\t"
#		 ."$row->{length}\t$row->{tracknb}");
#    last if ($stop);
#  }
#  if (!$stop){ 
#    gdio::putline ("");}	# End of result
#  $sth->finish;
#}

############################################################
### Query tracks & List
sub query_tracks_list{ #query tracks records and list them
  my $sth = do_tr_query(1, @_);   # perform database query
  send_tracks_v1($sth);
}

sub query_streams_list{ #query tracks records and list them
  my $sth = do_tr_query(2, @_);   # perform database query
  send_tracks_v1($sth);
}

############################################################
### Generic Query tracks & List
sub generic_track_query{ #query tracks records and list them
  my ($where) = @_;
  #print("WHERE clause: $where\n");
  my $sth = $dbh->prepare( "SELECT * FROM tracks $where" );
  my $rv = $sth->execute;
  print("$rv records found\n");
  send_tracks_v1($sth);
}

### Query a single tracks & List
#sub query_single_track{ #query one trackfor a given id
#  my ($trackid) = @_;
#  my $sth = $dbh->prepare( "SELECT * FROM tracks WHERE id='$trackid'" );
#  $sth->execute;
#  send_tracks_v0($sth);
#}


############################################################
### send artists
sub send_artist_line_v1{ # send a single record to the serial line
  my ($artist) = @_;
  my $stop=0;
  print ("\t");	# send an empty ID
  truncstr($artist, 10+$shrtFieldLen, 1); # dont send ellipsis
  print ("$artist\n");
  $stop = gdio::putline("\t$artist");
  return $stop;
}

#sub send_artist_line_v0{ # send a single record to the serial line
#  my ($artist) = @_;
#  my $stop=0;
#  print ("ESCn\t");	# ESC n: name (=artist)
#  print ("0\t");	# send an empty ID to be compatible with album record format
#  truncstr($artist, 10+$shrtFieldLen, 1); # dont send ellipsis
#  print ("$artist\t");
#  print (" \n");	# send an empty title to be compatible with album record format
#  $stop = gdio::putline("\en\t0\t$artist\t ");
#  return $stop;
#}

############################################################
### Query artist & List
sub query_artists_list{ #query track records and list their distinct artists
  #my ($artist,$title,$genre1,$genre2,$yearfrom,$yearto,
  #    $lang,$type,$rating) = @_;

  ### Prepare Query
  my $where = gddb::track_where_clause(1, @_);

  #print("WHERE clause: $where\n");

  generic_artists_query(
  	"SELECT DISTINCT artist FROM tracks WHERE $where ORDER BY artist" );
}


sub generic_artists_query{ #query track records and list their distinct artists
  ### Prepare Query
  my ($querystr) = @_;
  my $sth = $dbh->prepare( $querystr );
  my $rv = $sth->execute;
  #print("$rv artists found\n");
  ### Send results
  my $row;
  my $stop=0;
  while($row = $sth->fetchrow_hashref){
    $stop = send_artist_line_v1($row->{artist});
    last if ($stop);
  }
  if (!$stop){ 
    gdio::putline ("");}	# End of result
  $sth->finish;
}


############################################################
### Query albums 


#sub query_single_album{ 
    # query one albums with its tracks and list them
    # the album is specified by the 'albumid'.
#  my ($albumid) = @_;
#  my $albsth;
#  $albsth = $dbh->prepare( 
#          " SELECT * FROM album WHERE cddbid='$albumid'");
#  $albsth->execute;
#  send_albums_v0(1, $albsth);	# 1: with tracks
#}


sub generic_album_query{ 
    # query one albums with its tracks and list them
    # the album is specified by the 'albumid'.
  my ($querystr) = @_;
  my $albsth;
  my $albsth = $dbh->prepare( $querystr );
  $albsth->execute;
  send_albums_v1($albsth);
}


############################################################
### Query albums & list
sub query_albums{ 
    # query albums and list them
  my $albsth = do_album_query(@_);   # perform database query
  send_albums_v1($albsth);
}


############################################################
### Query albums at random
sub query_random_albums{ 
    # prepares a ramdom query operation
    # 1) performs an album query 
    # 2) selects album-ids and store them in a gobally accessible array
    # 3) returns the number of albums found
  my ($artist,$title,$genre1,$genre2,$yearfrom,$yearto,
      $lang,$type,$rating) = @_;
  my $nbRecords;

  my $where = gddb::album_where_clause(@_);
#  my $where=" 1 ";	# true AND ...
  ### Album: Artist
#  $where .= gddb::field_where_clause("album.artist",$artist);
  ### Album: Title
#  $where .= gddb::field_where_clause("album.title",$title);
  ### Track: genre, etc ...
#  $where.= gddb::attrib_where_clause($genre1,$genre2,$yearfrom,$yearto,$lang,$type,$rating);

  my $sth = $dbh->prepare(
          " SELECT album.cddbid,(ASCII(album.cddbid)*0)+RAND() AS randnum "
          ."FROM tracks JOIN album "
          ."WHERE album.cddbid=tracks.sourceid AND $where "
          ."GROUP BY album.cddbid ORDER BY randnum"
	);			# ORDER BY RAND() does not work in version 3.22
  $nbRecords = $sth->execute;

  ### store result in global array
  $random_result_table = $sth->fetchall_arrayref;
  $random_result_type = "alb";
  # my $i;  for $i( 0 .. $#{$random_result_table}){print ($random_result_table->[$i][0]."-".$random_result_table->[$i][1].",");}  print ("\n");
  $sth->finish;

  ### Send number of records
  print ("random album query: $nbRecords albums found\n");
  gdio::putline ("$nbRecords\n");
  gdio::putline ("");	# End of result
}


############################################################
### Query tracks at random
sub query_random_tracks{ 
    # prepares a ramdom query operation
    # 1) performs a track query 
    # 2) selects track-ids and store them in a gobally accessible array
    # 3) returns the number of tracks found
  my $nbRecords;

  my $where = gddb::track_where_clause(@_);
  my $sth = $dbh->prepare(
          " SELECT id,(id*0)+RAND() AS randnum FROM tracks "
          ."WHERE $where ORDER BY randnum"
	); # ORDER BY RAND() does not work in version 3.22
  $nbRecords = $sth->execute;

  ### store result in global array
  $random_result_table = $sth->fetchall_arrayref;
  $random_result_type = "trk";
# my $i;  for $i( 0 .. $#{$random_result_table}){print ($random_result_table->[$i][0]."-".$random_result_table->[$i][1].",");}  print ("\n");
  $sth->finish;

  ### Send number of records
  print ("random album query: $nbRecords tracks found\n");
  gdio::putline ("$nbRecords\n");
  gdio::putline ("");	# End of result
}


############################################################
### Query artists at random
sub query_random_artists{ 
    # prepares a ramdom query operation
    # 1) performs a track query 
    # 2) selects track-ids and store them in a gobally accessible array
    # 3) returns the number of tracks found
  my ($artist,$title,$genre1,$genre2,$yearfrom,$yearto,
      $lang,$type,$rating) = @_;
  my $nbRecords;

  my $where = gddb::track_where_clause(1, @_);
  my $sth = $dbh->prepare(
          " SELECT DISTINCT artist FROM tracks WHERE $where"
	); # can't "ORDER BY randnum" here!
  $nbRecords = $sth->execute;

  ### store result in global array
  $random_result_table = $sth->fetchall_arrayref;
  $random_result_type = "art";
# my $i;  for $i( 0 .. $#{$random_result_table}){print ($random_result_table->[$i][0]."-".$random_result_table->[$i][1].",");}  print ("\n");
  $sth->finish;
  ### randomize it - because SQL query can't do it
  shuffle_random_table();

  ### Send number of records
  print ("random artist query: $nbRecords artists found\n");
  gdio::putline ("$nbRecords\n");
  gdio::putline ("");	# End of result
}


############################################################
### send next slice of random result table to the palm client
sub collect_ids{
  my ($position,$nbRecords,$idIdent) = @_;
  my ($whereclause, $i);
  $whereclause = "0";
  # if $position < $#{$random_result_table}
  for $i( $position .. ($position+$nbRecords-1)){
    #print ($random_result_table->[$i][0]."\n");
    $whereclause .= " OR $idIdent='".$random_result_table->[$i][0]."'";
  }
  return $whereclause;
}

sub shuffle_random_table{
  my ($i, $j, $element);  
  for $i( 0 .. $#{$random_result_table}){
    $j = int rand ($#{$random_result_table});
#    print ("exchange $i and $j\n");
    $element                      = $random_result_table->[$i][0];
    $random_result_table->[$i][0] = $random_result_table->[$j][0];
    $random_result_table->[$j][0] = $element;
  }
}

sub get_random_result{ 
    # prepares a ramdom query operation
    # 1) performs an album query 
    # 2) selects album-ids and store them in a gobally accessibe array
    # 3) randomizes the order of the album-ids
    # 4) returns the number of albums found
  my ($position,$nbRecords) = @_;

  my ($whereclause, $sth);
  if ($random_result_type eq "alb"){
    $whereclause = collect_ids ($position,$nbRecords, "tracks.sourceid=album.cddbid AND album.cddbid");
    ### Perform the query
    #print("SELECT album.artist AS artist, album.title AS title, "
    #      ."album.cddbid AS cddbid, album.genre AS genre, tracks.year AS year "
    #      ."FROM tracks JOIN album "
    #      ."WHERE $whereclause"
    #      ."GROUP BY album.cddbid\n");

    $sth = $dbh->prepare(" SELECT album.artist AS artist, album.title AS title, "
          ."album.cddbid AS cddbid, album.genre AS genre, tracks.year AS year "
          ."FROM tracks JOIN album "
          ."WHERE $whereclause"
          ."GROUP BY album.cddbid");
    $sth->execute;
    send_albums_v1($sth);
    $sth->finish;
  }
  elsif ($random_result_type eq "pll"){
    $whereclause = collect_ids ($position,$nbRecords, "id");
    ### Perform the query
    print("SELECT * FROM playlist WHERE $whereclause\n");
    $sth = $dbh->prepare("SELECT * FROM playlist WHERE $whereclause");
    $sth->execute;
    my $prow;
    while($prow = $sth->fetchrow_hashref){
      print("$prow->{id}\t$prow->{author}\t$prow->{title}\n");
      gdio::putline("$prow->{id}\t$prow->{author}\t$prow->{title}");
    }
    gdio::putline ("");	# End of result
    $sth->finish;
  }
  elsif ($random_result_type eq "trk"){
    $whereclause = collect_ids ($position,$nbRecords, "id");
    ### Perform the query
    print("SELECT * FROM tracks WHERE $whereclause\n");
    $sth = $dbh->prepare("SELECT * FROM tracks WHERE $whereclause");
    $sth->execute;
    send_tracks_v1($sth);
    $sth->finish;
  }
  elsif ($random_result_type eq "art"){
    ### no additional query necessary with artists
    my $i;
    for $i( $position .. ($position+$nbRecords-1)){
      truncstr($random_result_table->[$i][0], 10+$shrtFieldLen);
      #print ($random_result_table->[$i][0]."\n");
      send_artist_line_v1($random_result_table->[$i][0]);
    }
    gdio::putline ("");	# End of result
  }
  else{
    print ("Error: random_result_type '$random_result_type' not properly initialized");
  }
}


############################################################
### Query albums with or without tracks & list
#sub send_albums_v0{ 
#  # sends the albums (specified by the record set handle '$albsth')
#  # if $withTracks is true, the according tracks are sent too
#
#  my ($withTracks, $albsth) = @_;
#  my ($arow, $trow); 		     # album and track row
#  my $trksth;
#  my $stop=0;
#
#  while($arow = $albsth->fetchrow_hashref){
#    truncstr($arow->{artist}, $shrtFieldLen);
#    truncstr($arow->{title}, $shrtFieldLen);
#    print ("ESCa\t");
#    print ("$arow->{cddbid}\t");
#    print ("$arow->{artist}\t");
#    print ("$arow->{title}\n");
#    # print album (preceded by ESC a)
#    $stop = gdio::putline("\ea\t$arow->{cddbid}\t$arow->{artist}\t$arow->{title}");
#
#    if ($withTracks){
#      ### get tracks of the album
#      my $trksth = do_tracks_of_album_query($arow->{cddbid});
#      while($trow = $trksth->fetchrow_hashref){
#        truncstr($trow->{artist}, $shrtFieldLen);
#        truncstr($trow->{title}, $shrtFieldLen);
#        print ("$trow->{id}\t");
#        print ("$trow->{artist}\t");
#        print ("$trow->{title}\t");
#        print ("$trow->{length}\n");
#        # print track
#        $stop = gdio::putline("$trow->{id}\t$trow->{artist}\t$trow->{title}\t"
#  			   ."$trow->{length}\t$trow->{tracknb}");
#        last if ($stop);
#      }
#    }
#    last if ($stop);
#  }
#  if (!$stop){ 
#    gdio::putline ("");}	# End of result
#  $albsth->finish;
#}

sub send_albums_v1{ 
  # sends the albums (specified by the record set handle '$albsth')
  # if $withTracks is true, the according tracks are sent too

  my ($albsth) = @_;
  my ($arow, $trow); 		     # album and track row
  my $trksth;
  my $stop=0;

  while($arow = $albsth->fetchrow_hashref){
    truncstr($arow->{artist}, $shrtFieldLen);
    truncstr($arow->{title}, $shrtFieldLen);
    print ("$arow->{cddbid}\t");
    print ("$arow->{artist}\t");
    print ("$arow->{title}\t");
    print ("\t$arow->{genre}\t$arow->{year}\n");      # length,genre,year
    # print album 
    $stop = gdio::putline("$arow->{cddbid}\t$arow->{artist}\t$arow->{title}\t\t$arow->{genre}\t$arow->{year}");
    last if ($stop);
  }
  if (!$stop){ 
    gdio::putline ("");}	# End of result
  $albsth->finish;
}

############################################################
### Query playlists & list
sub query_saved_playlists_list{ 
    # query playlists and list them
    #
    # List format: id, author, title

  my $pllsth = do_playlist_query(@_);   # perform database query
  my ($prow, $trow); 		        # playlist and track row
  my ($trksth, $trow, $irow, $plisth);
  my $stop=0;

  while($prow = $pllsth->fetchrow_hashref){
    truncstr($prow->{artist}, $shrtFieldLen);
    truncstr($prow->{title}, $shrtFieldLen);
    print ("$prow->{id}\t");
    print ("$prow->{author}\t");
    print ("$prow->{title}\n");

    $stop = gdio::putline("$prow->{id}\t$prow->{author}\t$prow->{title}");
    last if ($stop);
  }
  if (!$stop){ 
    gdio::putline ("");}	# End of result
  $pllsth->finish;
}



############################################################
### Query playlists at random
sub query_random_playlists{ 
    # prepares a ramdom query operation
    # 1) performs an playlist query 
    # 2) selects playlist-ids and store them in a gobally accessible array
    # 3) returns the number of playlists found
  my ($artist,$title,$genre1,$genre2,$yearfrom,$yearto,
      $lang,$type,$rating) = @_;
  my $nbRecords;

  my $where=" 1 ";	# true AND ...

  ### Artist
  $where .= gddb::field_where_clause("author",$artist);
  ### Title
  $where .= gddb::field_where_clause("title",$title);

  my $sth = $dbh->prepare(
          'SELECT id FROM playlist '
          ."WHERE $where ORDER BY RAND()"
	);

  $nbRecords = $sth->execute;

  ### store result in global array
  $random_result_table = $sth->fetchall_arrayref;
  $random_result_type = "pll";
  # my $i;  for $i( 0 .. $#{$random_result_table}){print ($random_result_table->[$i][0]."-".$random_result_table->[$i][1].",");}  print ("\n");
  $sth->finish;

  ### Send number of records
  print ("random album query: $nbRecords albums found\n");
  gdio::putline ("$nbRecords\n");
  gdio::putline ("");	# End of result
}


sub tracks_of_saved_playlist{ 
    # lists tracks of a saved playlist

  my ($plid) = @_;

  my ($trksth, $irow, $plisth, $row);
  #my $stop=0;


  ### get tracks of the playlist
  $plisth = do_tracks_of_playlist_query($plid);
  while($irow = $plisth->fetchrow_hashref){
    $trksth = $dbh->prepare("SELECT * FROM tracks WHERE id=".$irow->{trackid});
    $trksth->execute;
    if($row = $trksth->fetchrow_hashref){
      send_track($row);
    }
    $trksth->finish;
  }
  $plisth->finish;
  gdio::putline ("");	# End of result
}



############################################################
### Query tracks and send result to Palm client
sub simple_track_query{
  my ($query_cmd) = @_;
  my ($sth,$row);
  my $stop=0;

  my $sth = $dbh->prepare($query_cmd);
  my $rv = $sth->execute;
  print("$rv records found\n");

  while($row = $sth->fetchrow_hashref){
    truncstr($row->{artist}, $shrtFieldLen);
    truncstr($row->{title}, $shrtFieldLen);
    print ("$row->{id} $row->{artist} $row->{title}\t $row->{length}\n");
    $stop = gdio::putline("$row->{id}\t$row->{artist}\t$row->{title}\t"
		 ."$row->{length}\t$row->{tracknb}");
    last if ($stop);
  }
  if (!$stop){ 
    gdio::putline ("");}	# End of result
  $sth->finish;
}



############################################################
###               Play + Playlist commands               ###
############################################################


############################################################
### New Playlist commands

sub pl_play{   gdgentools::pl_play   ($dbh, $playerid, $audchannel0);}
sub pl_play_at{gdgentools::pl_play_at($dbh, $playerid, $audchannel0, @_);}
sub pl_stop{   gdgentools::pl_stop   ($dbh, $playerid, $audchannel0);}
sub pl_pause{  gdgentools::pl_pause  ($dbh, $playerid, $audchannel0);}
sub pl_rw{     gdgentools::pl_rw     ($dbh, $playerid, $audchannel0);}
sub pl_ff{     gdgentools::pl_ff     ($dbh, $playerid, $audchannel0);}
sub pl_goto{   gdgentools::pl_goto   ($dbh, $playerid, $audchannel0, @_);}
sub pl_prev{   gdgentools::pl_prev   ($dbh, $playerid, $audchannel0);}
sub pl_next{   gdgentools::pl_next   ($dbh, $playerid, $audchannel0);}



sub pl_append_tracks{
# appends tracks to the playlist file
# Parameters: list of track id's
  gdgentools::tracklist_append_list($dbh, $playerid, $pl_list, @_);
}

sub pl_new_list{
# Sets a new playlist. First, playing is stopped, then the current playlist
# is emptied, the new playlist is written and the current track index is set
# to zero.
# An example, where this routine is used is the shuffle function (Palm 
# randomizes playlist order and sends new playlist).
# Parameters: list of track id's
  #my $tracklist = @_;
  pl_empty();
  pl_append_tracks(@_);
}

sub pl_empty{
# empties (deletes) the playlist. The playstate is also reset
  my $base = gdparams::gdbase();
  # stop playing
  gdgentools::stop_play_processes($dbh, $playerid, $audchannel0);
  # reset list+state
  gdgentools::tracklist_delete($dbh, $playerid, 0);
  gdgentools::pll_write_playstate($dbh, $playerid, $audchannel0, 0, "st");
  gdgentools::pll_write_playtime($dbh, $playerid, $audchannel0, 0, 0);
  gdgentools::pll_write_shuffleparameters($dbh, $playerid, $audchannel0, "", "");
}


sub pl_remove_one{
# Removes the specified playlist item (index) from the playlist
# If the index corresponds to the currently played track, playing is
# stopped first.
# It the index is lower than the the index of the currently played track,
# then nothing is done.
# Parameter: index of the item to be removed

  my ($delitem) = @_;
  my ($trackind, $state, $frame, $shufflestat) = gdgentools::pll_get_playstate($dbh, $playerid, $audchannel0);
  if ($delitem >= $trackind){
    if (($delitem == $trackind) && $state ne "st" && $state ne "in"){
      gdgentools::stop_play_processes($dbh, $playerid, $audchannel0);
      gdgentools::pll_write_playstate($dbh, $playerid, $audchannel0, $trackind, "st");
    }
    
    ### delete it
    gdgentools::tracklist_del_item($dbh, $playerid, 0, $delitem);
  }
}



sub pl_reorder_one{
# Reorders the playlist.
# The specified playlist item (src) is moved to a new location (dest)
# src and dest must be higher than the currently played track. They
# therefore don't interfere with the track playing process.

  my ($srcpos, $destpos) = @_;
  ### move it
  gdgentools::tracklist_reorder_item($dbh, $playerid, 0, $srcpos, $destpos);
}


############################################################
### Shuffle play (query only)
sub pl_calc_shufflestats{ # query tracks  and send back statistics
  my $where = gddb::track_where_clause(1, @_);
  my $sth = $dbh->prepare( 
       "SELECT SUM(length) AS playlen, COUNT(*) AS nbtracks "
      ."FROM tracks WHERE $where" );
  $sth->execute;
  my ($row, $nbtracks, $playlen);
  if($row = $sth->fetchrow_hashref){
    $nbtracks = $row->{nbtracks};
    $playlen  = $row->{playlen};
  }
  else{
    print("Shuffle play: query error!\n");
  }
  $sth->finish;
  return ($nbtracks, $playlen);
}



sub pl_check_shuffleplay{ # query tracks  and send back statistics
  my ($nbtracks, $playlen) = pl_calc_shufflestats(@_);

  if($nbtracks > 10){
    gdio::putline("$nbtracks tracks found. Total play length is "
                 . gdgentools::seconds_to_hm($playlen)
                 .". Play them in random order?");
  }
  else{
    gdio::putline("Sorry! $nbtracks tracks are not enough to be played in shuffle mode.");
  }
  gdio::putline ("");
}



sub pl_start_shuffleplay{ 
  pl_empty();	# stop playing, empty playlist and clear playstate
  my ($nbtracks, $playlen) = pl_calc_shufflestats(@_);
  my $shuffleparam = join ("\t",@_);   ### Attention: does not join trailing empty parameters!
  if (length($shuffleparam)==0){$shuffleparam ="\t\t";}  ### join does not work on empty parameters -> cheat an empty parameter string!
  gdgentools::pll_write_shuffleparameters($dbh, $playerid, $audchannel0, 
    $shuffleparam, "$nbtracks Tr  [".gdgentools::seconds_to_hm($playlen)."]");
  gdgentools::pl_play($dbh, $playerid, $audchannel0);	# start playing - the player script knows how to handle the shuffle play parameters
}


############################################################


# send current playlist to the client (tracklist with ID, artist, title, length)
sub pl_reload_playlist{
  my @playlist = gdgentools::tracklist_get_all($dbh, $playerid, 0);
  my ($stop, $trackid, $sth, $trackrow, $row);

  while($trackid = shift @playlist){
    $sth = $dbh->prepare("SELECT * FROM tracks WHERE id=$trackid");
    $sth->execute;

    if($row = $sth->fetchrow_hashref){
      $stop = send_track($row);
#      truncstr($row->{artist}, $shrtFieldLen);
#      truncstr($row->{title}, $shrtFieldLen);
#      print ("$row->{id}\t$row->{artist}\t$row->{title}\t$row->{length}\n");
#      gdio::putline("$row->{id}\t$row->{artist}\t$row->{title}\t"
#  		 ."$row->{length}\t$row->{tracknb}");
      last if ($stop);
    }
    $sth->finish;
  }
  if (!$stop){ 
    gdio::putline ("");}	# End of result
}


# send current playstate to the client
sub pl_get_curr_playstate{
  # sends: pll-length, current-index, current-id, playtime, tracklength, state
  use integer;

print "get playstate for channel ".$gdgentools::audchannel1."\n";
  my ($trackind, $state, $frame, $shufflestat) = 
                         gdgentools::pll_get_playstate($dbh, $playerid, $audchannel0);
  my ($played, $total) = gdgentools::pll_get_playtime($dbh, $playerid, $audchannel0);
  my @playlist         = gdgentools::tracklist_get_all($dbh, $playerid, 0);
  my $playlistlen = @playlist;
  my $pltime = $played/gdgentools::frames_per_second();
  my $pllen  = $total/gdgentools::frames_per_second();

  #print ("Sending state: pl-len:$playlistlen ind:$trackind id:$playlist[$trackind] ptime:$pltime ptot:$pllen state:$state\n");
  gdio::putline ("$playlistlen\t$trackind\t$playlist[$trackind]\t$pltime\t$pllen\t$state\t$shufflestat");
  gdio::putline ("");	# End of result
}




################################################################
### Export Playlist:

sub pl_export_empty{
  my $base = gdparams::gdbase();

  unlink <$base/outbox/*>;  
}

sub pl_export_audiolinks{
# exports the current playlist as links to mp3-files to the outbox
  my $base = gdparams::gdbase();
  my ($trackid, $row, $tracknum, $trh, $trkrow, $linkname, $mp3fp);
  my ($audiofmt, $audioparam);

  ### run through playlist
  my $sth = $dbh->prepare( 
  	 "SELECT * FROM tracklistitem "
  	."WHERE playerid=$playerid AND listtype=$pl_list "
  	."ORDER BY tracknb " );
  my $nbrec = $sth->execute;
  while($row = $sth->fetchrow_hashref){
    $trackid = $row->{trackid};

    ### get track details
    $trh = $dbh->prepare("SELECT * FROM tracks WHERE id=$trackid");
    $trh->execute;
    if($trkrow = $trh->fetchrow_hashref){
      ####$trackid = $row->{trackid};
      ### create link
      $mp3fp = `ls $base/??/$trkrow->{mp3file}`;  # get full filename and path
      ($audiofmt, $audioparam) = gdgentools::bitrate_str_to_format_param($trkrow->{bitrate});  # split "mp3 128"
      chop($mp3fp);
      $linkname = gdgentools::ascii2filename(
      		gdgentools::iso2ascii($trkrow->{artist}." - ".$trkrow->{title}));
      #print("exporting link: "."$mp3fp --- $base/outbox/$linkname.mp3"."\n");
      symlink ("$mp3fp", "$base/outbox/$linkname.$audiofmt");
    }
    $trh->finish;

  }
  $sth->finish;
}



################################################################
### Upload Playlist to mp3 Player:

sub pl_upload_to_player_info{
print("EXTMP3 is:".$gdparms::extmp3player."\n");
  if (length($gdparms::extmp3player)>0){
    pl_export_empty();
    pl_export_audiolinks();
print("CMD: gdupload2player.pl --$gdparms::extmp3player --info\n");
    my $resline = `gdupload2player.pl --$gdparms::extmp3player --info`;
    gdio::putline ($resline);
  }
  gdio::putline ("");	# End of result
}

sub pl_upload_to_player{
  my ($addtrks) = @_;

  system( "gdupload2player.pl --$gdparms::extmp3player $addtrks&");
}




################################################################
### Saved Playlists:

sub pl_save_playlist{
# saves the playlist with the specified name
# Parameters: title, author, note, followed by list of track id's
  my $title  = shift(@_);
  my $author = shift(@_);
  my $note   = shift(@_);
  my ($playlistid, $trackid, $tracknum);

  my $nbtracks = @_;
  if (length($title)>0 && $nbtracks>0){
    ### check if playlist with that name already exists
    my $sth = $dbh->prepare("SELECT * FROM playlist WHERE title = ".$dbh->quote($title));
    my $count = $sth->execute;
    if ($count > 0){
      ### A playlist with this name exists already, get it's ID
      my $row = $sth->fetchrow_hashref;
      $playlistid = $row->{id};
      my $plh = $dbh->prepare(
         "SELECT * FROM playlistitem WHERE playlist = ".$playlistid
        ." ORDER BY tracknumber DESC LIMIT 1");
      $count = $plh->execute;
      if ($count>0){
        $row = $plh->fetchrow_hashref;
        $tracknum = $row->{tracknumber} +1;
      }
      else{
        $tracknum = 1;
      }
      $plh->finish;
      
    }
    else{
      ### A playlist with this name does not exist, create it
      my $sqlcmd = 
           "INSERT INTO playlist (title,author,note,created) "
          ."VALUES (".$dbh->quote($title).",".$dbh->quote($author)
          .",'$note',CURDATE()) ";
      $dbh->do($sqlcmd);
      $playlistid = $dbh->{'mysql_insertid'};
      $tracknum = 1;
    }
    $sth->finish;
    

    ### Append tracks to playlist $playlistid starting with number $tracknum
    while($trackid = shift(@_)){
      #print("appending track-ID $trackid as itemnb $tracknum\n");
      my $sqlcmd = 
           "INSERT INTO playlistitem (playlist,tracknumber,trackid) "
          ."VALUES ($playlistid,$tracknum,$trackid) ";
      $dbh->do($sqlcmd);
      $tracknum++;
    }
  }
  else{
    print("Can't save playlist: title or playlist are empty\n");
  }
}


sub pl_delete_playlist{
# deletes the playlist with the specified id
  my ($playlistid) = @_;
  my $sqlcmd;
 
  ### Delete all associated tracks
  $sqlcmd = ("DELETE FROM playlistitem WHERE playlist = ".$playlistid);
  $dbh->do($sqlcmd);

  ### Delete playlist
  $sqlcmd = ("DELETE FROM playlist WHERE id = ".$playlistid);
  $dbh->do($sqlcmd);
}



############################################################
### Play mp3 file commands

### directly play a track (in inbox, before trimming, etc)
sub cmd_direct_play_mp3_track{ 
#parameters: filename of mp3 file (without path)
#            startframe (optional)
#            nbframes   (optional)

  my ($audiofile, $startframe, $nbframes) = @_;
  my $base = gdparams::gdbase();

  ### kill early, for /dev/dsp to recover
  gdgentools::stop_play_processes($dbh, $playerid, $audchannel0);
  system("sleep 1s");  # sleep a second
  
  if($startframe > 0){
    $startframe = "-k ".$startframe;
  }
  else{
    $startframe = "-k 0";
  }
  if($nbframes > 0){
    $nbframes = "-n ".$nbframes;
  }
  else{
    $nbframes = "";
  }

  print("\nplaying inbox $audiofile $startframe $nbframes\n");
  # play it in inbox or in 00, 01 ...
  if (   gdgentools::audio_filetype($audiofile) eq "mp3"){
    system( "mpg123 $startframe $nbframes $base/[0-9i][0-9n]*/$audiofile&");
  }
  elsif(gdgentools::audio_filetype($audiofile) eq "ogg"){
    system( "ogg123 $base/[0-9i][0-9n]*/$audiofile&");
  }
}


### Command stop playing
sub cmd_stop_playing{ #
  gdgentools::kill_all_play_processes();
#  gdgentools::stop_play_processes($dbh, $playerid, $audchannel0);
  print("Playing stopped\n");
}

############################################################
### Play CD track commands

### Command stop playing cd
sub cmd_stop_playing_cd{ #
  system("cdplay stop");
}

### Command play CD track
sub cmd_play_cd_track{ #
  my ($tracknb) = @_;
  system("cdplay play $tracknb");
}


############################################################
###                       Listings                       ###
############################################################

############################################################
### List used genres
sub list_used_genres{ #get all used genres, count and list them

  my $sth = $dbh->prepare(
           'SELECT id, genre, freq '
          .'FROM genre '
          .'ORDER BY genre '
	);

  $sth->execute;
  my (@row, $stop);

  # "no genre" first
  print ("\t\t1\t0\t0\n");
  gdio::putline ("\t\t1\t0\t0");

  while(@row = $sth->fetchrow_array){
    print ("$row[0]\t$row[1]\t$row[2]\t0\t0\n");
    $stop = gdio::putline ("$row[0]\t$row[1]\t$row[2]\t0\t0");
    last if ($stop);
  }
  gdio::putline ("");	# End of result
  $sth->finish;
}

############################################################
### List used languages
sub list_used_languages{ #get all used languages, count and list them
  my $sth = $dbh->prepare(
           'SELECT id, language, freq '
          .'FROM language '
          .'ORDER BY freq DESC '
	);

  my $rv = $sth->execute;
  my (@row, $stop);

  # "no language" first
  print ("\t\t1\n");
  gdio::putline ("\t\t1");

  while(@row = $sth->fetchrow_array){
    print ("$row[0]\t$row[1]\t$row[2]\n");
    $stop = gdio::putline ("$row[0]\t$row[1]\t$row[2]");
    last if ($stop);
  }
  gdio::putline ("");	# End of result
  $sth->finish;
}

############################################################
### List music types
sub list_music_types{
  my $sth = $dbh->prepare('SELECT * FROM musictype ORDER BY id ');
  my $rv = $sth->execute;
  my ($row, $stop);

  while($row = $sth->fetchrow_hashref){
    print ("$row->{musictype}\n");
    $stop = gdio::putline ("$row->{musictype}");
    last if ($stop);
  }
  gdio::putline ("");	# End of result
  $sth->finish;
}

############################################################
### List music sources
sub list_music_sources{
  my $sth = $dbh->prepare('SELECT * FROM source ORDER BY id ');
  my $rv = $sth->execute;
  my ($row, $stop);

  while($row = $sth->fetchrow_hashref){
    print ("$row->{source}\n");
    $stop = gdio::putline ("$row->{source}");
    last if ($stop);
  }
  gdio::putline ("");	# End of result
  $sth->finish;
}


############################################################
### Calculate language hash
sub language_hash{  # returns the hash value of the language
		    # table
#  my $sth = $dbh->prepare(	# get used languages only
#           'SELECT language.id, language.language, COUNT(*) AS freq '
#          .'FROM tracks,language '
#          .'WHERE tracks.lang=language.id '
#          .'GROUP BY language.id '
#          .'ORDER BY freq DESC '
#	);

  my $sth = $dbh->prepare(
           'SELECT id, language, freq '
          .'FROM language '
          .'ORDER BY freq DESC '
	);

  my $rv = $sth->execute;
  my @row;
  my $newval;

  my $hashval = 0;

  while(@row = $sth->fetchrow_array){
    ### calc hash value
    $newval =  ord(chop($row[0]))<<8;
    $newval += ord(chop($row[0]));
    $hashval = myhash::addvaltohash($hashval, $newval);
  }
  $sth->finish;

#  my $sth = $dbh->prepare(	# get all languages alphabetically
#           'SELECT language.id '
#          .'FROM language '
#          .'ORDER BY language.id '
#	);
#
#  my $rv = $sth->execute;
#  my @row;
#
#  while(@row = $sth->fetchrow_array){
#    ### calc hash value
#    $newval =  ord(chop($row[0]))<<8;
#    $newval += ord(chop($row[0]));
#    $hashval = myhash::addvaltohash($hashval, $newval);
#  }
#  $sth->finish;

  return $hashval;
}


############################################################
### Get language hash
sub get_language_hash{  # sends the language hash val to th client
			# 

  my $hashval = language_hash();

  print("language hash value = $hashval \n");
  gdio::putline ("$hashval");	# send hash value
  gdio::putline ("");		# End of result
}


############################################################
### calculate genre hash value
sub genre_hash{  # returns hash value of the language
	             # table 

#  my $sth = $dbh->prepare(  # get used genres only
#           'SELECT genre.id,genre.genre,COUNT(*) AS freq '
#          .'FROM tracks,genre '
#          .'WHERE tracks.genre1=genre.id OR tracks.genre2=genre.id '
#          .'GROUP BY genre.id '
#          .'ORDER BY freq '
#	);

  my $sth = $dbh->prepare(
           'SELECT id, genre, freq '
          .'FROM genre '
          .'ORDER BY freq '
	);

  my $rv = $sth->execute;
  my @row;
  my $newval;

  my $hashval = 0;

  while(@row = $sth->fetchrow_array){
    ### calc hash value
    $newval =  ord(chop($row[0]))<<16;
    $newval += ord(chop($row[0]))<<8;
    $newval += ord(chop($row[0]));
    $hashval = myhash::addvaltohash($hashval, $newval);
  }
  $sth->finish;

#  my $sth = $dbh->prepare(	# get all genres alphabetically
#           'SELECT genre.id '
#          .'FROM genre '
#          .'ORDER BY genre.id '
#	);
#
#  my $rv = $sth->execute;
#
#  while(@row = $sth->fetchrow_array){
#    ### calc hash value
#    $newval =  ord(chop($row[0]))<<8;
#    $newval += ord(chop($row[0]));
#    $hashval = myhash::addvaltohash($hashval, $newval);
#  }
#
#  $sth->finish;

  return $hashval;
}



############################################################
### Get genre hash
sub get_genre_hash{  # send genre hash value to client
	             # 

  my $hashval = genre_hash();

  print("genre hash value = $hashval \n");
  gdio::putline ("$hashval");	# send hash value
  gdio::putline ("");		# End of result
}


############################################################
### calculate types hash value
sub types_hash{  # returns hash value of the
	         # musictype table

  my $sth = $dbh->prepare('SELECT * FROM musictype ORDER BY id ');
  my $rv = $sth->execute;
  my $row;
  my $hashval = 0;
  my $newval;

  while($row = $sth->fetchrow_hashref){
    ### calc hash value
    $newval =  ord(chop($row->{musictype}))<<16;
    $newval += ord(chop($row->{musictype}))<<8;
    $newval += ord(chop($row->{musictype}));
    $hashval = myhash::addvaltohash($hashval, $newval);
  }
  $sth->finish;

  return $hashval;
}

############################################################
### Get types hash
sub get_types_hash{  # send types hash value to client
	             # 

  my $hashval = types_hash();

  print("types hash value = $hashval \n");
  gdio::putline ("$hashval");	# send hash value
  gdio::putline ("");		# End of result
}


############################################################
### calculate sources hash value
sub sources_hash{  # returns hash value of the
	           # source table

  my $sth = $dbh->prepare('SELECT * FROM source ORDER BY id ');
  my $rv = $sth->execute;
  my $row;
  my $hashval = 0;
  my $newval;

  while($row = $sth->fetchrow_hashref){
    ### calc hash value
    $newval =  ord(chop($row->{source}))<<16;
    $newval += ord(chop($row->{source}))<<8;
    $newval += ord(chop($row->{source}));
    $hashval = myhash::addvaltohash($hashval, $newval);
  }
  $sth->finish;

  return $hashval;
}

############################################################
### Get sources hash
sub get_sources_hash{  # send sources hash value to client
	               # 

  my $hashval = sources_hash();

  print("sources hash value = $hashval \n");
  gdio::putline ("$hashval");	# send hash value
  gdio::putline ("");		# End of result
}

############################################################
### Get all hash
sub get_all_hashes{  # send hash values to client
	             # of all dynamic tables:
		     # language, genre, type

  my $languagehash = language_hash();
  my $genrehash    = genre_hash();
  my $typeshash    = types_hash();
  my $sourceshash  = sources_hash();

  print("hash values: $languagehash $genrehash $typeshash $sourceshash\n");
  gdio::putline ("$languagehash\t$genrehash\t$typeshash\t$sourceshash");
  gdio::putline ("");		# End of result
}


############################################################
### List all languages ordered
sub list_all_languages_ordered{ 
    #get all languages and list them ordered by their usage count
#  my $sth = $dbh->prepare(
#           'SELECT language.id, language.language, ' 
#          .'  (COUNT(*)-1) AS freq '
#          .'FROM language LEFT JOIN tracks '
#          .'ON tracks.lang=language.id '
#          .'GROUP BY language.id '
#          .'ORDER BY freq DESC, language.language ASC '
#	);
  my $sth = $dbh->prepare(
           'SELECT id, language, freq '
          .'FROM language  '
          .'ORDER BY freq DESC, language ASC '
	);

  my $rv = $sth->execute;
  my (@row, $stop);

  while(@row = $sth->fetchrow_array){
    #if (length($row[2])==0){$row[2]=0;}
    print ("$row[0]\t$row[1]\t$row[2]\n");
    $stop = gdio::putline ("$row[0]\t$row[1]\t$row[2]");
    last if ($stop);
  }
  gdio::putline ("");	# End of result
  $sth->finish;
}


############################################################
### List all genres ordered directly
sub list_all_genres_ordered{ 
    # get all genres and list them ordered alphabetically by genre
    # fields: id, genre, freq

  ### Get all genre entries
  my $sthA = $dbh->prepare(
            'SELECT id, genre FROM genre ORDER BY genre.id '
	);
  my $totcnt = $sthA->execute;

  ### Get used entries only with count
#  my $sthB = $dbh->prepare(
#            'SELECT genre.id,genre.genre,COUNT(*) AS freq '
#           .'FROM tracks,genre '
#           .'WHERE tracks.genre1=genre.id OR tracks.genre2=genre.id '
#           .'GROUP BY genre.id ORDER BY genre.id '
#	);
  my $sthB = $dbh->prepare(
           'SELECT id, genre, freq '
          .'FROM genre '
          .'ORDER BY id '
	);

  my $usedcnt = $sthB->execute;

### The following code should actually go to 'gdtablefreq.pl'  !

  ### Merge frequencies of list B into list A (both lists must be ordered by id!)
  # -> build "hierarchical sum": freq(node) = sum-of each freq(subnode) + proper freq
  my $rowB = $sthB->fetchrow_hashref;
  my ($i, $rowA, @listA, $rec);
  while ($rowA = $sthA->fetchrow_hashref){
    $rec = {};
    $rec->{"id"}    = $rowA->{id};
    $rec->{genre} = $rowA->{genre};
    $rec->{freq}  = 0;  # init freq counter
    if($rowA->{id} eq $rowB->{id}){
      # copy frequency of B to A
      $rec->{freq} = $rowB->{freq};
      $rowB = $sthB->fetchrow_hashref; # get next entry from B
    }
    push @listA, $rec;
  }

  recalc_genre_freq_arrayofhash(@listA);

  @listA = sort { $gdserv::a->{genre} cmp $gdserv::b->{genre} } @listA;

  # "no genre" first: freq=1
  print ("\t\t1\n");
  gdio::putline ("\t\t1");

  my $stop;
  for $i (0 .. $#listA){
    print ("$listA[$i]{id}\t$listA[$i]{genre}\t$listA[$i]{freq}\n");
    $stop = gdio::putline 
          ("$listA[$i]{id}\t$listA[$i]{genre}\t$listA[$i]{freq}");
    last if ($stop);
  }
  gdio::putline ("");	# End of result
  $sthA->finish;
  $sthB->finish;
}


############################################################
sub recalc_genre_freq_arrayofhash{
  ### new version: works on array of hashes (id, genre, freq)
  #   requires that the array is ordered by id!
  
  ### first pass, get maximal genre id length
  my $arrlen = @_;
  my $maxlen = 0;
  my $i;

  for $i( 0 .. $arrlen ){
    if(length($_[$i]{id}) > $maxlen){
      $maxlen = length($_[$i]{id});
    }
  }
  ### next passes: cumulate freq of ID with maxlen to ID with maxlen-1
  my ($parent, $ID);
  while ($maxlen > 1){
    for $i( 0 .. $arrlen ){
      # 0:id, 1:genre, 2:freq
      if(length($_[$i]{id}) == $maxlen){
        ### add freq of this item to freq of parent item
        $ID = $_[$i]{id};
        chop($ID);
        $parent=0;
        while($parent<$arrlen && $_[$parent]{id} ne $ID){
          $parent++;
        }
        if($_[$parent]{id} eq $ID){
          $_[$parent]{freq} += $_[$i]{freq};
        }
        else{print("ERROR: cant' find parent ID $ID \n");exit;}
      }
    }
    $maxlen--;
  }
}


############################################################
### Modify genres table

############################################################
### Modify Genres: Delete
sub gen_delete{ 
  ### Delete a genre and move all it's tracks to the supergenre
  my($genreid) = @_;	# the ID of the genre to be deleted

  my $targetid = $genreid;
  chop ($targetid);
  print ("Delete genre id $genreid translate it to $targetid\n");

  my ($retval1, $retval2, $gensdel);
  $retval1 = $dbh->do('UPDATE tracks SET genre1="'.$targetid.'" WHERE genre1 LIKE "'.$genreid.'%"');
  $retval2 = $dbh->do('UPDATE tracks SET genre2="'.$targetid.'" WHERE genre2 LIKE "'.$genreid.'%"');
  $gensdel= $dbh->do('DELETE FROM genre WHERE id LIKE "'.$genreid.'%"');
  print("$gensdel genre entries deleted. $retval1 + $retval2 track records changed to supergenre.\n");
 
  if($retval1 eq '0E0'){$retval1=0;}
  if($retval2 eq '0E0'){$retval2=0;}
  gdio::putline ("$gensdel genre entries deleted. ".($retval1+$retval2)." track records changed.");
  gdio::putline ("");	# End of result
}


############################################################
### find a free genre for a given prefix. It returns only the suffix.
sub gen_new_suffix{
  my ($prefix) = @_;
  my $suffix = "a";
  ### search for a new id
  my ($sth, $count);
  while($suffix ne "z"){
    $sth = $dbh->prepare("SELECT * FROM genre WHERE id = '".$prefix.$suffix."'");
    $count = $sth->execute;
    $sth->finish;
    last if ($count eq '0E0');
    $suffix++;
  }
  return $suffix;
}

############################################################
### Modify Genres: Move
sub gen_move{ 
  ### Move a genre and all its subgenres to a new location in the hierarchy
  my($genreidFrom, $genreidTo) = @_;	# the two IDs

  print ("Move genre $genreidFrom to $genreidTo\n");

  my $suffix = gen_new_suffix($genreidTo); # new subgenre in target genre

  if($suffix eq "z"){
    print ("No genre moved! You can't put more than 26 subgenres in a genre. \n");
    gdio::putline ("No genre moved! You can't put more than 26 subgenres in a genre.");
  }
  elsif(    (length($genreidFrom) <= length($genreidTo))
         && (substr($genreidTo, 0, length($genreidFrom)) eq $genreidFrom)
       ){
    print ("No genre moved! Target is a child of Source. \n");
    gdio::putline ("No genre moved! Target is a child of Source.");
  }
  else{
    ### replace all genre prefixes "$genreidFrom" by "$genreidTo.$suffix"
    ### in tracks and genre table

    my $retval1 = $dbh->do(
         'UPDATE tracks SET genre1=CONCAT("'.$genreidTo.'","'.$suffix.'",SUBSTRING(genre1, '.(length($genreidFrom)+1).')) '
        .'WHERE genre1 LIKE "'.$genreidFrom.'%"');
    my $retval2 = $dbh->do(
         'UPDATE tracks SET genre2=CONCAT("'.$genreidTo.'","'.$suffix.'",SUBSTRING(genre2, '.(length($genreidFrom)+1).')) '
        .'WHERE genre2 LIKE "'.$genreidFrom.'%"');
    my $retval3 = $dbh->do(
         'UPDATE genre SET id=CONCAT("'.$genreidTo.'","'.$suffix.'",SUBSTRING(id, '.(length($genreidFrom)+1).')) '
        .'WHERE id LIKE "'.$genreidFrom.'%"');
    gdio::putline (($retval1+$retval2)." track genres, $retval3 genre IDs moved.");
#    gdio::putline (" track genres moved.");
  }
  gdio::putline ("");	# End of result
}


############################################################
### Modify Genres: Rename
sub gen_rename{ 
  ### Rename an existing genre
  my($genreid, $newgenrename) = @_;	# the ID and the new name of the genre
  print ("Add genre $newgenrename to $genreid\n");

  my $retval = $dbh->do("UPDATE genre SET genre=".$dbh->quote($newgenrename)." WHERE id = '".$genreid."'");

  gdio::putline ("Genre renamed to $newgenrename.");
  gdio::putline ("");	# End of result
}


############################################################
### Modify Genres: Add
sub gen_add{ 
  ### Add a new genre
  my($genreid, $newgenrename) = @_;	# the ID of the genre where the new genre is added
  print ("Add genre $newgenrename to $genreid\n");

  my $suffix = gen_new_suffix($genreid);

  if($suffix eq "z"){
    print ("No genre added! You can't put more than 26 subgenres in a genre. \n");
    gdio::putline ("No genre added! You can't put more than 26 subgenres in a genre.");
  }
  else{
    my $retval = $dbh->do("INSERT INTO genre "
    	."(id, id3genre, genre) "
    	."VALUES('".$genreid.$suffix."',NULL,".$dbh->quote($newgenrename).")");
    gdio::putline ("Genre $newgenrename added.");
  }
  gdio::putline ("");	# End of result
}




############################################################
### Modify music types table

############################################################
### returns the number of records in the musictype table
sub typ_nb_records{
  my ($sth, $count);
  $sth = $dbh->prepare("SELECT * FROM musictype");
  $count = $sth->execute;
  $sth->finish;
  return $count;
}

############################################################
### Modify Types: Add
sub typ_add{ 
  ### Add a new type
  my($newtypename) = @_;
  print ("Add type $newtypename\n");

  my $index = typ_nb_records() + 1;

  my $retval = $dbh->do("INSERT INTO musictype "
    	."(musictype, id) VALUES(".$dbh->quote($newtypename).",'".$index."')");
  gdio::putline ("Music type $newtypename added.");
  gdio::putline ("");	# End of result
}

############################################################
### Modify Types: Delete
sub typ_delete{
  ### Delete a type

  my($delid) = @_;	# the ID of the type to be deleted
  my $lastid = typ_nb_records() - 1; 

  # sequence of id-numbers must be contiguous: -> "move" lastid over delid,
  # overwriting delid.
  
  # Note: track.type starts from 0, musictype.id starts from 1
  #       Bad, I know, but it's too late to change it. I ignore the num value
  #       of musictype.id, and only use it to define an order.
  print ("Delete type id $delid, put $lastid to its place\n");
  
  ### modify tracks table
  my ($retval1, $retval2, $typsdel);
  $retval1 = $dbh->do('UPDATE tracks SET type=  0        WHERE type = '.$delid);
  $retval2 = $dbh->do('UPDATE tracks SET type='.$delid.' WHERE type = '.$lastid);

  ### modify musictypes table
  $typsdel= $dbh->do('DELETE FROM musictype WHERE id ='.($delid+1));
  $retval2= $dbh->do('UPDATE musictype SET id='.($delid+1).' WHERE id = '.($lastid+1));

  print("Type deleted. Type of ".$retval1." tracks set to NULL.\n");
 
  if($retval1 eq '0E0'){$retval1=0;}
  if($retval2 eq '0E0'){$retval2=0;}
  gdio::putline ("Type deleted. Type of ".$retval1." tracks set to NULL.");
  gdio::putline ("");	# End of result
}


############################################################
### Send complete track
sub send_track_row{ 
  # Sends a complete track row obained from a SELECT * FROM tracks
  # query. 
  # The parameter is a track row (record) obtained by a 
  # $trackset->fetchrow_hashref command

  my ($trackrow) = @_;
    ## if ($nbtrks > 0){
    print(
	 $trackrow->{artist}.", "
	.$trackrow->{title}.", "
	.$trackrow->{genre1}.", "
	.$trackrow->{genre2}.", "
	.$trackrow->{year}.", "
	.$trackrow->{lang}.", "
	.$trackrow->{type}.", "
	.$trackrow->{rating}.", "
	.$trackrow->{length}.", "
	.$trackrow->{source}.", "
	.$trackrow->{sourceid}.", "
	.$trackrow->{tracknb}.", "
	.$trackrow->{mp3file}.", "
	.$trackrow->{condition}.", "
	.$trackrow->{voladjust}.", "
	.$trackrow->{created}.", "
	.$trackrow->{modified}.", "
	.$trackrow->{id}.", "
	.$trackrow->{bitrate}.", "
	.$trackrow->{haslyrics}
	."\n"
	);

    gdio::putline (
	 $trackrow->{artist}."\t"
	.$trackrow->{title}."\t"
	.$trackrow->{genre1}."\t"
	.$trackrow->{genre2}."\t"
	.$trackrow->{year}."\t"
	.$trackrow->{lang}."\t"
	.$trackrow->{type}."\t"
	.$trackrow->{rating}."\t"
	.$trackrow->{length}."\t"
	.$trackrow->{source}."\t"
	.$trackrow->{sourceid}."\t"
	.$trackrow->{tracknb}."\t"
	.$trackrow->{mp3file}."\t"
	.$trackrow->{condition}."\t"
	.$trackrow->{voladjust}."\t"
	.$trackrow->{created}."\t"
	.$trackrow->{modified}."\t"
	.$trackrow->{id}."\t"
	.$trackrow->{bitrate}."\t"
	.$trackrow->{haslyrics}
	);

  # These variables give a hint, if the frequencies should be recalculated (when a track is updated)
  $trk_last_id = $trackrow->{id};
  $trk_last_lang   = $trackrow->{lang};
  $trk_last_genre1 = $trackrow->{genre1};
  $trk_last_genre2 = $trackrow->{genre2};
}



############################################################
### Set CDDB_get parameters
my %cddb_config;
# following variables just need to be declared if different from defaults
$cddb_config{CDDB_HOST}="freedb.freedb.org";        # set cddb host
$cddb_config{CDDB_PORT}=8880;                       # set cddb port
$cddb_config{CDDB_MODE}="cddb";                     # set cddb mode: cddb or http
$cddb_config{CD_DEVICE}="/dev/cdrom";               # set cd device
# user interaction welcome?
$cddb_config{input}=0;   # 1: ask user if more than one possibility
                         # 0: no user interaction


############################################################
### Get and return id and toc of currently inserted CD
sub get_cd_diskid{ 
  use CDDB_get;
  # get id and track lengths
  my $diskid=CDDB_get::get_discids($cddb_config{CD_DEVICE});
#  $track[0]{cddbid} = sprintf "%lx", $diskid->[0]; # get hex notation
  $diskid->[0] = sprintf "%08lx", $diskid->[0]; # get hex notation
  return $diskid;
}

############################################################
### Get and return directory of currently inserted CD
sub get_cd_directory{ 
# returns array of hashes: index 0: cddbid; index 1..: length (in seconds)
# index 0:       {cddbid (hex, without '0x')}, {title}, {artist}
# other indexes: {tracklength-sec}, {title}, {artist}
#
# title and artist are only defined, if a cddb entry was found
#
# 1) if the system has internet access, a matching record is
#    searched at freedb.org
# 2) searches for a matching cddb record in ~/cddb/*
# 3) If a record couldn't be found, an array with default strings 
#    is returned
#

  use CDDB_get;
  #use Net::Ping;     #can't use it because it requires root privileges

  my @track;
  my $diskid = get_cd_diskid();
  my $base = gdparams::gdbase();
  $track[0]{artist} = "Artist";
  $track[0]{title}  = "Album Title";
  $track[0]{cddbid} = $diskid->[0];
  my $nbtracks = $diskid->[1];
  my $toc = $diskid->[2];
  my $cddbid = $diskid->[0];
  my ($lengthsec);
  my $i=1;
  while ($i <= $nbtracks) {#i<total incl. lead-out
    $lengthsec = ($toc->[$i]  ->{min}*60+$toc->[$i]  ->{sec})
                -($toc->[$i-1]->{min}*60+$toc->[$i-1]->{sec});
    $track[$i]{track}= $i;
    $track[$i]{length}= $lengthsec;
    $track[$i]{artist}=" ";            # some default values
    $track[$i]{title} ="CD Track $i";
    $i++;
  }


#print "\nBEGIN ping\n";
#my $p = Net::Ping->new("icmp");
#print "freedb.freedb.org is alive.\n" if $p->ping("freedb.freedb.org", 2);
#$p->close();
#print "END ping\n";

  if ($gdparms::systemonline){# Are we online?
    ##################################################################
    ### We are online, Try to get matching freedb entry
    my %cd=CDDB_get::get_cddb(\%cddb_config);
  
    if(defined $cd{title}) {
      $track[0]{artist} = $cd{artist};
      $track[0]{title}  = $cd{title};
      #$track[0]{year}   = $cd{year};
      ### Add track info to @track
      my $i=1;
      while ($i <= $nbtracks) {
        #print "$i: $cd{track}[$i-1] \n";
        $track[$i]{artist} = $cd{artist};
        $track[$i]{title}  = $cd{track}[$i-1];
        # ...{genre} = $cd{cat};
        $i++;
      }
    }
    else{
      print "freedb: NOT FOUND cddbid: $cd{id}\n";
    }
  }
  else{
    ##################################################################
    ### We are offline, try to find a cddb record on the local machine
    my $cddb_file = `find $base/cddb -name $cddbid -print`;
    $cddb_file = (split /\s+/, $cddb_file)[0]; # get first match only
    if(length($cddb_file) > 0){
      ### a cddb-file was found - get it
      my @cddblst = get_cddb_rec_offline("$cddb_file");
      $i = 0;
      while ($i <= $nbtracks){
      	$track[$i]{artist} = $cddblst[$i]{artist};
      	$track[$i]{title}  = $cddblst[$i]{title};
        $i++;
      }
    }
  }
  return @track;
}


#sub get_cd__directory___OLD___{ 
#sub get_cd__directory{ 
# returns array of hashes: index 0: cddbid; index 1..: length (in seconds)
#  my $tmpout="/tmp/cdda2wav.index.out";
#  my $nbtracks = 0;
#  my @track;
#  my $discid;
#  my $line;
#
#  system("rm $tmpout");
#  system("cdda2wav -D /dev/cdrom -N -d1 &> $tmpout");
#  open(TMPOUT, $tmpout);
#  while(<TMPOUT>){
#	$line = $_;
#	if($line=~m/total tracks/){
#		$line=~m/total tracks:([0-9]*)/;
#		$nbtracks = $1;
#	}
#	while($line=~m/[0-9]*\.\(\s*[0-9]*:[0-9]*\.[0-9]*\)/){
#		$line=~m/([0-9]*)\.\(\s*([0-9]*):([0-9]*)\.[0-9]*\)/;
#		$track[$1]{track}= $1;
#		$track[$1]{length}= ($2*60)+$3;
#
#		#push @track, $rec;
#	        $line=~s/[0-9]*\.\(\s*[0-9]*:[0-9]*\.[0-9]*\)/xx/;
#	}
#	if($line=~m/CDDB discid:\s*[0-9a-z]*/){
#	        $line=~m/CDDB discid:\s*([0-9a-z]*)/;
#		#print "Disc ID: $1\n";
#		$discid=$1;
#		$discid =~ m/0x(\S*)/;  # remove 0x
#		$track[0]{cddbid}= $1;
#	}
#  }
#  close(TMPOUT);
#  return @track;
#}



############################################################
### Read the specified cddb-file and return its content
sub get_cddb_rec_offline{ 
# returns array: index 0:   album artist, album title; 
#		 index 1..: track title; 
# examples: -> separator: '-' or '/'
# DTITLE=Die Aerzte - Die Bestie in Menschengestalt
# DTITLE=MC Lyte / Ain't No Other

  my ($cddb_file) = @_;

  my @track;
  my $line;

  open(CDDBF, $cddb_file) or die "can't open file $cddb_file\n";
  while(<CDDBF>){
	$line = $_;
	chop($line);
	if($line=~m/DTITLE=.+/){
  	  if   ($line=~m/DTITLE=(.+) \/ (.+)/){
  	    $track[0]{artist}= $1;
  	    $track[0]{title} = $2;
  	  }
  	  elsif($line=~m/DTITLE=(.+) \- (.+)/){
  	    $track[0]{artist}= $1;
  	    $track[0]{title} = $2;
  	  }
  	  elsif($line=~m/DTITLE=(.+)/){
  	    $track[0]{artist}= $1;
  	    $track[0]{title} = "";
  	  }
  	}
	
	if($line=~m/TTITLE([0-9]+)=(.+)/){
	  $track[$1+1]{artist}= $track[0]{artist};
	  $track[$1+1]{title} = $2;
	}
  }
  close(CDDBF);

  return @track;

}

############################################################
### List directory of currently inserted CD plus informations from DB
sub list_cd_directory_info{ 
  # takes no parameter or the cddb-id

  my ($cddbid) = @_;

  my @track;
  my $nbtracks;
  my $i;
  my $nbtrks;
  my $nbalbs;
  my $trseth;  # track set handle	
  my @row;
  my $trackrow;
  my $base = gdparams::gdbase();
  my $cddb_file="";
  my @cddblst;

  #########################################################
  ### Set default values
  @track = get_cd_directory();
  $nbtracks = @track - 1;

  #########################################################
  ### Send album record first
  $trseth = $dbh->prepare(	
           'SELECT * FROM album WHERE cddbid ="'. $cddbid . '" ');
  $nbalbs = $trseth->execute;
  if($nbalbs > 0){
    print("Matching album in database found");
    $trackrow = $trseth->fetchrow_hashref;
    $trackrow->{cddbid} = $cddbid;
    print("$trackrow->{artist}\t$trackrow->{title}\t$cddbid\n");
    gdio::putline("$trackrow->{artist}\t$trackrow->{title}\t$cddbid");
  }
  else{
    if(defined($track[0]{title})){
      ### a matching online freedb entry was found
      print("$track[0]{artist}\t$track[0]{title}\t$track[0]{cddbid}\n");
      gdio::putline("$track[0]{artist}\t$track[0]{title}\t$track[0]{cddbid}");
    }
    else{
      print "Error: can't read CD directory at all\n";
    }
  }
  $trseth->finish;

  #########################################################
  ### Send each track
  for ($i=1; $i<=$nbtracks; $i++){
    ### get first track with given cddb-id and track number

    if (length($cddbid)>=8){
      $trseth = $dbh->prepare(	
           'SELECT * FROM tracks '
          .'WHERE sourceid ="'. $cddbid . '" '
	  .' AND tracknb = '.$i );
      $nbtrks = $trseth->execute;
      $trackrow = $trseth->fetchrow_hashref;
    }

    if ((length($cddbid)<8) || ($nbtrks < 1)){
      ### set default values
      $nbtrks = 0;
      $trackrow->{artist} = $track[$i]{artist};
      $trackrow->{title}  = $track[$i]{title};
      $trackrow->{genre1} = "";
      $trackrow->{genre2} = "";
      $trackrow->{year}   = "";
      $trackrow->{lang}   = "";
      $trackrow->{type}   = 0;
      $trackrow->{rating} = 0;
      $trackrow->{length} = $track[$i]{length};
      $trackrow->{source} = 0;
      $trackrow->{sourceid} = $track[0]{cddbid};
      $trackrow->{tracknb}  = $i;
      $trackrow->{mp3file}  = "";
      $trackrow->{condition}= 0;
      $trackrow->{voladjust}= 0;
     #$trackrow->{created}  = 
     #$trackrow->{modified} =
      $trackrow->{bitrate}  = $gdparms::defrecbitrate;
      $trackrow->{haslyrics}="";
    }

    ### send all fields
    send_track_row($trackrow);

    $trseth->finish;
  }# end for

  gdio::putline ("");	# End of result
}
############################################################
### List directory of currently inserted CD plus informations from DB
sub list_inbox_album_directory{ 
  # takes as parameter the album directory (without /home/music/inbox prefix)

  my ($path) = @_;

  my ($curfile);
  my $base = gdparams::gdbase();
  my $fullpath = "$base/inbox/albums/$path";
  print "import album at $fullpath\n";

  unlink <$base/inbox/trxx*>;
  unlink <$base/inbox/tmp-album-dir>;
  system "cd \"$fullpath\"; rm -f trxx*";
  opendir INBOX, "$fullpath";
  my @inboxfile = readdir INBOX;
  closedir INBOX;

  ### generate new mp3-filename (get highest trxx... filename)
  my $fileid = gdgentools::last_imported_tracknb($dbh);
  $fileid += 1;   # next available id
  my $cddbid = sprintf("%08ld", $fileid);
  my $tracknb = 1;
  
  #########################################################
  ### Send album record first

  print         "$path\t$path\t$cddbid\n";
  gdio::putline("$path\t$path\t$cddbid");
  #print  "ln -s \"$fullpath\" \"$base/inbox/tmp-album-dir\"\n";
  system  "ln -s \"$fullpath\" \"$base/inbox/tmp-album-dir\"";

  #########################################################
  ### Send each track
  foreach $curfile (@inboxfile){
    ### check if file is audio format
    if (   gdgentools::audio_filetype($curfile) eq "mp3"
        || gdgentools::audio_filetype($curfile) eq "ogg"
        || gdgentools::audio_filetype($curfile) eq "flac"){
      send_inbox_track_info($curfile, $fileid, "$cddbid", $tracknb, "$fullpath");
      $fileid += 1;
      $tracknb += 1;
    }
  }# end foreach

  gdio::putline ("");	# End of result
}



############################################################
### List directory of current inbox plus informations from the metatags
sub list_inbox_directory_info{ 

  my $base = gdparams::gdbase();

  unlink <$base/inbox/trxx*>;
  opendir INBOX, "$base/inbox";
  my @ibfile = readdir INBOX;
  closedir INBOX;
  
  my ($curfile);


  ### generate new mp3-filename (get highest trxx... filename)
  my $fileid = gdgentools::last_imported_tracknb($dbh);

  ### Send each track
  foreach $curfile (@ibfile){
    ### check if file is mp3 format
    if (   gdgentools::audio_filetype($curfile) eq "mp3"
        || gdgentools::audio_filetype($curfile) eq "ogg"
        || gdgentools::audio_filetype($curfile) eq "flac"){
      $fileid += 1; #next available id
      send_inbox_track_info($curfile, $fileid, "", 1, "$base/inbox");
    }
  }# end foreach

  gdio::putline ("");	# End of result
}

############################################################
### the file $curfile must be valid audio file
#   if it is a audiofile, all available metadata and other track
#   informations are sent to the server
sub send_inbox_track_info{ 

  my ($curfile, $fileid, $cddbid, $tracknb, $path) = @_;
  my ($id3genre, $title, $audiofile, $trackrow);

  my $base = gdparams::gdbase();

  ### check if file has a legal audio format

  if (   gdgentools::audio_filetype($curfile) eq "mp3"
      || gdgentools::audio_filetype($curfile) eq "ogg"
      || gdgentools::audio_filetype($curfile) eq "flac"){
    print ("\ncurrentfile is: $curfile \n");

    $audiofile = sprintf("trxx%08ld.%s", $fileid, gdgentools::audio_filetype($curfile));
    symlink "$path/$curfile", "$base/inbox/$audiofile";

    # get info from meta tags
    $trackrow->{title}     = gdgentools::audiofile_title("$path/$curfile"); # takes filename as default of no metatag present
    $trackrow->{artist}    = gdgentools::audiofile_artist("$path/$curfile");
    $trackrow->{genre2}    = "";
    $trackrow->{year}      = gdgentools::audiofile_year("$path/$curfile");
    $trackrow->{lang}      = "-";
    $trackrow->{type}      = 1; # medium
    $trackrow->{rating}    = 0;
    $trackrow->{length}    = gdgentools::audiofile_lengthsec("$path/$curfile");
    $trackrow->{source}    = 0;  # CD
    $trackrow->{sourceid}  = $cddbid; # could also be empty
    $trackrow->{tracknb}   = $tracknb;
    $trackrow->{mp3file}   = $audiofile;
    $trackrow->{condition} = 0;  # OK
    $trackrow->{voladjust} = 0;
    $trackrow->{created}   = "";
    $trackrow->{modified}  = "";
    $trackrow->{id}        = ""; # a new one will be automatically generated
    $trackrow->{bitrate}   = gdgentools::get_bitrate_str("$path/$curfile");

    # get genre if available and translate to gd-genre
    $trackrow->{genre1} = gdgentools::audiofile_genre($dbh, "$path/$curfile");
#    $id3genre = gdgentools::audiofile_genre("$path/$curfile");
#    $trackrow->{genre1} = gdgentools::genre_id3togd($dbh, $id3genre);

    ### send all fields
    $trackrow->{haslyrics} = "";
    send_track_row($trackrow);

  }# end if is mp3file
}

############################################################
### import booklet/cover images
sub import_cover_img{ 
  # imports the jpeg images in a directory and associates them to an album
  # the images are imported in lexical order. 
  # Naming scheme: trxx(cd-id)-(num).jpg, where num is an automatically 
  # incremented counter. The file imgxx(cd-id)-00.jpg is the front cover, 
  # the other are the following pages in a booklet.
  
  # Parameters: 1) full directory path, 2) cd-id  (like 0x10ac77e0, xx00001496)
  
  gdgentools::import_cover_images($dbh, @_);
}


############################################################
### Get one track and send all information details from DB
sub get_track_details{ 
  # takes as parameter the track-ID

  my ($trackid) = @_;
  my $base = gdparams::gdbase();


  my $sth = $dbh->prepare("SELECT * FROM tracks WHERE id=$trackid");
  $sth->execute;
  my $trackrow = $sth->fetchrow_hashref;

  if(length($trackrow->{mp3file})>0){
    if(length($trackrow->{lyrics})>0){
      $trackrow->{haslyrics} = "1";
    }
  }
  ### send all fields
  send_track_row($trackrow);

  $sth->finish;

  gdio::putline ("");	# End of result
}


############################################################
### Delete specified track record and mp3 file
sub delete_track{ 
# deletes the playlist with the specified id
  my ($trackid) = @_;
  my $sqlcmd;
  my $base = gdparams::gdbase();
 
  ### Get track record
  my $sth = $dbh->prepare("SELECT * FROM tracks WHERE id=$trackid");
  $sth->execute;
  my $trackrow  = $sth->fetchrow_hashref;
  my $trackfile = $trackrow->{mp3file};
  truncstr($trackrow->{artist}, $shrtFieldLen);
  truncstr($trackrow->{title}, $shrtFieldLen);
  print("Deleting ($trackrow->{id}) $trackrow->{artist}, $trackrow->{title}\n");

  ### Delete track record
  $sqlcmd = ("DELETE FROM tracks WHERE id = ".$trackid);
#print"  \$dbh->do($sqlcmd);\n";
  $dbh->do($sqlcmd);

  ### Delete mp3 file
#print("rm $base/[0-9][0-9]/$trackfile \n");
  if (length($trackfile)>4){
    system("rm $base/[0-9][0-9]/$trackfile");
  }
  $sth->finish;

}


############################################################
### Delete specified album and all associated track records and mp3 files
sub delete_album_tracks{
# deletes the playlist with the specified id
  my ($albumid) = @_;

  if(length($albumid)>=6) {
    ### Delete all associated tracks
    ### get tracks of the album
    my $trksth = do_tracks_of_album_query($albumid);
    my $trow;
    while($trow = $trksth->fetchrow_hashref){
      delete_track($trow->{id});
    }
    $trksth->finish;

    ### Delete album record
    print("Deleting Album $albumid\n");
    my $sqlcmd = ("DELETE FROM album WHERE cddbid = '".$albumid."'");
#print"\$dbh->do($sqlcmd);\n"
    $dbh->do($sqlcmd);
  }
}


############################################################
### Get one track and send lyrics (or empty text)
sub get_track_lyrics{ 
  # takes as parameter the track-ID

  my ($trackid) = @_;

  my $sth = $dbh->prepare("SELECT * FROM tracks WHERE id=$trackid");
  $sth->execute;
  my $trackrow = $sth->fetchrow_hashref;

  ### insert one space into empty lines (protocol requires non empty content lines)
  my $lyrics = $trackrow->{lyrics};

  ### send lyrics
  my @lyrlines = split /\n/, $trackrow->{lyrics};
  chomp @lyrlines;
  my $line;
  print ("Sending lyrics: ".scalar(@lyrlines)." lines\n");
  foreach $line (@lyrlines){ # line end is a \015\012 sequence
    $line =~ s/\012$//;		# chop off very last newline, if there is one
    $line =~ s/\015$//;		# chop off very last newline, if there is one
    if (length($line)==0) {$line = " ";}
    gdio::putline ($line);
  }
  gdio::putline ("");	# End of result

  $sth->finish;
}

############################################################
### Update the lyrics of a track
sub update_track_lyrics{ 
  # parameters: track-ID, lyrics lines

  my $trackid = shift @_;
  my $lyrics = join "\n", @_;
  
  print("Lyrics recieved for $trackid:\n$lyrics\n");
  $dbh->do("UPDATE tracks "
  	  ."SET lyrics=".$dbh->quote($lyrics)
  	  ."WHERE id=$trackid");
  
}


############################################################
### Trim mp3 file
sub trim_mp3_file{ 
  # parameters: track-ID, lyrics lines

  my ($mp3file, $startframe, $endframe) = @_;
  my ($mp3fpath, $mp3directory, $undofname);
  my $base = gdparams::gdbase();

  $mp3fpath = gdgentools::get_full_audiofile_path($mp3file);
  $mp3directory = dirname($mp3fpath);
  $undofname = $mp3directory."/TrimUndo-".$mp3file;


  print ("rm $mp3directory/TrimUndo-*\n");	# remove old undofile
  system("rm $mp3directory/TrimUndo-*");
  print ("mv $mp3fpath $undofname\n");		# save original mp3file
  system("mv $mp3fpath $undofname");

  print ("gdmp3cutter $undofname $startframe $endframe $mp3fpath\n");
  #system("gdmp3cutter $undofname $startframe $endframe $mp3fpath");
  ### continuously send results, line by line
  my ($res, $resline);
  open CMDRES, "gdmp3cutter $undofname $startframe $endframe $mp3fpath |";
  #autoflush CMDRES 1;
  while($resline = <CMDRES>){
    chop($resline);
    print ("\"$resline\"\n");
    gdio::putline ($resline);
  }

  print ("END\n");
  gdio::putline ("");		# End of result
}

############################################################
### Get one album and send all information details from DB
sub trim_mp3_file_undo{ # restores the trimmed mp3 file (if possible)
  # parameters: track-ID, lyrics lines

  my ($mp3file) = @_;
  my ($mp3fpath, $mp3directory, $undofname);


  my $base = gdparams::gdbase();
  my $undofname = `ls $base/[0-9][0-9]/TrimUndo-$mp3file`; # get full path
  chop($undofname);

  $mp3directory = dirname($undofname);
  $mp3fpath = $mp3directory."/".$mp3file;

  #gdio::putline ("mv $undofname $mp3fpath");
  print ("mv $undofname $mp3fpath \n");	# restore original mp3file
  system("mv $undofname $mp3fpath");

  gdio::putline ("");		# End of result
}


############################################################
### Get one album and send all information details from DB
sub get_album_details{ 
  # takes as parameter the cddb-ID

  my ($albumid) = @_;

  if (length($albumid) > 8) {  ### sometimes, a distorted album id preceeded by 4 trashy characters
                                 #  is passed to this routine (reason is unknown).
                                 # this is just a workaround sent in by Frank (slightly modified by me)
    print "warning: albumid preceeded by trash! fixing it.\n";
    ($albumid) = substr($albumid,-8,8);
  } 

  my $base = gdparams::gdbase();


  my $sth = $dbh->prepare("SELECT * FROM album WHERE cddbid=\"$albumid\"");
  $sth->execute;
  my $row;
  if($row = $sth->fetchrow_hashref){
    ### send all fields
    print("$row->{artist}\n");
    print("$row->{title}\n");
    gdio::putline ("$row->{artist}");
    gdio::putline ("$row->{title}");
  }
  gdio::putline ("");	# End of result

  $sth->finish;
}

############################################################
### check currently inserted CD and compare to GDDB
sub check_current_cd{ 

# This routine finds all distinct cddb-ID's in the GDDB that have
# the same stem as the cddb-ID of the currently inserted CD.
# The routine is needed to solve the problem of potentially ambiguous
# cddb-ID's. It is usually called before grabbing a CD.

#  my @track = get_cd_directory();
  #$track[0]{cddbid} =~ m/0x(\S*)/;
  #my $cddbid = $track[0]{cddbid};
  my $diskid = get_cd_diskid();
  my $cddbid = $diskid->[0];

  ### Get all cddb-id's with root $cddbid
  my $sth = $dbh->prepare(	
           'SELECT DISTINCT sourceid '
          .'FROM tracks '
          .'WHERE sourceid LIKE "'. $cddbid. '%" '
          .'ORDER BY sourceid '
	);

  my $rv = $sth->execute;

  ### Get first track for each distinct cddb-id
  my $trseth;  # track set handle	
  my $row;
  my $trackrow;
  while($row = $sth->fetchrow_hashref){
    ### get first track with current cddb-id

    $trseth = $dbh->prepare(	
           'SELECT artist, title FROM tracks '
          .'WHERE sourceid ="'. $row->{sourceid} . '" ORDER BY tracknb ');
    $trseth->execute;
    $trackrow = $trseth->fetchrow_hashref;

    print("$row->{sourceid}\t$trackrow->{artist}, $trackrow->{title}\n");
    gdio::putline ("$row->{sourceid}\t$trackrow->{artist}, "
			            ."$trackrow->{title}");
    $trseth->finish;
  }
  if($rv > 0){ ### propose also new unambiguous key
    ### bulletproof is to take biggest extension instead of $rv!!!

    print ("$cddbid.$rv\tNone of these: New CD\n");
    gdio::putline("$cddbid.$rv\tNone of these: New CD");
  }
  gdio::putline ("");	# End of result

  $sth->finish;
}

############################################################
### read inbox album directories
sub list_inbox_album_directories{ 

# This routine finds all directoies in ~music/inbox/albums that
# contain mp3 or ogg files. Each directory is considered as an
# album

  my $base = gdparams::gdbase();

  my @filelist = `ls -R1 $base/inbox/albums`;
  my ($curfile, $directory);
  
  $directory="";
  
  foreach $curfile (@filelist){
    chop($curfile);
    if ($curfile =~ m/\/inbox\/albums\/(.*):/ ){
      #print "$curfile is a directory\n";
      $directory = $1;
    }
    if(length($directory)>0 and 
       (   gdgentools::audio_filetype($curfile) eq "ogg"
        or gdgentools::audio_filetype($curfile) eq "mp3"
        or gdgentools::audio_filetype($curfile) eq "flac")
       ){
      # we have a audio file in a valid directory (directory not yet printed)
      print         "$directory\n";
      gdio::putline "$directory";
      $directory = "";		# mark as printed
    }
  }

  gdio::putline ("");	# End of result

}


############################################################
### Open/close the CD tray
sub open_cd_tray{ 
  system("eject");
}
sub close_cd_tray{ 
  system("eject -t");
}

############################################################
### Get rip state
sub get_rip_state{   # send CD rip status to client
  my $base = gdparams::gdbase();
  # get first item of rip-list
  my $trid = gdgentools::tracklist_get_item($dbh, $playerid, $rp_list, 0); 
  if($trid != 0){
    my $sth = $dbh->prepare("SELECT * FROM tracks WHERE id=$trid");
    my $rv = $sth->execute;
    my ($wavf, $track, $wavfsize);
    if($track = $sth->fetchrow_hashref){
      if($track->{mp3file} =~ /(.*)\.\w+/){
        $wavf = `ls $base/[0-9][0-9]/$1.wav`;
        chop($wavf);
      }
      else{print("getripstate: can't find wavfile\n");}
      truncstr($track->{artist}, 20);
      truncstr($track->{title}, 20);
      print         ("$track->{tracknb}. $track->{artist} - $track->{title}\n");
      gdio::putline ("$track->{tracknb}. $track->{artist} - $track->{title}");
  
      $wavfsize = (-s "$wavf");
      my $wavtotsize = $track->{length} * 44100 * 4; # totalSize = length(sec)*sampRate*bytesPerSample
      $wavtotsize++; # prevent from divison by zero
      #print("length: $track->{length}, fsize: $wavfsize, soll:  $wavtotsize\n");
      my $percent = sprintf("%.2f%%", ($wavfsize * 100) / $wavtotsize);
      print         ("$percent percent\n");
      gdio::putline ("$percent");
      my $queuelen = gdgentools::tracklist_get_nb_items($dbh, $playerid, $rp_list);
      $queuelen--;
      print         ("$queuelen tracks queued\n");
      gdio::putline ("$queuelen tracks queued");
    }
    else{
      print         ("ERROR: can't find track in database\n");
      gdio::putline ("ERROR: can't find track in database");
    }
    $sth->finish;
  }
  gdio::putline ("");		# End of result
}


############################################################
### Get compress state
sub get_compress_state{   # send track compress status to client
  my $base = gdparams::gdbase();
  # get first item of compression-list
  my $trid = gdgentools::tracklist_get_item($dbh, $playerid, $co_list, 0); 
#print("top of compression list: track $trid\n");
  if($trid != 0){
    my $sth = $dbh->prepare("SELECT * FROM tracks WHERE id=$trid");
    my $rv = $sth->execute;
    my ($track, $audiofile, $currfilesize, $totalfilesize, $percent, $queuelen, $datarate);
    my ($audiofmt, $audioparam);
    if($track = $sth->fetchrow_hashref){
      $audiofile = `ls $base/[0-9][0-9]/$track->{mp3file}`;
      chop($audiofile);
      truncstr($track->{artist}, 20);
      truncstr($track->{title}, 20);
      print         ("$track->{tracknb}. $track->{artist} - $track->{title}\n");
      gdio::putline ("$track->{tracknb}. $track->{artist} - $track->{title}");
  
      $currfilesize = (-s "$audiofile");
      ($audiofmt, $datarate) = gdgentools::bitrate_str_to_format_param($track->{bitrate});  # split "mp3 128"
      if ($audiofmt eq "mp3" || $audiofmt eq "ogg"){
      	### datarate is bitrate
        $totalfilesize = ($track->{length} * $datarate * 1000) / 8; # totalSize = length*bitrate/BitsPerByte
      }
      if ($audiofmt eq "flac"){
      	### datarate is sampling rate
        $totalfilesize = ($track->{length} * $datarate * 1010 * 2); # totalSize = length*bitrate*BytesPerByte
      }
      $totalfilesize++; # prevent from divison by zero
      #print("file: $audiofile, length: $track->{length}, bitrate: $datarate, fsize: $currfilesize, soll:  $totalfilesize\n");
      $percent = sprintf("%.2f%%", ($currfilesize * 100) / $totalfilesize);
      print         ("$percent\n");
      gdio::putline ("$percent");

      $queuelen = gdgentools::tracklist_get_nb_items($dbh, $playerid, $co_list);
      $queuelen--;
      print         ("$queuelen wav-files queued\n");
      gdio::putline ("$queuelen wav-files queued");
    }
    $sth->finish;
  }
  gdio::putline ("");		# End of result
}


############################################################
###                    Misc commands                     ###
############################################################

############################################################
### command burn audio CD
sub pl_burn_cd{ # Arguments in @_ are the 

  my ($trackid, $sth);
  my $mp3list;
  my $row;
  my $base = gdparams::gdbase();

  $mp3list = "";

  ### Write track tiles+artists to file (for booklet printing etc.)
  open(RECLIST, ">$base/tmp/gdburnlist.txt");
  my $trkcnt = 1;

  foreach $trackid (@_){
    $sth = get_track($trackid);   # perform database query

    $row = $sth->fetchrow_hashref;
    $sth->finish;
    print("add to CD $base/??/$row->{mp3file} ($row->{title}) \n");
    print(RECLIST "$trkcnt.  $row->{artist}       $row->{title} \n");
    $mp3list = $mp3list . " " . "$base/??/" . $row->{mp3file};
    $trkcnt ++;
  }
  close(RECLIST);
  print("gdburn.sh " . $mp3list . "\n");
  system("gdburn.sh " . $mp3list . "&");
}



############################################################
### Update browse directory (external script)
sub update_browse_directory{
  my $base = gdparams::gdbase();
  system("$base/bin/gdmakelinks.pl -s  &"); # -s silent
}

### Update browse directory (external script)
sub export_id3_tags{
  my $base = gdparams::gdbase();
#  system("cd $base; $base/bin/gdexportdb.pl --id3  &");
  system("cd $base; $base/bin/gdexportdb.pl --metatags  &");
}

### Shut down entire server (external script)
sub shut_down_server{
  use vars qw($shutdowncmd);
  print("Executing $gdparms::shutdowncmd \n");
  system($gdparms::shutdowncmd);
#  system("/usr/sbin/usershutdown -h now  &");
}



############################################################
### General Query Command

### General database query
sub general_db_query{
  my ($dbquery) = @_;
  my ($sth, @row, $rowstr);

  ### Get # tracks
  $sth = $dbh->prepare($dbquery);
  $sth->execute;
  if(@row = $sth->fetchrow_array){
    $rowstr = join "\t", @row;
    print "$rowstr\n";
    gdio::putline ($rowstr);
  }
  gdio::putline ("");		# End of result
  $sth->finish;
}

sub general_db_query_count{ # needed because older mysql-versions have a bug with COUNT(DISTINCT ...
  my ($dbquery) = @_;
  my ($sth, @row, $rowstr, $nbrec);

  ### Get # tracks
#print "Database Query(count): $dbquery\n";
  $sth = $dbh->prepare($dbquery);
  $nbrec = $sth->execute;
  print "$nbrec\n";
  gdio::putline ($nbrec);
  gdio::putline ("");		# End of result
  $sth->finish;
}


############################################################
### Soundcard/Volume Commands
sub set_volume{
  my ($vol) = @_;
  gdsoundcard::sndc_set_volume(gdgentools::playerdefinition($dbh, $playerid, $audchannel0), $vol);
}

sub get_volume{
  my $vol = gdsoundcard::sndc_get_volume(gdgentools::playerdefinition($dbh, $playerid, $audchannel0));

  gdio::putline ($vol);
  gdio::putline ("");		# End of result
}

sub save_volume{
  gdsoundcard::sndc_save_volume(gdgentools::playerdefinition($dbh, $playerid, $audchannel0));
}
sub inc_volume{
  gdsoundcard::sndc_inc_volume(gdgentools::playerdefinition($dbh, $playerid, $audchannel0));
}
sub dec_volume{
  gdsoundcard::sndc_dec_volume(gdgentools::playerdefinition($dbh, $playerid, $audchannel0));
}

############################################################
### General Shell Command

### General shell command
sub general_sh_command{
  my ($shcommand) = @_;
  system($shcommand);
}

### General shell command and send back result
sub general_sh_command_res{
  my ($shcommand) = @_;
  my ($res, $resline);
  $res = `$shcommand`;
  my @reslines = split /\n/, $res;
  while ($resline = shift @reslines){
    print ("\"$resline\"\n");
    gdio::putline ($resline);
  }
  gdio::putline ("");		# End of result
}

sub general_sh_command_res_continuous{
# continuously sends results, line by line
  my ($shcommand) = @_;
  my ($res, $resline);
  open CMDRES, "$shcommand |";
  #autoflush CMDRES 1;
  while($resline = <CMDRES>){
    chop($resline);
    print ("\"$resline\"\n");
    gdio::putline ($resline);
  }
  gdio::putline ("");		# End of result
}

### General shell command and send back result
sub gd_basedir{
  my $base = gdparams::gdbase();
  print ("$base\n");
  gdio::putline ($base);
  gdio::putline ("");		# End of result
}



############################################################
### Database and Disc statistics
sub full_statistics{
  my $base = gdparams::gdbase();
  my ($sth, $row, $msg);

  gdio::putline ("DB statistics");
  ### Get # tracks
  $sth = $dbh->prepare("SELECT COUNT(*) AS cnt FROM tracks");
  $sth->execute;
  if($row = $sth->fetchrow_hashref){
    $msg = "  ".$row->{cnt}." Tracks";
    print "$msg \n";
    gdio::putline ($msg);
  }
  $sth->finish;

  ### Get # albums
  $sth = $dbh->prepare("SELECT COUNT(*) AS cnt FROM album");
  $sth->execute;
  if($row = $sth->fetchrow_hashref){
    $msg = "  ".$row->{cnt}." Albums";
    print "$msg \n";
    gdio::putline ($msg);
  }
  $sth->finish;

  gdio::putline ("  ");
  gdio::putline ("Disc statistics");
  gdio::putline (" (dir: used / free)");

  ### Get mp3 directories and check each directory
  my @mdir = gdparams::mp3dirs();
  my $i=0;
  my (@dfres, $totused, $totfree);

  $totused=0; $totfree=0;
  while($i < @mdir){
    if (-d "$base/$mdir[$i]"){
      @dfres = split / +/, `df -m $base/$mdir[$i]|tail -1`;
      $msg = "  ".$mdir[$i].": ".$dfres[2]."M / ".$dfres[3]."M";
      print "$msg \n";
      gdio::putline ($msg);
      $totused += $dfres[2];
      $totfree += $dfres[3];
    }
    else{print "$base/$mdir[$i] is not a directory or does not exist\n";}
    $i++;
  }

  $msg = " tot: ".$totused."M / ".$totfree."M";
  print "$msg \n";
  gdio::putline ($msg);

  gdio::putline ("");	# End of result
  
  ### "Side Effect": print player-id and -type
  print "\nStatus: playerid=$playerid, audiochannel=$audchannel0\n"; 

}


############################################################

sub server_alive_test{ # Ping
  print ("Ping: GiantDisc Server alive\n");
  gdio::putline ("GiantDisc Server alive");
  gdio::putline ("");	# End of result
}


sub serial_test{
  my $i;
  for($i=0; $i<5; $i++){
    print("$i ");
    gdio::putline(" 123456789 123456789 123456789 123456789 123456789 123456789 123456789");
  }
  gdio::putline ("");	# End of result
  print("\n");
}


END{
  ### close database connection
  print("### close database connection\n");
  $dbh->disconnect;

  if($rippipe_open){close_rippipe();}
}
#
1;
#
