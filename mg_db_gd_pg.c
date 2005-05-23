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

#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>

#include "mg_setup.h"
#include "mg_item_gd.h"
#include "mg_db_gd_pg.h"

#include <postgresql/pg_config.h>

using namespace std;

string DbName()
{
	return "Postgresql";
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
  "CREATE INDEX trk_artist ON tracks (artist)",
  "CREATE INDEX trk_title ON tracks (title)",
  "CREATE INDEX trk_mp3file ON tracks (mp3file)",
  "CREATE INDEX trk_genre1 ON tracks (genre1)",
  "CREATE INDEX trk_genre2 ON tracks (genre2)",
  "CREATE INDEX trk_year ON tracks (year)",
  "CREATE INDEX trk_lang ON tracks (lang)",
  "CREATE INDEX trk_type ON tracks (type)",
  "CREATE INDEX trk_rating ON tracks (rating)",
  "CREATE INDEX trk_sourceid ON tracks (sourceid)",
};


PGresult*
mgDbGd::sql_query(string sql)
{
	const char * optsql = optimize(sql).c_str();
  	mgDebug(4,"PQexec(%X,%s)",m_db,optsql);
	PGresult* result = PQexec(m_db,optsql);
	switch (PQresultStatus(result)) {
		case PGRES_COMMAND_OK:
    			m_rows = atol(PQcmdTuples(result));
			break;
		case PGRES_TUPLES_OK:
    			m_rows = PQntuples(result);
			break;
		default:	
			mgError("SQL Error in %s: %s",optsql,PQresultErrorMessage (result));
    			std::cout<<"ERROR in " << optsql << ":" << PQresultErrorMessage (result)<<std::endl;
			break;
	}
	return result;
}

void
mgDbGd::Execute(const string sql)
{
	PQclear(Query(sql));
}

PGresult*
mgDbGd::Query( const string sql)
{
  if (sql.empty())
	  return 0;
  if (!Connect())
	  return 0;
  return sql_query(sql);
}

string
mgDbGd::get_col0( const string sql)
{
	PGresult * sql_result = Query ( sql);
	if (!sql_result)
		return "NULL";
	string result;
	if (m_rows==0)
		result = "NULL";
	else
	{
		char *field = PQgetvalue(sql_result,0,0);
		if (strlen(field)==0)
			result = "NULL";
		else
			result = field;
	}
	PQclear (sql_result);
	return result;
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
	PGresult *res=sql_query (db_cmds[i]);
  	if (!res)
  	{
    		mgWarning("%20s: %s",db_cmds[i],PQerrorMessage (m_db));
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
  	int buflen=2*strlen(s)+5;
  	b = (char *) malloc( buflen);
  }
  b[0]='\'';
  PQescapeString(b+1,s,strlen(s));
  *(strchr(b,0)+1)=0;
  *(strchr(b,0))='\'';
  return b;
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
		const char *directory="substring(tracks.mp3file from '.*/(.*)')";

		char *where;
		asprintf(&where,"WHERE tracks.sourceid=album.cddbid "
			"AND %s=%s "
			"AND album.title=%s",
			directory,c_directory,
			c_album);

		// how many artists will the album have after adding this one?
		asprintf(&b,"SELECT distinct album.artist FROM tracks, album %s ",where);
        	PGresult *rows = Query (b);
		free(b);
		long new_album_artists = m_rows;
		char buf[520];
		if (new_album_artists==1)
		{
			sql_Cstring(PQgetvalue(rows,0,0),buf);
			if (strcmp(buf,c_artist))
				new_album_artists++;
		}
		else
		if (new_album_artists>1 && strcmp(buf,"'Various Artists'"))
			// is the album multi artist and not yet marked as such?
		{
			asprintf(&b,"SELECT album.cddbid FROM tracks, album %s",where);
			free(result);
			result=sql_Cstring(get_col0(b));
			free(b);
			asprintf(&b,"UPDATE album SET artist='Various Artists' WHERE cddbid=%s",result);
			Execute(b);
			// here we could change all tracks.sourceid to result and delete
			// the other album entries for this album, but that should only 
			// be needed if a pre 0.1.4 import has been done incorrectly, so we
			// don't bother
		}
		else
		{				// no usable album found
			free(result);
			result=Build_cddbid(c_artist);
			asprintf(&b,"INSERT INTO album (title,artist,cddbid) VALUES(%s,%s,%s)",
				c_album,c_artist,result);
			Execute(b);
			free(b);
		}
		PQclear(rows);	
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
	char *folders[4];
	char *fbuf=SeparateFolders(filename,folders,4);
	sql_Cstring(folders[0],c_folder1);
	sql_Cstring(folders[1],c_folder2);
	sql_Cstring(folders[2],c_folder3);
	sql_Cstring(folders[3],c_folder4);
	free(fbuf);
        sql_Cstring(f.tag()->artist(),c_artist);
	sql_Cstring(f.tag()->title(),c_title);
	if (f.tag()->album()=="")
		sql_Cstring("Unassigned",c_album);
	else
		sql_Cstring(f.tag()->album(),c_album);
	TagLib::String sgenre1=f.tag()->genre();
	const char *genrename=sgenre1.toCString();
	const char *genreid=m_Genres[genrename].c_str();
	if (strlen(genreid)==0)
		genreid="NULL";
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
	Execute(sql);
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
	Execute("BEGIN TRANSACTION");
	return true;
}

void
mgDbGd::SyncEnd()
{
	Execute("COMMIT TRANSACTION");
}

