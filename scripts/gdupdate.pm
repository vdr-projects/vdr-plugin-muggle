##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2000-2003, Rolf Brugger
#
##################################################


# Package for database modifications and consistency checks related
# to version updates


package gdupdate;

use strict;


BEGIN{
}


###############################################################################
### Version 1.32
###############################################################################

sub db_check_update_132{
  my ($dbh) = @_;
  my ($sth, $count, $res);

  my $update = 0;
  
  ### new field 'audio channel'
  $res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'audiochannel'");
  if ($res < 1){
    $update = 1;
  }

  $res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'processid'");
  if ($res < 1){
    $update = 1;
  }


  return $update;
}


sub db_update_132{
  my ($dbh) = @_;
  my ($sth, $count, $res);

  ### usage frequencies
  $res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'audiochannel'");
  if ($res < 1){
    print("Update table playerstate to version 1.32\n");
    print("rename index column playertype into audiochannel\n");
    $dbh->do("ALTER TABLE playerstate CHANGE playertype audiochannel INT NOT NULL");
    print("Adding field playertype to table playerstate\n");
    $dbh->do("ALTER TABLE playerstate ADD COLUMN playertype INT AFTER audiochannel");    
  }
  
  ### player process id
  $res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'processid'");
  if ($res < 1){
    print("Update table playerstate to version 1.12\n");
    print("Adding field processid to table playerstate\n");
    $dbh->do("ALTER TABLE playerstate ADD COLUMN processid  INT AFTER audiochannel");
  }
  
}


###############################################################################
### Version 1.31
###############################################################################

sub db_check_update_131{
  my ($dbh) = @_;
  my ($sth, $count, $res);

  my $update = 0;
  
  ### usage frequencies
  $res = $dbh->do("SHOW COLUMNS FROM album LIKE 'genre'");
  if ($res < 1){
    $update = 1;
  }
  
  ### Optimizations
  my $row;
  $sth = $dbh->prepare("SHOW TABLE STATUS FROM GiantDisc LIKE 'playerstate'");
  $count = $sth->execute;
  if($row = $sth->fetchrow_hashref){
    if ($row->{Type} ne "HEAP"){
      $update = 1;
    }
  }
  $sth->finish;
  

  my ($dbh, $table, $column, $indexspec) = @_;

  my ($sth, $count, $row);
  $sth = $dbh->prepare("SHOW index FROM tracks");
  $count = $sth->execute;
  while($row = $sth->fetchrow_hashref){
    last if ($row->{Key_name} eq "artist");
  }
  if($row->{Key_name} eq "artist"){
    #print "index artist exists\n";
    ;
  }
  else{
    print "Alert: no additional indexes defined\n";
    print "       consider upgrading the db-structure with 'gdupdatedb.pl'\n";
  }
  $sth->finish;


  return $update;
}


sub db_update_131{
  my ($dbh) = @_;
  my ($sth, $count, $res);

  ### usage frequencies
  $res = $dbh->do("SHOW COLUMNS FROM album LIKE 'genre'");
  if ($res < 1){
    print("Update table album to version 1.31\n");
    print("Adding field genre to table album\n");
    $dbh->do("ALTER TABLE album ADD COLUMN genre VARCHAR(10) AFTER modified");
  }
  
  ### Optimizations
  my $row;
  $sth = $dbh->prepare("SHOW TABLE STATUS FROM GiantDisc LIKE 'playerstate'");
  $count = $sth->execute;
  if($row = $sth->fetchrow_hashref){
    if ($row->{Type} ne "HEAP"){
      print "Set table 'playerstate' to type HEAP\n";
      $dbh->do("ALTER TABLE playerstate TYPE=HEAP");
    }
  }
  $sth->finish;
  

  test_and_add_index($dbh, "tracks", "artist",  "artist(artist(10))");
  test_and_add_index($dbh, "tracks", "title",   "title(title(10))");
  test_and_add_index($dbh, "tracks", "genre1",  "(genre1)");
  test_and_add_index($dbh, "tracks", "genre2",  "(genre2)");
  test_and_add_index($dbh, "tracks", "year",    "(year)");
  test_and_add_index($dbh, "tracks", "lang",    "(lang)");
  test_and_add_index($dbh, "tracks", "type",    "(type)");
  test_and_add_index($dbh, "tracks", "rating",  "(rating)");
  test_and_add_index($dbh, "tracks", "sourceid","(sourceid)");
  test_and_add_index($dbh, "tracks", "mp3file", "mp3file(mp3file(10))");

  test_and_add_index($dbh, "album", "artist",   "artist(artist(10))");
  test_and_add_index($dbh, "album", "title",    "title(title(10))");
  test_and_add_index($dbh, "album", "genre",    "(genre)");
  test_and_add_index($dbh, "album", "modified", "(modified)");
  
}

