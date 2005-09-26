/*! 
 * \file   mg_db_gd_mysql.c
 * \brief  A capsule around database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-04-13 17:42:54 +0100 (Thu, 13 Apr 2005) $
 * \author  Wolfgang Rohdewald * \author  Responsible author: $Author: wolfgang61 $ */

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
#include "mg_db_gd_mysql.h"

using namespace std;

mgSQLStringMySQL::~mgSQLStringMySQL()
{
	if (m_unquoted)
		free(m_unquoted);
}

mgSQLStringMySQL::mgSQLStringMySQL(const char*s)
{
	m_unquoted = 0;
	m_original = s;
}

char* 
mgSQLStringMySQL::unquoted() const
{
	if (!m_unquoted)
	{
		int buflen=2*strlen(m_original)+3;
  		m_unquoted = (char *) malloc( buflen);
		mgDb* esc = DbServer->EscapeDb();
  		if (esc && esc->DbHandle())
  			mysql_real_escape_string( (MYSQL*)esc->DbHandle(),
				m_unquoted, m_original, strlen(m_original) );
  		else
			strcpy(m_unquoted,m_original);
	}
	return m_unquoted;
}


mgQueryMySQL::mgQueryMySQL(void* db,string sql,mgQueryNoise noise)
	: mgQueryImp(db,sql,noise)
{
	m_db = (MYSQL*)m_db_handle;
	m_table = 0;
	if ((m_rc=mysql_query(m_db,m_optsql)))
		m_errormessage = mysql_error (m_db);
	else
	{
		m_table = mysql_store_result (m_db);
      		m_rows = mysql_affected_rows(m_db);
		if (m_table)
        		m_columns = mysql_num_fields(m_table);
	}
	HandleErrors();
}

mgQueryMySQL::~mgQueryMySQL()
{
	mysql_free_result (m_table);
	m_table = 0;
}

char **
mgQueryMySQL::Next()
{
	if (!m_table)
		return 0;
	else
        	return mysql_fetch_row(m_table);
}

const char*
mgDbGd::Options() const
{
#if !defined(HAVE_ONLY_SERVER) && MYSQL_VERSION_ID < 40111
	return "";
#else
	return "hspuw";
#endif
}

const char*
mgDbGd::HelpText() const
{
#if !defined(HAVE_ONLY_SERVER) && MYSQL_VERSION_ID < 40111
	return "";
#else
	return
	"  -h HHHH,  --host=HHHH     specify database host (default is embedded or localhost)\n"
	"                            if the specified host is localhost, sockets will\n"
	"                            be used if possible.\n"
	"			     Otherwise the -s parameter will be ignored\n"
        "  -s SSSS   --socket=PATH   specify database socket\n"
        "  -p PPPP,  --port=PPPP     specify port of database server (default is )\n"
        "  -u UUUU,  --user=UUUU     specify database user (default is )\n"
        "  -w WWWW,  --password=WWWW specify database password (default is empty)\n";
#endif
}

mgDb* GenerateDB(bool SeparateThread)
{
	// \todo should return different backends according to the_setup.Variant
	return new mgDbGd(SeparateThread);
}

static bool needGenre2;
static bool needGenre2_set=false;

bool UsingEmbeddedMySQL();

mgDbGd::mgDbGd(bool SeparateThread)
{
        m_db = 0;
	if (m_separate_thread)
	{
		if (Threadsafe())
			mysql_thread_init();
		else
			mgError("Your Mysql version is not thread safe");
	}
}

mgDbGd::~mgDbGd()
{
  if (m_db)
	 mysql_close (m_db);
  m_db = 0;
#if MYSQL_VERSION_ID >=400000
  if (m_separate_thread)
	mysql_thread_end();
#endif
}

bool
mgDbGd::Threadsafe()
{
#if defined THREAD_SAFE_CLIENT && MYSQL_VERSION_ID >=400000
	// 3.23 does define THREAD_SAFE_CLIENT but has no mysql_thread_init.
	// So we assume we should better not assume threading to be safe
	return true;
#else
	return false;
#endif
}