int
mgDbGd::AddToCollection( const string Name, const vector<mgItem*>&items,mgParts *what)
{
    if (Name.empty())
	    return 0;
    if (!Connect())
	    return 0;
    CreateCollection(Name);
    string listid = KeyMaps.id(keyGdCollection,Name);
    if (listid.empty())
	    return 0;
    Execute("BEGIN TRANSACTION");
    // insert all tracks:
    int result = 0;
    for (unsigned int i = 0; i < items.size(); i++)
    {
	Execute("INSERT INTO playlistitem VALUES( " + listid + ","
			+ ltos (items[i]->getItemid ()) +")");
	result += m_rows;
    }
    Execute("COMMIT");
    return result;
}

int
mgDbGd::RemoveFromCollection (const string Name, const vector<mgItem*>&items,mgParts* what)
{
    if (Name.empty())
	    return 0;
    if (!Connect())
	    return 0;
    string listid = KeyMaps.id(keyGdCollection,Name);
    if (listid.empty())
	    return 0;
    Execute("BEGIN TRANSACTION");
    // remove all tracks:
    int result = 0;
    for (unsigned int i = 0; i < items.size(); i++)
    {
	Execute("DELETE FROM playlistitem WHERE playlist="+listid+
			" AND trackid = " + ltos (items[i]->getItemid ()));
	result += m_rows;
    }
    Execute("COMMIT");
    return result;
}

bool
mgDbGd::DeleteCollection (const string Name)
{
    if (!Connect()) return false;
    ClearCollection(Name);
    Execute ("DELETE FROM playlist WHERE title=" + sql_string (Name));
    return m_rows == 1;
}

void
mgDbGd::ClearCollection (const string Name)
{
    if (!Connect()) return;
    string listid = KeyMaps.id(keyGdCollection,Name);
    Execute ("DELETE FROM playlistitem WHERE playlist="+sql_string(listid));
}

bool
mgDbGd::CreateCollection (const string Name)
{
    if (!Connect()) return false;
    string name = sql_string(Name);
    if (exec_count("SELECT count(title) FROM playlist WHERE title = " + name)>0) 
	return false;
    Execute ("INSERT INTO playlist (title,author,created) VALUES(" + name + ",'VDR',CURRENT_TIMESTAMP)");
    return true;
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

void
mgDbGd::LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap)
{
	if (!valmap && !idmap)
		return;
	if (!Connect())
		return;
	PGresult *rows = Query (sql);
	if (rows) 
	{
		int ntuples = m_rows;
		for (int idx=0;idx<ntuples;idx++)
		{
			char *r0 = PQgetvalue(rows,idx,0);
			char *r1 = PQgetvalue(rows,idx,1);
			if (r0[0] && r1[0])
			{
				if (valmap) (*valmap)[r0] = r1;
				if (idmap) (*idmap)[r1] = r0;
			}
		}
		PQclear (rows);
	}
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
	what.idfields.push_back("tracks.tracknb");
	what.tables.push_back("tracks");
	what.tables.push_back("album");
	string result = what.sql_select(false); 
	PGresult *rows = Query (result);
	if (rows)
	{
		for (unsigned int idx=0;idx<items.size();idx++)
			delete items[idx];
		items.clear ();
		char *row[50];
		assert(PQnfields(rows)<50);
		for (int rowidx=0;rowidx<50;rowidx++)
			row[rowidx]=0;
		for (int rowidx=0;rowidx<m_rows;rowidx++)
		{
			for (int idx=0;idx<PQnfields(rows);idx++)
				row[idx] = PQgetvalue(rows,rowidx,idx);
			items.push_back (new mgItemGd (row));
		}
		PQclear (rows);
	}
	return result;
}

string
mgDbGd::LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems,bool distinct)
{
    	if (!Connect())
		return "";
	string result = what.sql_select(distinct);
	PGresult *rows = Query (result);
        if (rows)
	{
        	listitems.clear ();
        	unsigned int num_fields = PQnfields(rows);
		assert(num_fields>=1);
		for (int idx=0;idx<m_rows;idx++)
        	{
			string r0 = PQgetvalue(rows,idx,0);
			if (r0.empty()) continue;
			mgListItem* n = new mgListItem;
			long count=1;
			if (num_fields>1)
				count = atol(PQgetvalue(rows,idx,num_fields-1));
			if (num_fields==3)
			{
				if (PQgetisnull(rows,idx,1))
					continue;
				n->set(r0,PQgetvalue(rows,idx,1),count);
			}
			else
				n->set(KeyMaps.value(tp,r0),r0,count);
			listitems.push_back(n);
        	}
        	PQclear (rows);
	}
	return result;
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
    return true;
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
		return "select id,genre from genre";
	else
		return string("select id,genre from genre where length(id)<="+ltos(genrelevel()));
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
		string map_sql() const { return "select id,language from language"; }
};

class mgKeyGdCollection: public mgKeyNormal {
	public:
  	  mgKeyGdCollection() : mgKeyNormal(keyGdCollection,"playlist","id") {};
	  mgParts Parts(mgDb *db,bool groupby=false) const;
	protected:
	 string map_sql() const { return "select id,title from playlist"; }
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
		string expr() const { return "substring(10 * floor(tracks.year/10),3)"; }
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
	AddIdClause(db,result,"tracks.title");
	if (groupby)
	{
		// if you change tracks.title, please also
		// change mgItemGd::getKeyItem()
		result.idfields.push_back("tracks.title");
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
	AddIdClause(db,result,"tracks.title");
	if (groupby)
	{
		result.tables.push_back("tracks");
		result.idfields.push_back("tracks.title");
	}
	return result;
}

