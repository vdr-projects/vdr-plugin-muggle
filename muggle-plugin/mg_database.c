/*! \file   mg_database.c
 *  \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 */

#include "mg_database.h"
#include "mg_tools.h"

#include <stdarg.h>

static const int MAX_QUERY_BUFLEN = 2048;

static char *db_cmds[] = {
  "use GiantDisc;"
  "DROP DATABASE IF EXISTS GiantDisc; CREATE DATABASE GiantDisc;",
  "grant all privileges on GiantDisc.* to vdr@localhost;",
  "drop table if exists album; CREATE TABLE album ( artist varchar(255) default NULL, title varchar(255) default NULL, cddbid varchar(20) NOT NULL default '', coverimg varchar(255) default NULL, covertxt mediumtext, modified date default NULL, genre varchar(10) default NULL, PRIMARY KEY  (cddbid), KEY artist (artist(10)), KEY title (title(10)), KEY genre (genre), KEY modified (modified)) TYPE=MyISAM;",
  "drop table if exists genre; CREATE TABLE genre (id varchar(10) NOT NULL default '', id3genre smallint(6) default NULL, genre varchar(255) default NULL, freq int(11) default NULL, PRIMARY KEY (id)) TYPE=MyISAM;",
  "drop table if exists language; CREATE TABLE language (id varchar(4) NOT NULL default '', language varchar(40) default NULL, freq int(11) default NULL, PRIMARY KEY  (id)) TYPE=MyISAM;",
  "drop table if exists musictype; CREATE TABLE musictype (musictype varchar(40) default NULL, id tinyint(3) unsigned NOT NULL auto_increment, PRIMARY KEY  (id)) TYPE=MyISAM;",
  "drop table if exists player;CREATE TABLE player ( ipaddr varchar(255) NOT NULL default '', uichannel varchar(255) NOT NULL default '', logtarget int(11) default NULL, cdripper varchar(255) default NULL, mp3encoder varchar(255) default NULL, cdromdev varchar(255) default NULL, cdrwdev varchar(255) default NULL, id int(11) NOT NULL default '0', PRIMARY KEY  (id)) TYPE=MyISAM;",
  "drop table if exists playerstate;CREATE TABLE playerstate ( playerid int(11) NOT NULL default '0', playertype int(11) NOT NULL default '0', snddevice varchar(255) default NULL, playerapp varchar(255) default NULL, playerparams varchar(255) default NULL, ptlogger varchar(255) default NULL, currtracknb int(11) default NULL, state varchar(4) default NULL, shufflepar varchar(255) default NULL, shufflestat varchar(255) default NULL, pauseframe int(11) default NULL, framesplayed int(11) default NULL, framestotal int(11) default NULL, anchortime bigint(20) default NULL, PRIMARY KEY  (playerid,playertype)) TYPE=HEAP;",
  "drop table if exists playlist;CREATE TABLE playlist ( title varchar(255) default NULL, author varchar(255) default NULL, note varchar(255) default NULL, created timestamp(8) NOT NULL, id int(10) unsigned NOT NULL auto_increment, PRIMARY KEY  (id)) TYPE=MyISAM;",
  "drop table if exists playlistitem;CREATE TABLE playlistitem ( playlist int(11) NOT NULL default '0', tracknumber mediumint(9) NOT NULL default '0', trackid int(11) default NULL, PRIMARY KEY  (playlist,tracknumber)) TYPE=MyISAM;",
  "drop table if exists playlog;CREATE TABLE playlog ( trackid int(11) default NULL, played date default NULL, id tinyint(3) unsigned NOT NULL auto_increment, PRIMARY KEY  (id)) TYPE=MyISAM;",
  "drop table if exists recordingitem;CREATE TABLE recordingitem ( trackid int(11) default NULL, recdate date default NULL, rectime time default NULL, reclength int(11) default NULL, enddate date default NULL, endtime time default NULL,  repeat varchar(10) default NULL, initcmd varchar(255) default NULL, parameters varchar(255) default NULL, atqjob int(11) default NULL, id int(11) NOT NULL default '0', PRIMARY KEY  (id)) TYPE=MyISAM;",
  "drop table if exists source; CREATE TABLE source ( source varchar(40) default NULL, id tinyint(3) unsigned NOT NULL auto_increment, PRIMARY KEY  (id)) TYPE=MyISAM;",
  "drop table if exists tracklistitem;CREATE TABLE tracklistitem ( playerid int(11) NOT NULL default '0', listtype smallint(6) NOT NULL default '0', tracknb int(11) NOT NULL default '0', trackid int(11) NOT NULL default '0', PRIMARY KEY  (playerid,listtype,tracknb)) TYPE=MyISAM;",
  "drop table if exists tracks;CREATE TABLE tracks ( artist varchar(255) default NULL, title varchar(255) default NULL, genre1 varchar(10) default NULL, genre2 varchar(10) default NULL, year smallint(5) unsigned default NULL, lang varchar(4) default NULL, type tinyint(3) unsigned default NULL, rating tinyint(3) unsigned default NULL, length smallint(5) unsigned default NULL, source tinyint(3) unsigned default NULL, sourceid varchar(20) default NULL, tracknb tinyint(3) unsigned default NULL, mp3file varchar(255) default NULL, condition tinyint(3) unsigned default NULL, voladjust smallint(6) default '0', lengthfrm mediumint(9) default '0', startfrm mediumint(9) default '0', bpm smallint(6) default '0', lyrics mediumtext, bitrate varchar(10) default NULL, created date default NULL, modified date default NULL, backup tinyint(3) unsigned default NULL, samplerate int(7) unsigned default NULL, channels   tinyint(3) unsigned default NULL,  id int(11) NOT NULL auto_increment, PRIMARY KEY  (id), KEY title (title(10)), KEY mp3file (mp3file(10)), KEY genre1 (genre1), KEY genre2 (genre2), KEY year (year), KEY lang (lang), KEY type (type), KEY rating (rating), KEY sourceid (sourceid), KEY artist (artist(10))) TYPE=MyISAM;"
};


