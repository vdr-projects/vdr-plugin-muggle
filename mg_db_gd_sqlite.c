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
#include "mg_db_gd_sqlite.h"

using namespace std;


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
	{
		if (!Threadsafe())
			mgError("Your sqlite3 version is not thread safe");
	}
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
  "drop table album;",
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
  "CREATE INDEX idx_album_cddbid ON album (cddbid)",
  "drop table genre;",
  "CREATE TABLE genre ("
	  "id varchar(10) NOT NULL default '', "
	  "id3genre smallint(6) default NULL, "
	  "genre varchar(255) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY (id));",
  "drop table language;",
  "CREATE TABLE language ("
	  "id varchar(4) NOT NULL default '', "
	  "language varchar(40) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY  (id));",
  "drop table musictype;",
  "CREATE TABLE musictype ("
	  "musictype varchar(40) default NULL, "
	  "id integer PRIMARY KEY autoincrement); ",
  "drop table player;",
  "CREATE TABLE player ( "
	  "ipaddr varchar(255) NOT NULL default '', "
	  "uichannel varchar(255) NOT NULL default '', "
	  "logtarget int(11) default NULL, "
	  "cdripper varchar(255) default NULL, "
	  "mp3encoder varchar(255) default NULL, "
	  "cdromdev varchar(255) default NULL, "
	  "cdrwdev varchar(255) default NULL, "
	  "id integer PRIMARY KEY default '0'); ",
  "drop table playerstate;",
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
	  "PRIMARY KEY  (playerid,playertype));",
  "drop table playlist;",
  "CREATE TABLE playlist ( "
	  "title varchar(255) default NULL, "
	  "author varchar(255) default NULL, "
	  "note varchar(255) default NULL, "
	  "created timestamp(8) NOT NULL, "
	  "id integer PRIMARY KEY autoincrement); ",
  "drop table playlistitem;",
  "CREATE TABLE playlistitem ( "
	  "playlist integer, "
	  "tracknumber integer, "
	  "trackid int(11) default NULL, "
	  "PRIMARY KEY (playlist,tracknumber));",
  "drop table playlog;",
  "CREATE TABLE playlog ( "
	  "trackid int(11) default NULL, "
	  "played date default NULL, "
	  "id integer PRIMARY KEY autoincrement); ",
  "drop table recordingitem;",
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
	  "id integer PRIMARY KEY default '0');",
  "drop table source",
	  "CREATE TABLE source ( "
	  "source varchar(40) default NULL, "
	  "id integer PRIMARY KEY autoincrement); ",
  "drop table tracklistitem;",
  "CREATE TABLE tracklistitem ( "
	  "playerid int(11) NOT NULL default '0', "
	  "listtype smallint(6) NOT NULL default '0', "
	  "tracknb int(11) NOT NULL default '0', "
	  "trackid int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (playerid,listtype,tracknb));",
  "drop table tracks;",
  "CREATE TABLE tracks ( "
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
	  "id integer PRIMARY KEY autoincrement, "
	  "folder1 varchar(255), "
	  "folder2 varchar(255), "
	  "folder3 varchar(255), "
	  "folder4 varchar(255)); ",
  "CREATE INDEX tracks_title on tracks (title)",
  "CREATE INDEX tracks_sourceid on tracks (sourceid)",
  "CREATE INDEX tracks_mp3file on tracks (mp3file)",
  "CREATE INDEX tracks_genre1 on tracks (genre1)",
  "CREATE INDEX tracks_genre2 on tracks (genre2)",
  "CREATE INDEX tracks_year on tracks (year)",
  "CREATE INDEX tracks_lang on tracks (lang)",
  "CREATE INDEX tracks_artist on tracks (artist)",
  "CREATE INDEX tracks_folder1 on tracks (folder1)",
  "CREATE INDEX tracks_folder2 on tracks (folder2)",
  "CREATE INDEX tracks_folder3 on tracks (folder3)",
  "CREATE INDEX tracks_folder4 on tracks (folder4)",
};


