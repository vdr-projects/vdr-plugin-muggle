##################################################
#
# GiantDisc mp3 Jukebok
# 
# © 2000, Rolf Brugger
#
##################################################


###
### GiantDisc: Common database routines (mainly db modification)
###

package gddb;

use strict;

### Init constants


#BEGIN{
#}

############################################################
###                    TOOL ROUTINES                     ###
############################################################

############################################################

sub sql_escape_squotes{
  ### escapes single quotes of the string passed as argument
  #
  # Parameter: string to be quoted 
  # Returns:   quoted string

  # Usually you would use $dbh->quote instead, except if you don't want
  # to add single quotes around the string
  
  my $sqlstring = $_[0];
  $sqlstring =~ s/'/\\'/g;
  return $sqlstring;
}


############################################################
###
sub field_where_clause{ # returns a string with a SQL-where clause
			# for a text field like artist or title. If
			# keyword is empty, an empty string is returned
			#
			# - Multiple keywords are space separated
			# - Field begins are matched using the * wildcard
			
  # Examples: 
  #		field="artist" keyword="abc"
  #		-> "AND artist LIKE 
  #
  #		field="artist" keyword="abc xyz"
  #		-> "AND artist LIKE "%abc%" AND artist LIKE "%xyz%" "
  #
  #		field="artist" keyword="abc*"   (everything after * is ignored)
  #		-> "AND artist LIKE "abc%" "
  #			

  my ($fieldname,$keywords) = @_;
  $keywords = sql_escape_squotes($keywords);
  my $cmd = "";
  
  if ($keywords ne ""){
    if($keywords =~ m/\*/ ){
    	  ### wildcard expression
      my @words = split (/\*/, $keywords );
      $cmd.=" AND $fieldname LIKE '$words[0]%' ";
    }
    else{ ### non wildcard expression
      my @words = split (/ /, $keywords );
      my $current;
      while($current = shift(@words)){
        $cmd.=" AND $fieldname LIKE '%$current%' ";
      }
    }
  }
  return $cmd;
}

############################################################
###
my $get_all=0;
my $get_tracks=1;
my $get_streams=2;

sub attrib_where_clause{ # returns a string with a SQL-where clause
			# for genre, year, language, type and rating
  # field names are prefixed by 'tracks.' that the clauses can also be 
  # used in JOIN queries.
  
  my ($get_what, $genre1,$genre2,$yearfrom,$yearto,$lang,$type,$rating) = @_;

  my $tmpcmd;
  my $cmd=" ";

  ### Genre
  $tmpcmd="0 ";		# false OR ...
  if ($genre1 ne ""){ 
    $tmpcmd.="OR tracks.genre1 LIKE '$genre1%' OR tracks.genre2 LIKE '$genre1%' ";};
  if ($genre2 ne ""){ 
    $tmpcmd.="OR tracks.genre1 LIKE '$genre2%' OR tracks.genre2 LIKE '$genre2%' ";};
  if (length($tmpcmd)>3){ # genre query not empty?
    $cmd .= " AND ($tmpcmd) ";
  }

  ### Year
  if(length($yearfrom)==4){ $cmd.=" AND tracks.year >= ".$yearfrom;}
  if(length($yearto)==4){   $cmd.=" AND tracks.year <= ".$yearto;}

  ### Language
  if ($lang ne ""){ $cmd.=" AND tracks.lang = '$lang' ";};

  ### type
  if ($type ne ""){ $cmd.=" AND tracks.type = '$type' ";};

  ### rating
  if ($rating ne ""){ 
    if ($rating == 0) {
      $cmd.=" AND tracks.rating = $rating ";
    }
    else{
      $cmd.=" AND tracks.rating >= $rating ";
    }
  }
  
  ### track/stream/all
  if($get_what==$get_tracks){
    $cmd.=" AND mp3file NOT LIKE 'http://%' ";
  }
  if($get_what==$get_streams){
    $cmd.=" AND mp3file LIKE 'http://%' ";
  }
  
  return $cmd;
}

############################################################
###
sub track_where_clause{ # returns the where clause without keyword "WHERE"
  my ($get_what, $artist,$title,$genre1,$genre2,$yearfrom,$yearto,
      $lang,$type,$rating,
      $ordercmd) = @_;

  my $where=" 1 ";	# true AND ...

  ### Artist
  $where .= field_where_clause("artist",$artist);
  ### Title
  $where .= field_where_clause("title",$title);
  ### genre, etc ...
  $where.= attrib_where_clause($get_what, $genre1,$genre2,$yearfrom,$yearto,$lang,$type,$rating);

  return $where;
}

############################################################

sub track_order_clause{ # returns the order clause with keyword "ORDER BY"
  my ($get_what, $artist,$title,$genre1,$genre2,$yearfrom,$yearto,$lang,$type,$rating,
      $ordercmd) = @_;
  
  my $order = "";
  
  if(length($ordercmd)>1){
    $order = "ORDER BY ";
    if   ($ordercmd =~ m/random/ ){
      $order .= "RAND() ";
    }
    elsif($ordercmd =~ m/year/ ){
      $order .= "year ";
    }
    elsif($ordercmd =~ m/recd/ ){
      $order .= "created ";
    }
    elsif($ordercmd =~ m/modd/ ){
      $order .= "modified ";
    }
    elsif($ordercmd =~ m/recmod/ ){
      $order .= "GREATEST(created,modified) ";
    }
    
    if($ordercmd =~ m/-inv/ ){
      $order .= " DESC ";
    }
  }
  
  return $order;
}

