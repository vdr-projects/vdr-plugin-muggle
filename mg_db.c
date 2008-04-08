/*!  * \file   mg_db.c
 * \brief  A capsule around database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-04-13 17:42:54 +0100 (Thu, 13 Apr 2005) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wolfgang61 $
 */

#include <cstring>
#include <string>
#include <assert.h>

using namespace std;

#include "mg_db.h"
#include "mg_item_gd.h"

#include "mg_setup.h"
#include "mg_tools.h"
#ifdef HAVE_SQLITE
#include "mg_db_gd_sqlite.h"
#elif HAVE_PG
#include "mg_db_gd_pg.h"
#else
#include "mg_db_gd_mysql.h"
#endif


#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fts.h>
#include <errno.h>
#include <mpegfile.h>
#include <flacfile.h>

static map <mgKeyTypes, map<string,string> > map_values;
static map <mgKeyTypes, map<string,string> > map_ids;

mgDbServer* DbServer;

mgDbServer::mgDbServer()
{
#ifdef HAVE_SQLITE
	m_server = new mgDbServerSQLite;
#elif HAVE_PG
	m_server = new mgDbServerPG;
#else
	m_server = new mgDbServerMySQL;
#endif
}

mgDbServer::~mgDbServer()
{
	delete m_server;
	m_server = 0;
}

mgSQLString::~mgSQLString()
{
	delete m_str;
	free(m_original);
}

void
mgSQLString::Init(const char* s)
{
	// strip trailing spaces

	m_original = strdup(s);
	char *p=strrchr(m_original,' ');
	if (p)
		if (p+1 == strchr(m_original,0))
			while (p>=m_original && *p==' ')
				*p-- = 0;

#ifdef HAVE_SQLITE
	m_str = new mgSQLStringSQLite(m_original);
#elif HAVE_PG
	m_str = new mgSQLStringPG(m_original);
#else
	m_str = new mgSQLStringMySQL(m_original);
#endif
}

mgSQLString::mgSQLString(const char*s)
{
	Init(s);
}

mgSQLString::mgSQLString(const mgSQLString& s)
{
	Init(s.original());
}

mgSQLString::mgSQLString(string s)
{
	Init(s.c_str());
}

mgSQLString::mgSQLString(TagLib::String s)
{
	Init(s.toCString(the_setup.utf8));
}

const char* 
mgSQLString::original() const
{
	return m_str->original();
}

char* 
mgSQLString::unquoted() const
{
	return m_str->unquoted();
}


char* 
mgSQLString::quoted() const
{
	return m_str->quoted();
}

bool
mgSQLString::operator==(const mgSQLString& b) const
{
	return !strcmp(m_str->original(),b.original());
}

bool
mgSQLString::operator==(const char* b) const
{
	return !strcmp(m_str->original(),b);
}

bool
mgSQLString::operator==(string b) const
{
	return !strcmp(m_str->original(),b.c_str());
}

bool
mgSQLString::operator!=(const mgSQLString& b) const
{
	return strcmp(m_str->original(),b.original());
}

bool
mgSQLString::operator!=(const char* b) const
{
	return strcmp(m_str->original(),b);
}

bool
mgSQLString::operator!=(string b) const
{
	return strcmp(m_str->original(),b.c_str());
}

void
mgSQLString::operator=(const char* b)
{
	delete m_str;
	free(m_original);
	m_original = 0;
	Init(b);
}

void
mgSQLString::operator=(const mgSQLString& b)
{
	delete m_str;
	free(m_original);
	m_original = 0;
	Init(b.original());
}

mgSQLStringImp::mgSQLStringImp()
{
	m_quoted = 0;
}

mgSQLStringImp::~mgSQLStringImp()
{
	if (m_quoted)
		free(m_quoted);
	m_quoted=0;
}


char* 
mgSQLStringImp::quoted() const
{
	if (!m_quoted)
	{
		msprintf(&m_quoted,"'%s'",unquoted());
	}
	return m_quoted;
}

mgQuery::mgQuery(void *db,const char *s,mgQueryNoise noise)
{
	Init(db,s,noise);
}

void
mgQuery::Init(void *db,const char *s,mgQueryNoise noise)
{
#ifdef HAVE_SQLITE
	m_q = new mgQuerySQLite(db,s,noise);
#elif HAVE_PG
	m_q = new mgQueryPG(db,s,noise);
#else
	m_q = new mgQueryMySQL(db,s,noise);
#endif
}

mgQuery::mgQuery(void *db,string s,mgQueryNoise noise)
{
	Init(db,s.c_str(),noise);
}

mgQuery::~mgQuery()
{
	delete m_q;
}

mgQueryImp::mgQueryImp(void *db,string sql,mgQueryNoise noise)
{
	m_db_handle = db;
	m_sql = sql;
	m_noise = noise;
	m_rows = 0;
	m_columns = 0;
	m_cursor = 0;
	m_errormessage=0;
	m_optsql = optimize(m_sql).c_str();
	m_rc = 0;
}

