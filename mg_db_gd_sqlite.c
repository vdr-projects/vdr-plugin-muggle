/*! 
 * \file   mg_db_gd_sqlite.c
 * \brief  A capsule around database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-04-13 17:42:54 +0100 (Thu, 13 Apr 2005) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wolfgang61 $
 */

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>


#include "mg_setup.h"
#include "mg_item_gd.h"
#include "mg_db_gd_sqlite.h"

using namespace std;

mgQuerySQLite::mgQuerySQLite(void* db,string sql,mgQueryNoise noise)
	: mgQueryImp(db,sql,noise)
{
	sqlite3* m_db = (sqlite3*)m_db_handle;
	m_table = 0;
	char *p;
	if (!strncmp(m_optsql,"SELECT ",7))
		m_rc = sqlite3_get_table(m_db,m_optsql,&m_table,&m_rows,&m_columns,&p);
	else
	{
		m_rc = sqlite3_exec(m_db,m_optsql,0,0,&p);
		if (m_rc==SQLITE_OK)
			m_rows = sqlite3_changes(m_db);
	}
	m_errormessage= (const char*)p;
	HandleErrors();
}

mgQuerySQLite::~mgQuerySQLite()
{
	sqlite3_free_table(m_table);
}

char **
mgQuerySQLite::Next()
{
	if (m_cursor>=m_rows)
		return 0;
	m_cursor++;
	return &m_table[m_columns*m_cursor];	// skip header row
}

mgSQLStringSQLite::~mgSQLStringSQLite()
{
	if (m_unquoted)
		sqlite3_free(m_unquoted);
}

mgSQLStringSQLite::mgSQLStringSQLite(const char*s)
{
	m_unquoted = 0;
	m_original = s;
}

char* 
mgSQLStringSQLite::unquoted() const
{
	if (!m_unquoted)
		m_unquoted = sqlite3_mprintf("%q",m_original);
	return m_unquoted;
}

const char*
mgDbGd::Options() const
{
	return "";
}

const char*
mgDbGd::HelpText() const
{
	return "";
}

mgDb* GenerateDB(bool SeparateThread)
{
	// \todo should return different backends according to the_setup.Variant
	return new mgDbGd(SeparateThread);
}


mgDbGd::mgDbGd(bool SeparateThread)
{
        m_db = 0;
	if (m_separate_thread)
		if (!Threadsafe())
			mgError("Your database library is not thread safe");
}

mgDbGd::~mgDbGd()
{
  sqlite3_close (m_db);
  m_db = 0;
}

bool
mgDbGd::Threadsafe()
{
	return false;
}

static char *db_cmds[] = 
{
  "drop table album",
  "CREATE TABLE album ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "cddbid varchar(20) NOT NULL default '', "
	  "coverimg varchar(255) default NULL, "
	  "covertxt mediumtext, "
	  "modified date default NULL, "
	  "genre varchar(10) default NULL, "
	  "PRIMARY KEY  (cddbid))",
  "CREATE INDEX idx_album_artist ON album (artist)",
  "CREATE INDEX idx_album_title ON album (title)",
  "drop table genre",
  "CREATE TABLE genre ("
	  "id varchar(10) NOT NULL default '', "
	  "id3genre smallint(6) default NULL, "
	  "genre varchar(255) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY (id))",
  "drop table language",
  "CREATE TABLE language ("
	  "id varchar(4) NOT NULL default '', "
	  "language varchar(40) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY  (id))",
  "drop table musictype;",
  "CREATE TABLE musictype ("
	  "musictype varchar(40) default NULL, "
	  "id integer PRIMARY KEY autoincrement)",
  "drop table playlist",
  "CREATE TABLE playlist ( "
	  "title varchar(255) default NULL, "
	  "author varchar(255) default NULL, "
	  "note varchar(255) default NULL, "
	  "created timestamp(8) NOT NULL, "
	  "id integer PRIMARY KEY autoincrement)",
"drop table playlistitem",
  "CREATE TABLE playlistitem ( "
	  "playlist integer NOT NULL, "
	  "trackid int(11) NOT NULL)",
  "CREATE INDEX playlistitem_1 on playlistitem (playlist)",
  "drop table tracklistitem",
  "CREATE TABLE tracklistitem ( "
	  "playerid int(11) NOT NULL default '0', "
	  "listtype smallint(6) NOT NULL default '0', "
	  "tracknb int(11) NOT NULL default '0', "
	  "trackid int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (playerid,listtype,tracknb))",
  "drop table source",
  "CREATE TABLE source ( "
	  "source varchar(40) default NULL, "
	  "id integer PRIMARY KEY autoincrement)",
  "drop table tracks",
  "CREATE TABLE tracks ( "
	  "id integer PRIMARY KEY autoincrement, "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "genre1 varchar(10) default NULL, "
	  "genre2 varchar(10) default NULL, "
	  "year smallint(5) default NULL, "
	  "lang varchar(4) default NULL, "
	  "type tinyint(3) default NULL, "
	  "rating tinyint(3) default NULL, "
	  "length smallint(5) default NULL, "
	  "source tinyint(3) default NULL, "
	  "sourceid varchar(20) default NULL, "
	  "tracknb tinyint(3) default NULL, "
	  "mp3file varchar(255) default NULL, "
	  "condition tinyint(3) default NULL, "
	  "voladjust smallint(6) default '0', "
	  "lengthfrm mediumint(9) default '0', "
	  "startfrm mediumint(9) default '0', "
	  "bpm smallint(6) default '0', "
	  "lyrics mediumtext, "
	  "bitrate varchar(10) default NULL, "
	  "created date default NULL, "
	  "modified date default NULL, "
	  "backup tinyint(3) default NULL, "
	  "samplerate int(7) default NULL, "
	  "channels   tinyint(3) default NULL,  "
	  "folder1 varchar(255), "
	  "folder2 varchar(255), "
	  "folder3 varchar(255), "
	  "folder4 varchar(255)); ",
  "CREATE INDEX tracks_title ON tracks (title)",
  "CREATE INDEX tracks_sourceid ON tracks (sourceid)",
  "CREATE INDEX tracks_mp3file ON tracks (mp3file)",
  "CREATE INDEX tracks_genre1 ON tracks (genre1)",
  "CREATE INDEX tracks_year ON tracks (year)",
  "CREATE INDEX tracks_lang ON tracks (lang)",
  "CREATE INDEX tracks_artist ON tracks (artist)",
  "CREATE INDEX tracks_rating ON tracks (rating)",
  "CREATE INDEX tracks_folder1 ON tracks (folder1)",
  "CREATE INDEX tracks_folder2 ON tracks (folder2)",
  "CREATE INDEX tracks_folder3 ON tracks (folder3)",
  "CREATE INDEX tracks_folder4 ON tracks (folder4)",
};

