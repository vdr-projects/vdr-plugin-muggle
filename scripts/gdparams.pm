##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2000, Rolf Brugger
#
##################################################

package gdparams;

#use lib '/usr/local/bin';
use strict;
use Getopt::Long;

my @mp3dirs;

############################################################
### Global variables (accessible from anywhere)

# IMPORTANT ERROR!  all modules refer to gdparms::varname instead
# of gdparams::varname
# ... however, it works ... I just don't know why  :-/


#my ($dbhost, $shutdowncmd, $extmp3player, $defrecbitrate,
#    $systemonline);


### Global constants
my $minfreehdspace = 1000;	# minimal space required for a directory, 
				# that it can be used to record and 
				# compress a cd (in MB)


############################################################
###

sub get_configfile_params{
  ### Parameters: Call-by-reference

  my ($dbhost, 
    $commmode,
    $serialdevice, $serialspeed,
    $tcpiphost, $tcpipport,
    $playerhost, $playertype, $snddevice, 
    $playerapp, $mp3playerparams, $oggplayerparams, $ptlogger, 
    $mp3encoder, $logtarget, $shutdowncmd,
    $extmp3player, $defrecbitrate, $systemonline
    ) 		= @_;

  
  ### Read configuration file im home directory
  open (CONF, "< ".gdbase()."/.gdconfig")
    or die "Error: could not open configuration file .gdconfig\n";
    
  my $line;
  while(<CONF>){
    $line = $_;
    chop $line;
    $line =~ tr/\r/ /;
	# \w word-char
	# \w non-word-char
	# \s whitespace-char
	# \S non-whitespace-char

    if($line =~ m/^dbhost.*=\s*(\S+)/i ){
      $$dbhost = $1;
    }

    if($line =~ m/^commmode.*=\s*([0-9]+)/i ){
      $$commmode = $1;
    }

    if($line =~ m/^serialdevice.*=\s*([\w\/]+)/i ){
      $$serialdevice = $1;
    }

    if($line =~ m/^serialspeed.*=\s*([0-9]+)/i ){
      $$serialspeed = $1;
    }

    if($line =~ m/^tcpiphost.*=\s*(\S+)/i ){
      $$tcpiphost = $1;
    }

    if($line =~ m/^tcpipport.*=\s*([0-9]+)/i ){
      $$tcpipport = $1;
    }

    if($line =~ m/^playerhost.*=\s*(\S+)/i ){
      $$playerhost = $1;
    }

    if($line =~ m/^playertype.*=\s*([0-9]+)/i ){
      $$playertype = $1;
    }

    if($line =~ m/^playerapp.*=\s*(\S+)/i ){
      $$playerapp = $1;
    }

    if($line =~ m/^playerparams.*=\s*(.*)$/i ){ # match anything to end of line
      print "\nWARNING: as of v1.20 the option 'playerparams' has been replaced by\n";
      print "         'mp3playerparams' and 'oggplayerparams'. Update .gdconfig accordingly\n\n";
      #$$playerparams = $1;
    }
    if($line =~ m/^mp3playerparams.*=\s*(.*)$/i ){ # match anything to end of line
      $$mp3playerparams = $1;
    }
    if($line =~ m/^oggplayerparams.*=\s*(.*)$/i ){ # match anything to end of line
      $$oggplayerparams = $1;
    }

    if($line =~ m/^shutdowncmd.*=\s*(.*)$/i ){ # match anything to end of line
      $$shutdowncmd = $1;
    }

    if($line =~ m/^extmp3player.*=\s*(.*)$/i ){ # match anything to end of line
      $$extmp3player = $1;
    }

    if($line =~ m/^sounddevice.*=\s*(\S+)/i ){
      $$snddevice = $1;
    }

    if($line =~ m/^mp3encoder.*=\s*(\w+)/i ){
      $$mp3encoder = $1;
    }

    if($line =~ m/^logtarget.*=\s*(\w+)/i ){
      if ($1 eq "stdout" || $1 eq "logfile" || $1 eq "devnull"){
        $$logtarget = $1;
      }
    }

    if($line =~ m/^ptlogger.*=\s*(\S+)/i ){
      $$ptlogger = $1;
    }

    if($line =~ m/^defrecbitrate.*=\s*(.+)/i ){ # match anything to end of line
      $$defrecbitrate = $1;
    }

    if($line =~ m/^systemonline.*=\s*([0-9])/i ){
      $$systemonline = $1;
    }

  }

  close (CONF);
}


############################################################
###

