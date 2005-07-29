/*! 
 * \file   mg_db_gd_pg.c
 * \brief  A capsule around postgresql database access
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
#include "mg_db_gd_pg.h"

#include <pg_config.h>

using namespace std;

mgQueryPG::mgQueryPG(void* db,string sql,mgQueryNoise noise)
	: mgQueryImp(db,sql,noise)
{
	m_db = (PGconn*)m_db_handle;
	m_cursor = 0;
	m_table = PQexec(m_db,m_optsql);
	switch ((m_rc=PQresultStatus(m_table))) {
		case PGRES_COMMAND_OK:
    			m_rows = atol(PQcmdTuples(m_table));
			break;;
		case PGRES_TUPLES_OK:
    			m_rows = PQntuples(m_table);
			m_columns = PQnfields(m_table);
			break;
		default:	
			m_errormessage = PQresultErrorMessage (m_table);
			break;
	}
	HandleErrors();
}

mgQueryPG::~mgQueryPG()
{
	PQclear(m_table);
}

char **
mgQueryPG::Next()
{
        if (m_cursor>=Rows())
		return 0;
	assert(Columns()<100);
	memset(m_rowpointers,0,sizeof(m_rowpointers));
	for (int idx=0;idx<Columns();idx++)
		if (!PQgetisnull(m_table,m_cursor,idx))
			m_rowpointers[idx] = PQgetvalue(m_table,m_cursor,idx);
	m_cursor++;
	return m_rowpointers;
}

mgSQLStringPG::~mgSQLStringPG()
{
	if (m_unquoted)
		free(m_unquoted);
}

mgSQLStringPG::mgSQLStringPG(const char*s)
{
	m_unquoted = 0;
	m_original = s;
}

char* 
mgSQLStringPG::unquoted() const
{
	if (!m_unquoted)
	{
  		int buflen=2*strlen(m_original)+5;
  		m_unquoted = (char *) malloc( buflen);
  		PQescapeString(m_unquoted,m_original,strlen(m_original));
	}
	return m_unquoted;
}

const char*
mgDbGd::Options() const
{
	return "hspuw";
}

const char*
mgDbGd::HelpText() const
{
        return
	"  -h HHHH,  --host=HHHH     specify database host (default is localhost)\n"
        "  -s SSSS   --socket=PATH   specify database socket\n"
        "  -p PPPP,  --port=PPPP     specify port of database server (default is )\n"
        "  -u UUUU,  --user=UUUU     specify database user (default is )\n"
        "  -w WWWW,  --password=WWWW specify database password (default is empty)\n";
}

mgDb* GenerateDB(bool SeparateThread)
{
	// \todo should return different backends according to the_setup.Variant
	return new mgDbGd(SeparateThread);
}

static bool needGenre2;
static bool needGenre2_set=false;

mgDbGd::mgDbGd(bool SeparateThread)
{
        m_db = 0;
	if (m_separate_thread)
		if (!Threadsafe())
			mgError("Your database library is not thread safe");
}

mgDbGd::~mgDbGd()
{
  PQfinish (m_db);
  m_db = 0;
}

bool
mgDbGd::Threadsafe()
{
	return ENABLE_THREAD_SAFETY;
}

static char *db_cmds[] = 
{
  "CREATE TABLE album ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "cddbid varchar(20) default '', "
	  "coverimg varchar(255) default NULL, "
	  "covertxt text, "
	  "modified date default NULL, "
	  "genre varchar(10) default NULL, "
	  "PRIMARY KEY  (cddbid))",
  "CREATE INDEX alb_artist ON album (artist)",
  "CREATE INDEX alb_title ON album (title)",
  "CREATE TABLE genre ("
	  "id varchar(10) NOT NULL default '', "
	  "id3genre smallint default NULL, "
	  "genre varchar(255) default NULL, "
	  "freq int default NULL, "
	  "PRIMARY KEY (id))",
  "CREATE TABLE language ("
	  "id varchar(4) NOT NULL default '', "
	  "language varchar(40) default NULL, "
	  "freq int default NULL, "
	  "PRIMARY KEY  (id))",
  "CREATE TABLE musictype ("
	  "musictype varchar(40) default NULL, "
	  "id serial, "
	  "PRIMARY KEY  (id)) ",
  "CREATE TABLE playlist ( "
	  "title varchar(255) default NULL, "
	  "author varchar(255) default NULL, "
	  "note varchar(255) default NULL, "
	  "created timestamp default NULL, "
	  "id serial, "
	  "PRIMARY KEY  (id))",
  "CREATE TABLE playlistitem ( "
	  "playlist int NOT NULL,"
	  "trackid int NOT NULL) WITH OIDS",
   "CREATE TABLE source ( "
	  "source varchar(40) default NULL, "
	  "id serial, "
	  "PRIMARY KEY  (id)) ",
  "CREATE TABLE tracks ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "genre1 varchar(10) default NULL, "
	  "genre2 varchar(10) default NULL, "
	  "year smallint default NULL, "
	  "lang varchar(4) default NULL, "
	  "type smallint default NULL, "
	  "rating smallint default NULL, "
	  "length smallint default NULL, "
	  "source smallint default NULL, "
	  "sourceid varchar(20) default NULL, "
	  "tracknb smallint default NULL, "
	  "mp3file varchar(255) default NULL, "
	  "condition smallint default NULL, "
	  "voladjust smallint default '0', "
	  "lengthfrm int default '0', "
	  "startfrm int default '0', "
	  "bpm smallint default '0', "
	  "lyrics text, "
	  "bitrate varchar(10) default NULL, "
	  "created date default NULL, "
	  "modified date default NULL, "
	  "backup smallint default NULL, "
	  "samplerate int default NULL, "
	  "channels   smallint default NULL,  "
	  "id serial, "
	  "folder1 varchar(255), "
	  "folder2 varchar(255), "
	  "folder3 varchar(255), "
	  "folder4 varchar(255), "
	  "PRIMARY KEY  (id))",
  "CREATE INDEX tracks_title ON tracks (title)",
  "CREATE INDEX tracks_sourceid ON tracks (sourceid)",
  "CREATE INDEX tracks_mp3file ON tracks (mp3file)",
  "CREATE INDEX tracks_genre1 ON tracks (genre1)",
  "CREATE INDEX tracks_genre2 ON tracks (genre2)",
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
  if (!Connect())
	  return false;
  return myCreate();
}

bool
mgDbGd::myCreate()
{
  // create database and tables
  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=0; i < len; i ++ )
    {
	mgQuery q(m_db,db_cmds[i],mgQueryWarnOnly);
  	if (!q.ErrorMessage().empty())
		return false;
    }
  m_database_found=true;
  FillTables();
  return true;
}

bool
mgDbGd::ServerConnect () 
{
    return true;
}


bool
mgDbGd::Connect ()
{
    if (m_database_found)
	return true;
    char conninfo[500];
    char port[20];
    char host[200];
    if (the_setup.DbPort>0)
	    sprintf(port," port = %d ",the_setup.DbPort);
    else
	    port[0]=0;
    if (notempty(the_setup.DbHost))
	snprintf(host,199," host = %s ",the_setup.DbHost);
    else if (notempty(the_setup.DbSocket))
	snprintf(host,199," host = %s ",the_setup.DbSocket);
    else
	host[0]=0;
    snprintf(conninfo,499,"%s %s dbname = %s user = %s ",
		    host,port,the_setup.DbName,the_setup.DbUser);
    m_db = PQconnectdb(conninfo);
    if (PQstatus(m_db) != CONNECTION_OK)
    {
	    mgWarning("Failed to connect to postgres server using %s:%s",conninfo,PQerrorMessage(m_db));
	    return false;
    }
    m_database_found = true;	// otherwise we get into a recursion
    m_database_found = exec_count("SELECT COUNT(*) FROM information_schema.tables WHERE table_name='album'");
    if (m_database_found)
	return true;
    if (time(0)<m_create_time+10)
	return false;
    m_create_time=time(0);
    extern bool create_question();
    extern bool import();
    if (create_question())
    {
	    m_database_found = true;
	    if (myCreate())
	    {
	    	import();
		return true;
	    }
    }
    m_database_found = false;
    mgWarning(PQerrorMessage(m_db));
    return false;
}

bool 
mgDbGd::NeedGenre2() 
{
    if (!needGenre2_set && Connect())
    {
	needGenre2_set=true;
	needGenre2=exec_count("SELECT COUNT(DISTINCT genre2) FROM tracks")>1;
    }
    return needGenre2;
}


bool
mgDbGd::FieldExists(string table, string field)
{
    	if (!Connect()) 
		return false;
    	char *b;
        asprintf(&b,"SELECT COUNT(*) FROM information_schema.columns WHERE table_name='album' AND column_name='%s'",field.c_str());
    	bool result = exec_count(b)==1;
	free(b);
	return result;
}

const char*
mgDbGd::DecadeExpr()
{
	return "substring(10 * floor(tracks.year/10),3)";
}