void
mgDbGd::StartTransaction()
{
	Execute("BEGIN TRANSACTION");
}

void
mgDbGd::Commit()
{
	Execute("COMMIT");
}

bool
mgDbGd::Create()
{
  // create database and tables
  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=0; i < len; i ++ )
    {
	mgQuery q(m_db,db_cmds[i],mgQuerySilent);
	if (!q.ErrorMessage().empty())
		if (strncmp(db_cmds[i],"drop ",5))
  	{
    		mgWarning("%20s: %s",db_cmds[i],q.ErrorMessage().c_str());
		return false;
	}
    }
  m_database_found=true;
  FillTables();
  return true;
}

void
mgDirectory(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	assert(argc==1);
	char *buf=strdup((char*)sqlite3_value_text(argv[0]));
	char *slash=strrchr(buf,'/');
	if (!slash)
		slash=buf;
	*slash=0;
	sqlite3_result_text(context,buf,strlen(buf),free);
}

void
mgDecade(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	assert(argc==1);
	unsigned int year=sqlite3_value_int(argv[0]);
	char *buf;
	asprintf(&buf,"%02d",(year-year%10)%100);
	sqlite3_result_text(context,buf,2,free);
}

void
mgSubstring(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	assert(argc==3);
	char*full=(char*)sqlite3_value_text(argv[0]);
	unsigned int opos=sqlite3_value_int(argv[1]);
	unsigned int pos=opos;
	unsigned int olen=sqlite3_value_int(argv[2]);
	unsigned int len=olen;
	if (pos>strlen(full))
		pos=strlen(full);
	if (len==0)
		len=99999;
	if (len>strlen(full+pos-1))
		len=strlen(full+pos-1);
	char *buf=strndup(full+pos-1,len);
	sqlite3_result_text(context,buf,len,free);
}

bool
mgDbGd::Connect ()
{
    if (m_database_found)
	return true;
    if (time(0)<m_create_time+10)
	return false;
    m_create_time=time(0);
    char *s=sqlite3_mprintf("%s/%s.sqlite",the_setup.DbDatadir,the_setup.DbName);
    mgDebug(1,"%X opening data base %s",m_db,s);
    int rc = sqlite3_open(s,&m_db);
    m_database_found = rc==SQLITE_OK;
    if (!m_database_found)
    {
	mgWarning("Cannot open/create SQLite database %s:%d/%s",
		s,rc,sqlite3_errmsg(m_db));
    	sqlite3_free(s);
	return false;
     }
     sqlite3_free(s);
     rc = sqlite3_create_function(m_db,"mgDirectory",1,SQLITE_UTF8,
	    0,&mgDirectory,0,0);
     if (rc!=SQLITE_OK)
     {
     	mgWarning("Cannot define mgDirectory:%d/%s",rc,sqlite3_errmsg);
	return false;
     }
     rc = sqlite3_create_function(m_db,"substring",3,SQLITE_UTF8,
	    0,&mgSubstring,0,0);
     if (rc!=SQLITE_OK)
     {
     	mgWarning("Cannot define mgSubstring:%d/%s",rc,sqlite3_errmsg);
	return false;
     }
     rc = sqlite3_create_function(m_db,"decade",1,SQLITE_UTF8,
	    0,&mgDecade,0,0);
     if (rc!=SQLITE_OK)
     {
     	mgWarning("Cannot define decade:%d/%s",rc,sqlite3_errmsg);
	return false;
     }
     if (!FieldExists("tracks","id"))
     {
     	extern bool create_question();
     	if (!create_question())
     		return false;
     	if (!Create())
		return false;
     	extern bool import();
     	import();
     }
     return true;
}

bool 
mgDbGd::NeedGenre2() 
{
//    we do not support genre2 because queries like
//    SELECT title FROM tracks WHERE (genre1='m' or genre2='m')
//    are very slow with sqlite
    return false;
}

bool
mgDbGd::FieldExists(string table, string field)
{
    	if (!Connect()) 
		return false;
    	char *b;
    	asprintf(&b,"SELECT %s FROM %s LIMIT 1",field.c_str(),table.c_str());
	mgQuery q(m_db,b,mgQuerySilent);
    	free(b);
	return q.ErrorMessage().empty();
}

const char*
mgDbGd::DecadeExpr()
{
	return "Decade(tracks.year)";
}