sub test_and_add_index
{
  my ($dbh, $table, $column, $indexspec) = @_;

  my ($sth, $count, $row);
  $sth = $dbh->prepare("SHOW index FROM $table");
  $count = $sth->execute;
  while($row = $sth->fetchrow_hashref){
    last if ($row->{Key_name} eq $column);
  }
  if($row->{Key_name} eq $column){
    #print "index $column exists\n";
    ;
  }
  else{
    print "creating index $column\n";
    $dbh->do("ALTER TABLE $table ADD INDEX $indexspec");
  }
  $sth->finish;
}


###############################################################################
### Version 1.14
###############################################################################

sub db_update_114{
  my ($dbh) = @_;
  my ($sth, $count, $res);

  ### bitrate
  $sth = $dbh->prepare("SELECT id FROM tracks WHERE length(bitrate)<4");
  $count = $sth->execute;
  if ($count > 0){
    print("Update table tracks to version 1.14\n");
    print("enlarging field bitrate to 10 characters\n");
    $dbh->do("ALTER TABLE tracks MODIFY COLUMN bitrate VARCHAR(10)");

    ### add prefix "mp3 " to all bitrate fields of mp3 tracks
    $res=print("add prefix \"mp3 \" to all bitrate fields of mp3 tracks\n");
    $dbh->do("UPDATE tracks SET bitrate=CONCAT('mp3 ',bitrate) "
            ."WHERE length(bitrate)<4 AND ( mp3file LIKE '%.mp3'"
            ."                           OR mp3file LIKE 'http%')");
    print "mp3 track records updated\n";
    $dbh->do("UPDATE tracks SET bitrate=CONCAT('ogg ',bitrate) "
            ."WHERE length(bitrate)<4 AND mp3file LIKE '%.ogg'");
    
    ### check result
    $sth = $dbh->prepare("SELECT id FROM tracks WHERE length(bitrate)<4");
    $count = $sth->execute;
    if ($count > 0){
      print "\n";
      print "Warning: some track records could not be properly translated.\n";
      print "         You might have tracks in your database with no audio-\n";
      print "         file associated!\n\n";
    }
  }
  $sth->finish;
}



###############################################################################
### Version 1.12
###############################################################################

sub db_update_112{
  my ($dbh) = @_;
  my $res;

  ### add player process id  -> killing pid with killall and killfam is unstable and too slow!
  #$res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'processid'");
  #if ($res < 1){
  #  print("Update table playerstate to version 1.12\n");
  #  print("Adding field processid to table playerstate\n");
  #  $dbh->do("ALTER TABLE playerstate ADD COLUMN processid INT AFTER snddevice");
  #}
  
  ### usage frequencies
  $res = $dbh->do("SHOW COLUMNS FROM language LIKE 'freq'");
  if ($res < 1){
    print("Update table language to version 1.12\n");
    print("Adding field freq to table language\n");
    $dbh->do("ALTER TABLE language ADD COLUMN freq  INT AFTER language");
  }
  $res = $dbh->do("SHOW COLUMNS FROM genre LIKE 'freq'");
  if ($res < 1){
    print("Update table genre to version 1.12\n");
    print("Adding field freq to table genre\n");
    $dbh->do("ALTER TABLE genre ADD COLUMN freq  INT AFTER genre");
  }
#else {print("-- column modified exists -> DB needs not be updated\n");}
}



