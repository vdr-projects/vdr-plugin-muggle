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

class mgKeyGdTrack : public mgKeyNormal {
	public:
		mgKeyGdTrack() : mgKeyNormal(keyGdTrack,"tracks","tracknb") {};
		mgParts Parts(mgDb *db,bool groupby=false) const;
		mgSortBy SortBy() const { return mgSortByIdNum; }
};

class mgKeyGdAlbum : public mgKeyNormal {
	public:
		mgKeyGdAlbum() : mgKeyNormal(keyGdAlbum,"album","title") {};
		mgParts Parts(mgDb *db,bool groupby=false) const;
};

mgParts
mgKeyGdAlbum::Parts(mgDb *db,bool groupby) const
{
	mgParts result = mgKeyNormal::Parts(db,groupby);
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
		mgParts Parts(mgDb *db,bool groupby=false) const;
	protected:
		string map_sql() const;
		virtual unsigned int genrelevel() const { return 4; }
	private:
		string GenreClauses(mgDb *db,bool groupby) const;
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
		return "SELECT id,genre FROM genre";
	else
		return string("SELECT id,genre FROM genre WHERE LENGTH(id)<="+ltos(genrelevel()));
}

string
mgKeyGdGenres::GenreClauses(mgDb *db,bool groupby) const
{
	strlist g1;
	strlist g2;

	if (groupby)
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
mgKeyGdGenres::Parts(mgDb *db,bool groupby) const
{
	mgParts result;
   	result.clauses.push_back(GenreClauses(db,groupby));
	result.tables.push_back("tracks");
	if (groupby)
	{
		result.valuefields.push_back("genre.genre");
		if (genrelevel()==4)
			result.idfields.push_back("genre.id");
		else
			result.idfields.push_back("substring(genre.id,1,"+ltos(genrelevel())+")");
		result.tables.push_back("genre");
	}
	return result;
}


class mgKeyGdLanguage : public mgKeyNormal {
	public:
		mgKeyGdLanguage() : mgKeyNormal(keyGdLanguage,"tracks","lang") {};
		mgParts Parts(mgDb *db,bool groupby=false) const;
	protected:
		string map_sql() const { return "SELECT id,language FROM language"; }
};

class mgKeyGdCollection: public mgKeyNormal {
	public:
  	  mgKeyGdCollection() : mgKeyNormal(keyGdCollection,"playlist","id") {};
	  mgParts Parts(mgDb *db,bool groupby=false) const;
	protected:
	 string map_sql() const { return "SELECT id,title FROM playlist"; }
};
class mgKeyGdCollectionItem : public mgKeyNormal {
	public:
		mgKeyGdCollectionItem() : mgKeyNormal(keyGdCollectionItem,"playlistitem","trackid") {};
		mgParts Parts(mgDb *db,bool groupby=false) const;
		mgSortBy SortBy() const { return mgSortNone; }
};

class mgKeyDecade : public mgKeyNormal {
	public:
		mgKeyDecade() : mgKeyNormal(keyGdDecade,"tracks","year") {}
		string expr(mgDb* db) const { return db->DecadeExpr(); }
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
mgKeyGdTrack::Parts(mgDb *db,bool groupby) const
{
	mgParts result;
	result.tables.push_back("tracks");
	AddIdClause(db,result,"tracks.tracknb");
	if (groupby)
	{
		result.valuefields.push_back("tracks.title");
		result.idfields.push_back("tracks.tracknb");
	}
	return result;
}

mgParts
mgKeyGdLanguage::Parts(mgDb *db,bool groupby) const
{
	mgParts result;
	AddIdClause(db,result,"tracks.lang");
	result.tables.push_back("tracks");
	if (groupby)
	{
		result.valuefields.push_back("language.language");
		result.idfields.push_back("tracks.lang");
		result.tables.push_back("language");
	}
	return result;
}

mgParts
mgKeyGdCollection::Parts(mgDb *db,bool groupby) const
{
	mgParts result;
	if (groupby)
	{
		result.tables.push_back("playlist");
		AddIdClause(db,result,"playlist.id");
		result.valuefields.push_back("playlist.title");
		result.idfields.push_back("playlist.id");
	}
	else
	{
		result.tables.push_back("playlistitem");
		AddIdClause(db,result,"playlistitem.playlist");
	}
	return result;
}

mgParts
mgKeyGdCollectionItem::Parts(mgDb *db,bool groupby) const
{
	mgParts result;
	result.tables.push_back("playlistitem");
	if (groupby)
	{
		result.tables.push_back("tracks");
		result.valuefields.push_back("tracks.title");
	}
	return result;
}