char **
mgDbGd::query(string sql)
{
	const char * optsql = optimize(sql).c_str();
  	mgDebug(5,"query(%X,%s)",m_db,optsql);
	char **result=0;
	int rc = sqlite3_get_table(m_db,optsql,&result,&m_rows,&m_cols,&m_errmsg);
  	mgDebug(5,"finishes with result %d, rows:%d, cols:%d",rc,m_rows,m_cols);
	if (rc!=SQLITE_OK)
	{
    		mgError("SQL Error in %s: %d/%s",optsql,rc,m_errmsg);
    		std::cout<<"ERROR in " << optsql << ":" << rc << "/" << m_errmsg<<std::endl;
		return false;
	}
	return result;
}


void
mgDbGd::execute( string sql)
{
	int rc = silent_execute(sql);
	if (rc!=SQLITE_OK)
	{
    		mgError("SQL Error in %s: %s",sql.c_str(),m_errmsg);
    		std::cout<<"ERROR in " << sql.c_str() << ":" << m_errmsg<<std::endl;
	}
}

int
mgDbGd::silent_execute( string sql)
{
	if (sql.empty())
		return 0;
	if (!Connect())
		return 0;
	const char * optsql = optimize(sql).c_str();
  	mgDebug(5,"execute(%X,%s)",m_db,optsql);
	int result = sqlite3_exec(m_db,sql.c_str(),0,0,&m_errmsg);
  	mgDebug(5,"finishes with result %d",result);
	return result;
}

string
mgDbGd::get_col0( const string sql)
{
	char **table = query(sql);
	string result;
	if (table && m_rows && m_cols)
		result = table[m_cols];
	else
		result = "NULL";
	sqlite3_free_table(table);
	return result;
}

unsigned long
mgDbGd::exec_count( const string sql) 
{
	if (!Connect())
		return 0;
	return atol (get_col0 ( sql).c_str ());
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
  char *b;
  int len = sizeof( genres ) / sizeof( genres_t );
  for( int i=0; i < len; i ++ )
  {
	  char id3genre[5];
	  if (genres[i].id3genre>=0)
	  	sprintf(id3genre,"%d",genres[i].id3genre);
	  else
		strcpy(id3genre,"NULL");
	  string genre = sql_string(genres[i].name);
	  b=sqlite3_mprintf("INSERT INTO genre (id,id3genre,genre) VALUES('%s', %s, %s)",
			  genres[i].id,id3genre,genre.c_str());
	  execute(b);
	  sqlite3_free(b);
  }
  len = sizeof( languages ) / sizeof( lang_t );
  for( int i=0; i < len; i ++ )
  {
	  b=sqlite3_mprintf("INSERT INTO language (id,language) VALUES('%q', '%q')",
			  languages[i].id,languages[i].name);
	  execute(b);
	  sqlite3_free(b);
  }
  len = sizeof( musictypes ) / sizeof( musictypes_t );
  for( int i=0; i < len; i ++ )
  {
	  b=sqlite3_mprintf("INSERT INTO musictype (musictype) VALUES('%q')",
			  musictypes[i].name);
	  execute(b);
	  sqlite3_free(b);
  }
  len = sizeof( sources ) / sizeof( sources_t );
  for( int i=0; i < len; i ++ )
  {
	  b=sqlite3_mprintf("INSERT INTO source (source) VALUES('%q')",
			  sources[i].name);
	  execute(b);
  }
}


