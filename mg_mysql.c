/*! \file   mg_mysql.c
 *  \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-02-10 17:42:54 +0100 (Thu, 10 Feb 2005) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  file owner: $Author: LarsAC $
 */

#include "mg_mysql.h"
#include "mg_tools.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "mg_setup.h"

bool needGenre2;
static bool needGenre2_set;
bool UsingEmbedded();

class mysqlhandle_t {
	public:
		mysqlhandle_t();
		~mysqlhandle_t();
};


static char *datadir;

static char *embedded_args[] =
{
	"muggle",	
	"--datadir=/tmp",	// stupid default
	"--key_buffer_size=32M"
};

#ifndef HAVE_ONLY_SERVER
static char *embedded_groups[] =
{
	"embedded",
	"server",
	"muggle_SERVER",
	0
};
#endif

void
set_datadir(char *dir)
{
	mgDebug(1,"setting datadir to %s",dir);
	struct stat stbuf;
	datadir=strdup(dir);
	asprintf(&embedded_args[1],"--datadir=%s",datadir);
	if (stat(datadir,&stbuf))
		mkdir(datadir,0755);
	if (stat(datadir,&stbuf))
	{
		mgError("Cannot access datadir %s: errno=%d",datadir,errno);
	}
}


mysqlhandle_t::mysqlhandle_t()
{
#ifndef HAVE_ONLY_SERVER
	int argv_size;
	if (UsingEmbedded())
	{
		mgDebug(1,"calling mysql_server_init for embedded");
		argv_size = sizeof(embedded_args) / sizeof(char *);
	}
	else
	{
		if (strcmp(MYSQLCLIENTVERSION,"4.1.11")<0)
			mgError("You have embedded mysql. For accessing external servers "
				"you need mysql 4.1.11 but you have only %s", MYSQLCLIENTVERSION);
		mgDebug(1,"calling mysql_server_init for external");
		argv_size = -1;
	}
	if (mysql_server_init(argv_size, embedded_args, embedded_groups))
		mgDebug(3,"mysql_server_init failed");
#endif
}

mysqlhandle_t::~mysqlhandle_t()
{
#ifndef HAVE_ONLY_SERVER
  mgDebug(3,"calling mysql_server_end");
  	mysql_server_end();
#endif
}

static mysqlhandle_t* mysqlhandle;

mgmySql::mgmySql() 
{
	m_database_found=false;
	m_hasfolderfields=false;
	if (!mysqlhandle)
		mysqlhandle = new mysqlhandle_t;
	m_db = 0;
	Connect();
}

mgmySql::~mgmySql()
{
    if (m_db) 
    {
	    mgDebug(3,"%X: closing DB connection",this);
       mysql_close (m_db);
       m_db = 0;
    }
}

