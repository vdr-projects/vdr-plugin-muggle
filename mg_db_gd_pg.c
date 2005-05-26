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

#include <postgresql/pg_config.h>

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