void
mgQueryImp::HandleErrors()
{
  	mgDebug(5,"%X:%d rows: %s",m_db_handle,m_rows,m_optsql);
	if (m_errormessage && strlen(m_errormessage))
		switch (m_noise) {
			case mgQueryNormal:
    				mgError("SQL Error in %s: %d/%s",m_optsql,m_rc,m_errormessage);
				break;
			case mgQueryWarnOnly:
    				mgWarning("SQL Error in %s: %d/%s",m_optsql,m_rc,m_errormessage);
				break;
			case mgQuerySilent:
				break;
		}
}

mgDb::mgDb(bool SeparateThread)
{
	m_database_found=false;
	m_hasfolderfields=false;
	m_separate_thread=SeparateThread;
	m_connect_time=0;
	m_create_time=0;
}

mgDb::~mgDb()
{
}

void *
mgDb::DbHandle() {
	ServerConnect();
	return ImplDbHandle();
}

string
optimize (string & spar)
{
	string s = spar;
	string::size_type tmp = s.find (" WHERE");
	if (tmp != string::npos)
		s.erase (tmp, 9999);
	tmp = s.find (" ORDER");
	if (tmp != string::npos)
		s.erase (tmp, 9999);
	string::size_type frompos = s.find (" FROM ") + 6;
	if (s.substr (frompos).find (",") == string::npos)
	{
		string from = s.substr (frompos, 999) + '.';
		string::size_type tbl;
		while ((tbl = spar.find (from)) != string::npos)
		{
    			spar.erase (tbl, from.size ());
		}
	}
	return spar;
}

bool
mgDb::Connect ()
{
    if (m_database_found)
        return true;
    if (!ServerConnect())
        return false;
    if (time(0)<m_create_time+10)
        return false;
    m_create_time=time(0);
    m_database_found=ConnectDatabase();
    if (!m_database_found && !Creatable())
    {
       	mgWarning("database not found");
        return false;
    }
    bool clearwanted=false;
    if (the_setup.IsMugglei())
	clearwanted=the_setup.CreateMode;
    else
        if (!m_database_found)
	{
                extern bool create_question();
                clearwanted=create_question();
        }
    if (clearwanted)
    {
	if (!m_database_found && Creatable())
        {
		m_database_found = true; // avoid recursion
  		mgWarning("Dropping and recreating database %s",the_setup.DbName);
		m_database_found = Create();
    		if (m_database_found)
                {
			m_database_found = true; // avoid recursion
			m_database_found = ConnectDatabase();
                }
	}
    }
    if (m_database_found)
    {
    	if (clearwanted || !FieldExists("tracks","id"))
    	{
		if (Clear())
  			mgWarning(trdb("empty database created"));
    	}
    	if (m_database_found && exec_count("SELECT COUNT(1) FROM genre")==0)
  		FillTables();
    }
    return m_database_found;
}

bool
mgDb::SyncStart()
{
  	if (!Connect())
    		return false;
	// init random number generator
	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv, &tz );
	srandom( tv.tv_usec );
	CreateFolderFields();
	return true;
}

void
mgDb::Sync(char * const * path_argv)
{
	if (!SyncStart())
		return;
  	extern void showimportcount(unsigned int,bool final=false);

	LoadMapInto("SELECT id,genre from genre",&m_Genres,0);
	LoadMapInto("SELECT genre,id3genre from genre",&m_GenreIds,0);

	StartTransaction();
	if (the_setup.DeleteStaleReferences)
	{
		int count=0;
		mgParts all;
		vector<mgItem*> items;
		LoadItemsInto(all,items);
		for (unsigned int idx=0;idx<items.size();idx++)
		{
			mgItem* item = items[idx];
			string fullpath=item->getSourceFile(true,true);
			if (!item->Valid(true))
			{
				char *b;
				msprintf(&b,"DELETE FROM tracks WHERE id=%ld",item->getItemid());
				count += Execute(b);
				free(b);
			}
		}
		mgDebug(1,"Deleted %d entries because the file did not exist",count);
	}

	unsigned int importcount=0;
	FTS *fts;
	FTSENT *ftsent;
	fts = fts_open( path_argv, FTS_LOGICAL, 0);
	if (fts)
	{
		while ( (ftsent = fts_read(fts)) != NULL)
		{
			if (ftsent->fts_path[0]=='/' && ftsent->fts_info!=FTS_DP)
			{
				mgWarning("Ignoring absolute path %s",ftsent->fts_path);
				fts_set(fts,ftsent,FTS_SKIP);
				continue;
			}
			switch (ftsent->fts_info) {
				case FTS_DC:
					mgDebug(1,"Ignoring directory %s, would cycle already seen %s",
						ftsent->fts_path,ftsent->fts_cycle->fts_path);
					break;
				case FTS_DNR:
					mgDebug(1,"Ignoring unreadable directory %s: error %d",
						ftsent->fts_path,ftsent->fts_errno);
					break;
				case FTS_DOT:
					mgDebug(1,"Ignoring dot file %s:",
						ftsent->fts_path);
					break;
				case FTS_SLNONE:
					mgDebug(1,"Ignoring broken symbolic link %s",
						ftsent->fts_path);
					break;
				case FTS_NSOK:	// should never happen because we do not do FTS_NOSTAT
				case FTS_SL:	// should never happen because we do FTS_LOGICAL
				case FTS_ERR:
					mgDebug(1,"Ignoring %s: error %d",
						ftsent->fts_path,ftsent->fts_errno);
					break;
				case FTS_D:
					mgDebug(1,"Importing from %s",ftsent->fts_path);
				case FTS_DP:
					break;
				case FTS_F:
					if (!ftsent->fts_path)
						mgDebug(1,"internal error: fts_path is 0");
					else if (access(ftsent->fts_path,R_OK))
						mgDebug(1,"Ignoring unreadable file %s: %s",
							ftsent->fts_path,strerror(errno));
					else
					{
						if (SyncFile(ftsent->fts_path))
							importcount++;
						if (importcount%1000==0)
							showimportcount(importcount);
					}
					break;
				case FTS_NS:
					mgDebug(1,"Ignoring unstatable file %s: error %d",
						ftsent->fts_path,ftsent->fts_errno);
					break;
				default:
					mgDebug(1,"Ignoring %s: unknown fts_info value %d",
						ftsent->fts_path,ftsent->fts_info);
			}
		}
		fts_close(fts);
	}
	mgDebug(1,"Deleted %d albums because they have no tracks",
		Execute("DELETE FROM album WHERE NOT EXISTS(SELECT 1 FROM"
		" tracks WHERE tracks.sourceid=album.cddbid)"));
	Commit();
	SyncEnd();
	showimportcount(importcount,true);
}