static char *db_cmds[] = 
{
  "drop table if exists album;",
  "CREATE TABLE album ( "
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
  "drop table if exists genre;",
  "CREATE TABLE genre ("
	  "id varchar(10) NOT NULL default '', "
	  "id3genre smallint(6) default NULL, "
	  "genre varchar(255) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists language;",
  "CREATE TABLE language ("
	  "id varchar(4) NOT NULL default '', "
	  "language varchar(40) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists musictype;",
  "CREATE TABLE musictype ("
	  "musictype varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists player;",
  "CREATE TABLE player ( "
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
  "drop table if exists playerstate;",
  "CREATE TABLE playerstate ( "
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
  "drop table if exists playlist;",
  "CREATE TABLE playlist ( "
	  "title varchar(255) default NULL, "
	  "author varchar(255) default NULL, "
	  "note varchar(255) default NULL, "
	  "created timestamp(8) NOT NULL, "
	  "id int(10) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists playlistitem;",
  "CREATE TABLE playlistitem ( "
	  "playlist int(11) NOT NULL default '0', "
	  "tracknumber mediumint(9) NOT NULL default '0', "
	  "trackid int(11) default NULL, "
	  "PRIMARY KEY  (playlist,tracknumber)) "
	  "TYPE=MyISAM;",
  "drop table if exists playlog;",
  "CREATE TABLE playlog ( "
	  "trackid int(11) default NULL, "
	  "played date default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists recordingitem;",
  "CREATE TABLE recordingitem ( "
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
  "drop table if exists source",
	  "CREATE TABLE source ( "
	  "source varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists tracklistitem;",
  "CREATE TABLE tracklistitem ( "
	  "playerid int(11) NOT NULL default '0', "
	  "listtype smallint(6) NOT NULL default '0', "
	  "tracknb int(11) NOT NULL default '0', "
	  "trackid int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (playerid,listtype,tracknb)) "
	  "TYPE=MyISAM;",
  "drop table if exists tracks;",
  "CREATE TABLE tracks ( "
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

bool
mgmySql::sql_query(const char *sql)
{
	return mysql_query(m_db,sql);
}


MYSQL_RES*
mgmySql::exec_sql( string query)
{
  if (!m_db || query.empty())
	  return 0;
  mgDebug(4,"exec_sql(%X,%s)",m_db,query.c_str());
  if (sql_query (query.c_str ()))
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

struct genres_t {
	char *id;
	int id3genre;
	char *name;
};

struct lang_t {
	char *id;
	char *name;
};

struct musictypes_t {
	char *name;
};

struct sources_t {
	char *name;
};

#include "mg_tables.h"
void mgmySql::FillTables()
{
  int len = sizeof( genres ) / sizeof( genres_t );
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  char id3genre[5];
	  if (genres[i].id3genre>=0)
	  	sprintf(id3genre,"%d",genres[i].id3genre);
	  else
		strcpy(id3genre,"NULL");
	  string genre = sql_string(genres[i].name);
	  sprintf(b,"INSERT INTO genre SET id='%s', id3genre=%s, genre=%s",
			  genres[i].id,id3genre,genre.c_str());
	  exec_sql(b);
  }
  len = sizeof( languages ) / sizeof( lang_t );
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  sprintf(b,"INSERT INTO language SET id='%s', language=",
			  languages[i].id);
	  sql_Cstring(languages[i].name,strchr(b,0));
	  exec_sql(b);
  }
  len = sizeof( musictypes ) / sizeof( musictypes_t );
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  sprintf(b,"INSERT INTO musictype SET musictype='%s'",
			  musictypes[i].name);
	  exec_sql(b);
  }
  len = sizeof( sources ) / sizeof( sources_t );
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  sprintf(b,"INSERT INTO source SET source='%s'",
			  sources[i].name);
	  exec_sql(b);
  }
}

time_t createtime;

void mgmySql::Create()
{
  createtime=time(0);
  // create database and tables
  mgDebug(1,"Dropping and recreating database %s",the_setup.DbName);
  char buffer[500];
  sprintf(buffer,"DROP DATABASE IF EXISTS %s",the_setup.DbName);
  if (strlen(buffer)>400)
	  mgError("name of database too long: %s",the_setup.DbName);
  if (sql_query(buffer))
  {
  	mgWarning("Cannot drop %s:%s",the_setup.DbName,
			mysql_error (m_db));
	return;
  }
  sprintf(buffer,"CREATE DATABASE %s",the_setup.DbName);
  if (sql_query(buffer))
  {
  	mgWarning("Cannot create %s:%s",the_setup.DbName,mysql_error (m_db));
	return;
  }

  if (!UsingEmbedded())
	sprintf(buffer,"grant all privileges on %s.* to vdr@localhost",
			the_setup.DbName);
  	sql_query(buffer);
  	// ignore error. If we can create the data base, we can do everything
  	// with it anyway.

  if (mysql_select_db(m_db,the_setup.DbName))
	  mgError("mysql_select_db(%s) failed with %s",mysql_error(m_db));

  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=0; i < len; i ++ )
    {
  	if (sql_query (db_cmds[i]))
  	{
    		mgWarning("%20s: %s",db_cmds[i],mysql_error (m_db));
		sprintf(buffer,"DROP DATABASE IF EXISTS %s",the_setup.DbName);
  		sql_query(buffer);	// clean up
		return;
	}
    }
  m_database_found=true;
  FillTables();
}