############################################################
###
sub album_where_clause{
  my ($artist,$title,$genre1,$genre2,$yearfrom,$yearto,
      $lang,$type,$rating,
      $ordercmd) = @_;

  my $where=" 1 ";	# true AND ...
  ### Album: Artist
  $where .= gddb::field_where_clause("album.artist",$artist);
  ### Album: Title
  $where .= gddb::field_where_clause("album.title",$title);
  ### Track: genre, etc ...
  $where.= gddb::attrib_where_clause(1, $genre1,$genre2,$yearfrom,$yearto,$lang,$type,$rating);

#print "ALBUM WHERE: $where\n";
  return $where;
}

############################################################

sub album_order_clause{ # returns the order clause with keyword "ORDER BY"
  my ($artist,$title,$genre1,$genre2,$yearfrom,$yearto,$lang,$type,$rating,
      $ordercmd) = @_;
  
  my $order = "";

  if(length($ordercmd)>1){
    $order = "ORDER BY ";
    if   ($ordercmd =~ m/random/ ){
      $order .= "RAND() ";
    }
    elsif($ordercmd =~ m/year/ ){
      $order .= "tracks.year ";
    }
    elsif($ordercmd =~ m/recd/ ){
      $order .= "album.modified ";
    }
    elsif($ordercmd =~ m/modd/ ){
      $order .= "album.modified ";
    }
    elsif($ordercmd =~ m/recmod/ ){
      $order .= "album.modified ";
    }
    
    if($ordercmd =~ m/-inv/ ){
      $order .= " DESC ";
    }
  }
  
  return $order;
}

############################################################
###                 DB CREATION & UPDATE                 ###
############################################################

############################################################
### Creates/updates a new track record 
  # If $id is empty, a new record is created. Otherwise, the record
  # is updated.
  # Returns the (new) id

sub insert_track_record{ 
  my ($dbh,$artist,$title,$genre1,$genre2,$year,
      $lang,$type,$rating,$length,$source,$sourceid,
      $tracknb,$mp3file,$condition,$voladjust,
      $lengthfrm,$startfrm,$bpm,$bitrate,
      $created,$id,    ### these two fields are only defined on update!
      ) = @_;

  if(length($artist)==0){$artist="-";};
  if(length($title)==0) {$title="-";};
  if(length($year)<4)   {$year="0";};
  if(length($type)==0)  {$type="NULL";};
  if(length($rating)==0){$rating="NULL";};
  if(length($length)==0){$length="0";};
  if(length($source)==0){$source="0";};
  if(length($tracknb)==0){$tracknb="0";};
  if(length($condition)==0){$condition="0";};
  if(length($voladjust)==0){$voladjust="0";};
  if(length($lengthfrm)==0){$lengthfrm="0";};
  if(length($startfrm)==0) {$startfrm="0";};
  if(length($bpm)==0)      {$bpm="0";};
  if(length($bitrate)==0)  {$bitrate="128";};
  if(length($created)==0)  {$created="CURDATE()";};


  my $sqlcmd;
  $sqlcmd = 
    	   "artist=".$dbh->quote($artist).", "	# quote adds single quotes around the string!
    	  ."title=".$dbh->quote($title).", "
    	  ."genre1='$genre1', "
    	  ."genre2='$genre2', "
    	  ."year  = $year, "
    	  ."lang  ='$lang', "
    	  ."type  = $type, "
    	  ."rating=$rating, "
    	  ."length=$length, "
    	  ."source=$source, "
    	  ."sourceid='$sourceid', "
    	  ."tracknb=$tracknb, "
    	  ."mp3file='$mp3file', "
    	  ."condition=$condition, "
    	  ."voladjust=$voladjust, "
    	  ."lengthfrm=$lengthfrm, "
    	  ."startfrm=$startfrm, "
    	  ."bpm=$bpm, "
    	  ."bitrate='$bitrate', ";

  if(length($id)==0){
    ### INSERT a new record
    $sqlcmd = "INSERT tracks SET ".$sqlcmd
    	  ."created=CURDATE() ";
  }
  else{
    ### REPLACE an existing record
    $sqlcmd = "UPDATE tracks SET ".$sqlcmd
    	  ."created='$created', "
    	  ."modified=CURDATE() "
    	  ."WHERE id=$id ";
  }

  #print("SQL: $sqlcmd \n"); 
  my $sth = $dbh->prepare($sqlcmd);
  $sth->execute;

  if(length($id)==0){ ### if new record created
    $id = $sth->{mysql_insertid};
  }  
  $sth->finish;
  return $id;
}





############################################################
###                       QUERIES                        ###
############################################################
#





#### end
1;
