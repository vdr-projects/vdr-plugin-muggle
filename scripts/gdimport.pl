#!/usr/bin/perl

##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2000, Rolf Brugger
#
##################################################


# Import script for mp3 tracks.
#
# Non interactive batch import of mp3 tracks. Mp3 informations (title, artist, etc)
# are read from the id3-tags. Well defined id3-tags are mandatory for a
# inclusion of mp3 tracks and a proper update of the database.


use lib '/home/lvw/Development/muggle-plugin/scripts';
#use lib '/home/andi/muggle/import';
use gdparams;
use gdgentools;
use gddb;
use DBI;
use Cwd;	# get current working dir

### main
  #print ("Args are: @ARGV \n");
  if ($#ARGV < 0){
    print "\nusage: gdimport.pl [options] audiofiles... \n";
    print "\nimports the specified audio files into the GiantDisc system.\n";
    print "It reads the associated metadata (id3/vorbis tags) of each file and \n";
    print "creates a track record for each of it. \n";
    print "Other than in GD, files are not copied!!\n";
    print "\n";
    print "Currently supported file formats: mp3, ogg vorbis, flac.\n";
    print "\n";
    print "Instead of a set of audio files a directory can be specified. In\n";
    print "this case, all audio tracks in the directory are imported. This is\n";
    print "particularly useful to import a bunch of albums with the following\n";
    print "command typed in the shell:\n";
    print "\n";
    print "  find . -type d -print -exec gdimport.pl -a {} \\;\n";
    print "\n";
    print "Options:\n";
    print "  -t  Test. Just display what would be done, whithout changing\n";
    print "      the database or copying/moving/linking files\n";
    print "      It is recommended to test an import session before really doing it\n";
    print "\n";
    print "  -a  Interpret the audio files as tracks of an album. An album record\n";
    print "      is created and the tracks are imported as usual.\n";
    print "    Note: - The first audio file must have a valid album metadata tag\n";
    print "            (i.e. id3 tag) to get the album title.\n";
    print "          - only one album can be imported at a time\n";
    print "          - default values: rating=-, type=1, source=CD.\n";
    print "\n";
    print "  -i cddbid  importdir\n";
    print "      Import all jpg images of the directory 'importdir' and associate\n";
    print "      them to the album specified by 'cddbid' (include prefix '0x', 'xx'!). \n";
    print "\n";
    print "\n";
    exit;
  }

  #exit;
  ### Init constants/variables
  my ($fileformat, $cddbid);
  my $makealbum = 0;
  my $album_created = 0;
  my $linkfiles = 1;
  my $movefiles = 0;
  my $moveimages = 0;
  my $test = 0;
  my $tracknb = 1;
  my ($dbh, $trifile);
  my $base = gdparams::gdbase();		# music files base

  ### Open database connection
  print ("### Open database connection\n");
  $dbh = DBI->connect("DBI:mysql:GiantDisc", "music", undef)
    or die "unable to connect to GiantDisc db";

  ### init other variables
  my $fileid = gdgentools::last_imported_tracknb($dbh);
  my @trackrow;
  
  
  ### extract all options first
  my $shifted=1;
  while ($shifted){
    if ($ARGV[0] eq "-a"){
      print "set option make album\n";
      $makealbum = 1;
      shift @ARGV;
    }
    elsif ($ARGV[0] eq "-l"){
      print "set option link files\n";
      $linkfiles = 1;
      shift @ARGV;
    }
    elsif ($ARGV[0] eq "-m"){
      print "set option move files\n";
      $movefiles = 1;
      shift @ARGV;
    }
    elsif ($ARGV[0] eq "-i"){
      print "import booklet images\n";
      $moveimages = 1;
      shift @ARGV;
      $cddbid = shift @ARGV;
    }
    elsif ($ARGV[0] eq "-t"){
      print "set option TEST\n";
      $test = 1;
      shift @ARGV;
    }
    else {
      $shifted = 0;
    }
  }
  
  if ($moveimages){
    $dirname = $ARGV[0];
    if (-d $dirname){
      # ok. It's a directory
      gdgentools::import_cover_images($dbh, $dirname, $cddbid, $test);
    }
    else{
      print "Error importing images: '$dirname' is not a directory\n";
    }
    exit;
  }
  
  
  my @musicfiles;
  my $dirname="";
  ### Is the first file argument a directory?
  if (-d $ARGV[0]){
    # It's a directory -> read the audio filenames in it
    $dirname = $ARGV[0];
    opendir CURDIR, $dirname;
    @musicfiles = grep /[\.f](mp3$)|(ogg$)|(flac$)/, readdir CURDIR; # match .mp3, .ogg, flac  --  Thanks to Merlot!
#    @musicfiles = grep /[\.f][Mmol][Ppga][c3g]$/, readdir CURDIR; # match .mp3, .ogg, flac
    @musicfiles = sort @musicfiles;
    closedir CURDIR;
    chdir $dirname;
  }
  else{
    # the remaining arguments seem to be audio filenames
    @musicfiles = @ARGV;
  }
  
  ### Loop through audio files
  while ($curfile = shift @musicfiles){
    $fileformat = gdgentools::audio_filetype($curfile);
    print "Importing $fileformat file: $curfile\n";
    $fileid += 1;
#    $audiofile = sprintf("trxx%08ld.%s", $fileid, $fileformat);
    $audiofile = $dirname ."/". $curfile;

    # get info from metatags
    $trackrow->{title}     = gdgentools::audiofile_title($curfile);
    $trackrow->{artist}    = gdgentools::audiofile_artist($curfile);
    $trackrow->{genre2}    = "";
    $trackrow->{year}      = gdgentools::audiofile_year($curfile);
    $trackrow->{lang}      = "-";
    $trackrow->{type}      = 1; # medium
    $trackrow->{rating}    = 0;
    $trackrow->{length}    = gdgentools::audiofile_lengthsec($curfile);
    $trackrow->{source}    = 0;  # CD
    if(!$album_created){
      if($makealbum && !$album_created){
        $trackrow->{sourceid}  = sprintf("%08ld", $fileid); # fake CDDB-ID 
        $albumtitle = gdgentools::audiofile_album($curfile);
        if (length($albumtitle)<1){
          $albumtitle = $dirname;
        }
      }
      else{
        $trackrow->{sourceid}  = ""; # no CDDB-ID available
      }
    }
    $trackrow->{tracknb}   = gdgentools::audiofile_tracknumber($curfile, $tracknb);
    $trackrow->{mp3file}   = $audiofile;
    $trackrow->{condition} = 0;  # OK
    $trackrow->{voladjust} = 0;
    $trackrow->{lengthfrm} = 0;
    $trackrow->{startfrm}  = 0;
    $trackrow->{bpm}       = 0;
    $trackrow->{bitrate}   = $fileformat." ".gdgentools::audiofile_bitrate($curfile);
    $trackrow->{created}   = "";
    $trackrow->{modified}  = "";
    $trackrow->{id}        = ""; # a new one will be automatically generated

    # get genre if available and translate to gd-genre
    $trackrow->{genre1} = gdgentools::audiofile_genre($dbh, $curfile);

    if (length($trackrow->{title})>0 && length($trackrow->{artist})>0){
      ### create track record

        print "Insert track in database:$trackrow->{artist} - $trackrow->{title} \n";
        if (!$test){
          if (-e $curfile){
            $trid = gddb::insert_track_record($dbh,
		$trackrow->{artist},$trackrow->{title},
		$trackrow->{genre1},$trackrow->{genre2},$trackrow->{year},
		$trackrow->{lang},$trackrow->{type},$trackrow->{rating},
		$trackrow->{length},$trackrow->{source},$trackrow->{sourceid},
		$trackrow->{tracknb},$trackrow->{mp3file},
		$trackrow->{condition},$trackrow->{voladjust},
		$trackrow->{lengthfrm},$trackrow->{startfrm},$trackrow->{bpm},
		$trackrow->{bitrate},
		$trackrow->{created},$trackrow->{id});
	  }
	  else{
	    print "Error: file ".$trackrow->{mp3file}." dows not exist. Skipping\n";
	  }
	}
	else{
        print "TEST>".join ':',	" art",$trackrow->{artist}," tit",$trackrow->{title},		," gen1",$trackrow->{genre1}," gen2",$trackrow->{genre2}," year",$trackrow->{year},		" lang",$trackrow->{lang}," tp",$trackrow->{type}," ratng",$trackrow->{rating},		" len",$trackrow->{length}," src",$trackrow->{source}," cddbid",$trackrow->{sourceid},		" trnb",$trackrow->{tracknb}," file",$trackrow->{mp3file},		" cond",$trackrow->{condition}," vol",$trackrow->{voladjust},		" lenfrm",$trackrow->{lengthfrm}," strt",$trackrow->{startfrm}," bpm",$trackrow->{bpm},		" bitrate",$trackrow->{bitrate},		" created",$trackrow->{created}," id",$trackrow->{id},"\n\n";
	}
	
 	### create album record
        if ($makealbum && !$album_created && length($albumtitle)>0){
          print "Insert album in database:$trackrow->{artist} - $albumtitle \n";
          if (!$test){
            $dbh->do("INSERT INTO album (artist,title,modified,cddbid) "
                ."VALUES (".$dbh->quote($trackrow->{artist})
                .",".$dbh->quote($albumtitle)
                .",CURDATE() "
                .",'".$trackrow->{sourceid}."') ");
          }
          $album_created = 1;
        }
	
	### put mp3file to target directory
        $tracknb++;
    }
    else{
      print "No title or artist available for $curfile (skipped)\n";
    }

  }#end loop
  print("### close database connection\n");
  $dbh->disconnect;

  exit;



###############################################################################




