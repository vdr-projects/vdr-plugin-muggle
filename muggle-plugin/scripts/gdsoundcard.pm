##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2003, Rolf Brugger
#
##################################################

package gdsoundcard;

#
# soundcard drivers to control volume etc
#
#
#

#use lib '/usr/local/bin';
use strict;

############################################################
### Constants & global variables



############################################################

sub sndc_init{
### initialize

# actually not used yet

  my ($playertype, $playerhost, $sounddevice) = @_;
  
  if    ($playertype == 0){  # local oss soundcard
    ;
  }
  
  elsif ($playertype == 20){ # exstreamer
    ;
  }
  else{
    print "Warning: unknown player type $playertype\n";
  }
}



############################################################
### Volume routines
#   0 <= volume <= 100


sub sndc_set_volume{
### Set volume

  my ($playertype, $playerhost, $sounddevice, $volume) = @_;
  
  if    ($playertype == 0){  # local oss soundcard
    system "aumix -v$volume";
  }
  
  elsif ($playertype == 20){ # exstreamer
    use integer;
    my $cmd = "v=".($volume/5)."\n";
    gdgentools::exstreamer_command($playerhost, $cmd);
  }
  else{
    print "Warning: unknown player type $playertype\n";
  }
}


sub sndc_get_volume{
### Get volume
#   returns the currently set volume. 

  my ($playertype, $playerhost, $sounddevice) = @_;
  my $volume = 0;
  
  if    ($playertype == 0){  # local oss soundcard
    my ($shcommand) = @_;
    my ($res, $resline, @reslines);
    $res = `aumix -vq`;
    @reslines = split /\n/, $res;    
    $resline = shift (@reslines);    
    if ($resline =~ m/\D*(\d+).*/){
      $volume = $1;
    }
    else{print "Warning: Get volume - can't match aumix output\n";}
  }
  
  elsif ($playertype == 20){ # exstreamer
    my $cmd = "v=\n";  # cmd get volume 
    my $res = gdgentools::exstreamer_command_res($playerhost, $cmd);
    if ($res =~ m/\<.*\>(\d+)\<.*\>/){
      $volume = ($1)*5;
    }
    else{
      $volume = 50;
    }
  }
  else{
    print "Warning: unknown player type $playertype\n";
  }
  return $volume;
}


sub sndc_save_volume{
### Save default volume

  my ($playertype, $playerhost, $sounddevice, $volume) = @_;
  
  if    ($playertype == 0){  # local oss soundcard
    system "aumix -S";
  }
  elsif ($playertype == 20){ # exstreamer
    # the exstreamer always saves the volume setting in its flash rom
    ;
  }
  else{
    print "Warning: unknown player type $playertype\n";
  }
}

############################################################

sub sndc_inc_volume{
### Increases volume by 5%
  my ($playertype, $playerhost, $sounddevice) = @_;
  my $volume = sndc_get_volume($playertype, $playerhost, $sounddevice);
  $volume += 5;
  if ($volume>100){$volume=100;}
  sndc_set_volume($playertype, $playerhost, $sounddevice, $volume);
}

sub sndc_dec_volume{
### decreases volume by 5%
  my ($playertype, $playerhost, $sounddevice) = @_;
  my $volume = sndc_get_volume($playertype, $playerhost, $sounddevice);
  $volume -= 5;
  if ($volume<0){$volume=0;}
  sndc_set_volume($playertype, $playerhost, $sounddevice, $volume);
}

############################################################

1;
#