#ifndef HAVE_ONLY_SERVER
static char *mysql_embedded_args[] =
{
	"muggle",	
	0,	// placeholder for --datadir=		
	"--key_buffer_size=32M"
};

static void
wrong_embedded_mysql_for_external_server(int version)
{
	mgError("You are using the embedded mysql library. For accessing external servers "
		"you need mysql 040111 but you have only %06d", version);
	abort();
}
#endif

mgDbServerMySQL::mgDbServerMySQL()
{
#ifndef HAVE_ONLY_SERVER
	static char *mysql_embedded_groups[] =
	{
		"embedded",
		"server",
		"muggle_SERVER",
		0
	};
	int argv_size;
	if (UsingEmbeddedMySQL())
	{
		argv_size = sizeof(mysql_embedded_args) / sizeof(char *);
		struct stat stbuf;
		if (stat(the_setup.DbDatadir,&stbuf))
			mkdir(the_setup.DbDatadir,0755);
		if (stat(the_setup.DbDatadir,&stbuf))
		{
			mgError("Cannot access mysqldata directory %s: errno=%d",
					the_setup.DbDatadir,errno);
			abort();
		}
		asprintf(&mysql_embedded_args[1],"--datadir=%s",the_setup.DbDatadir);
		mgDebug(1,"calling mysql_server_init for embedded in %s",the_setup.DbDatadir);
	}
	else
	{
#if MYSQL_VERSION_ID < 40111
		// compile time check
		wrong_embedded_mysql_for_external_server(MYSQL_VERSION_ID);
#endif
#if MYSQL_VERSION_ID >= 40101
		// runtime check
		if (mysql_get_client_version()<40111) // this function was added for embedded library in MySQL 4.1.1
			wrong_embedded_mysql_for_external_server(mysql_get_client_version());
#endif
		mgDebug(1,"calling mysql_server_init for external server");
		argv_size = -1;
	}
	if (mysql_server_init(argv_size, mysql_embedded_args, mysql_embedded_groups))
	{
		mgError("mysql_server_init failed");
		abort();
	}
#endif
	m_escape_db = new mgDbGd;
}

mgDbServerMySQL::~mgDbServerMySQL()
{
#ifndef HAVE_ONLY_SERVER
  mgDebug(3,"calling mysql_server_end");
  	mysql_server_end();
#endif
}



static char *db_cmds[] = 
{
  "drop table if exists album",
  "CREATE TABLE album ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "composer varchar(255) default NULL, "
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
	  "TYPE=MyISAM",
  "drop table if exists genre",
  "CREATE TABLE genre ("
	  "id varchar(10) NOT NULL default '', "
	  "id3genre smallint(6) default NULL, "
	  "genre varchar(255) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY (id)) "
	  "TYPE=MyISAM",
  "drop table if exists language",
  "CREATE TABLE language ("
	  "id varchar(4) NOT NULL default '', "
	  "language varchar(40) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM",
  "drop table if exists musictype",
  "CREATE TABLE musictype ("
	  "musictype varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM",
  "drop table if exists player",
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
	  "TYPE=MyISAM",
  "drop table if exists playerstate",
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
	  "TYPE=HEAP",
  "drop table if exists playlist",
  "CREATE TABLE playlist ( "
	  "title varchar(255) default NULL, "
	  "author varchar(255) default NULL, "
	  "note varchar(255) default NULL, "
	  "created timestamp(8) NOT NULL, "
	  "id int(10) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM",
  "drop table if exists playlistitem",
  "CREATE TABLE playlistitem ( "
	  "playlist int(11) NOT NULL default '0', "
	  "tracknumber mediumint(9) NOT NULL default '0', "
	  "trackid int(11) default NULL, "
	  "PRIMARY KEY  (playlist,tracknumber)) "
	  "TYPE=MyISAM",
  "drop table if exists playlog",
  "CREATE TABLE playlog ( "
	  "trackid int(11) default NULL, "
	  "played date default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM",
  "drop table if exists recordingitem",
  "CREATE TABLE recordingitem ( "
	  "trackid int(11) default NULL, "
	  "recdate date default NULL, "
	  "rectime time default NULL, "
	  "reclength int(11) default NULL, "
	  "enddate date default NULL, "
	  "endtime time default NULL,  "
	  "repeating varchar(10) default NULL, "
	  "initcmd varchar(255) default NULL, "
	  "parameters varchar(255) default NULL, "
	  "atqjob int(11) default NULL, "
	  "id int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM",
  "drop table if exists source",
	  "CREATE TABLE source ( "
	  "source varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM",
  "drop table if exists tracklistitem",
  "CREATE TABLE tracklistitem ( "
	  "playerid int(11) NOT NULL default '0', "
	  "listtype smallint(6) NOT NULL default '0', "
	  "tracknb int(11) NOT NULL default '0', "
	  "trackid int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (playerid,listtype,tracknb)) "
	  "TYPE=MyISAM",
  "drop table if exists tracks",
  "CREATE TABLE tracks ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "composer varchar(255) default NULL, "
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
	  "quality tinyint(3) unsigned default NULL, "
	  "voladjust smallint(6) default '0', "
	  "lengthfrm mediumint(9) default '0', "
	  "startfrm mediumint(9) default '0', "
	  "bpm smallint(6) default '0', "
	  "lyrics mediumtext, "
	  "moreinfo mediumtext, "
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
	  "TYPE=MyISAM"
};