string
mgKeyNormal::id() const
{
	if (m_item)
		return m_item->id();
	else
		return "";
}

bool
mgKeyNormal::valid() const
{
	return m_item && m_item->valid();
}

string
mgKeyNormal::value() const
{
	if (m_item)
		return m_item->value();
	else
		return "";
}


mgKeyNormal::mgKeyNormal(const mgKeyNormal& k)
{
	m_kt = k.m_kt;
	m_table = k.m_table;
	m_field = k.m_field;
	m_item = k.m_item->Clone();
}

mgKeyNormal::mgKeyNormal(const mgKeyTypes kt, string table, string field)
{
	m_kt = kt;
	m_table = table;
	m_field = field;
	m_item = 0;
}

void
mgKeyNormal::set(mgListItem* item)
{
	m_item=item->Clone();
}

mgListItem*
mgKeyNormal::get()
{
	return m_item;
}

mgParts
mgKeyNormal::Parts(mgDb *db, bool groupby) const
{
	mgParts result;
	result.tables.push_back(table());
	AddIdClause(db,result,expr(db));
	if (groupby)
		result.idfields.push_back(expr(db));
	return result;
}

string
mgKeyNormal::IdClause(mgDb *db,string what,string::size_type start,string::size_type len) const
{
	if (len==0)
		len=string::npos;
       	if (id() == "'NULL'")
		return what + " is NULL";
       	else if (len==string::npos)
		return what + "=" + mgSQLString(id()).quoted();
	else
	{
		return "substring("+what + ","+ltos(start+1)+","+ltos(len)+")="
			+ mgSQLString(id().substr(start,len)).quoted();
	}
}

void
mgKeyNormal::AddIdClause(mgDb *db,mgParts &result,string what) const
{
	if (valid())
       		result.clauses.push_back(IdClause(db,what));
}

bool
mgKey::LoadMap() const
{
	if (map_sql().empty())
		return false;
	mgDb *db = GenerateDB();
	db->LoadMapInto(map_sql(), &map_ids[Type()], &map_values[Type()]);
	delete db;
	return true;
}


mgKeyMaps KeyMaps;
bool
mgKeyMaps::loadvalues (mgKeyTypes kt) const
{
	if (map_ids[kt].size()>0) 
		return true;
	mgKey* k = ktGenerate(kt);
	bool result = k->LoadMap();
	delete k;
	return result;
}

string
mgKeyMaps::value(mgKeyTypes kt, string idstr) const
{
	if (idstr.empty())
		return idstr;
	if (idstr=="NULL")
		return idstr;
	if (loadvalues (kt))
	{
		map<string,string>& valmap = map_values[kt];
		map<string,string>::iterator it;
		it = valmap.find(idstr);
		if (it!=valmap.end())
		{
			string r = it->second;
			if (!r.empty())
				return r;
		}
		map_ids[kt].clear();
		loadvalues(kt);
		it = valmap.find(idstr);
		if (it!=valmap.end())
			return valmap[idstr];
	}
	return idstr;
}

string
mgKeyMaps::id(mgKeyTypes kt, string valstr) const
{
	if (loadvalues (kt))
	{
		map<string,string>& idmap = map_ids[kt];
		return idmap[valstr];
	}
	return valstr;
}


class mgRefParts : public mgParts {
	public:
		mgRefParts(const mgReference* r);
};


strlist& operator+=(strlist&a, strlist b) 
{
	a.insert(a.end(), b.begin(),b.end());
	return a;
}

strlist operator+(strlist&a, strlist&b) 
{
	strlist result;
	result.insert(result.end(),a.begin(),a.end());
	result.insert(result.end(),b.begin(),b.end());
	return result;
}