bool
mgDbGd::Create()
{
  // create database and tables
  mgDebug(1,"Creating database %s/%s",the_setup.DbDatadir,the_setup.DbName);

  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=0; i < len; i ++ )
    {
  	if (silent_execute (db_cmds[i])!=SQLITE_OK)
		if (strncmp(db_cmds[i],"drop ",5))
  	{
    		mgWarning("%20s: %s",db_cmds[i],sqlite3_errmsg (m_db));
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
  return sqlite3_mprintf("'%q'",s);
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

void
mgDirectory(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	assert(argc==1);
	char *buf=strdup((char*)sqlite3_value_text(argv[0]));
	char *slash=strrchr(buf,'/');
	if (!slash)
		slash=buf;
	*slash=0;
	sqlite3_result_text(context,buf,2,free);
}

bool
mgDbGd::Connect ()
{
    if (m_database_found)
	return true;
    if (time(0)<m_create_time+10)
	return false;
    m_create_time=time(0);
    char *s=sqlite3_mprintf("%s/%s",the_setup.DbDatadir,the_setup.DbName);
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
     if (rc==SQLITE_OK)
	return true;
     mgWarning("Cannot define mgDirectory:%d/%s",rc,sqlite3_errmsg);
     extern bool create_question();
     if (!create_question())
     	return false;
     if (!Create())
	return false;
     extern bool import();
     import();
     return true;
}

bool 
mgDbGd::NeedGenre2() 
{
    if (!needGenre2_set && Connect())
    {
	needGenre2_set=true;
	needGenre2=exec_count("SELECT COUNT(genre2) FROM tracks GROUP BY genre2")>1;
    }
    return needGenre2;
}

void
mgDbGd::CreateFolderFields()
{
	if (HasFolderFields())
		return;
	m_hasfolderfields = FieldExists("tracks","folder1");
	if (!m_hasfolderfields)
      	{
	      char *errmsg;
	      m_hasfolderfields = sqlite3_exec(m_db,
	              "alter table tracks add column folder1 varchar(255),"
		      "add column folder2 varchar(255),"
		      "add column folder3 varchar(255),"
		      "add column folder4 varchar(255)",0,0,&errmsg) == SQLITE_OK;
        }
}

void
mgDbGd::ServerEnd()
{
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
		char *c_directory;
		c_directory=sql_Cstring(filename,c_directory);
		char *slash=strrchr(c_directory,'/');
		if (slash)
		{
			*slash='\'';
			*(slash+1)=0;
		}
		const char *directory="mgDirectory(mp3file)";
		char *where;
		asprintf(&where,"WHERE tracks.sourceid=album.cddbid "
			"AND %s=%s "
			"AND album.title=%s",
			directory,c_directory,
			c_album);
		sqlite3_free(c_directory);
		// how many artists will the album have after adding this one?
		asprintf(&b,"SELECT distinct album.artist FROM album, tracks %s ",where);
        	char **table = query (b);
		free(b);
		long new_album_artists = m_rows-1;
		char *buf=sqlite3_mprintf("");
		if (new_album_artists==1)
		{
			buf=sql_Cstring(table[m_cols]);
			if (strcmp(buf,c_artist))
				new_album_artists++;
		}
		else
		sqlite3_free_table(table);
		if (new_album_artists>1 && strcmp(buf,"'Various Artists'"))
			// is the album multi artist and not yet marked as such?
		{
			asprintf(&b,"SELECT album.cddbid FROM album, tracks %s",where);
			free(result);
			result=sql_Cstring(get_col0(b));
			free(b);
			asprintf(&b,"UPDATE album SET artist='Various Artists' WHERE cddbid=%s",result);
			execute(b);
			// here we could change all tracks.sourceid to result and delete
			// the other album entries for this album, but that should only 
			// be needed if a pre 0.1.4 import has been done incorrectly, so we
			// don't bother
		}
		else
		{				// no usable album found
			sqlite3_free(result);
			result = sqlite3_mprintf("'%ld-%9s",random(),c_artist+1);
			char *p=strchr(result,0)-1;
			if (*p!='\'')
				*p='\'';
			b=sqlite3_mprintf("INSERT INTO album (title,artist,cddbid) "
					"VALUES(%s,%s,%s)",
				c_album,c_artist,result);
			execute(b);
			free(b);
		}
		sqlite3_free(buf);
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
	mgDebug(2,"Importing %s",filename);
        TagLib::AudioProperties *ap = f.audioProperties();
	char *c_artist;
	char *c_title;
	char *c_album;
	char *c_genre1;
	char *c_lang;
	char *c_mp3file;
	char *c_folder1;
	char *c_folder2;
	char *c_folder3;
	char *c_folder4;
	if (HasFolderFields())
	{
	    char *folders[4];
	    char *fbuf=SeparateFolders(filename,folders,4);
	    c_folder1=sql_Cstring(folders[0],c_folder1);
	    c_folder2=sql_Cstring(folders[1],c_folder2);
	    c_folder3=sql_Cstring(folders[2],c_folder3);
	    c_folder4=sql_Cstring(folders[3],c_folder4);
	    free(fbuf);
	}
	else
	{
	    c_folder1=0;
	    c_folder2=0;
	    c_folder3=0;
	    c_folder4=0;
	}
        c_artist=sql_Cstring(f.tag()->artist(),c_artist);
	c_title=sql_Cstring(f.tag()->title(),c_title);
	if (f.tag()->album()=="")
		c_album = sqlite3_mprintf("'Unassigned'");
	else
		c_album = sqlite3_mprintf("'%q'",f.tag()->album().toCString());
	TagLib::String sgenre1=f.tag()->genre();
	const char *genrename=sgenre1.toCString();
	const char *genreid=m_Genres[genrename].c_str();
	c_genre1=sql_Cstring(genreid,c_genre1);
	c_lang=sql_Cstring(getlanguage(filename),c_lang);
	char sql[7000];
	char *c_cddbid=getAlbum(filename,c_album,c_artist);
	c_mp3file=sql_Cstring(cfilename,c_mp3file);
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
		sprintf(sql,"INSERT INTO tracks "
			"(artist,title,year,sourceid,"
			"tracknb,mp3file,length,bitrate,samplerate,"
			"channels,genre1,genre2,lang,folder1,folder2,"
			"folder3,folder4) "
			"VALUES (%s,%s,%u,%s,"
			"%u,%s,%d,%d,%d,"
			"%d,%s,'',%s,%s,%s,%s,%s)",
			c_artist,c_title,f.tag()->year(),c_cddbid,
			f.tag()->track(),c_mp3file,ap->length(),ap->bitrate(),ap->sampleRate(),
			ap->channels(),c_genre1,c_lang,c_folder1,c_folder2,c_folder3,c_folder4);
	}
	free(c_cddbid);
	sqlite3_free(c_artist);
	sqlite3_free(c_title);
	sqlite3_free(c_album);
	sqlite3_free(c_genre1);
	sqlite3_free(c_lang);
	sqlite3_free(c_mp3file);
	sqlite3_free(c_folder1);
	sqlite3_free(c_folder2);
	sqlite3_free(c_folder3);
	sqlite3_free(c_folder4);
	execute(sql);
}

bool
mgDbGd::SyncStart()
{
  	if (!Connect())
	{
		mgDebug(1,"could not connect");
    		return false;
	}
	LoadMapInto("SELECT id,genre from genre",&m_Genres,0);
	// init random number generator
	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv, &tz );
	srandom( tv.tv_usec );
	CreateFolderFields();
	execute("BEGIN TRANSACTION");
	return true;
}

