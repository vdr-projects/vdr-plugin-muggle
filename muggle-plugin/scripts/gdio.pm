##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2000, Rolf Brugger
#
##################################################

package gdio;

#use lib '/home/music/bin';

use IO::Socket;
use strict;

# GiantDisc IO routines

### Constants & global variables
my $no_mode = 0;
my $rs232_mode = 1;
my $tcpip_srv_mode = 2;
my $tcpip_cli_mode = 3;

my $commMode = $no_mode;


### Serial communication parameters:
my $serdev;
my $serbaud;

### TCP/IP communication parameters
my $gdsocket;
my $connection;

undef $connection;

#################################################################
### initialization routines:

sub serio_init{
  ### Init serial port
  my $logtarget;
  ($logtarget, $serdev, $serbaud) = @_;
  
  print("### Init serial port $serdev at $serbaud bps\n");
  
  # Config serial port: 
  #	- disable echo 
  #	- enable hardware flow control
  system("stty $serbaud -echo crtscts <$serdev");
  open (SERIN, "<$serdev");
  open (SEROUT, ">$serdev");

  ### Alternative?
  #sysopen (SERIN, $serdev, O_RDRW | O_NODELAY | O_NOCTTY)
  #or die "Can't open $serdev: $!";
  #my $ofh = select(SEROUT); $| = 1; 
  #select($ofh);

  $commMode = $rs232_mode;
}



sub tcpip_srv_init{
  my ($logtarget, $tcpiphost, $tcpipport) = @_;

  print "Open socket on $tcpiphost at port $tcpipport as server\n";
  $gdsocket = new IO::Socket::INET( LocalHost => $tcpiphost,
                                 LocalPort => $tcpipport,
                                 Proto     => 'tcp',
                                 Listen    => 1,
                                 Reuse     => 1);

  die "could not open socket: $!\n" unless $gdsocket;

  $commMode = $tcpip_srv_mode;
}

sub tcpip_cli_init{
  my ($logtarget, $tcpiphost, $tcpipport) = @_;

  print "Open socket on $tcpiphost at port $tcpipport as client\n";
  $gdsocket = new IO::Socket::INET( PeerAddr => $tcpiphost,
                                 PeerPort => $tcpipport,
                                 Proto    => 'tcp');

  die "could not open socket: $!\n" unless $gdsocket;

  $commMode = $tcpip_cli_mode;
}

#################################################################

sub serio_getline { # returns one line
  my $line = <SERIN>;
  return $line;
}

sub putline_nack { # sends one line, without waiting for acknowledge (currently not used)
  my ($line)=@_;
  print (SEROUT "$line\n");
}

sub serio_putline
### The soutine returns true, if the client does not want to receive
### more lines (acknowledged with a 'stop' command).
###
### If the routines has to wait too long for an acknowledge (>20sec), the
### transmission is aborted. Such situations could be caused by crashed
### clients or broken transmission lines.
{
  my ($line)=@_;
  my $ackstring;
  print (SEROUT "$line\n");
  ### wait for acknowledge (max 20 sec)
  eval{
    local $SIG{ALRM} = sub { die "ACK timeout" };	# set Alarm event handler
    alarm 20;						# 20 sec timeout
    
    $ackstring=gdio::getline();
    chop($ackstring);
    
    alarm 0;						# deactivate Alarm
  };
  if ($@ and $@ =~ /ACK timeout/ ) { 
    print ("\nACKNOWLEDGE TIMEOUT\n\n");
    $ackstring = "s"; #abort current transmission
  };

  if($ackstring eq "s"){
    print ("\nACK-stop received\n");
    return 1;	# stop transmission
  }
  else{
    return 0;
  }
}

#################################################################
#
# If we experienced and error reading for the socket, then
# $connection would be set to "0" and so we need to wait for the 
# palm client to reconnect.
#
# Thanks to Viktor for the tcpip code snippets

sub tcpip_srv_check_connection{
    my $timeout;
    print "connection open? ... ";
    if ( ! defined $connection ) {
    	print "no\n";
	print "Server accepting socket ... \n";
	$connection = $gdsocket->accept();
	$timeout = $connection->timeout();
	#print "Server Connection time out is [$timeout] \n";
	$timeout = $connection->timeout(60);
	#print "Server Connection time out set to [$timeout] \n";
    }
    else{
    	print "yes\n";
    }

    return $connection;
}