void
mgDbGd::StartTransaction()
{
	Execute("START TRANSACTION");
}

void
mgDbGd::Commit()
{
	Execute("COMMIT");
}

bool
mgDbGd::Create()
{
  if (!ServerConnect())
	  return false;
  // create database and tables
  mgDebug(1,"Dropping and recreating database %s",the_setup.DbName);
  char buffer[500];
  sprintf(buffer,"DROP DATABASE IF EXISTS %s",the_setup.DbName);
  if (strlen(buffer)>400)
	  mgError("name of database too long: %s",the_setup.DbName);
  mgQuery q(m_db,buffer);
  if (!q.ErrorMessage().empty())
	return false;
  sprintf(buffer,"CREATE DATABASE %s",the_setup.DbName);
  mgQuery q1(m_db,buffer);
  if (!q1.ErrorMessage().empty())
	return false;
  if (!UsingEmbeddedMySQL())
	sprintf(buffer,"grant all privileges on %s.* to vdr@localhost",
			the_setup.DbName);
  	Execute(buffer);
  	// ignore error. If we can create the data base, we can do everything
  	// with it anyway.

  if (mysql_select_db(m_db,the_setup.DbName))
	  mgError("mysql_select_db(%s) failed with %s",mysql_error(m_db));

  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=0; i < len; i ++ )
    {
        mgQuery q(m_db, db_cmds[i],mgQueryWarnOnly);
	if (!q.ErrorMessage().empty())
  	{
		sprintf(buffer,"DROP DATABASE IF EXISTS %s",the_setup.DbName);
		Execute(buffer);
		return false;
	}
    }
  m_database_found=true;
  FillTables();
  return true;
}