string
sql_list (string prefix,strlist v,string sep,string postfix)
{
	string result = "";
	for (list < string >::iterator it = v.begin (); it != v.end (); ++it)
		addsep (result, sep, *it);
	if (!result.empty())
	{
    		result.insert(0,prefix+" ");
    		result += postfix;
	}
	return result;
}


mgParts&
mgParts::operator+=(mgParts a)
{
	valuefields += a.valuefields;
	idfields += a.idfields;
	tables += a.tables;
	clauses += a.clauses;
	return *this;
}

mgRefParts::mgRefParts(const mgReference* r)
{
	tables.push_back(r->t1());
	tables.push_back(r->t2());
	clauses.push_back(r->t1() + '.' + r->f1() + '=' + r->t2() + '.' + r->f2());
}

void
mgParts::Dump(string where) const
{
	mgDebug(1,"%X:%s:tables:%s",this,where.c_str(),sql_list(" FROM",tables).c_str());
	mgDebug(1,"   clauses:%s",sql_list(" WHERE",clauses," AND ").c_str());
	mgDebug(1,"   idfields:%s",sql_list("SELECT",idfields).c_str());
	mgDebug(1,"   valuefields:%s",sql_list("SELECT",valuefields).c_str());
}

void
mgParts::ConnectAllTables()
{
	tables.sort();
	tables.unique();
	strlist::reverse_iterator rit;
	string prevtable = "";
	rest.InitReferences();
	positives.clear();
	for (rit = tables.rbegin(); rit != tables.rend(); ++rit)
	{
		if (!prevtable.empty())
		{
			rest.InitReferences();
			ConnectTables(prevtable,*rit);
		}
		prevtable = *rit;
	}
	for (unsigned int i = 0 ; i < positives.size(); i++)
	{
		*this += mgRefParts(positives[i]);
	}
	tables.sort();
	tables.unique();
	push_table_to_front("tracks");
	push_table_to_front("playlistitem");
}

void
mgParts::Prepare()
{
	ConnectAllTables();
	clauses.sort();
	clauses.unique();
	valuefields.sort();
	valuefields.unique();
	idfields.sort();
	idfields.unique();
}

void
mgParts::push_table_to_front(string table)
{
	strlist::iterator it;
	for (it = tables.begin(); it != tables.end(); ++it)
	{
		if (*it==table)
		{
			tables.erase(it);
			tables.push_front(table);
			break;
		}
	}
}

string
mgParts::sql_select(bool distinct)
{
	if (!special_statement.empty())
		return special_statement;
	Prepare();
	string result;
	if (distinct)
	{
		idfields.push_front("1");
		idfields.push_front("COUNT(*)");
		result = sql_list("SELECT",idfields);
		idfields.pop_front();
		idfields.pop_front();
	}
	else
	{
		strlist::iterator p = find(tables.begin(),tables.end(),"tracks");
		if (p!=tables.end())
			idfields.push_front("tracks.id");
		idfields.push_front("1");
		result = sql_list("SELECT",idfields+valuefields);
		idfields.pop_front();
	}
	if (!result.empty())
	{
		result += sql_list(" FROM",tables);
		result += sql_list(" WHERE",clauses," AND ");
		if (distinct)
		{
			result += sql_list(" GROUP BY",idfields);
		}
	}
	return result;
}

string
mgParts::sql_selectitems()
{
	ConnectAllTables();
	string result;
	result = sql_list("SELECT",idfields);
	result += sql_list(" FROM",tables);
	result += sql_list(" WHERE",clauses," AND ");
	return result;
}

string
mgParts::sql_count()
{
	Prepare();
#if defined (HAVE_PG) || defined(HAVE_SQLITE) || MYSQL_VERSION_ID >= 40111
	string result = sql_list("SELECT COUNT(*) FROM ( SELECT",idfields,",","");
	if (result.empty())
		return result;
	result += sql_list(" FROM",tables);
	result += sql_list(" WHERE",clauses," AND ");
	result += sql_list(" GROUP BY",idfields);
	result += ") AS xx";
#else
	string result = sql_list("SELECT COUNT(DISTINCT",idfields,",",")");
	if (result.empty())
		return result;
	result += sql_list(" FROM",tables);
	result += sql_list(" WHERE",clauses," AND ");
	optimize(result);
#endif
	return result;
}

mgReference::mgReference(string t1,string f1,string t2,string f2)
{
	m_t1 = t1;
	m_f1 = f1;
	m_t2 = t2;
	m_f2 = f2;
}

void
mgReferences::InitReferences()
{
	// define them such that no circle is possible
	for (unsigned int idx = 0 ; idx < size() ; idx ++)
		delete operator[](idx);
	clear();
	push_back(new mgReference ("tracks","id","playlistitem","trackid"));
	push_back(new mgReference ("playlist","id","playlistitem","playlist"));
	push_back(new mgReference ("tracks","sourceid","album","cddbid"));
	push_back(new mgReference ("tracks","lang","language","id"));
}