mgDB::mgDB() 
{
}

mgDB::mgDB(std::string host, std::string name, 
	   std::string user, std::string pass,
	   int port)
{
}

mgDB::~mgDB()
{
}

MYSQL_RES *
mgDB::exec_sql(std::string query)
{
  if (query.empty())
    return 0;
  mgDebug( 3, "exec_sql(%X,%s)", &m_dbase, query.c_str() );
  if (mysql_query (&m_dbase, (query + ';').c_str ()))
    {
      mgError("SQL Error in %s: %s",query.c_str(),mysql_error (&m_dbase));
      std::cout << "ERROR in " << query << ":" << mysql_error(&m_dbase) << std::endl;
      return 0;
    }
  return mysql_store_result(&m_dbase);
}

void mgDB::initialize()
{
  // create database
  exec_sql( std::string(db_cmds[1]) );
  exec_sql( std::string(db_cmds[0]) );
  exec_sql( std::string(db_cmds[2]) );

  // create tables
  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=3; i < len; i ++ )
    {
      exec_sql( std::string( db_cmds[i] ) );
    }
}

MYSQL mgDB::getDBHandle()
{

  return m_dbase;
}

std::string mgDB::escape_string( MYSQL *db, std::string s )
{
  char *escbuf = (char *) malloc( 2*s.size() + 1 );

  mysql_real_escape_string( db, escbuf, s.c_str(), s.size() );

  std::string r = std::string( escbuf );
  free( escbuf );

  return r;
}

MYSQL_RES* mgDB::read_query( const char *fmt, ...)
{
  char querybuf[MAX_QUERY_BUFLEN];
  va_list ap;
  va_start( ap, fmt );  
  vsnprintf( querybuf, MAX_QUERY_BUFLEN-1, fmt, ap );
  
  if( mysql_query( &m_dbase, querybuf) )
    {
      mgError( "SQL error in MUGGLE:\n%s\n", querybuf );
    }
  
  MYSQL_RES *result = mysql_store_result( &m_dbase );
  
  va_end(ap);
  return result;
}

void mgDB::write_query( const char *fmt, ... )
{
  char querybuf[MAX_QUERY_BUFLEN];
  va_list ap;
  va_start( ap, fmt );
  vsnprintf( querybuf, MAX_QUERY_BUFLEN-1, fmt, ap );
  
  if( mysql_query( &m_dbase, querybuf ) )
    {
      mgError( "SQL error in MUGGLE:\n%s\n", querybuf );
    }
  
  va_end(ap);
}
