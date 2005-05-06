/*! 
 * \file   mg_db_gd.c
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

#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>

#include "mg_setup.h"
#include "mg_item_gd.h"
#include "mg_db_gd.h"

using namespace std;


static bool needGenre2;
static bool needGenre2_set=false;

//! \brief adds string n to string s, using a comma to separate them
static string comma (string &s, string n);

static string
comma (string & s, string n)
{
	    return addsep (s, ",", n);
}

class mysqlhandle_t {
	public:
		mysqlhandle_t();
		~mysqlhandle_t();
};

static mysqlhandle_t* mysqlhandle;

bool UsingEmbeddedMySQL();

mgDbGd::mgDbGd(bool SeparateThread)
{
        m_db = 0;
        if (!mysqlhandle)
                mysqlhandle = new mysqlhandle_t;
#if MYSQL_VERSION_ID >=400000
	// when exactly was this introduced? After 3.23 anyway
	if (m_separate_thread)
		mysql_thread_init();
#endif
}

mgDbGd::~mgDbGd()
{
  mysql_close (m_db);
  m_db = 0;
#if MYSQL_VERSION_ID >=400000
	// when exactly was this introduced? After 3.23 anyway
  if (m_separate_thread)
	mysql_thread_end();
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

mysqlhandle_t::mysqlhandle_t()
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
}

mysqlhandle_t::~mysqlhandle_t()
{
#ifndef HAVE_ONLY_SERVER
  mgDebug(3,"calling mysql_server_end");
  	mysql_server_end();
#endif
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
mgDbGd::sql_query(string sql)
{
	//const char * optsql = optimize(sql).c_str();
	const char * optsql = sql.c_str();
  	mgDebug(4,"mysql_query(%X,%s)",m_db,optsql);
	bool result = !mysql_query(m_db,optsql);
	if (!result)
	{
    		mgError("SQL Error in %s: %s",optsql,mysql_error (m_db));
    		std::cout<<"ERROR in " << optsql << ":" << mysql_error(m_db)<<std::endl;
	}
	return result;
}


MYSQL_RES*
mgDbGd::exec_sql( string query)
{
  if (query.empty())
	  return 0;
  if (!Connect())
	  return 0;
  if (!sql_query (query))
	  return 0;
  else
	  return mysql_store_result (m_db);
}

string
mgDbGd::get_col0( const string query)
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
mgDbGd::exec_count( const string query) 
{
	if (!Connect())
		return 0;
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

void mgDbGd::FillTables()
{
#include "mg_tables.h"
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
  if (sql_query(buffer))
  {
  	mgWarning("Cannot drop %s:%s",the_setup.DbName,
			mysql_error (m_db));
	return false;
  }
  sprintf(buffer,"CREATE DATABASE %s",the_setup.DbName);
  if (sql_query(buffer))
  {
  	mgWarning("Cannot create %s:%s",the_setup.DbName,mysql_error (m_db));
	return false;
  }

  if (!UsingEmbeddedMySQL())
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
		return false;
	}
    }
  m_database_found=true;
  FillTables();
  return true;
}

char*
mgDbGd::sql_Cstring( const char *s, char *buf)
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
  if (!ServerConnect())
	strcpy(b+1,s);
  else
  	mysql_real_escape_string( m_db, b+1, s, strlen(s) );
  *(strchr(b,0)+1)=0;
  *(strchr(b,0))='\'';
  return b;
}

bool
mgDbGd::ServerConnect () 
{
    if (m_db)
	return true;
    if (time(0)<m_connect_time+10)
	return false;
    m_connect_time=time(0);
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
    extern bool import();
    if (create_question())
    {
	    char *argv[2];
	    argv[0]=".";
	    argv[1]=0;
	    if (Create())
	    {
	    	import();
		return true;
	    }
    }
    mgWarning(mysql_error(m_db));
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

void
mgDbGd::CreateFolderFields()
{
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
mgDbGd::ServerEnd()
{
	delete mysqlhandle;
	mysqlhandle=0;
}

char *
mgDbGd::sql_Cstring(TagLib::String s,char *buf)
{
	return sql_Cstring(s.toCString(),buf);
}


TagLib::String
mgDbGd::getlanguage(const char *filename)
{
      TagLib::String result = "";
      TagLib::ID3v2::Tag * id3v2tag=0;
      if (!strcasecmp(extension(filename),"flac"))
      {
      	TagLib::FLAC::File f(filename);
	id3v2tag = f.ID3v2Tag();
      	if (id3v2tag)
      	{
      	  TagLib::ID3v2::FrameList l = id3v2tag->frameListMap()["TLAN"];
      	  if (!l.isEmpty())
  	    result = l.front()->toString();
        }
      }
      else if (!strcasecmp(extension(filename),"mp3"))
      {
      	TagLib::MPEG::File f(filename);
	id3v2tag = f.ID3v2Tag();
        if (id3v2tag)
        {
      	  TagLib::ID3v2::FrameList l = id3v2tag->frameListMap()["TLAN"];
      	  if (!l.isEmpty())
  	    result = l.front()->toString();
        }
      }
      return result;
}

char *
mgDbGd::getAlbum(const char *filename,const char *c_album,const char *c_artist)
{
	char * result;
	char *b;
	asprintf(&b,"SELECT cddbid FROM album"
			" WHERE title=%s AND artist=%s",c_album,c_artist);
	result=sql_Cstring(get_col0(b));
	free(b);
	if (!strcmp(result,"'NULL'"))
	{
		char c_directory[520];
		sql_Cstring(filename,c_directory);
		char *slash=strrchr(c_directory,'/');
		if (slash)
		{
			*slash='\'';
			*(slash+1)=0;
		}
		const char *directory="substring(tracks.mp3file,1,length(tracks.mp3file)"
			"-instr(reverse(tracks.mp3file),'/'))";
		char *where;
		asprintf(&where,"WHERE tracks.sourceid=album.cddbid "
			"AND %s=%s "
			"AND album.title=%s",
			directory,c_directory,
			c_album);

		// how many artists will the album have after adding this one?
		asprintf(&b,"SELECT distinct album.artist FROM tracks, album %s ",where);
        	MYSQL_RES *rows = exec_sql (b);
		free(b);
		long new_album_artists = m_db->affected_rows;
		char buf[520];
		if (new_album_artists==1)
		{
			MYSQL_ROW row = mysql_fetch_row(rows);
			sql_Cstring(row[0],buf);
			mgDebug(1,"c_artist=%s,buf=%s",c_artist,buf);
			if (strcmp(buf,c_artist))
				new_album_artists++;
		}
		else
			mgDebug(1,"c_artist is unique:%s",c_artist);
		if (new_album_artists>1 && strcmp(buf,"'Various Artists'"))
			// is the album multi artist and not yet marked as such?
		{
			asprintf(&b,"SELECT album.cddbid FROM tracks, album %s",where);
			free(result);
			result=sql_Cstring(get_col0(b));
			free(b);
			asprintf(&b,"UPDATE album SET artist='Various Artists' WHERE cddbid=%s",result);
			exec_sql(b);
			// here we could change all tracks.sourceid to result and delete
			// the other album entries for this album, but that should only 
			// be needed if a pre 0.1.4 import has been done incorrectly, so we
			// don't bother
		}
		else
		{				// no usable album found
			free(result);
			asprintf(&result,"'%ld-%9s",random(),c_artist+1);
			char *p=strchr(result,0)-1;
			if (*p!='\'')
				*p='\'';
			asprintf(&b,"INSERT INTO album SET title=%s,artist=%s,cddbid=%s",
				c_album,c_artist,result);
			exec_sql(b);
			free(b);
		}
		mysql_free_result(rows);	
		free(where);
	}
	return result;
}

void
mgDbGd::SyncFile(const char *filename)
{
      	char *ext = extension(filename);
	if (strcasecmp(ext,"flac")
		&& strcasecmp(ext,"ogg")
		&& strcasecmp(ext,"mp3"))
		return;
	if (!strncmp(filename,"./",2))	// strip leading ./
		filename += 2;
	const char *cfilename=filename;
	if (isdigit(filename[0]) && isdigit(filename[1]) && filename[2]=='/' && !strchr(filename+3,'/'))
		cfilename=cfilename+3;
	if (strlen(cfilename)>255)
	{
		mgWarning("Length of file exceeds database field capacity: %s", filename);
		return;
	}
	TagLib::FileRef f( filename) ;
	if (f.isNull())
		return;
	if (!f.tag())
		return;
	mgDebug(3,"Importing %s",filename);
        TagLib::AudioProperties *ap = f.audioProperties();
	char c_artist[520];
	char c_title[520];
	char c_album[520];
	char c_genre1[520];
	char c_lang[520];
	char c_mp3file[520];
	char c_folder1[520];
	char c_folder2[520];
	char c_folder3[520];
	char c_folder4[520];
	if (HasFolderFields())
	{
	    char *folders[4];
	    char *fbuf=SeparateFolders(filename,folders,4);
	    sql_Cstring(folders[0],c_folder1);
	    sql_Cstring(folders[1],c_folder2);
	    sql_Cstring(folders[2],c_folder3);
	    sql_Cstring(folders[3],c_folder4);
	    free(fbuf);
	}
	else
	{
	    c_folder1[0]=0;
	    c_folder2[0]=0;
	    c_folder3[0]=0;
	    c_folder4[0]=0;
	}
        sql_Cstring(f.tag()->artist(),c_artist);
	sql_Cstring(f.tag()->title(),c_title);
	if (f.tag()->album()=="")
		sql_Cstring("Unassigned",c_album);
	else
		sql_Cstring(f.tag()->album(),c_album);
	TagLib::String sgenre1=f.tag()->genre();
	const char *genrename=sgenre1.toCString();
	const char *genreid=m_Genres[genrename].c_str();
	sql_Cstring(genreid,c_genre1);
	sql_Cstring(getlanguage(filename),c_lang);
	char sql[7000];
	char *c_cddbid=getAlbum(filename,c_album,c_artist);
	sql_Cstring(cfilename,c_mp3file);
	sprintf(sql,"SELECT id from tracks WHERE mp3file=%s",c_mp3file);
	string id = get_col0(sql);
	if (id!="NULL")
	{
		sprintf(sql,"UPDATE tracks SET artist=%s, title=%s,year=%d,sourceid=%s,"
			"tracknb=%d,length=%d,bitrate=%d,samplerate=%d,"
			"channels=%d,genre1=%s,lang=%s WHERE id=%ld",
			c_artist,c_title,f.tag()->year(),c_cddbid,
			f.tag()->track(),ap->length(),ap->bitrate(),ap->sampleRate(),
			ap->channels(),c_genre1,c_lang,atol(id.c_str()));
	}
	else
	{
		sprintf(sql,"INSERT INTO tracks SET artist=%s,title=%s,year=%u,sourceid=%s,"
			"tracknb=%u,mp3file=%s,length=%d,bitrate=%d,samplerate=%d,"
			"channels=%d,genre1=%s,genre2='',lang=%s,"
			"folder1=%s,folder2=%s,folder3=%s,folder4=%s",
			c_artist,c_title,f.tag()->year(),c_cddbid,
			f.tag()->track(),c_mp3file,ap->length(),ap->bitrate(),ap->sampleRate(),
			ap->channels(),c_genre1,c_lang,
			c_folder1,c_folder2,c_folder3,c_folder4);
	}
	free(c_cddbid);
	exec_sql(sql);
}

bool
mgDbGd::SyncStart()
{
  	if (!Connect())
    		return false;
	LoadMapInto("SELECT id,genre from genre",&m_Genres,0);
	// init random number generator
	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv, &tz );
	srandom( tv.tv_usec );
	CreateFolderFields();
	return true;
}

int
mgDbGd::AddToCollection( const string Name,const vector<mgItem*>&items)
{
    if (!Connect()) return 0;
    CreateCollection(Name);
    string listid = sql_string (get_col0
        ("SELECT id FROM playlist WHERE title=" + sql_string (Name)));
    unsigned int tracksize = items.size();
    if (tracksize==0)
	    return 0;

    // this code is rather complicated but works in a multi user
    // environment:
 
    // insert a unique trackid:
    string trackid = ltos(thread_id()+1000000);
    exec_sql("INSERT INTO playlistitem SELECT "+listid+","
	   "MAX(tracknumber)+"+ltos(tracksize)+","+trackid+
	   " FROM playlistitem WHERE playlist="+listid);
    
    // find tracknumber of the trackid we just inserted:
    string sql = string("SELECT tracknumber FROM playlistitem WHERE "
		    "playlist=")+listid+" AND trackid="+trackid;
    long first = atol(get_col0(sql).c_str()) - tracksize + 1;

    // replace the place holder trackid by the correct value:
    exec_sql("UPDATE playlistitem SET trackid="+ltos(items[tracksize-1]->getItemid())+
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
    		exec_sql (sql_prefix+sql);
		sql = "";
	}
    }
    if (!sql.empty()) exec_sql (sql_prefix+sql);
    return tracksize;
}

int
mgDbGd::RemoveFromCollection (const string Name, mgParts& what)
{
    if (Name.empty())
	    return 0;
    if (!Connect()) return 0;
    string pid = KeyMaps.id(keyGdCollection,Name);
    if (pid.empty())
	    return 0;
    what.Prepare();
    what.tables.push_front("playlistitem as del");
    what.clauses.push_back("del.playlist="+pid);
    bool usesTracks = false;
    for (list < string >::iterator it = what.tables.begin (); it != what.tables.end (); ++it)
	if (*it == "tracks")
	{
		usesTracks = true;
		break;
	}
    if (usesTracks)
	what.clauses.push_back("del.trackid=tracks.id");
    else
	what.clauses.push_back("del.trackid=playlistitem.trackid");
    string sql = "DELETE playlistitem";
    sql += sql_list(" FROM",what.tables);
    sql += sql_list(" WHERE",what.clauses," AND ");
    exec_sql (sql);
    return m_db->affected_rows;
}

bool
mgDbGd::DeleteCollection (const string Name)
{
    if (!Connect()) return false;
    ClearCollection(Name);
    exec_sql ("DELETE FROM playlist WHERE title=" + sql_string (Name));
    return (m_db->affected_rows == 1);
}

void
mgDbGd::ClearCollection (const string Name)
{
    if (!Connect()) return;
    string listid = KeyMaps.id(keyGdCollection,Name);
    exec_sql ("DELETE FROM playlistitem WHERE playlist="+sql_string(listid));
}

bool
mgDbGd::CreateCollection (const string Name)
{
    if (!Connect()) return false;
    string name = sql_string(Name);
    if (exec_count("SELECT count(title) FROM playlist WHERE title = " + name)>0) 
	return false;
    exec_sql ("INSERT playlist VALUES(" + name + ",'VDR',NULL,NULL,NULL)");
    return true;
}

bool
mgDbGd::FieldExists(string table, string field)
{
    	if (!Connect()) 
		return false;
    	char *b;
    	asprintf(&b,"DESCRIBE %s %s",table.c_str(),field.c_str());
    	MYSQL_RES *rows = exec_sql (b);
    	free(b);
	bool result=false;
    	if (rows)
	{
		result = mysql_num_rows(rows) == 1;
    		mysql_free_result (rows);
	}
	return result;
}

void
mgDbGd::LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap)
{
	if (!valmap && !idmap)
		return;
	if (!Connect())
		return;
	MYSQL_RES *rows = exec_sql (sql);
	if (rows) 
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row (rows)) != 0)
		{
			if (row[0] && row[1])
			{
				if (valmap) (*valmap)[row[0]] = row[1];
				if (idmap) (*idmap)[row[1]] = row[0];
			}
		}
		mysql_free_result (rows);
	}
}

string
mgDbGd::LoadItemsInto(mgParts& what,vector<mgItem*>& items)
{
    	if (!Connect())
		return "";
	what.fields.clear();
	what.fields.push_back("tracks.id");
	what.fields.push_back("tracks.title");
	what.fields.push_back("tracks.mp3file");
	what.fields.push_back("tracks.artist");
	what.fields.push_back("album.title");
	what.fields.push_back("tracks.genre1");
	what.fields.push_back("tracks.genre2");
	what.fields.push_back("tracks.bitrate");
	what.fields.push_back("tracks.year");
	what.fields.push_back("tracks.rating");
	what.fields.push_back("tracks.length");
	what.fields.push_back("tracks.samplerate");
	what.fields.push_back("tracks.channels");
	what.fields.push_back("tracks.lang");
	what.tables.push_back("tracks");
	what.tables.push_back("album");
	string result = what.sql_select(false); 
	MYSQL_RES *rows = exec_sql (result);
	if (rows)
	{
		for (unsigned int idx=0;idx<items.size();idx++)
			delete items[idx];
		items.clear ();
		MYSQL_ROW row;
		while ((row = mysql_fetch_row (rows)) != 0)
			items.push_back (new mgItemGd (row));
		mysql_free_result (rows);
	}
	return result;
}

string
mgDbGd::LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems)
{
    	if (!Connect())
		return "";
	string result = what.sql_select();		
	MYSQL_RES *rows = exec_sql (result);
        if (rows)
	{
        	listitems.clear ();
        	unsigned int num_fields = mysql_num_fields(rows);
		assert(num_fields>=2);
		MYSQL_ROW row;
        	while ((row = mysql_fetch_row (rows)) != 0)
        	{
			if (!row[0]) continue;
			string r0 = row[0];
			if (!strcmp(row[0],"NULL")) // there is a genre NULL!
				continue;
			mgListItem* n = new mgListItem;
			if (num_fields==3)
			{
				if (!row[1]) continue;
				n->set(row[0],row[1],atol(row[2]));
			}
			else
        			n->set(KeyMaps.value(tp,r0),r0,atol(row[1]));
			listitems.push_back(n);
        	}
        	mysql_free_result (rows);
	}
	return result;
}

class mgKeyGdTrack : public mgKeyNormal {
	public:
		mgKeyGdTrack() : mgKeyNormal(keyGdTrack,"tracks","tracknb") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
};

class mgKeyGdAlbum : public mgKeyNormal {
	public:
		mgKeyGdAlbum() : mgKeyNormal(keyGdAlbum,"album","title") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
};

mgParts
mgKeyGdAlbum::Parts(mgDb *db,bool orderby) const
{
	mgParts result = mgKeyNormal::Parts(db,orderby);
	result.tables.push_back("tracks");
	return result;
}

class mgKeyGdFolder : public mgKeyNormal {
	public:
		mgKeyGdFolder(mgKeyTypes kt,const char *fname)
			: mgKeyNormal(kt,"tracks",fname) { m_enabled=-1;};
		bool Enabled(mgDb *db);
	private:
		int m_enabled;
};


bool
mgKeyGdFolder::Enabled(mgDb *db)
{
    if (m_enabled<0)
	m_enabled = db->FieldExists("tracks", m_field);
    return (m_enabled==1);
}

class mgKeyGdGenres : public mgKeyNormal {
	public:
		mgKeyGdGenres() : mgKeyNormal(keyGdGenres,"tracks","genre1") {};
		mgKeyGdGenres(mgKeyTypes kt) : mgKeyNormal(kt,"tracks","genre1") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
	protected:
		string map_sql() const;
		virtual unsigned int genrelevel() const { return 4; }
	private:
		string GenreClauses(mgDb *db,bool orderby) const;
};

class mgKeyGdGenre1 : public mgKeyGdGenres
{
	public:
		mgKeyGdGenre1() : mgKeyGdGenres(keyGdGenre1) {}
		unsigned int genrelevel() const { return 1; }
};

class mgKeyGdGenre2 : public mgKeyGdGenres
{
	public:
		mgKeyGdGenre2() : mgKeyGdGenres(keyGdGenre2) {}
		unsigned int genrelevel() const { return 2; }
};

class mgKeyGdGenre3 : public mgKeyGdGenres
{
	public:
		mgKeyGdGenre3() : mgKeyGdGenres(keyGdGenre3) {}
		unsigned int genrelevel() const { return 3; }
};

string
mgKeyGdGenres::map_sql() const
{
	if (genrelevel()==4)
		return "select id,genre from genre;";
	else
		return string("select id,genre from genre where length(id)="+ltos(genrelevel())+";");
}

string
mgKeyGdGenres::GenreClauses(mgDb *db,bool orderby) const
{
	strlist g1;
	strlist g2;

	if (orderby)
		if (genrelevel()==4)
		{
			g1.push_back("tracks.genre1=genre.id");
			g2.push_back("tracks.genre2=genre.id");
		}
		else
		{
			g1.push_back("substring(tracks.genre1,1,"+ltos(genrelevel())+")=genre.id");
			g2.push_back("substring(tracks.genre2,1,"+ltos(genrelevel())+")=genre.id");
		}

	if (valid())
	{
		unsigned int len=genrelevel();
		if (len==4) len=0;
      		g1.push_back(IdClause(db,"tracks.genre1",0,genrelevel()));
      		g2.push_back(IdClause(db,"tracks.genre2",0,genrelevel()));
	}

	if (db->NeedGenre2())
	{
		string o1=sql_list("(",g1," AND ",")");
		if (o1.empty())
			return "";
		string o2=sql_list("(",g2," AND ",")");
		return string("(") + o1 + " OR " + o2 + string(")");
	}
	else
		return sql_list("",g1," AND ");
}


mgParts
mgKeyGdGenres::Parts(mgDb *db,bool orderby) const
{
	mgParts result;
   	result.clauses.push_back(GenreClauses(db,orderby));
	result.tables.push_back("tracks");
	if (orderby)
	{
		result.fields.push_back("genre.genre");
		if (genrelevel()==4)
			result.fields.push_back("genre.id");
		else
			result.fields.push_back("substring(genre.id,1,"+ltos(genrelevel())+")");
		result.tables.push_back("genre");
       		result.orders.push_back("genre.genre");
	}
	return result;
}


class mgKeyGdLanguage : public mgKeyNormal {
	public:
		mgKeyGdLanguage() : mgKeyNormal(keyGdLanguage,"tracks","lang") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
	protected:
		string map_sql() const { return "select id,language from language;"; }
};

class mgKeyGdCollection: public mgKeyNormal {
	public:
  	  mgKeyGdCollection() : mgKeyNormal(keyGdCollection,"playlist","id") {};
	  mgParts Parts(mgDb *db,bool orderby=false) const;
	protected:
	 string map_sql() const { return "select id,title from playlist;"; }
};
class mgKeyGdCollectionItem : public mgKeyNormal {
	public:
		mgKeyGdCollectionItem() : mgKeyNormal(keyGdCollectionItem,"playlistitem","tracknumber") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
};

class mgKeyDecade : public mgKeyNormal {
	public:
		mgKeyDecade() : mgKeyNormal(keyGdDecade,"tracks","year") {}
		string expr() const { return "substring(convert(10 * floor(tracks.year/10), char),3)"; }
};

mgKey*
ktGenerate(const mgKeyTypes kt)
{
	mgKey* result = 0;
	switch (kt)
	{
		case keyGdGenres:	result = new mgKeyGdGenres;break;
		case keyGdGenre1:	result = new mgKeyGdGenre1;break;
		case keyGdGenre2:	result = new mgKeyGdGenre2;break;
		case keyGdGenre3:	result = new mgKeyGdGenre3;break;
		case keyGdFolder1:result = new mgKeyGdFolder(keyGdFolder1,"folder1");break;
		case keyGdFolder2:result = new mgKeyGdFolder(keyGdFolder2,"folder2");break;
		case keyGdFolder3:result = new mgKeyGdFolder(keyGdFolder3,"folder3");break;
		case keyGdFolder4:result = new mgKeyGdFolder(keyGdFolder4,"folder4");break;
		case keyGdArtist: result = new mgKeyNormal(kt,"tracks","artist");break;
		case keyGdArtistABC: result = new mgKeyABC(kt,"tracks","artist");break;
		case keyGdTitle: result = new mgKeyNormal(kt,"tracks","title");break;
		case keyGdTitleABC: result = new mgKeyABC(kt,"tracks","title");break;
		case keyGdTrack: result = new mgKeyGdTrack;break;
		case keyGdDecade: result = new mgKeyDecade;break;
		case keyGdAlbum: result = new mgKeyGdAlbum;break;
		case keyGdCreated: result = new mgKeyDate(kt,"tracks","created");break;
		case keyGdModified: result = new mgKeyDate(kt,"tracks","modified");break;
		case keyGdCollection: result = new mgKeyGdCollection;break;
		case keyGdCollectionItem: result = new mgKeyGdCollectionItem;break;
		case keyGdLanguage: result = new mgKeyGdLanguage;break;
		case keyGdRating: result = new mgKeyNormal(kt,"tracks","rating");break;
		case keyGdYear: result = new mgKeyNormal(kt,"tracks","year");break;
		default: result = 0; break;
	}
	return result;
}

mgParts
mgKeyGdTrack::Parts(mgDb *db,bool orderby) const
{
	mgParts result;
	result.tables.push_back("tracks");
	AddIdClause(db,result,"tracks.title");
	if (orderby)
	{
		// if you change tracks.title, please also
		// change mgItemGd::getKeyItem()
		result.fields.push_back("tracks.title");
       		result.orders.push_back("tracks.tracknb");
	}
	return result;
}

mgParts
mgKeyGdLanguage::Parts(mgDb *db,bool orderby) const
{
	mgParts result;
	AddIdClause(db,result,"tracks.lang");
	result.tables.push_back("tracks");
	if (orderby)
	{
		result.fields.push_back("language.language");
		result.fields.push_back("tracks.lang");
		result.tables.push_back("language");
       		result.orders.push_back("language.language");
	}
	return result;
}

mgParts
mgKeyGdCollection::Parts(mgDb *db,bool orderby) const
{
	mgParts result;
	if (orderby)
	{
		result.tables.push_back("playlist");
		AddIdClause(db,result,"playlist.id");
		result.fields.push_back("playlist.title");
		result.fields.push_back("playlist.id");
       		result.orders.push_back("playlist.title");
	}
	else
	{
		result.tables.push_back("playlistitem");
		AddIdClause(db,result,"playlistitem.playlist");
	}
	return result;
}

mgParts
mgKeyGdCollectionItem::Parts(mgDb *db,bool orderby) const
{
	mgParts result;
	result.tables.push_back("playlistitem");
	AddIdClause(db,result,"playlistitem.tracknumber");
	if (orderby)
	{
		// tracks nur hier, fuer sql_delete_from_coll wollen wir es nicht
		result.tables.push_back("tracks");
		result.fields.push_back("tracks.title");
		result.fields.push_back("playlistitem.tracknumber");
       		result.orders.push_back("playlistitem.tracknumber");
	}
	return result;
}