unsigned int
mgReferences::CountTable(string table) const
{
	unsigned int result = 0;
	for (unsigned int i=0 ; i<size(); i++ )
	{
		mgReference* r = operator[](i);
		if (table==r->t1() || table==r->t2())
			result++;
	}
	return result;
}

bool
mgReference::Equal(string table1, string table2) const
{
	return ((t1()==table1) && (t2()==table2))
	   || ((t1()==table2) && (t2()==table1));
}

void
mgParts::ConnectTables(string table1, string table2)
{
	// same table?
	if (table1 == table2)
		return;

	// backend specific:
	if (table1=="genre") 
	{
		ConnectTables("tracks",table2);
		return;
	}
	if (table2=="genre") 
	{
		ConnectTables("tracks",table1);
		return;
	}
	// do not connect aliases. See sql_delete_from_collection
	if (table1.find(" as ")!=string::npos) return;
	if (table2.find(" as ")!=string::npos) return;
	if (table1.find(" AS ")!=string::npos) return;
	if (table2.find(" AS ")!=string::npos) return;

	// now the generic part:
	for (unsigned int i=0 ; i<rest.size(); i++ )
	{
		mgReference* r = rest[i];
		if (r->Equal(table1,table2))
		{
			rest.erase(rest.begin()+i);
			positives.push_back(r);
			return;
		}
	}
again:
	for (unsigned int i=0 ; i<rest.size(); i++ )
	{
		mgReference* r = rest[i];
		unsigned int ct1=rest.CountTable(r->t1());
		unsigned int ct2=rest.CountTable(r->t2());
		if (ct1==1 || ct2==1)
		{
			rest.erase(rest.begin()+i);
			if (ct1==1 && ct2==1)
			{
				if (r->Equal(table1,table2))
				{
					positives.push_back(r);
					return;
				}
				else
				{
					delete r;
					continue;
				}
			}
			else if (ct1==1)
			{
				if (r->t1()==table1)
				{
					positives.push_back(r);
					ConnectTables(r->t2(),table2);
				}
				else if (r->t1()==table2)
				{
					positives.push_back(r);
					ConnectTables(table1,r->t2());
				}
				else
				{
					delete r;
					goto again;
				}
			}
			else
			{
				if (r->t2()==table1)
				{
					positives.push_back(r);
					ConnectTables(r->t1(),table2);
				}
				else if (r->t2()==table2)
				{
					positives.push_back(r);
					ConnectTables(table1,r->t1());
				}
				else
				{
					delete r;
					goto again;
				}
			}
		}
	}
}


mgParts::mgParts()
{
	special_statement="";
	orderByCount = false;
}

mgParts::~mgParts()
{
}

unsigned long
mgDb::exec_count( const string sql) 
{
	unsigned long result = 0;
	if (Connect())
		result = atol (get_col0 ( sql).c_str ());
	return result;
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

void mgDb::FillTables()
{
#include "mg_tables.h"
  int len = sizeof( genres ) / sizeof( genres_t );
  StartTransaction();
  Execute("INSERT INTO genre (id,genre) VALUES('NULL','No Genre')");
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  char id3genre[5];
	  if (genres[i].id3genre>=0)
	  	sprintf(id3genre,"%d",genres[i].id3genre);
	  else
		strcpy(id3genre,"NULL");
	  sprintf(b,"INSERT INTO genre (id,id3genre,genre) VALUES ('%s',%s,%s)",
			  genres[i].id,id3genre,mgSQLString(genres[i].name).quoted());
	  Execute(b);
  }
  len = sizeof( languages ) / sizeof( lang_t );
  Execute("INSERT INTO language (id,language) VALUES('NULL','Instrumental')");
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  char id[4];
	  char lang[41];
	  strncpy(id,languages[i].id,3);
	  id[4]=0;
	  strncpy(lang,languages[i].name,40);
	  lang[40]=0;
	  sprintf(b,"INSERT INTO language (id,language) VALUES('%s',%s)",
		id,
	  	mgSQLString(lang).quoted());
	  Execute(b);
  }
  len = sizeof( musictypes ) / sizeof( musictypes_t );
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  sprintf(b,"INSERT INTO musictype (musictype) VALUES('%s')",
			  musictypes[i].name);
	  Execute(b);
  }
  len = sizeof( sources ) / sizeof( sources_t );
  for( int i=0; i < len; i ++ )
  {
	  char b[600];
	  sprintf(b,"INSERT INTO source (source) VALUES('%s')",
			  sources[i].name);
	  Execute(b);
  }
  Commit();
}

mgSQLString
mgDb::Build_cddbid(const mgSQLString& artist) const
{
	char *s;
	msprintf(&s,"%ld-%.9s",random(),artist.original());
	if (!s)
		msprintf(&s,"%ld-X",random());
	mgSQLString result = mgSQLString(s);
	free(s);
	return result;
}

