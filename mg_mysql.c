/*! \file   mg_mysql.c
 *  \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-02-10 17:42:54 +0100 (Thu, 10 Feb 2005) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 */

#include "mg_mysql.h"
#include "mg_tools.h"

#include <stdarg.h>

#include "vdr_setup.h"

bool needGenre2;
static bool needGenre2_set;


char *db_cmds[] = 
{
  "DROP DATABASE IF EXISTS GiantDisc; CREATE DATABASE GiantDisc;",
  "grant all privileges on GiantDisc.* to vdr@localhost;",
  "use GiantDisc;",
  "drop table if exists album; CREATE TABLE album ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "cddbid varchar(20) NOT NULL default '', "
	  "coverimg varchar(255) default NULL, "
	  "covertxt mediumtext, "
	  "modified date default NULL, "
	  "genre varchar(10) default NULL, "
	  "PRIMARY KEY  (cddbid), "
	  "KEY artist (artist(10)), "
	  "KEY title (title(10)), "
	  "KEY genre (genre), "
	  "KEY modified (modified)) "
	  "TYPE=MyISAM;",
  "drop table if exists genre; CREATE TABLE genre ("
	  "id varchar(10) NOT NULL default '', "
	  "id3genre smallint(6) default NULL, "
	  "genre varchar(255) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists language; CREATE TABLE language ("
	  "id varchar(4) NOT NULL default '', "
	  "language varchar(40) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists musictype; CREATE TABLE musictype ("
	  "musictype varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists player;CREATE TABLE player ( "
	  "ipaddr varchar(255) NOT NULL default '', "
	  "uichannel varchar(255) NOT NULL default '', "
	  "logtarget int(11) default NULL, "
	  "cdripper varchar(255) default NULL, "
	  "mp3encoder varchar(255) default NULL, "
	  "cdromdev varchar(255) default NULL, "
	  "cdrwdev varchar(255) default NULL, "
	  "id int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists playerstate;CREATE TABLE playerstate ( "
	  "playerid int(11) NOT NULL default '0', "
	  "playertype int(11) NOT NULL default '0', "
	  "snddevice varchar(255) default NULL, "
	  "playerapp varchar(255) default NULL, "
	  "playerparams varchar(255) default NULL, "
	  "ptlogger varchar(255) default NULL, "
	  "currtracknb int(11) default NULL, "
	  "state varchar(4) default NULL, "
	  "shufflepar varchar(255) default NULL, "
	  "shufflestat varchar(255) default NULL, "
	  "pauseframe int(11) default NULL, "
	  "framesplayed int(11) default NULL, "
	  "framestotal int(11) default NULL, "
	  "anchortime bigint(20) default NULL, "
	  "PRIMARY KEY  (playerid,playertype)) "
	  "TYPE=HEAP;",
  "drop table if exists playlist;CREATE TABLE playlist ( "
	  "title varchar(255) default NULL, "
	  "author varchar(255) default NULL, "
	  "note varchar(255) default NULL, "
	  "created timestamp(8) NOT NULL, "
	  "id int(10) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists playlistitem;CREATE TABLE playlistitem ( "
	  "playlist int(11) NOT NULL default '0', "
	  "tracknumber mediumint(9) NOT NULL default '0', "
	  "trackid int(11) default NULL, "
	  "PRIMARY KEY  (playlist,tracknumber)) "
	  "TYPE=MyISAM;",
  "drop table if exists playlog;CREATE TABLE playlog ( "
	  "trackid int(11) default NULL, "
	  "played date default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists recordingitem;CREATE TABLE recordingitem ( "
	  "trackid int(11) default NULL, "
	  "recdate date default NULL, "
	  "rectime time default NULL, "
	  "reclength int(11) default NULL, "
	  "enddate date default NULL, "
	  "endtime time default NULL,  "
	  "repeat varchar(10) default NULL, "
	  "initcmd varchar(255) default NULL, "
	  "parameters varchar(255) default NULL, "
	  "atqjob int(11) default NULL, "
	  "id int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists source; CREATE TABLE source ( "
	  "source varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists tracklistitem;CREATE TABLE tracklistitem ( "
	  "playerid int(11) NOT NULL default '0', "
	  "listtype smallint(6) NOT NULL default '0', "
	  "tracknb int(11) NOT NULL default '0', "
	  "trackid int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (playerid,listtype,tracknb)) "
	  "TYPE=MyISAM;",
  "drop table if exists tracks;CREATE TABLE tracks ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "genre1 varchar(10) default NULL, "
	  "genre2 varchar(10) default NULL, "
	  "year smallint(5) unsigned default NULL, "
	  "lang varchar(4) default NULL, "
	  "type tinyint(3) unsigned default NULL, "
	  "rating tinyint(3) unsigned default NULL, "
	  "length smallint(5) unsigned default NULL, "
	  "source tinyint(3) unsigned default NULL, "
	  "sourceid varchar(20) default NULL, "
	  "tracknb tinyint(3) unsigned default NULL, "
	  "mp3file varchar(255) default NULL, "
	  "condition tinyint(3) unsigned default NULL, "
	  "voladjust smallint(6) default '0', "
	  "lengthfrm mediumint(9) default '0', "
	  "startfrm mediumint(9) default '0', "
	  "bpm smallint(6) default '0', "
	  "lyrics mediumtext, "
	  "bitrate varchar(10) default NULL, "
	  "created date default NULL, "
	  "modified date default NULL, "
	  "backup tinyint(3) unsigned default NULL, "
	  "samplerate int(7) unsigned default NULL, "
	  "channels   tinyint(3) unsigned default NULL,  "
	  "id int(11) NOT NULL auto_increment, "
	  "folder1 varchar(255), "
	  "folder2 varchar(255), "
	  "folder3 varchar(255), "
	  "folder4 varchar(255), "
	  "PRIMARY KEY  (id), "
	  "KEY title (title(10)), "
	  "KEY mp3file (mp3file(10)), "
	  "KEY genre1 (genre1), "
	  "KEY genre2 (genre2), "
	  "KEY year (year), "
	  "KEY lang (lang), "
	  "KEY type (type), "
	  "KEY rating (rating), "
	  "KEY sourceid (sourceid), "
	  "KEY artist (artist(10))) "
	  "TYPE=MyISAM;"
};