void
mgDbGd::SyncEnd()
{
	execute("COMMIT TRANSACTION");
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
    execute("INSERT INTO playlistitem SELECT "+listid+","
	   "MAX(tracknumber)+"+ltos(tracksize)+","+trackid+
	   " FROM playlistitem WHERE playlist="+listid);
    
    // find tracknumber of the trackid we just inserted:
    string sql = string("SELECT tracknumber FROM playlistitem WHERE "
		    "playlist=")+listid+" AND trackid="+trackid;
    long first = atol(get_col0(sql).c_str()) - tracksize + 1;

    // replace the place holder trackid by the correct value:
    execute("UPDATE playlistitem SET trackid="+ltos(items[tracksize-1]->getItemid())+
		    " WHERE playlist="+listid+" AND trackid="+trackid);
    
    // insert all other tracks:
    for (unsigned int i = 0; i < tracksize-1; i++)
	execute("INSERT INTO playlistitem VALUES (" + listid + "," + ltos (first + i) + "," +
            ltos (items[i]->getItemid ()) + ")");
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
    sqlite3_free_table(query (sql));
    return sqlite3_changes(m_db);
}

bool
mgDbGd::DeleteCollection (const string Name)
{
    if (!Connect()) return false;
    ClearCollection(Name);
    execute ("DELETE FROM playlist WHERE title=" + sql_string (Name));
    return (sqlite3_changes(m_db) == 1);
}

void
mgDbGd::ClearCollection (const string Name)
{
    if (!Connect()) return;
    string listid = KeyMaps.id(keyGdCollection,Name);
    execute ("DELETE FROM playlistitem WHERE playlist="+sql_string(listid));
}