mgSQLString
mgDb::getAlbum(const char *filename,const mgSQLString& c_album,
		const mgSQLString& c_artist)
{
	char *b;
	msprintf(&b,"SELECT cddbid FROM album"
			" WHERE title=%s AND artist=%s",c_album.quoted(),c_artist.quoted());
	mgSQLString result(get_col0(b));
	free(b);
	if (result=="NULL")
	{
		char *directory = strdup(filename);
		char *slash=strrchr(directory,'/');
		if (slash)
			*slash=0;
		else
			*directory=0;
		mgSQLString c_directory(directory);
		free(directory);
		char *where;
		msprintf(&where,"WHERE tracks.sourceid=album.cddbid "
			"AND %s=%s "
			"AND album.title=%s",
			Directory().c_str(),c_directory.quoted(),
			c_album.quoted());
		// how many artists will the album have after adding this one?
		msprintf(&b,"SELECT distinct album.artist FROM album, tracks %s ",where);
		mgQuery q(DbHandle(),b);
		free(b);
		long new_album_artists = q.Rows();
		mgSQLString buf("");
		if (new_album_artists==1)
		{
			buf=mgSQLString(q.Next()[0]);
			if (buf==c_artist)
				new_album_artists++;
		}
		else
			buf="";
		if (new_album_artists>1 && strcmp(buf.original(),"Various Artists"))
			// is the album multi artist and not yet marked as such?
		{
			msprintf(&b,"SELECT album.cddbid FROM album, tracks %s",where);
			result=mgSQLString(get_col0(b));
			free(b);
			msprintf(&b,"UPDATE album SET artist='Various Artists' WHERE cddbid=%s",result.quoted());
			Execute(b);
			free(b);
			// here we could change all tracks.sourceid to result and delete
			// the other album entries for this album, but that should only 
			// be needed if a pre 0.1.4 import has been done incorrectly, so we
			// don't bother
		}
		else
		{				// no usable album found
			result=Build_cddbid(c_artist);
			char *b;
			msprintf(&b,"INSERT INTO album (title,artist,cddbid) "
					"VALUES(%s,%s,%s)",
				c_album.quoted(),c_artist.quoted(),result.quoted());
			int rows = Execute(b);
			free(b);
			if (!rows)
				result="NULL";
		}
		free(where);
	}
	return result;
}

TagLib::String
mgDb::getId3v2Tag(TagLib::ID3v2::Tag *id3v2tags,const char *name) const
{
      TagLib::String result;
      TagLib::ID3v2::FrameList l = id3v2tags->frameListMap()[name];
      if (!l.isEmpty())
     	 result = l.front()->toString();
      return result;
}

void
mgDb::get_tags(TagLib::ID3v2::Tag *tags)
{
        if (!tags)
		return;
	m_TLAN = getId3v2Tag(tags,"TLAN");
	m_TCON = getId3v2Tag(tags,"TCON");
}

void
mgDb::get_ID3v2_Tags(const char *filename)
{
      if (!strcasecmp(extension(filename),"flac"))
      {
      	TagLib::FLAC::File f(filename);
	get_tags(f.ID3v2Tag());
      }
      else if (!strcasecmp(extension(filename),"mp3"))
      {
      	TagLib::MPEG::File f(filename);
	get_tags(f.ID3v2Tag());
      }
}

void
mgDb::DefineGenre(const string genre)
{
    mgQuery q1(DbHandle(),"SELECT id FROM genre WHERE id ='z'" );
    if (q1.Rows()==0)
    {
	Execute("INSERT INTO genre (id,genre) VALUES('z','Extra')");
	for (char c='a';c<='z';c++)
	{
		char g[20];
		strcpy(g,"Extra");
		if (c!='a')
			sprintf(strchr(g,0)," %c",c);
		char *b;
		msprintf(&b,"INSERT INTO genre (id,genre) VALUES('z%c','%s')",c,g);
		Execute(b);
		free(b);
	}
    }
    mgQuery q(DbHandle(),"SELECT id FROM genre WHERE id LIKE 'z__'" );
    char **r;
    char *newid=0;
    while ((r = q.Next()))
    {
	newid = r[0];
    }
    if (!newid)
	newid="zaa";
    else
    {
	newid[2]++;
	if (newid[2]>'z')
	{
		newid[1]++;
		if (newid[1]>'z')
			return;
		newid[2]='a';
	}
    }
    char *b;
    mgSQLString c_genre(genre);
    msprintf(&b,"INSERT INTO genre (id,genre) VALUES('%s',%s)",newid,c_genre.quoted());
    Execute(b);
    free(b);
    m_Genres[genre]=newid;
    mgDebug(1,"Added new genre %s",genre.c_str());
}

mgSQLString 
mgDb::getGenre1(TagLib::FileRef& f)
{
	string genre1 = f.tag()->genre().toCString(the_setup.utf8);
	if (genre1.empty())
	{
		genre1 = m_TCON.toCString(the_setup.utf8);
		const char *tcon=genre1.c_str();
		char *rparen=strchr(tcon,')');
		if (tcon[0]=='(' && rparen)
		{
			*rparen=0;
			genre1 = m_GenreIds[tcon+1];
		}
	}
	if (genre1.empty())
		return mgSQLString("NULL");
	if (m_Genres[genre1]=="")
		DefineGenre(genre1);
	return m_Genres[genre1];
}