bool
mgDbGd::ServerConnect () 
{
    if (m_db)
	return true;
    if (time(0)<m_connect_time+10)
	return false;
    m_connect_time=time(0);
    if (!DbServer)
	DbServer = new mgDbServer;
    m_db = mysql_init (0);
    if (!m_db)
        return false;
    if (UsingEmbeddedMySQL())
    {
#ifndef HAVE_ONLY_SERVER
    	if (!mysql_real_connect(m_db, 0, 0, 0, 0, 0, 0, 0))
		mgWarning("Failed to connect to embedded mysql in %s:%s ",
				the_setup.DbDatadir,mysql_error(m_db));
    	else
		mgDebug(1,"Connected to embedded mysql in %s",
				the_setup.DbDatadir);
#endif
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
    return m_db!=0;
}


bool
UsingEmbeddedMySQL()
{
#ifdef HAVE_ONLY_SERVER
	return false;
#else
	return the_setup.NoHost();
#endif
}

bool
mgDbGd::Connect ()
{
    if (m_database_found)
	return true;
    if (!ServerConnect())
    	return false;
    if (time(0)<m_create_time+10)
	return false;
    m_create_time=time(0);
    m_database_found = mysql_select_db(m_db,the_setup.DbName)==0;
    if (m_database_found)
	return true;
    extern bool create_question();
    if (!create_question())
    {
    	mgWarning("Database not created");
    	return false;
    }
    m_database_found = Create();
    if (!m_database_found)
    	mgWarning("Cannot create database:%s",mysql_error(m_db));
    return m_database_found;
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

void
mgDbGd::CreateFolderFields()
{
  if (HasFolderFields())
	  return;
  mgQuery q(m_db, "DESCRIBE tracks folder1");
  m_hasfolderfields = q.Rows()>0;
  if (!m_hasfolderfields)
  {
  	mgQuery q(m_db,
		"alter table tracks add column folder1 varchar(255),"
    		"add column folder2 varchar(255),"
    		"add column folder3 varchar(255),"
    		"add column folder4 varchar(255)");
	m_hasfolderfields = q.ErrorMessage().empty();

  }
}

int
mgDbGd::AddToCollection( const string Name,const vector<mgItem*>&items, mgParts* what)
{
    if (!Connect()) return 0;
    CreateCollection(Name);
    string listid = mgSQLString (get_col0
        (string("SELECT id FROM playlist WHERE title=")
	 + mgSQLString(Name).quoted())).quoted();
    unsigned int tracksize = items.size();
    if (tracksize==0)
	    return 0;

    // this code is rather complicated but works in a multi user
    // environment:
 
    // insert a unique trackid:
    string trackid = ltos(thread_id()+1000000);
    Execute("INSERT INTO playlistitem SELECT "+listid+","
	   "MAX(tracknumber)+"+ltos(tracksize)+","+trackid+
	   " FROM playlistitem WHERE playlist="+listid);
    
    // find tracknumber of the trackid we just inserted:
    string sql = string("SELECT tracknumber FROM playlistitem WHERE "
		    "playlist=")+listid+" AND trackid="+trackid;
    long first = atol(get_col0(sql).c_str()) - tracksize + 1;

    // replace the place holder trackid by the correct value:
    Execute("UPDATE playlistitem SET trackid="+ltos(items[tracksize-1]->getItemid())+
		    " WHERE playlist="+listid+" AND trackid="+trackid);
    
    // insert all other tracks:
    const char *sql_prefix = "INSERT INTO playlistitem VALUES ";
    sql = "";
    for (unsigned int i = 0; i < tracksize-1; i++)
    {
	string item = "(" + listid + "," + ltos (first + i) + "," +
            ltos (items[i]->getItemid ()) + ")";
	comma(sql, item);
	if ((i%100)==99)
	{
    		Execute (sql_prefix+sql);
		sql = "";
	}
    }
    if (!sql.empty()) Execute (sql_prefix+sql);
    return tracksize;
}

int
mgDbGd::RemoveFromCollection (const string Name, const vector<mgItem*>&items, mgParts* what)
{
    if (Name.empty())
	    return 0;
    if (!Connect()) return 0;
    string pid = KeyMaps.id(keyGdCollection,Name);
    if (pid.empty())
	    return 0;
    what->Prepare();
    what->tables.push_front("playlistitem as del");
    what->clauses.push_back("del.playlist="+pid);
    bool usesTracks = false;
    for (list < string >::iterator it = what->tables.begin (); it != what->tables.end (); ++it)
	if (*it == "tracks")
	{
		usesTracks = true;
		break;
	}
    if (usesTracks)
	what->clauses.push_back("del.trackid=tracks.id");
    else
	what->clauses.push_back("del.trackid=playlistitem.trackid");
    string sql = "DELETE playlistitem";
    sql += sql_list(" FROM",what->tables);
    sql += sql_list(" WHERE",what->clauses," AND ");
    return Execute (sql);
}

bool
mgDbGd::FieldExists(string table, string field)
{
    	if (!Connect()) 
		return false;
    	char *b;
    	asprintf(&b,"DESCRIBE %s %s",table.c_str(),field.c_str());
    	mgQuery q(m_db,b);
    	free(b);
    	if (q.Next())
		return q.Rows() == 1;
	else
		return false;
}

const char*
mgDbGd::DecadeExpr()
{
	return "substring(10 * floor(tracks.year/10),3)";
}

