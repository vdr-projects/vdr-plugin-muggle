package gdgentools;

##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2000, Rolf Brugger
#
##################################################



### General tool routines

use lib '/home/lvw/Development/muggle-import/scripts';
use gdparams;
use IO::Socket;


### Constants
my $pl_list = 0;	# play list
my $rp_list = 1;	# rip list
my $co_list = 2;	# compression list
my $cr_list = 3;	# cd recording list





#use lib '/usr/local/bin';
#BEGIN{;}
#END{;}


############################################################
### Shuffle routine
sub fisher_yates_shuffle {# generate a random permutation of @array in place
    my $array = shift;
    my $i;
    for ($i = @$array; --$i; ) {
        my $j = int rand ($i+1);
        next if $i == $j;
        @$array[$i,$j] = @$array[$j,$i];
    }
}
# USAGE:
# fisher_yates_shuffle( \@array );    # permutes @array in placesub query_random_artists{ 





############################################################
### barix exstreamer routines

sub exstreamer_command{
  # sends a command string to the exstreamer at 'playerhost', without 
  # waiting for a result that is sent back by the exstreamer
  
  my ($playerhost, $command) = @_; 
  my $port = 12302;    # default exstreamer tcp command port
  my $sock = new IO::Socket::INET( PeerAddr => $playerhost,
                                   PeerPort => $port,
                                   Proto    => 'tcp');
  die "Error: could not open socket $opt_h:$port. $!\n" unless $sock;
  
  ### send the command
  print $sock $command."\n";
  close($sock);
}


sub exstreamer_command_res{
  # sends a command string to the exstreamer at 'playerhost'. 
  # It returns the first line that is sent back by the exstreamer (last \n chopped off)
  
  my ($playerhost, $command) = @_; 
  my $port = 12302;    # default exstreamer tcp command port
  my $sock = new IO::Socket::INET( PeerAddr => $playerhost,
                                   PeerPort => $port,
                                   Proto    => 'tcp');
  die "Error: could not open socket $opt_h:$port. $!\n" unless $sock;
  
  ### send a command
  print $sock $command."\n";
  my $res = <$sock>;
  close($sock);
  chop $res;
  return $res;
}


############################################################
### Playlist routines


# creates new playstate record. If one with the same playerid/audiochannel
# exists, it is overwritten.
sub pll_new_playstate{
  my ($dbh, $playerid, $audiochannel, $playertype, $snddevice, $playerapp, $playerparams, $ptlogger) = @_; 
  $dbh->do("REPLACE INTO playerstate SET "
    ."playerid=$playerid, "
    ."audiochannel=$audiochannel, "
    ."playertype=$playertype, "
    ."snddevice='$snddevice', "
    ."playerapp='$playerapp', "
    ."playerparams='$playerparams', "
    ."ptlogger='$ptlogger', "
    ."currtracknb=0, "
    ."state='st', "	# stopped
    ."framesplayed=0, "
    ."framestotal=0 "
    ); 
}      
       
# deletes playstate record
sub pll_del_playstate{
  my ($dbh, $playerid, $audiochannel) = @_; #parameters: database handle, player id, sound out it
  $dbh->do("REPLACE FROM playerstate "
    ."WHERE playerid=$playerid AND audiochannel=$audiochannel ");
}      

       
# Returns th specified player-type
sub pll_get_playertype{
  my ($dbh, $playerid, $audiochannel) = @_; #parameters: database handle, player id, sound out it
       
  my $sth = $dbh->prepare( 
         "SELECT playertype,ipaddr,snddevice   FROM  player,playerstate "
        ."WHERE player.id=playerstate.playerid "
        ."  AND player.id=$playerid "
        ."  AND playerstate.audiochannel=$audiochannel");

  my $nbrec = $sth->execute;
  #print("$nbrec playstates found (should be exactly 1)\n");

  my ($playertype,$playerhost,$snddevice);

  if($row = $sth->fetchrow_hashref){
    $playertype = $row->{playertype};
    $playerhost = $row->{ipaddr};
    $snddevice  = $row->{snddevice};
  }
  else{
    ### This case should not happen! make the best out of it
    print ("ERROR: can't get playertype for player $playerid channel $audiochannel\n");
    print ("       no player/playerstate record found\n");
  }
  $sth->finish;

  return ($playertype,$playerhost,$snddevice);
}