string
mgmySql::sql_string( const string s )
{
  char *b = sql_Cstring(s);
  string result = string( b);
  free( b);
  return result;
}

char*
mgmySql::sql_Cstring( const string s, char *buf )
{
  return sql_Cstring(s.c_str(),buf);
}

char*
mgmySql::sql_Cstring( const char *s, char *buf)
{
  char *b;
  if (buf)
	b=buf;
  else
  {
  	int buflen;
  	if (!this)
	  	buflen=strlen(s)+2;
  	else
		buflen=2*strlen(s)+3;
  	b = (char *) malloc( buflen);
  }
  b[0]='\'';
  if (!this)
	strcpy(b+1,s);
  else
  	mysql_real_escape_string( m_db, b+1, s, strlen(s) );
  *(strchr(b,0)+1)=0;
  *(strchr(b,0))='\'';
  return b;
}

bool
mgmySql::ServerConnected () const
{
	return m_db;
}

bool
mgmySql::Connected () const
{
	return m_database_found;
}


bool
UsingEmbedded()
{
#ifdef HAVE_ONLY_SERVER
	return false;
#else
	return the_setup.NoHost();
#endif
}

void
mgmySql::Connect ()
{
    assert(!m_db);
    m_db = mysql_init (0);
    if (!m_db)
        return;
    if (UsingEmbedded())
    {
    	if (!mysql_real_connect(m_db, 0, 0, 0, 0, 0, 0, 0))
		mgWarning("Failed to connect to embedded mysql in %s:%s ",datadir,mysql_error(m_db));
    	else
		mgDebug(1,"Connected to embedded mysql in %s",datadir);
    }
    else
    {
    	if (the_setup.NoHost() || !strcmp(the_setup.DbHost,"localhost"))
      		mgDebug(1,"Using socket %s for connecting to local system as user %s.",
		      	the_setup.DbSocket, the_setup.DbUser);
    	else
      		mgDebug(1,"Using TCP for connecting to server %s as user %s.",
		      	the_setup.DbHost, the_setup.DbUser);
    	if (!mysql_real_connect( m_db,
		      	the_setup.DbHost, the_setup.DbUser, the_setup.DbPass, 0,
		      	the_setup.DbPort, the_setup.DbSocket, 0 ) != 0 )
    	{
		mgWarning("Failed to connect to server '%s' as User '%s', Password '%s': %s",
			    	the_setup.DbHost,the_setup.DbUser,the_setup.DbPass,mysql_error(m_db));
        	mysql_close (m_db);
		m_db = 0;
    	}
    }
    if (m_db)
    {
	    m_database_found = mysql_select_db(m_db,the_setup.DbName)==0;
	    {
		    if (!Connected())
		    	if (!createtime)
			    mgWarning(mysql_error(m_db));
	    }
    }
    if (!needGenre2_set && Connected())
    {
	needGenre2_set=true;
	needGenre2=exec_count("SELECT COUNT(DISTINCT genre2) from tracks")>1;
    }
    return;
}


void
mgmySql::CreateFolderFields()
{
  if (!Connected())
	  return;
  if (HasFolderFields())
	  return;
  sql_query("DESCRIBE tracks folder1");
  MYSQL_RES *rows = mysql_store_result(m_db);
  if (rows)
    {
      m_hasfolderfields = mysql_num_rows(rows)>0;
      mysql_free_result(rows);
      if (!m_hasfolderfields)
      {
	      m_hasfolderfields = !sql_query(
	              "alter table tracks add column folder1 varchar(255),"
		      "add column folder2 varchar(255),"
		      "add column folder3 varchar(255),"
		      "add column folder4 varchar(255)");

      }
    }
}

void
database_end()
{
	delete mysqlhandle;
	mysqlhandle=0;
}