#
# If we have an error reading the input assume client has
# disconnected. We close the socket and return a "NULL" command
#
sub tcpip_srv_getline{
  print "server listening ...\n";
  my $line;
  if ( $line = <$connection> ) {
    #print "line recieved: [$line] \n";
    return $line;
  }
  else {
    print "Client disconnected ...\n";
    close ($connection) ;
    undef $connection;
    return "NULL\n";
  }
}

sub tcpip_cli_getline{
  print "listening ...\n";
  my $line;
  if ( $line = <$gdsocket> ) {
    #print "line recieved: [$line] \n";
    return $line;
  }
}


sub tcpip_srv_getline_SINGLECHAR{  ### just for tests ...
  print "server listening ...\n";
  my $line="";
  my $cbuf;
  while ( read $connection, $cbuf, 1 ) {
    print "char recieved: [$cbuf] \n";
    $line .= $cbuf;
    last if ($cbuf eq "\n");
  }
  print "line recieved: [$line] \n";
  return $line;
}


sub tcpip_srv_putline{
# There is currently a problem with this routine.
# The Palm client should be able to interrupt the transmission, if too many
# lines are sent to it. When the client shuts down the socket, the server does
# not directly recognize this. The 'if (print $connection $line."\n" )' is never
# false, unless the Palm is turned off.
# This has probably to do with the fact, that there is already sent data in a
# buffer?
#
# The current workaround is, that the Palm receives the supernumerous lines, but
# doesn't store them. Not very efficient ...  :-(

  my ($line)=@_;
  #print "Sending to socket: \"$line\"\n";
#if (defined($connection->connected())){
#print "WE ARE CONNECTED\n";}
#else{
#print "WE ARE _NOT_ CONNECTED\n";}

  if (print $connection $line."\n" ) {
    return 0; #don't stop
  }
  else{
    print "Client disconnected ...\n";
    close ($connection) ;
    undef $connection;
    return 1; #stop transmission
  }
}


sub tcpip_cli_putline_NOACK{
  my ($line)=@_;
  #print "Sending to socket: \"$line\"\n";

  if (print $gdsocket $line."\n" ) {
    return 0; #don't stop
  }
  return 0;
}
sub tcpip_cli_putline
{
  my ($line)=@_;
  my $ackstring;
  print $gdsocket $line."\n";
  ### wait for acknowledge (max 20 sec)
  eval{
    local $SIG{ALRM} = sub { die "ACK timeout" };	# set Alarm event handler
    alarm 20;						# 20 sec timeout
    
    $ackstring=<$gdsocket>;
    chop($ackstring);
    
    alarm 0;						# deactivate Alarm
  };
  if ($@ and $@ =~ /ACK timeout/ ) { 
    print ("\nACKNOWLEDGE TIMEOUT\n\n");
    $ackstring = "s"; #abort current transmission
  };

  if($ackstring eq "s"){
    print ("\nACK-stop received\n");
    return 1;	# stop transmission
  }
  else{
    return 0;
  }
}



#sub close_connection
#{
#  print "Closing Connection - $connection  \n";
#  close($connection)
#}

#################################################################
### public routines:

sub check_connection
{
  if    ($commMode == $rs232_mode){
    return 1;
  }
  elsif ($commMode == $tcpip_srv_mode){
    return tcpip_srv_check_connection();
  }
  elsif ($commMode == $tcpip_cli_mode){
    return 1;
  }
  else{
    print "Error: unknown communication mode\n";
    exit;
  }
}

sub putline
{
  if    ($commMode == $rs232_mode){
    return serio_putline(@_);
  }
  elsif ($commMode == $tcpip_srv_mode){
    return tcpip_srv_putline(@_);
  }
  elsif ($commMode == $tcpip_cli_mode){
    return tcpip_cli_putline(@_);
  }
  else{
    print "Error: unknown communication mode\n";
    exit;
  }
}

sub getline
{
  if    ($commMode == $rs232_mode){
    return serio_getline();
  }
  elsif ($commMode == $tcpip_srv_mode){
    return tcpip_srv_getline();
  }
  elsif ($commMode == $tcpip_cli_mode){
    return tcpip_cli_getline();
  }
  else{
    print "Error: unknown communication mode\n";
    exit;
  }
}


#################################################################

END{
  print ("io modul finished\n");
}

#
1;