###############################################################################
### Version 1.11
###############################################################################

sub db_update_111{
  my ($dbh) = @_;
  my $res;

  ### Table recordingitem
  $res = $dbh->do("SHOW TABLES LIKE 'recordingitem'");
  if ($res < 1){
    print("recordingitem does not exist (upgrading...)\n");
    $dbh->do(
	"create table recordingitem("
	."trackid	int,"
	."recdate	date,"
	."rectime	time,"
	."reclength	int,"
	."enddate	date,"
	."endtime	time,"
	."repeat	varchar(10),"
	."initcmd	varchar(255),"
	."parameters	varchar(255),"
	."atqjob	int,"
	."id		int not null,"
	."primary key(id)"
	.")");
  }
#else{print("-- recordingitem does exist\n");}
}



###############################################################################
### Version 0.97
###############################################################################

sub db_update_097{
  my ($dbh) = @_;
  my $res;

  ### shuffle parameter
  $res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'shufflepar'");
  if ($res < 1){
    print("Update table album to version 0.97\n");
    print("Adding modified field shufflepar,shufflestat to table playerstate\n");
    $dbh->do("ALTER TABLE playerstate ADD COLUMN shufflepar  varchar(255) AFTER state");
    $dbh->do("ALTER TABLE playerstate ADD COLUMN shufflestat varchar(255) AFTER shufflepar");
  }
#else {print("-- column modified exists -> DB needs not be updated\n");}
}



###############################################################################
### Version 0.96
###############################################################################

sub db_update_096{
  my ($dbh) = @_;
  my $res;

  ### album modification time
  $res = $dbh->do("SHOW COLUMNS FROM album LIKE 'modified'");
  if ($res < 1){
    print("Update table album to version 0.96\n");
    print("Adding modified field to table album\n");
    $dbh->do("ALTER TABLE album ADD COLUMN modified date AFTER covertxt");
  }
#else {print("-- column modified exists -> DB needs not be updated\n");}
}


###############################################################################
### Version 0.95
###############################################################################

sub db_update_095{
  my ($dbh) = @_;
  my $res;

  ### anchortime
  $res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'anchortime'");
  if ($res < 1){
    print("Update table playerstate to version 0.95\n");
    print("Adding anchortime field to table playerstate\n");
    $dbh->do("ALTER TABLE playerstate ADD COLUMN anchortime bigint AFTER framesremain");
  }
#else {print("-- column anchortime exists -> DB needs not be updated\n");}

  ### framestotal
  $res = $dbh->do("SHOW COLUMNS FROM playerstate LIKE 'framestotal'");
  if ($res < 1){
    print("Update table playerstate to version 0.95\n");
    print("Renaming framesremain field to framestotal\n");
    $dbh->do("ALTER TABLE playerstate CHANGE framesremain framestotal INT");
  }
#else {print("-- column framestotal exists -> DB needs not be updated\n");}
}


###############################################################################
### Version 0.94
###############################################################################

sub check_new_mp3info080{
# This routine checks if the version of mp3info is at least 0.8.0, which
# is required as of GD-version 0.94
# A warning message is printed if mp3info should be updated
  
  my $infostr = `mp3info`;
  if ($infostr =~ /MP3Info\D*([0-9]*).([0-9]*).([0-9]*)/){
    if ($2 < 8){
      print ("\n\n");
      print ("Warning: The Version of 'mp3info' on your system is $1.$2.$3\n");
      print ("         At least version 0.8.0 is required. You can get it\n");
      print ("         from http://www.ibiblio.org/mp3info\n");
      print ("\n\n");
      exit(0);
    }
  }
  else{
    print ("Warning: could not extract version number of 'mp3info'\n");
    print ("         'mp3info' is not installed?\n");
    exit(0);
  }
}