sub get_otherclients_params{
  ### Parameters: Call-by-reference

  my ($keymap  # type: reference to an empty hash
    )= @_;

  
  ### Read configuration file im home directory
  open (CONF, "< ".gdbase()."/.gdconfig")
    or die "Error: could not open configuration file .gdconfig\n";
    
  my $line;
  while(<CONF>){
    $line = $_;
    chop $line;
    $line =~ tr/\r/ /;
	# \w word-char
	# \w non-word-char
	# \s whitespace-char
	# \S non-whitespace-char

    if($line =~ m/^keymap.*=\s*(\S+)\s*-\s*(\S+)\s*$/i ){
      print ("keymap: key: $1, val: $2\n");
      $$keymap{$1} = $2;
    }


  }

  close (CONF);
}


############################################################
### translate logtarget string to integer
sub logtarget_to_int{
  my ($logtargetstr) = @_;
  if ($logtargetstr eq "devnull") {return 0;}
  if ($logtargetstr eq "logfile") {return 1;}
  if ($logtargetstr eq "stdout" ) {return 2;}
  return 2; # default
}


############################################################
###

sub get_commandline_params{

  my ($dbhost, 
    $commmode,
    $serialdevice, $serialspeed,
    $tcpiphost, $tcpipport,
    $playerhost, $playertype, $snddevice, 
    $playerapp, 
    $mp3playerparams, $oggplayerparams, 
    $ptlogger, 
    $mp3encoder, $logtarget) 		= @_;
	# ARGV passed implicitly


  my $help;
  $Getopt::Long::autoabbrev=1;
  GetOptions(
	"help" 		 => \$help, 
	"dbhost:s" 	 => $dbhost,	# $dbhost is already a reference
	"commmode:i"     => $commmode,
	"serialdevice:s" => $serialdevice,
	"serialspeed:i"  => $serialspeed,
	"tcpiphost:s"    => $tcpiphost,
	"tcpipport:i"    => $tcpipport,
	"playertype:i"   => $playertype,
	"playerhost:s"   => $playerhost,
	"sounddevice:s"  => $snddevice,
	"mp3playerparams=s" => $mp3playerparams,
	"oggplayerparams=s" => $oggplayerparams,
	"ptlogger:s"	 => $ptlogger,
	"mp3encoder:s"	 => $mp3encoder,
	"logtarget:s"	 => $logtarget
	);

  if ($help){
  print <<EOF;
gdd.pl: GiantDisc Server Script
Option         possible values  

--dbhost (string)          address or hostname of mysql database server
                           Default value: localhost    

--commmode (number)        Communication mode between Palm and server.
                           1: serial line RS232
                           2: generic TCP/IP (server - accepts incoming connections)
                           3: generic TCP/IP (client - connects to server)
                           Default value: 1

--serialdevice (string)    Device, where Palm is connected to. 
                           Only used if --commmode 1
                           Default value: /dev/ttyS0

--serialspeed  (number)    Communication speed over serial line in
                           Bits per second.
                           Only used if --commmode 1
                           Default value: 19200

--tcpiphost (string)       Host, that accepts socket connections.
                           Only used if --commmode 2 or 3
                           Default value: localhost
                                  
--tcpipport  (number)      Port used for TCP/IP communication mode.
                           Only used if --commmode 2 or 3
                           Default value: 26468

--playertype (number)      main audio out device type
                           0:  soundcard (first audio channel)
                           20: network attached audio streaming device
                           Default value: 0

--playerhost (string)      address or hostname where the audio stream
                           is sent to.
                           playertype 0:  host of soundcard (not used)
                           playertype 20: audio stream decoder host
                           Default value: localhost

--sounddevice  (string)    Device where audio stream is sent to.
                           playertype 0:  soundcard device (not used)
                           playertype 20: port number of TCP stream
                           Default value: 2020

--mp3playerparams (string) Optional parameters to the mp3 or ogg 
--oggplayerparams (string) decoder. Example: "--buffer 1024" to 
                           increase the audio output buffer size of
                           "mpg123".
                           Default value: 

--ptlogger     (string)    The application that updates at least
                           once per second the playtime of the
                           currently played track (needed by Palm
                           to display the playtime).
                           Default value: gdplatimefilter

--mp3encoder   lame,       Application to be used to encode ripped
               notlame,    wav files into mp3 format.
               l3enc       Default value: lame

--logtarget    stdout,     Target, where log-messages of the 
               logfile,    server script should be sent to. 
               or devnull  'stdout' sends to standard out, 'logfile'
                           sends to a log file in ~music/tmp and
                           devnull supresses all logging.
                           Default value: stdout

EOF
  exit 0;
  }
}


############################################################
###

sub gdbase{ # returns base path of gd files 
	    # The path must not end with a slash!
  return "/home/music";
}

sub mp3dirs{	# returns list of mp3 directories
		# Scans the music directory for 2-digit directories

  opendir MUSICDIR, gdbase() or die "ERROR: can't scan music directory\n";
  @mp3dirs = grep /^[0-9][0-9]$/ , readdir MUSICDIR;
  closedir MUSICDIR;
  #print "MP3dirs: ",@mp3dirs, "\n";
  return ( @mp3dirs );
}




1;
#