bool
mgDb::SyncFile(const char *filename) // returns true if a new file is imported
{
      	char *ext = extension(filename);
	if (strcasecmp(ext,"flac")
		&& strcasecmp(ext,"wav")
		&& strcasecmp(ext,"ogg")
		&& strcasecmp(ext,"mp3"))
		return false;

	char sql[7000];
	if (!strncmp(filename,"./",2))	// strip leading ./
		filename += 2;
	const char *cfilename=filename;
	if (isdigit(filename[0]) && isdigit(filename[1]) && filename[2]=='/' && !strchr(filename+3,'/'))
		cfilename=cfilename+3;
	if (strlen(cfilename)>255)
	{
		mgWarning("Length of file exceeds database field capacity: %s", filename);
		return false;
	}
	TagLib::FileRef f( filename) ;

	mgDebug(2,"Importing %s",filename);
	get_ID3v2_Tags(filename);
	char *folders[4];
	char *fbuf=SeparateFolders(filename,folders,4);
	mgSQLString c_folder1(folders[0]);
	mgSQLString c_folder2(folders[1]);
	mgSQLString c_folder3(folders[2]);
	mgSQLString c_folder4(folders[3]);
	free(fbuf);
	mgSQLString c_artist("Unknown");
	mgSQLString c_album("Unassigned");
	mgSQLString c_title("Unknown");
	mgSQLString c_genre1("NULL");
	int year = 0;
	int track = 0;
	int length = 0;
	int bitrate = 0;
	int samplerate = 0;
	int channels = 0;
        if (!f.isNull())
	{
		TagLib::AudioProperties *ap = f.audioProperties();
		if (ap)
		{
			length = ap->length();
			bitrate = ap->bitrate();
			samplerate = ap->sampleRate();
			channels = ap->channels();
		}
	}
	if (!f.isNull() && f.tag())
	{
		c_artist = f.tag()->artist();
		c_album = f.tag()->album();
		c_title = f.tag()->title();
		c_genre1 = getGenre1(f);
		year = f.tag()->year();
		track = f.tag()->track();
	}
	if (strlen(c_album.original())==0)
		c_album = "Unassigned";
	mgSQLString c_lang(m_TLAN);
	mgSQLString c_cddbid(getAlbum(filename,c_album,c_artist));
	mgSQLString c_mp3file(cfilename);
	sprintf(sql,"SELECT id from tracks WHERE mp3file=%s",c_mp3file.quoted());
	string id = get_col0(sql);
	if (id!="NULL")
	  {
	    sprintf(sql,"UPDATE tracks SET artist=%s, title=%s,year=%d,sourceid=%s,"
		    "tracknb=%d,length=%d,bitrate=%d,samplerate=%d,"
		    "channels=%d,genre1=%s,lang=%s WHERE id=%ld",
		    c_artist.quoted(),c_title.quoted(),year,c_cddbid.quoted(),
		    track,length,bitrate,samplerate,
		    channels,c_genre1.quoted(),c_lang.quoted(),atol(id.c_str()));
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
		    c_artist.quoted(),c_title.quoted(),year,c_cddbid.quoted(),
		    track,c_mp3file.quoted(),length,bitrate,samplerate,
		    channels,c_genre1.quoted(),c_lang.quoted(),
		    c_folder1.quoted(),c_folder2.quoted(),
		    c_folder3.quoted(),c_folder4.quoted());
	  }
	return Execute(sql) == 1 && id=="NULL";
}

string
mgDb::get_col0( const string sql)
{
	mgQuery q(DbHandle(),sql);
	char **r = q.Next();
	if (r)
		return r[0];
	else
		return "NULL";
}

int
mgDb::Execute(const string sql)
{
  if (sql.empty())
	  return 0;
  if (!Connect())
	  return 0;
  mgQuery q(DbHandle(),sql);
  return q.Rows();
}

void
mgDb::LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap)
{
	if (!valmap && !idmap)
		return;
	if (!Connect())
		return;
	mgQuery q(DbHandle(),sql);
	char **row;
	while ((row = q.Next()))
		if (row[0] && row[1])
		{
			if (valmap) (*valmap)[row[0]] = row[1];
			if (idmap) (*idmap)[row[1]] = row[0];
		}
}

string
mgDb::LoadItemsInto(mgParts& what,vector<mgItem*>& items)
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
	what.idfields.push_back("album.coverimg");
	what.tables.push_back("tracks");
	what.tables.push_back("album");
	string result = what.sql_selectitems(); 
	for (unsigned int idx=0;idx<items.size();idx++)
		delete items[idx];
	items.clear ();
	mgQuery q(DbHandle(),result);
	char **row;
	while ((row = q.Next()))
		items.push_back (new mgItemGd (row));
	return result;
}