bool
mgDbGd::CreateCollection (const string Name)
{
    if (!Connect()) return false;
    string name = sql_string(Name);
    if (exec_count("SELECT count(title) FROM playlist WHERE title = " + name)>0) 
	return false;
    execute ("INSERT INTO playlist VALUES(" + name + ",'VDR',NULL,strftime('%s','now'),NULL)");
    return true;
}

bool
mgDbGd::FieldExists(string table, string field)
{
    	if (!Connect()) 
		return false;
    	char *b;
    	asprintf(&b,"SELECT %s FROM %s LIMIT 1",field.c_str(),table.c_str());
	bool result = silent_execute(b) == SQLITE_OK;
    	free(b);
	return result;
}

void
mgDbGd::LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap)
{
	if (!valmap && !idmap)
		return;
	if (!Connect())
		return;
	char **table = query(sql);
	if (table && m_cols && m_rows>1)
	{
		for (int idx=1; idx<=m_rows; idx++)
		{
			char *i = table[m_cols*idx];
			char *v = table[m_cols*idx+1];
			if (i && v)
			{
				if (valmap) (*valmap)[i] = v;
				if (idmap) (*idmap)[v] = i;
			}
		}
	}
	sqlite3_free_table(table);
}

string
mgDbGd::LoadItemsInto(mgParts& what,vector<mgItem*>& items)
{
    	if (!Connect())
		return "";
	what.idfields.clear();
	what.valuefields.clear();
	what.idfields.push_back("tracks.id");
	what.idfields.push_back("tracks.title");
	what.idfields.push_back("tracks.mp3file");
	what.idfields.push_back("tracks.artist");
	what.idfields.push_back("album.title");
	what.idfields.push_back("tracks.genre1");
	what.idfields.push_back("tracks.genre2");
	what.idfields.push_back("tracks.bitrate");
	what.idfields.push_back("tracks.year");
	what.idfields.push_back("tracks.rating");
	what.idfields.push_back("tracks.length");
	what.idfields.push_back("tracks.samplerate");
	what.idfields.push_back("tracks.channels");
	what.idfields.push_back("tracks.lang");
	what.tables.push_back("tracks");
	what.tables.push_back("album");
	string result = what.sql_select(false); 
	char **table = query(result);
	if (table && m_cols && m_rows>1)
	{
		for (unsigned int idx=0;idx<items.size();idx++)
			delete items[idx];
		items.clear ();
		for (int idx=1; idx<=m_rows; idx++)
			items.push_back (new mgItemGd (&table[idx*m_cols]));
	}
	sqlite3_free_table(table);
	return result;
}


string
mgDbGd::LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems)
{
    	if (!Connect())
		return "";
	string result = what.sql_select();		
        listitems.clear ();
	char **table = query(result);
	if (table && m_rows && m_cols>=2)
		for (int idx=1; idx<=m_rows; idx++)
		{
			char **row = &table[idx*m_cols];
			if (!row[0]) continue;
			string r0 = row[0];
			if (!strcmp(row[0],"NULL")) // there is a genre NULL!
				continue;
			mgListItem* n = new mgListItem;
			if (m_cols==3)
			{
				if (!row[1]) return 0;
				n->set(row[0],row[1],atol(row[2]));
			}
			else
       				n->set(KeyMaps.value(tp,r0),r0,atol(row[1]));
			listitems.push_back(n);
		}
	sqlite3_free_table(table);
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
			g1.push_back("substr(tracks.genre1,1,"+ltos(genrelevel())+")=genre.id");
			g2.push_back("substr(tracks.genre2,1,"+ltos(genrelevel())+")=genre.id");
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
		result.valuefields.push_back("genre.genre");
		if (genrelevel()==4)
			result.idfields.push_back("genre.id");
		else
			result.idfields.push_back("substr(genre.id,1,"+ltos(genrelevel())+")");
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
		string expr() const { return "substr(convert(10 * floor(tracks.year/10), char),3)"; }
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
		result.idfields.push_back("tracks.title");
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
		result.valuefields.push_back("language.language");
		result.idfields.push_back("tracks.lang");
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
		result.valuefields.push_back("playlist.title");
		result.idfields.push_back("playlist.id");
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
		result.idfields.push_back("tracks.title");
		result.idfields.push_back("playlistitem.tracknumber");
       		result.orders.push_back("playlistitem.tracknumber");
	}
	return result;
}