mgmySql::mgmySql() 
{
	m_db = 0;
	Connect();
}

mgmySql::~mgmySql()
{
    if (m_db) 
    {
       mysql_close (m_db);
       m_db = 0;
    }
}


MYSQL_RES*
mgmySql::exec_sql( string query)
{
  if (!m_db || query.empty())
	  return 0;
  mgDebug(3,"exec_sql(%X,%s)",m_db,query.c_str());
  if (mysql_query (m_db, (query + ';').c_str ()))
  {
    mgError("SQL Error in %s: %s",query.c_str(),mysql_error (m_db));
    std::cout<<"ERROR in " << query << ":" << mysql_error(m_db)<<std::endl;
    return 0;
  }
  return mysql_store_result (m_db);
}

string
mgmySql::get_col0( const string query)
{
	MYSQL_RES * sql_result = exec_sql ( query);
	if (!sql_result)
		return "NULL";
	MYSQL_ROW row = mysql_fetch_row (sql_result);
	string result;
	if (row == NULL)
		result = "NULL";
	else if (row[0] == NULL)
		result = "NULL";
	else
		result = row[0];
	mysql_free_result (sql_result);
	return result;
}

unsigned long
mgmySql::exec_count( const string query) 
{
	return atol (get_col0 ( query).c_str ());
}


void mgmySql::Create()
{
  // create database and tables
  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=0; i < len; i ++ )
    {
      exec_sql( string( db_cmds[i] ) );
    }
}

string
mgmySql::sql_string( const string s )
{
  if (!this)
     return "'" + s + "'";

  char *buf = (char *) malloc( 2*s.size() + 1 );
  mysql_real_escape_string( m_db, buf, s.c_str(), s.size() );
  string result = "'" + string( buf ) + "'";
  free( buf );
  return result;
}

void
mgmySql::Connect ()
{
    if (m_db) 
    {
       mysql_close (m_db);
       m_db = 0;
    }
    if (the_setup.DbHost == "") return;
    m_db = mysql_init (0);
    if (!m_db)
        return;
    bool success;
    if (the_setup.DbSocket != NULL)
    {
      mgDebug(1,"Using socket %s for connecting to Database %s as user %s.",
		      the_setup.DbSocket,
		      the_setup.DbName,
		      the_setup.DbUser);
      mgDebug(3,"DbPassword is: '%s'",the_setup.DbPass);
      success = (mysql_real_connect( m_db,
			      "",
			      the_setup.DbUser,       
			      the_setup.DbPass, 
			      the_setup.DbName,
			      0,
			      the_setup.DbSocket, 0 ) != 0 );
    }
    else
    {
      mgDebug(1,"Using TCP-%s for connecting to Database %s as user %s.",
		      the_setup.DbHost,
		      the_setup.DbName,
		      the_setup.DbUser);
      mgDebug(3,"DbPassword is: '%s'",the_setup.DbPass);
      success = ( mysql_real_connect( m_db,
			      the_setup.DbHost, 
			      the_setup.DbUser,       
			      the_setup.DbPass, 
			      the_setup.DbName,
			      the_setup.DbPort,
			      0, 0 ) != 0 );
    }
    if (!success)
    {
	mgWarning("Failed to connect to host '%s' as User '%s', Password '%s': Error: %s",
			    the_setup.DbHost,the_setup.DbUser,the_setup.DbPass,mysql_error(m_db));
        mysql_close (m_db);
	m_db = 0;
    }
    if (!needGenre2_set && m_db)
    {
	needGenre2_set=true;
	needGenre2=exec_count("SELECT COUNT(DISTINCT genre2) from tracks")>1;
    }
    return;
}