string
mgDb::LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems,bool distinct)
{
    	if (!Connect())
		return "";
	string result = what.sql_select(distinct);
        listitems.clear ();
	mgQuery q(DbHandle(),result);
	if (q.Rows())
		assert(q.Columns()==4 || q.Columns()==3);
	char **row;
	while ((row = q.Next()))
	{
		if (!row[0]) continue;
		if (!row[1]) continue;
		if (!row[2]) continue;
		if (q.Columns()==4 && !row[3]) continue;
		mgListItem* n = new mgListItem;
		long count = atol(row[0]);
		n->set_unique_id(row[1]);
		if (q.Columns()==4)
			n->set(row[3],row[2],count);
		else
			n->set(KeyMaps.value(tp,row[2]),row[2],count);
		listitems.push_back(n);
	}
	return result;
}

int
mgDb::AddToCollection( const string Name, const vector<mgItem*>&items,mgParts *what)
{
    if (Name.empty())
	    return 0;
    if (!Connect())
	    return 0;
    CreateCollection(Name);
    string listid = KeyMaps.id(keyGdCollection,Name);
    if (listid.empty())
	    return 0;
    StartTransaction();
    // insert all tracks:
    int result = 0;
    for (unsigned int i = 0; i < items.size(); i++)
	result += Execute("INSERT INTO playlistitem VALUES( " + listid + ","
			+ ltos (items[i]->getItemid ()) +")");
    Commit();
    return result;
}

int
mgDb::RemoveFromCollection (const string Name, const vector<mgItem*>&items,mgParts* what)
{
    if (Name.empty())
	    return 0;
    if (!Connect())
	    return 0;
    string listid = KeyMaps.id(keyGdCollection,Name);
    if (listid.empty())
	    return 0;
    StartTransaction();
    // remove all tracks:
    int result = 0;
    for (unsigned int i = 0; i < items.size(); i++)
	result += Execute("DELETE FROM playlistitem WHERE playlist="+listid+
			" AND trackid = " + ltos (items[i]->getItemid ()));
    Commit();
    return result;
}

bool
mgDb::DeleteCollection (const string Name)
{
    if (!Connect()) return false;
    ClearCollection(Name);
    return Execute (string("DELETE FROM playlist WHERE title=") + mgSQLString (Name).quoted()) == 1;
}

void
mgDb::ClearCollection (const string Name)
{
    if (!Connect()) return;
    string listid = KeyMaps.id(keyGdCollection,Name);
    Execute (string("DELETE FROM playlistitem WHERE playlist=")+mgSQLString(listid).quoted());
}

bool
mgDb::CreateCollection (const string Name)
{
    if (!Connect()) return false;
    string name = mgSQLString(Name).quoted();
    if (exec_count("SELECT count(title) FROM playlist WHERE title = " + name)>0) 
	return false;
    Execute ("INSERT INTO playlist (title,author,created) VALUES(" + name + ",'VDR',"+Now()+")");
    return true;
}

class mgKeyGdUnique : public mgKeyNormal {
	public:
		mgKeyGdUnique() : mgKeyNormal(keyGdTrack,"tracks","id") {};
		mgParts Parts(mgDb *db,bool groupby) const;
		mgSortBy SortBy() const { return mgSortByIdNum; }
};

class mgKeyGdTrack : public mgKeyNormal {
	public:
		mgKeyGdTrack() : mgKeyNormal(keyGdTrack,"tracks","tracknb") {};
		mgParts Parts(mgDb *db,bool groupby) const;
		mgSortBy SortBy() const { return mgSortByIdNum; }
};

class mgKeyGdAlbum : public mgKeyNormal {
	public:
		mgKeyGdAlbum() : mgKeyNormal(keyGdAlbum,"album","title") {};
		mgParts Parts(mgDb *db,bool groupby) const;
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
    {
	if (!db->Connect()) return false;
	m_enabled = db->FieldExists("tracks", m_field);
    }
    return (m_enabled==1);
}

class mgKeyGdGenres : public mgKeyNormal {
	public:
		mgKeyGdGenres() : mgKeyNormal(keyGdGenres,"tracks","genre1") {};
		mgKeyGdGenres(mgKeyTypes kt) : mgKeyNormal(kt,"tracks","genre1") {};
		mgParts Parts(mgDb *db,bool groupby) const;
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
		mgParts Parts(mgDb *db,bool groupby) const;
	protected:
		string map_sql() const { return "SELECT id,language FROM language"; }
};

class mgKeyGdCollection: public mgKeyNormal {
	public:
  	  mgKeyGdCollection() : mgKeyNormal(keyGdCollection,"playlist","id") {};
	  mgParts Parts(mgDb *db,bool groupby) const;
	protected:
	 string map_sql() const { return "SELECT id,title FROM playlist"; }
};
class mgKeyGdCollectionItem : public mgKeyNormal {
	public:
		mgKeyGdCollectionItem() : mgKeyNormal(keyGdCollectionItem,"playlistitem","trackid") {};
		mgParts Parts(mgDb *db,bool groupby) const;
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
		case keyGdUnique: result = new mgKeyNormal(kt,"tracks","id");break;
		default: result = 0; break;
	}
	return result;
}

mgParts
mgKeyGdUnique::Parts(mgDb *db,bool groupby) const
{
	mgParts result;
	result.tables.push_back("tracks");
	AddIdClause(db,result,"tracks.id");
	result.idfields.push_back("tracks.id");
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
		result.idfields.push_back("tracks.id");
	}
	return result;
}