# Returns the current playstate
sub pll_get_playstate{
  my ($dbh, $playerid, $audiochannel) = @_; #parameters: database handle, player id, sound out it
       
  my $sth = $dbh->prepare( 
  	 "SELECT * FROM playerstate "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel" );

  my $nbrec = $sth->execute;
  #print("$nbrec playstates found (should be exactly 1)\n");

  my $currtracknb = 0;
  my $state       = "st";
  my $shufflestat = "";

  if($row = $sth->fetchrow_hashref){
    $currtracknb = $row->{currtracknb};
    $state       = $row->{state};
    $framesplayed= $row->{framesplayed};
    $shufflestat = $row->{shufflestat};
  }
  else{
    ### This case should not happen! make the best out of it
    #pll_new_playstate($dbh, $playerid, $audiochannel, "", "", "");
    print ("ERROR: can't get playstate for player $playerid channel $audiochannel\n");
    print ("       no playerstate record found\n");
  }
  $sth->finish;

  return ($currtracknb, $state, $framesplayed, $shufflestat);
}


# Returns 
sub playerdefinition{
  my ($dbh, $playerid, $audiochannel) = @_; #parameters: database handle, player id, sound out it
       
  my $sth = $dbh->prepare( 
         "SELECT playertype, ipaddr, snddevice FROM player,playerstate "
        ."WHERE player.id=$playerid AND playerstate.playerid=$playerid AND audiochannel=$audiochannel");

  my $nbrec = $sth->execute;
  #print("$nbrec playstates found (should be exactly 1)\n");

  my ($playertype, $playerhost, $sounddevice);
  my $playertype = 0;
  my $playerhost = "localhost";
  my $sounddevice = "/dev/dsp";

  if($row = $sth->fetchrow_hashref){
    $playertype  = $row->{playertype};
    $playerhost  = $row->{ipaddr};
    $sounddevice = $row->{snddevice};
  }
  else{
    ### This case should not happen! 
    print ("ERROR: can't get player definition for player $playerid channel $audiochannel\n");
    print ("       no playerstate record found\n");
  }
  $sth->finish;

  return ($playertype, $playerhost, $sounddevice);
}


# Returns the process id of the player (that was previously stored in the DB)
sub pll_get_player_processid{
  my ($dbh, $playerid, $audiochannel) = @_; #parameters: database handle, player id, sound out it
  my $sth = $dbh->prepare( 
  	 "SELECT processid FROM playerstate "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel" );
  my $nbrec = $sth->execute;
  my $playerpid = -1;
  if($row = $sth->fetchrow_hashref){
    $playerpid = $row->{processid};
  }
  else{
    ### This case should not happen! make the best out of it
    print ("ERROR: can't get playstate for player $playerid channel $audiochannel\n       no playerstate record found\n");
  }
  $sth->finish;
  return ($playerpid);
}



# Writes the playstate to the playstate record
# Parameters: 
#	currtracknb 
#	state 	    (one of: pl, st, in, ff, Ff, rw, Rw)      [in=pause(interrupted)]
sub pll_write_playstate{
  my ($dbh, $playerid, $audiochannel, $currtracknb, $state) = @_;
  my $retval = $dbh->do(
  	 "UPDATE playerstate "
  	."SET currtracknb=$currtracknb, state='$state' "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel");
}


# Writes the players process id to the playstate record
# The programs 'gdplayd.pl' and 'gdplaytmsim.pl' write their process id into 
# the DB. This is necessary to efficiently stop playing or rewinding.
sub pll_write_player_processid{
  my ($dbh, $playerid, $audiochannel, $playerpid) = @_;
    my $retval = $dbh->do(
  	 "UPDATE playerstate "
  	."SET processid=$playerpid "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel");
}

#sub pll_clear_player_processid{
#  my ($dbh, $playerid, $audiochannel, $playerpid) = @_;
#  if ($gdparms::multiclients){
##print "clearing pid \n";
#    my $retval = $dbh->do(
#  	 "UPDATE playerstate "
#  	."SET processid=0 "
#  	."WHERE playerid=$playerid AND audiochannel=$audiochannel");
#  }
#}


### Shuffle Play Parameters
sub pll_write_shuffleparameters{
  my ($dbh, $playerid, $audiochannel, $parameterstring, $statisticsstring) = @_;
  print("writing shuffleparamstring: '$parameterstring'\n");
  my $retval = $dbh->do(
  	 "UPDATE playerstate "
  	."SET shufflepar =".$dbh->quote($parameterstring).", "
  	."    shufflestat=".$dbh->quote($statisticsstring)." "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel");
}



#######################################################################
### Stop playing
#
# Stopping to play means killing the specifig playing-deamon 'gdplayd.pl'
# and all the sub-processes it has spawned.
#
# As long as only one instance of a GiantDisc server is running on a
# machine, we can just blindly kill all potentially spawned processes by
# name (see routine 'kill_all_play_processes').
#
# If we have more than one server instance on the same machine, this blindly
# killing method doesn't work anymore. Stopping to play on one instance would
# terminate the play processes of all other server instances. It is therefore
# necessary to specifically kill the involved processes. I have tried many 
# (really a lot of) approaches during 2 years - nothing really worked well.
# There were 2 main problems:
#   - killing the processes was far too slow
#   - multiple instances of gdplayd.pl and it sub-processes appeared leading
#     to locked soundcards etc. ( This especially happened, when playing was 
#     quickly stopped and restarted, or after fast sequences of 'play next'
#     commands.
#
# The currently adopted however method seems to work fine, it is robust and 
# efficient enough. It is explained in detail below, see routines plmsg_...
#


### tested and not well working: Killall and Killfam!
### Comment: Proc::Killall is terribly slow (much slower than system("killall..."))
###          using Proc::Killfam is unstable because of overlapping playing
###          commands, that inevitably lead to multiple instances of gdplayd.pl
###          (mainly because pid of gdplayd.pl is known too late?)
###          Finally - Proc::Killfam is too slow too!


sub stop_play_processes{
  
  my ($dbh, $playerid, $audiochannel) = @_;
  my $new_kill_player = 1;
#  if ($gdparms::multiclients){
  if ($new_kill_player){
    my $playerpid = pll_get_player_processid($dbh, $playerid, $audiochannel);

    if ($playerpid >0){
      ### get all child processes of $playerpid
      my $chprocs = `pstree $playerpid -p |tr -d '\012'`;  # get process id's, on one text line
      my @chpids = split /\(/, $chprocs;
      shift @chpids;  # pop off first element
      foreach $elem (@chpids){
      	$elem =~ s/\).*//;
      }
      
      if (scalar(@chpids)>0){
        #print "  specifically killing ".join (",",@chpids)."\n";
        system "kill ".join (" ",@chpids);
      }
    }
  }
  else{
    kill_all_play_processes();
  }
}


sub kill_all_play_processes{
  #print "killing blindly\n";
  system("killall -q gdplayd.pl; "
        ."killall -q mpg123; "
        ."killall -q ogg123; "
        ."killall -q flac; "
        ."killall -q rawplay; "
        ."killall -q gdplaytmsim.pl; "
        ."killall -q gdstream.pl");

  #killall and killfam are terribly slow!
}


#######################################################################
### IPC Message routines to make sure, that a stop operation really kills
#   all player processes.
#   Method:
#     1) the server starts the player 'gdplayd.pl' in background
#     2) right after starting gdplayd.pl, the server waits for a message of 
#        gdplayd.pl
#     3) in the init phase of gdplayd.pl, it writes it's own process-id to the
#        database and sends then a message to the message queue
#     4) the server receives the message and can continue to process requests
#        from the palm
#     5) when the server should now stop playing, it is 100% sure, that the
#        correct process-id o the playing process is in the db.
#        Killing it (and it's subprocesses) stops playing.

use IPC::SysV qw(IPC_CREAT S_IRWXU ftok);
use IPC::Msg;

# Die folgenden Konstanten kennzeichnen die Message-Queue im System.
# RENDEZVOUS muss der Name einer _existierenden_ Datei sein!
# Nur die unteren 8 Bits von RVID sind wichtig und muessen !=0 sein!
# ftok(RENDEZVOUS, RVID) erzeugt einen immer identischen Schluessel,
# so lange die Datei RENDEZVOUS nicht neu angelegt wurde.

use constant RENDEZVOUS => "/etc";
#use constant RVID       => 121;

sub rendevous_id{
  my ($playerid, $audiochannel) = @_;
#print "p=$playerid, chn=$audiochannel -> rdvid=".($playerid*16 + $audiochannel + 1)."\n";
  if ($playerid>15 || $audiochannel>15){
    print "\n  WARNING:\n  ";
    print "too many palyers/audiochannels\n";
    print "  p=$playerid, chn=$audiochannel -> rdvid=".($playerid*16 + $audiochannel + 1)."\n\n";
  }
  return $playerid*16 + $audiochannel + 1;
}
sub plmsg_newqueue{
  ### creates messae queue RENDEVOUS, if it doesn't already exist
  my ($playerid, $audiochannel) = @_;
  my $rdvid = rendevous_id($playerid, $audiochannel);
  use vars qw($msg);
  $msg = new IPC::Msg(ftok(RENDEZVOUS, $rdvid), S_IRWXU | IPC_CREAT);

}

sub plmsg_send{
  ### appends a message to the queue
  my ($playerid, $audiochannel) = @_;
  my $rdvid = rendevous_id($playerid, $audiochannel);
  my $msg = new IPC::Msg(ftok(RENDEZVOUS, $rdvid), 0);
  my ($prio, $text)=(1,"player started");# = @MESSAGES[$i,$i+1];
  $msg->snd($prio, $text, 0);
}

sub plmsg_waitrcv{
  my ($playerid, $audiochannel) = @_;
  ### pulls one message from the message queue, waits until one message is there
  my $rdvid = rendevous_id($playerid, $audiochannel);
  my $msg = new IPC::Msg(ftok(RENDEZVOUS, $rdvid), 0);
  my $buflen = 256;
  $prio = $msg->rcv($buf, $buflen, 0, 0);
#  print "Found: ($buf, $prio)\n";
}


sub pl_start_playd_and_wait{
# starts playing-deamon and waits until it has written it's pid in the db
  my ($dbhost, $playerid, $audiochannel) = @_;
#print "--> start gdplayd.pl\n";
  system("gdplayd.pl $dbhost $playerid $audiochannel & ");
#print "--> wait for gdplayd.pl to be started\n";
  plmsg_waitrcv($playerid, $audiochannel);
#print "--> message received\n";
}


#######################################################################
#######################################################################
#
# Basic Play Controls (also used as API)


sub pl_play{
# starts playing
  my ($dbh, $playerid, $audiochannel) = @_;
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  if(tracklist_get_item($dbh, $playerid, 0, $trackind) < 1){$trackind=1;} # playstate error
  stop_play_processes($dbh, $playerid, $audiochannel);
  pll_write_playstate($dbh, $playerid, $audiochannel, $trackind, "pl");
  pl_start_playd_and_wait($gdparms::dbhost, $playerid, $audiochannel);
#  system("gdplayd.pl $gdparms::dbhost $playerid $audiochannel & ");
}

sub pl_play_at{
# starts playing at specified position (seconds)
  my ($dbh, $playerid, $audiochannel, $songpos_sec) = @_;
#  my ($songpos_sec) = @_;
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  if(tracklist_get_item($dbh, $playerid, 0, $trackind) < 1){$trackind=1;} # playstate error
  stop_play_processes($dbh, $playerid, $audiochannel);
  my $startframe = $songpos_sec * frames_per_second();
  pll_write_playtime_only($dbh, $playerid, $audiochannel, $startframe);
  pll_write_playstate($dbh, $playerid, $audiochannel, $trackind, "pl");
  pl_start_playd_and_wait($gdparms::dbhost, $playerid, $audiochannel);
#  system("gdplayd.pl $gdparms::dbhost $playerid $audiochannel & ");
}

sub pl_stop{
# stops playing and reset playtime-state
  my ($dbh, $playerid, $audiochannel) = @_;
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  my($played, $total) = pll_get_playtime($dbh, $playerid, $audiochannel);
  stop_play_processes($dbh, $playerid, $audiochannel);
  pll_write_playstate($dbh, $playerid, $audiochannel, $trackind, "st");
  pll_write_playtime($dbh, $playerid, $audiochannel, 0, $total);
}

sub pl_pause{
# stops playing and preserve playtime-state
  my ($dbh, $playerid, $audiochannel) = @_;
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  my($played, $total) = pll_get_playtime($dbh, $playerid, $audiochannel);
  stop_play_processes($dbh, $playerid, $audiochannel);
  pll_write_playstate($dbh, $playerid, $audiochannel, 
			  	$trackind, "in"); # state: interrupted
}

sub pl_rw{
# similar as pause
  my ($dbh, $playerid, $audiochannel) = @_;
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  my ($played, $total) = pll_get_playtime($dbh, $playerid, $audiochannel);
  stop_play_processes($dbh, $playerid, $audiochannel);
  pll_write_playstate($dbh, $playerid, $audiochannel, 
			  	$trackind, "rw"); # state: rw

  ### continuously write current playtime to db (every second)
  system("gdplaytmsim.pl $gdparms::dbhost $playerid $audiochannel & ");
}

sub pl_ff{
# similar as pause
  my ($dbh, $playerid, $audiochannel) = @_;
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  my ($played, $total) = pll_get_playtime($dbh, $playerid, $audiochannel);
  stop_play_processes($dbh, $playerid, $audiochannel);
  pll_write_playstate($dbh, $playerid, $audiochannel, 
			  	$trackind, "ff"); # state: ff

  ### continuously write current playtime to db (every second)
  system("gdplaytmsim.pl $gdparms::dbhost $playerid $audiochannel & ");
}

sub pl_goto{
  ### makes a new track the current track
  #   sets the current playtime-posititon to zero
  #   the rest of the playstate is preserved
  
  use integer;
  my ($dbh, $playerid, $audiochannel, $newind) = @_;
#  my ($newind) = @_;	# the new index (must be valid)
  
  stop_play_processes($dbh, $playerid, $audiochannel);

  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  pll_write_playstate($dbh, $playerid, $audiochannel, $newind, $state);
  pll_write_playtime($dbh, $playerid, $audiochannel, 0, 0);
  if($state eq "pl"){ # restart player
    pl_start_playd_and_wait($gdparms::dbhost, $playerid, $audiochannel);
#    system("gdplayd.pl $gdparms::dbhost $playerid $audiochannel & ");
  }
  else{ # new 'current' track: load it's total length
    my $trackid = tracklist_get_item($dbh, $playerid, 0, $newind);
    if (length($trackid)>0) {
      my $sth = $dbh->prepare("SELECT * FROM tracks WHERE id = $trackid");
      my $cnt = $sth->execute;
      if ($cnt > 0){
        my $row = $sth->fetchrow_hashref;
        pll_write_playtime($dbh, $playerid, $audiochannel, 0, $row->{length}*frames_per_second());
      }
      else{
        pll_write_playtime($dbh, $playerid, $audiochannel, 0, 0);
      }
      $sth->finish;
    }
    else{
      pll_write_playtime($dbh, $playerid, $audiochannel, 0, 0);
    }
  }
}

sub pl_next{
  my ($dbh, $playerid, $audiochannel) = @_;
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  my $listlen = tracklist_get_nb_items($dbh, $playerid, $pl_list);
  if ($trackind < $listlen) {$trackind++;}
  pl_goto($dbh, $playerid, $audiochannel, $trackind);
}

sub pl_prev{
  my ($dbh, $playerid, $audiochannel) = @_;
  my $frames5sec = 5*frames_per_second();  # nb frames in 5 sec
  my ($trackind, $state, $frame, $shufflestat) = pll_get_playstate($dbh, $playerid, $audiochannel);
  my ($played, $total) = pll_get_playtime($dbh, $playerid, $audiochannel);
  if ($trackind>0 && $played<$frames5sec){$trackind--;}
  pl_goto($dbh, $playerid, $audiochannel, $trackind);
}

#######################################################################
#######################################################################





#######################################################################
#
# Playtime routines:
# The current position is always saved in the field 'framesplayed'
# If the player app can't continuously write this field, another realtime
# app has to write the current playtime (and ff, rew position)

# mp3 constants
my $samplesPerFrame  = 1152;
my $samplesPerSecond = 44100;

sub frames_per_second{
  use integer;
  return $samplesPerSecond/$samplesPerFrame;  # = 38.281
}
sub samples_per_frame{
  return $samplesPerFrame;
}
sub samples_per_second{
  return $samplesPerSecond;
}


# Writes the playtime to the playtime record. 
# Parameters: played, total  (units: frames)
sub pll_write_playtime{
  my ($dbh, $playerid, $audiochannel, $played, $total) = @_;
  my $retval = $dbh->do(
  	 "UPDATE playerstate "
  	."SET framesplayed=$played, framestotal=$total "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel");
}

sub pll_write_playtime_only{ # like pll_write_playtime without changing 'framestotal'
  my ($dbh, $playerid, $audiochannel, $played) = @_;
  my $retval = $dbh->do(
  	 "UPDATE playerstate "
  	."SET framesplayed=$played "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel");
}


# Returns the current playtime (frames played and total frames)
sub pll_get_playtime{
  my ($dbh, $playerid, $audiochannel) = @_;
  my $played  = 0;
  my $total   = 0;

  use integer;

  my $sth = $dbh->prepare( 
  	 "SELECT * FROM playerstate "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel" );

  my $nbrec = $sth->execute;
  #print("$nbrec playtime found (should be exactly 1)\n");

  if($row = $sth->fetchrow_hashref){
    $total  = $row->{framestotal};
    $played = $row->{framesplayed};
  }
  $sth->finish;

  return ($played, $total);
}


sub seconds_to_hm{
  my ($seconds) = @_;
  my ($hours, $minutes);
  use integer;  # switch to int math
  $hours   =  $seconds / 3600;
  $minutes = ($seconds % 3600)/60;
  return sprintf("%ih%02im", $hours, $minutes);
}

sub seconds_to_sm{
  my ($seconds) = @_;
  my ($minutes, $sec);
  use integer;  # switch to int math
  $minutes =  $seconds / 60;
  $sec     =  $seconds % 60;
  return sprintf("%i:%02i", $minutes, $sec);
}


### is the string a mp3-file or a mp3-stream?
sub is_mp3stream{
  my ($mp3filename) = @_;
  return ($mp3filename =~ /^http:\/\/.*/);  # matches "http://" at the beginning?
}


#######################################################################

# Returns the player parameter ($snddevice, $playerapp, $playerparams, $ptlogger, $shufflepar)
sub pll_get_playparams{
  my ($dbh, $playerid, $audiochannel) = @_;
  my ($snddevice, $playerapp, $playerparams, $ptlogger);

  my $sth = $dbh->prepare( 
  	 "SELECT * FROM playerstate "
  	."WHERE playerid=$playerid AND audiochannel=$audiochannel" );
  my $nbrec = $sth->execute;

  if($row = $sth->fetchrow_hashref){
    $snddevice    = $row->{snddevice};
    $playerapp    = $row->{playerapp};
    $playerparams = $row->{playerparams};
    $ptlogger     = $row->{ptlogger};
    $shufflepar   = $row->{shufflepar};
  }
  $sth->finish;

  return ($snddevice, $playerapp, $playerparams, $ptlogger, $shufflepar);
}


############################################################
# Returns the main player parameters 
# ($ipaddr, $uichannel, $logtarget, $cdripper, $mp3encoder, $cdromdev, $cdrwdev)
sub pll_get_mainparams{
  my ($dbh, $playerid) = @_;
  my ($ipaddr, $uichannel, $logtarget, $cdripper, $mp3encoder, $cdromdev, $cdrwdev);

  my $sth = $dbh->prepare( 
  	 "SELECT * FROM player "
  	."WHERE id=$playerid" );
  my $nbrec = $sth->execute;

  if($row = $sth->fetchrow_hashref){
    $ipaddr    = $row->{ipaddr};
    $uichannel = $row->{uichannel};
    $logtarget = $row->{logtarget};
    $cdripper  = $row->{cdripper};
    $mp3encoder= $row->{mp3encoder};
    $cdromdev  = $row->{cdromdev};
    $cdrwdev   = $row->{cdrwdev};
  }
  $sth->finish;

  return ($ipaddr, $uichannel, $logtarget, $cdripper, $mp3encoder, $cdromdev, $cdrwdev);
}


############################################################
### General tracklist functions (table tracklistitem)
#
# 'tracknb' always starts with 0 and is contiguous 
#   ex: 0,1,2,3 is legal,  0,1,2,4,5 is illegal
#

sub tracklist_get_nb_items{
  # returns the of tracks in the specified tracklist
  my ($dbh, $playerid, $listtype) = @_;
  my $listlen = 0;
  my $sth = $dbh->prepare( 
    	  "SELECT COUNT(*) FROM tracklistitem "
    	 ."WHERE playerid=$playerid AND listtype=$listtype" );
  $sth->execute;
  my @row;
  if(@row = $sth->fetchrow_array){
    $listlen = $row[0];
  }
  $sth->finish;
  return $listlen;
}

sub tracklist_append_list{
  # appends a list of trackid's to the specified tracklist. 
  # Parameters: dbh, playerid, listtype, trackids...
  my ($dbh)      = shift(@_);
  my ($playerid) = shift(@_);
  my ($listtype) = shift(@_);

  my $curritem = tracklist_get_nb_items($dbh, $playerid, $listtype);
  
  while($trackid = shift(@_)){
    $dbh->do("INSERT INTO tracklistitem SET "
    	."playerid=$playerid, "
    	."listtype=$listtype, "
    	."tracknb=$curritem, "
    	."trackid=$trackid ");
    $curritem++;
  }
}


# moves the specified list chunk down by one 
sub tracklist_move_chunk_up{
  my ($dbh, $playerid, $listtype, $first, $last) = @_;
  # "shift" specified items up (to higher index) by 1
  # have to do increment item by item because order is important
  my $sth = $dbh->prepare( 
  	 "SELECT * FROM tracklistitem "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb>=$first AND tracknb<=$last "
  	."ORDER BY tracknb DESC" ); # order: highest index first!
  $sth->execute;
  my $row;
  while($row = $sth->fetchrow_hashref){
    $dbh->do( 
  	 "UPDATE tracklistitem "
  	."SET tracknb=tracknb+1 "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb=".$row->{tracknb} );
  }
  $sth->finish;
}


# removes the specified list item from the list (item index starts with 0)
sub tracklist_del_item{
  my ($dbh, $playerid, $listtype, $trackindex) = @_;
  $dbh->do( 
  	 "DELETE FROM tracklistitem "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb=$trackindex" );

### should write a routine 'tracklist_move_chunk_down' (like tracklist_move_chunk_up)
  # "shift" following items down by 1
  # have to do decrement item by item because order is important
  my $sth = $dbh->prepare( 
  	 "SELECT * FROM tracklistitem "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb>$trackindex "
  	."ORDER BY tracknb" );
  $sth->execute;
  my $row;
  while($row = $sth->fetchrow_hashref){
    $dbh->do( 
  	 "UPDATE tracklistitem "
  	."SET tracknb=tracknb-1 "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb=".$row->{tracknb} );
  }
  $sth->finish;
}

# removes the all list items from 0 to 'trackindex' 
sub tracklist_del_upto_item{

### VERY INEFFICIENT IMPLEMENTATION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  my ($dbh, $playerid, $listtype, $trackindex) = @_;
  print("deleting up to $trackindex\n");
  my $counter=0;
  while ($counter < $trackindex){
    tracklist_del_item($dbh, $playerid, $listtype, 0);
    $counter++;
  }
}


# reorders the specified list item in the list (item index starts with 0)
# ->  'destpos' must be lower than 'srcpos'!
sub tracklist_reorder_item{
  my ($dbh, $playerid, $listtype, $srcpos, $destpos) = @_;
  
  ### "move" src-item to a save location
  my $savepos = -1000; # a bit too hacky?
  $dbh->do( 
  	 "UPDATE tracklistitem "
  	."SET tracknb=$savepos "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb=$srcpos" );

  # "shift" following items up by 1
  tracklist_move_chunk_up($dbh, $playerid, $listtype, $destpos, ($srcpos)-1);

  ### "move" src-item from save location to destination
  $dbh->do( 
  	 "UPDATE tracklistitem "
  	."SET tracknb=$destpos "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb=$savepos" );
}



# empties the specified tracklist 
sub tracklist_delete{
  my ($dbh, $playerid, $listtype) = @_;
  $dbh->do( 
  	 "DELETE FROM tracklistitem "
  	."WHERE playerid=$playerid AND listtype=$listtype" );
}


# gets the trackid of the specified list item (item index starts with 0)
# If an error occurs, 0 is returned.
sub tracklist_get_item{
  my ($dbh, $playerid, $listtype, $trackindex) = @_;
  my $trackid = 0;
  my $sth = $dbh->prepare( 
  	 "SELECT * FROM tracklistitem "
  	."WHERE playerid=$playerid AND listtype=$listtype AND tracknb=$trackindex" );
  my $nbrec = $sth->execute;
  if($row = $sth->fetchrow_hashref){
    $trackid = $row->{trackid};
  }
  $sth->finish;
  return $trackid;
}


# Returns the current playlist (list of track ID's)
sub tracklist_get_all{
  my ($dbh, $playerid, $listtype) = @_;
  my @playlist=();

  my $sth = $dbh->prepare( 
  	 "SELECT * FROM tracklistitem "
  	."WHERE playerid=$playerid AND listtype=$listtype "
  	."ORDER BY tracknb" );
  my $nbrec = $sth->execute;
  while($row = $sth->fetchrow_hashref){
    $trackid = $row->{trackid};
    push @playlist, $row->{trackid};
  }
  $sth->finish;

  return @playlist;
}



############################################################
### id3tag functions

### returns the value of a id3 tag or other mp3 parameters for the specified file
### Possible tagcodes are 

### NEW version:

# %a     Artist [string]
# %b     Number of corrupt audio frames [integer]
# %c     Comment [string]
# %C     Copyright flag [string]
# %e     Emphasis [string]
# %E     CRC Error protection [string]
# %f     Filename without the path [string]
# %F     Filename with the path [string]
# %g     Musical genre [string]
# %G     Musical genre [integer]
# %l     Album name [string]
# %L     MPEG Layer [string]
# %m     Playing time: minutes only [integer]
# %n     Track [integer]
# %O     Original material flag [string]
# %o     Stereo/mono mode [string]
# %p     Padding [string]
# %Q     Sampling frequency in Hz [integer]
# %q     Sampling frequency in KHz [integer]
# %r     Bit  Rate  in  KB/s  (type  and   meaning
#        affected by -r option)
# %s     Playing   time:  seconds  only  [integer]
#        (usually used in conjunction with # %m)
# %S     Total playing time in seconds [integer]
# %t     Track Title [string]
# %u     Number of good audio frames [integer]
# %v     MPEG Version [float]
# %y     Year [string]
# %%     A single percent sign

sub get_mp3info{ 
  my ($tagcode, $filename) = @_;
  my $base = gdparams::gdbase();
  my $res = `mp3info -p "$tagcode" "$filename"`;

  ### Error cases
  if ($tagcode eq "%S" && length($res)==0){
    my $res2 = `mpg123 -v -t -n 0 "$filename" 2> $base/tmp/gdinfo.tmp;grep Frame $base/tmp/gdinfo.tmp`;
    $res2 =~ m/.*\[(.+):(.+).(.+)\].*/;
    my $min = $1;
    my $sec = $2;
    $res = $min*60 + $sec;
    print("playlength fixed to $res\n");
  }
  if($tagcode eq "%r" && ($res eq "Variable"))
  { #check for Variable Bitrate
#     $res="Var".`mp3info -p %r -r m "$filename"`;       ### don't predeed wit 'wav' - it messes up bitrate calculations of streamer
     $res=`mp3info -p %r -r m "$filename"`;
     return $res;
  }
  if ($tagcode eq "%r" && ($res <= 0 )){
    $res = "128";
  }
  
  return $res;
}


############################################################
### audio metadata with ogg support.
#
# standard ogg tags according to 
# http://www.xiph.org/ogg/vorbis/doc/v-comment.html
# -> ARTIST TITLE ALBUM TRACKNUMBER YEAR GENRE COMMENT
# and 

### typical output of ogginfo
#>ogginfo file.ogg
#filename=file.ogg
#
#serial=6039
#header_integrity=pass
#ALBUM=ob die Engel auch Beine haben
#TITLE=Die Schampullamaschine
#ARTIST=Zentriert ins Antlitz
#DATE=2002
#TRACKNUMBER=7
#GENRE=Industrial
#ORGANIZATION=-
#COMMENT=ZIA Ogg Vorbis 1.0 Final
#vendor=Xiph.Org libVorbis I 20020717
#version=0
#channels=2
#rate=44100
#bitrate_upper=none
#bitrate_nominal=128000
#bitrate_lower=none
#stream_integrity=pass
#bitrate_average=126915
#length=50.248005
#playtime=0:50
#stream_truncated=false
#
#total_length=50.248005
#total_playtime=0:50

sub oggfile_title{
  my($audiofile) = @_;
  $line = `ogginfo "$audiofile" |grep --ignore-case ^title=`;
  chop $line;
  return substr($line, 6);
}
sub oggfile_artist{
  my($audiofile) = @_;
  $line = `ogginfo "$audiofile" |grep --ignore-case ^artist=`;
  chop $line;
  return substr($line, 7);
}
sub oggfile_album{
  my($audiofile) = @_;
  $line = `ogginfo "$audiofile" |grep --ignore-case ^album=`;
  chop $line;
  return substr($line, 6);
}
sub oggfile_year{
  my($audiofile) = @_;
  $line = `ogginfo "$audiofile" |grep --ignore-case ^date=`;
  chop $line;
  $line = substr($line, 5); 
  if ($line =~ /\D*(\d*).*/){
    return $1;
  }
  else {
    return 0;
  }
}
sub oggfile_lengthsec{
  my($audiofile) = @_;
  $line = `ogginfo "$audiofile" |grep --ignore-case ^length=`;
  chop $line;
  my $lengthsec = substr($line, 7);
  return int($lengthsec);
}
sub oggfile_bitrate{
  my($audiofile) = @_;
  $line = `ogginfo "$audiofile" |grep --ignore-case ^bitrate_nominal=`;
  chop $line;
  my $bitrate = substr($line, 16);
  return (int(($bitrate/4000)+0.5))*4; # round to modulo 4
}
sub oggfile_tracknumber{
  my($audiofile) = @_;
  $line = `ogginfo "$audiofile" |grep --ignore-case ^tracknumber=`;
  chop $line;
  $line = substr($line, 12);
  return int($line);
} 
sub oggfile_genre{
  return "";
}

###~CU~ #FLAC_BEGIN

############################################################
### FLAC metadata functions
### Autor: Christian Uebber
### 
### Uses metaflac (see http://flac.sourceforge.net/)
### for extracting VORBIS_COMMENT metadata.
###
### Future versions may also support extracting
### cue-sheet information.
###

sub flacfile_title{
  my($audiofile) = @_;
  $line = `metaflac --show-vc-field=title "$audiofile"`;
  chop $line;
  return substr($line, 6);
}

sub flacfile_artist{
  my($audiofile) = @_;
  $line = `metaflac --show-vc-field=artist "$audiofile"`;
  chop $line;
  return substr($line, 7);
}
sub flacfile_album{
  my($audiofile) = @_;
  $line = `metaflac --show-vc-field=album "$audiofile"`;
  chop $line;
  return substr($line, 6);
}

sub flacfile_year{
  my($audiofile) = @_;
  $line = `metaflac --show-vc-field=date "$audiofile"`;
  chop $line;
  $line = substr($line, 5); 
  if ($line =~ /\D*(\d*).*/){
    return $1;
  }
  else {
    return 0;
  }
}

sub flacfile_lengthsec{
  my($audiofile) = @_;
  $line = `metaflac --show-total-samples "$audiofile"`;
  chop $line;
  return int($line/44100);		## Please verify (theoretically correct)
}

sub flacfile_bitrate{
  my($audiofile) = @_;
  $line = `metaflac --show-sample-rate "$audiofile"`;
  chop $line;
  return (int($line/1000)); 		# respect maximum field length
#  return ($line/1000); 		# respect maximum field length
#  return (int($line)/100); 		# respect maximum field length
}

### Alternative:
#sub flacfile_type{
#  my($audiofile) = @_;
#  $line = `metaflac --show-sample-rate "$audiofile"`;
#  chop $line;
#  if ($line="44100"){
#    return "cda";
#  }
#  else {
#    return "";
#  }
#}

sub flacfile_tracknumber{
  my($audiofile) = @_;
  $line = `metaflac --show-vc-field=tracknumber "$audiofile"`;
  chop $line;
  $line = substr($line, 12);
  return int($line);
} 
sub flacfile_genre{
  my($audiofile) = @_;
  $line = `metaflac --show-vc-field=genre "$audiofile"`;
  chop $line;
  return substr($line, 6);
}

sub flacfile_tracknumber{
  my($audiofile) = @_;
  $line = `metaflac --show-vc-field=tracknumber "$audiofile"`;
  chop $line;
  $line = substr($line, 12);
  return int($line);
}

###~CU~ #FLAC_END

############################################################

sub audiofile_title{
  my($audiofile) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  my $title = "";
  if ($ftype eq "mp3"){
    $title = get_mp3info("%t", $audiofile);
  }
  elsif ($ftype eq "ogg"){
    $title = oggfile_title($audiofile);
  }
  elsif ($ftype eq "flac"){			###~CU~
    $title = flacfile_title($audiofile);
  }
  
  if (length($title) > 0){
    return $title;
  }
  else {
    use File::Basename;
    return basename($audiofile);
  }
}


sub audiofile_artist{
  my($audiofile) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  my $artist = "";
  if ($ftype eq "mp3"){
    $artist = get_mp3info("%a", $audiofile);
  }
  elsif ($ftype eq "ogg"){
    $artist = oggfile_artist($audiofile);
  }
  elsif ($ftype eq "flac"){
    $artist = flacfile_artist($audiofile);		###~CU~
  }

  if (length($artist) > 0){
    return $artist;
  }
  else {
    use File::Basename;
    return basename($audiofile);
  }
}


sub audiofile_album{
  my($audiofile) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  if ($ftype eq "mp3"){
    return get_mp3info("%l", $audiofile);
  }
  elsif ($ftype eq "ogg"){
    return oggfile_album($audiofile);
  }
  elsif ($ftype eq "flac"){
    return flacfile_album($audiofile);		###~CU~
  }
  else {
    return "Album name";
  }
}


sub audiofile_year{
  my($audiofile) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  if ($ftype eq "mp3"){
    return get_mp3info("%y", $audiofile);
  }
  elsif ($ftype eq "ogg"){
    return oggfile_year($audiofile);
  }
  elsif ($ftype eq "flac"){
    return flacfile_year($audiofile);		###~CU~
  }
  else {
    return 1990;
  }
}


sub audiofile_lengthsec{
  my($audiofile) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  if ($ftype eq "mp3"){
    return get_mp3info("%S", $audiofile);
  }
  elsif ($ftype eq "ogg"){
    return oggfile_lengthsec($audiofile);
  }
  elsif ($ftype eq "flac"){
    return flacfile_lengthsec($audiofile);	###~CU~
  }
  else {
    return 0;
  }
}


sub audiofile_bitrate{
  my($audiofile) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  if ($ftype eq "mp3"){
    return get_mp3info("%r", $audiofile);
  }
  elsif ($ftype eq "ogg"){
    return oggfile_bitrate($audiofile);
  }
  elsif ($ftype eq "flac"){
    return flacfile_bitrate($audiofile);  # ###~CU~ flac: return sampling rate / please check alternative
  }
  else {
    return 128;
  }
}


sub audiofile_tracknumber{
  my($audiofile, $default_tracknb) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  my $tracknb=0;
  if ($ftype eq "mp3"){
    $tracknb = get_mp3info("%n", $audiofile);
  }
  elsif ($ftype eq "ogg"){
    $tracknb = oggfile_tracknumber($audiofile);
  }
  elsif ($ftype eq "flac"){
    $tracknb = flacfile_tracknumber($audiofile);  
  }
  
  if($tracknb > 0){
    return $tracknb;
  }
  else {
    return $default_tracknb;
  }
}


sub audiofile_genre{
  ### extracts id3 gerne code or genre string (depends on filetype) 
  ### returns GD-genre code
  my($dbh, $audiofile) = @_;   # must be full filename with path!
  my $ftype = audio_filetype($audiofile);
  if ($ftype eq "mp3"){
    my $id3genre = get_mp3info("%G", $audiofile);
    return genre_id3togd($dbh, $id3genre);
  }
  elsif ($ftype eq "ogg"){
    my $genrestring = oggfile_genre($audiofile);
    return genre_stringtogd($dbh, $genrestring);
  }
  elsif ($ftype eq "flac"){
    my $genrestring = flacfile_genre($audiofile);
    return genre_stringtogd($dbh, $genrestring);
  }
  else {
    return "";
  }
}


############################################################

sub audio_filetype{
  ### the filetype is derived form the filename extension
  my($audiofile) = @_;

  if ($audiofile =~ /[Mm][Pp]3$/){ 
    return "mp3";
  }
  if ($audiofile =~ /ogg$/){
    return "ogg"; 
  }
  if ($audiofile =~ /flac$/){
    return "flac"; 
  }
}



############################################################

sub get_bitrate_str{
  # returns format/bitrate in kBit/s (ex. "mp3 128", "ogg 112") 
  # takes as argument the audio file name with its full path.
  my ($audiofile) = @_;
  my $bitrate = audiofile_bitrate($audiofile);
  my $ftype = audio_filetype($audiofile);
  return "$ftype $bitrate";
}

sub bitrate_str_to_format_param{
  # gets a bitrate string like "mp3 128" and returns array ("mp3", "128")
  my ($bitratestr) = @_;
  my ($audiofmt, $audioparam) = split ' ', $bitratestr;  # split "mp3 128"
  if (length($audiofmt)<2){ # something went wrong
    $audiofmt = "mp3";      # set reasonable default values
    $audioparam = 128;
  }
  return ($audiofmt, $audioparam);
}


sub get_full_audiofile_path{
  # returns the full path of an audio file in a 00, 01, 02, ... directory
  # or an empty string if the file doesn't exist
  my ($audiofile) = @_;
  my $base = gdparams::gdbase();
  my $fname = `ls $base/[0-9][0-9]/$audiofile`; # get full path
  chop($fname);
  return $fname;
}

############################################################
### Returns the first music directory with enough space
  # to save ripped files. If no large enough directory is found
  # an empty string is returned.
  # Parameter: minimum free space in Mbytes

sub get_ripdirectory{
  my ($minfreeMbytes) = @_;
  my $base = gdparams::gdbase();

  ### Get mp3 directories
  my @mdir = gdparams::mp3dirs();

  ### Get an mp3 directory with enough space left (1GB)
  my $i=0;
  my @dfres;
  my $mbfree;
  my $ripdir="";

  while($i < @mdir){
    if (-d "$base/$mdir[$i]"){
      @dfres = split / +/, `df -m $base/$mdir[$i]|tail -1`;
      $mbfree = $dfres[3];
      #print "$base/$mdir[$i] has $mbfree MB free \n";
      if($mbfree > $minfreeMbytes){
        $ripdir = $mdir[$i];
        last;	 # break
      }
    }
    else{print "$base/$mdir[$i] is not a directory or does not exist\n";}
    $i++;
  }
  #print("Rip directory: $ripdir \n");
  return $ripdir;
}


############################################################
### import booklet/cover images
sub import_cover_images{ 
  # imports the jpeg images in a directory and associates them to an album
  # the images are imported in lexical order. 
  # Naming scheme: trxx(cd-id)-(num).jpg, where num is an automatically 
  # incremented counter. The file imgxx(cd-id)-00.jpg is the front cover, 
  # the other are the following pages in a booklet.
  
  # Parameters: 1) dbh, 
  #             2) full directory path, 
  #             3) cd-id   (like 0x10ac77e0, xx00001496)
  #             4) test?   (if set to 1, only show what would be done)
  
  my ($dbh, $fullpath, $cdid, $test) = @_;

  my ($sourcefile, $targetfile);
  my $base = gdparams::gdbase();
  print "import images at $fullpath for id $cdid\n";

  if (length($cdid)<10){
    print "Error (import_cover_images): illegal format of cdid '$cdid'\n";
    return;
  }
  
  opendir INBOX, "$fullpath";
  my @imgfilelist = sort (grep /\.jpg$/, readdir INBOX);
  closedir INBOX;

  #print "imgs: ".join (":",@imgfilelist)."\n";
  if (scalar(@imgfilelist) > 0){

    ### Get a directory with enough space left (1GB)
    my $ripdir=get_ripdirectory($gdparams::minfreehdspace);
  
    if($ripdir ne ""){### put image-file in music directory
      my $imgnum = 0;
  
      foreach $sourcefile (@imgfilelist){
        $targetfile = sprintf ("$base/$ripdir/img%s-%02ld.jpg", $cdid, $imgnum);
        if ($test){
          print "test: move '$fullpath/$sourcefile' to '$targetfile'\n";
        }
        else{
          print "move '$fullpath/$sourcefile' to '$targetfile'\n";
          system "mv \"$fullpath/$sourcefile\" \"$targetfile\"";
          if ($imgnum == 0){
            
            my $sqlcmd = ("UPDATE album SET coverimg='img$cdid-00.jpg' WHERE cddbid = SUBSTRING('$cdid',3)");
            #print"\$dbh->do($sqlcmd);\n"
            $dbh->do($sqlcmd);
          }
        }
        $imgnum += 1;
      }# end foreach
    }
    else{
      print("Not enough space left on disc \n");
    }
  }
  else{
    print "no jpeg images found in $fullpath \n";
  }
}


############################################################
### common tool routines

### returns highest number of imported (=not recorded from audio-CD, =no
### CDDB-ID associated) mp3 tracks. 
### Parameter: database handle.
sub last_imported_tracknb{
  my ($dbh) = @_;

  my $mp3num;
  my $trkseth = $dbh->prepare('select mp3file from tracks '. # get last record
  	'where mp3file like "trxx________%" order by mp3file desc limit 1');
#  	'where mp3file like "trxx%" order by mp3file desc limit 1');
  my $nbmp3files = $trkseth->execute;
  if ($nbmp3files > 0){
    $tracks = $trkseth->fetchrow_hashref;
    $tracks->{mp3file} =~ m/trxx([0-9]*)\.\w/;   # extract number
    $mp3num = $1;
  }
  else{
    $mp3num = 0;
  }
  $trkseth->finish;
  return $mp3num;
}


### Translates an id3 genre (numeric) to a GiantDisc genre code (string)
### Parameters: database handle, id3-genre. 
### Returns:    GiantDisc genre code (or empty string, if no match found)
sub genre_id3togd{
  my ($dbh, $id3genre)= @_;

  my $gdcode = "";
  if(length($id3genre)>0 && $id3genre >= 0){
    my $genseth = $dbh->prepare('select * from genre where id3genre="'. $id3genre . '" ');
    my $nbgenres = $genseth->execute;
    if ($nbgenres > 0){
      my $genres = $genseth->fetchrow_hashref;
      $gdcode = $genres->{id};
    }
    $genseth->finish;
  }
  #print("Translating id3:\"$id3genre\" to gdgenre \"$gdcode\" \n\n");
  return $gdcode;
}

### Translates a genre string to a GiantDisc genre code (string)
### Parameters: database handle, genre string. 
### Returns:    GiantDisc genre code (or empty string, if no match found)
sub genre_stringtogd{
  my ($dbh, $genrestring)= @_;

  my $gdcode = "";
  if(length($genrestring)>0){
    my $genseth = $dbh->prepare( # get best=shortest match
         "SELECT id,length(genre) AS len FROM genre "
        ."WHERE genre like \"%".$genrestring."%\" ORDER BY len");
#    'select id, from genre where genre="'. $genrestring . '" ');
    my $nbgenres = $genseth->execute;
    if ($nbgenres > 0){
      my $genres = $genseth->fetchrow_hashref;
      $gdcode = $genres->{id};
    }
    $genseth->finish;
  }
  #print("Translating genrestring:\"$genrestring\" to gdgenre \"$gdcode\" \n\n");
  return $gdcode;
}



###############################################################################

sub iso2ascii{
  # converts all non-ascii characters in the passed string as good 
  # as possible to ascii characters, and returns the result
  
  my ($str) = @_;

  $str =~ tr/\xc0-\xc6/A/;
  $str =~ tr/\xc7/C/;
  $str =~ tr/\xc8-\xcb/E/;
  $str =~ tr/\xcc-\xcf/I/;
  $str =~ tr/\xd0\xd1/DN/;
  $str =~ tr/\xd2-\xd8/O/;
  $str =~ tr/\xd9-\xdc/U/;
  $str =~ tr/\xdd\xde\xdf/YTs/;

  $str =~ tr/\xe0-\xe6/a/;
  $str =~ tr/\xe7/c/;
  $str =~ tr/\xe8-\xeb/e/;
  $str =~ tr/\xec-\xef/i/;
  $str =~ tr/\xf0\xf1/dn/;
  $str =~ tr/\xf2-\xf8/o/;
  $str =~ tr/\xf9-\xfc/u/;
  $str =~ tr/\xfd\xfe\xff/yts/;

  $str =~ tr/\xa0-\xff/_/;
  
  return $str;
}

sub ascii2filename{
  # converts all ascii characters in the passed string as good 
  # as possible to ascii characters that are allowed in filenames, 
  #and returns the result
  
  my ($str) = @_;

  $str =~ tr/\//-/;     # translate / to -
  $str =~ tr/\x2f/_/;
  $str =~ s/"/''/g;
  
  return $str;
}

###############################################################################


1;
#