sub db_update_094{
  my ($dbh) = @_;
  my $res;

  ### Bitrate
  $res = $dbh->do("SHOW COLUMNS FROM tracks LIKE 'bitrate'");
  if ($res < 1){
    print("Update table tracks to version 0.94\n");
    print("Adding bitrate field to table tracks\n");
    $dbh->do("ALTER TABLE tracks ADD COLUMN bitrate CHAR(4) AFTER lyrics");
    
    ### Update records
    my $base = gdparams::gdbase();
    my ($sth, $count, $row, $fname, $bitrate);
    $sth = $dbh->prepare("SELECT * FROM tracks WHERE bitrate IS NULL OR bitrate=''");
    $count = $sth->execute;
    print("I have to update the bitrate of $count records\n");
    while($row = $sth->fetchrow_hashref){
      $fname = `ls $base/[0-9][0-9]/$row->{mp3file}`; # get full path
      chop($fname);
      $bitrate = gdgentools::get_bitrate_str($fname);
      print("Set bitrate of $row->{artist}/$row->{title} to $bitrate\n");
      $dbh->do( "UPDATE tracks SET bitrate='$bitrate' WHERE id=$row->{id}");

    }
    $sth->finish;    
  }
#else {print("-- column bitrate exists -> DB needs not be updated\n");}


  ### Table player
  $res = $dbh->do("SHOW TABLES LIKE 'player'");
  if ($res < 1){
    print("player does not exist (upgrading...)\n");
    $dbh->do(
	"create table player( "
	."ipaddr	varchar(255) not null,"
	."uichannel	varchar(255) not null,"
	."logtarget	int,"
	."cdripper	varchar(255),"
	."mp3encoder	varchar(255),"
	."cdromdev	varchar(255),"
	."cdrwdev	varchar(255),"
	."id		int not null,"
	."primary key(id)"
	.")");
  }
#else{print("-- player does exist\n");}


  ### Table playerstate
  $res = $dbh->do("SHOW TABLES LIKE 'playerstate'");
  if ($res < 1){
    print("playerstate does not exist (upgrading...)\n");
    $dbh->do(
	"create table playerstate("
	."playerid	int not null,"
	."playertype	int not null,"
	."snddevice	varchar(255),"
	."playerapp	varchar(255),"
	."playerparams	varchar(255),"
	."ptlogger	varchar(255),"
	."currtracknb	int,"
	."state		varchar(4),"
	."pauseframe	int,	"
	."framesplayed	int,"
	."framesremain	int,"
	."primary key(playerid, playertype)"
	.")");
  }
#else{print("-- playerstate does exist\n");}


  ### Table tracklistitem
  $res = $dbh->do("SHOW TABLES LIKE 'tracklistitem'");
  if ($res < 1){
    print("tracklistitem does not exist (upgrading...)\n");
    $dbh->do(
	"create table tracklistitem("
	."playerid	int not null,"
	."listtype	smallint not null,"
	."tracknb	int not null,"
	."trackid	int not null,"
	."primary key(playerid, listtype, tracknb)"
	.")");
  }
#else{print("-- tracklistitem does exist\n");}

}

###############################################################################
### Version 0.92
###############################################################################

sub fix_leading_slash_bug
### removes leading / in the column tracks.mp3file
{
  my ($dbh) = @_;
  my $numrec = $dbh->do( "UPDATE tracks SET mp3file=SUBSTRING(mp3file,2) "
  			."WHERE mp3file LIKE '/%'");
  if ($numrec>0){
    print("fix_leading_slash_bug: $numrec records fixed!\n\n");
  }
}



###############################################################################

END{
  ;
}


#
1;
