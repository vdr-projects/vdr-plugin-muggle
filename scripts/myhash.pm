package myhash;


##################################################
#
# GiantDisc mp3 Jukebox
# 
# © 2000, Rolf Brugger
#
##################################################

#use lib '/usr/local/bin';
#BEGIN{;}
#END{;}


############################################################
###
sub addvaltohash{ # gets a current hashval and a new elment
		  # returns new hashval
  my ($hashval,$newelement) = @_;

  return (($hashval << 5) ^ ($hashval >> 27)) ^ $newelement;
  # (^ is bitwise EXOR)
}


1;
#
