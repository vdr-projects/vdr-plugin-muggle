/*! 
 * \file   mg_db.c
 * \brief  A capsule around database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-04-13 17:42:54 +0100 (Thu, 13 Apr 2005) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wolfgang61 $
 */

#include <string>

using namespace std;

#include "mg_db.h"

#include "mg_setup.h"
#include "mg_tools.h"


#include <sys/stat.h>
#include <fts.h>

static map <mgKeyTypes, map<string,string> > map_values;
static map <mgKeyTypes, map<string,string> > map_ids;

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

string
mgDb::sql_string( const string s )
{
  char *b = sql_Cstring(s);
  string result = string( b);
  free( b);
  return result;
}

char*
mgDb::sql_Cstring( const string s, char *buf )
{
  return sql_Cstring(s.c_str(),buf);
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

void
mgDb::Sync(char * const * path_argv)
{
	if (!SyncStart())
		return;
  	extern void showimportcount(unsigned int,bool final=false);

	unsigned int importcount=0;
	chdir(the_setup.ToplevelDir);
	FTS *fts;
	FTSENT *ftsent;
	if (!path_argv)
	{
		static char *default_args[] = { ".", 0};
		path_argv = default_args;
	}
	fts = fts_open( path_argv, FTS_LOGICAL, 0);
	if (fts)
	{
		while ( (ftsent = fts_read(fts)) != NULL)
		{
			mode_t mode = ftsent->fts_statp->st_mode;
			if (mode&S_IFDIR && ftsent->fts_info&FTS_D)
				mgDebug(1,"Importing from %s",ftsent->fts_path);
			if (!(mode&S_IFREG))
				continue;
			SyncFile(ftsent->fts_path);
			importcount++;
			if (importcount%1000==0)
				showimportcount(importcount);
		}
		fts_close(fts);
	}
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
mgKeyNormal::Parts(mgDb *db, bool orderby) const
{
	mgParts result;
	result.tables.push_back(table());
	AddIdClause(db,result,expr());
	if (orderby)
	{
		result.idfields.push_back(expr());
       		result.orders.push_back(expr());
	}
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
		return what + "=" + db->sql_string(id());
	else
	{
		return "substring("+what + ","+ltos(start+1)+","+ltos(len)+")="
			+ db->sql_string(id().substr(start,len));
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
    		result.insert(0," "+prefix+" ");
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
	orders += a.orders;
	return *this;
}

mgRefParts::mgRefParts(const mgReference* r)
{
	tables.push_back(r->t1());
	tables.push_back(r->t2());
	clauses.push_back(r->t1() + '.' + r->f1() + '=' + r->t2() + '.' + r->f2());
}

void
mgParts::Prepare()
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
	strlist::iterator it;
	for (it = tables.begin(); it != tables.end(); ++it)
	{
		if (*it=="tracks")
		{
			tables.erase(it);
			tables.push_front("tracks");
			break;
		}
	}
	clauses.sort();
	clauses.unique();
	orders.unique();
}

string
mgParts::sql_select(bool distinct)
{
	if (!special_statement.empty())
		return special_statement;
	Prepare();
	if (idfields.empty())
		return "";
	string result;
	if (distinct)
	{
		idfields.push_back("COUNT(*) AS mgcount");
		result = sql_list("SELECT",idfields);
		idfields.pop_back();
	}
	else
		result = sql_list("SELECT",idfields+valuefields);
	result += sql_list("FROM",tables);
	result += sql_list("WHERE",clauses," AND ");
	if (distinct)
	{
		result += sql_list("GROUP BY",idfields);
 		if (orderByCount)
			orders.insert(orders.begin(),"mgcount desc");
	}
	result += sql_list("ORDER BY",orders);
	return result;
}

string
mgParts::sql_count()
{
	Prepare();
	string result = sql_list("SELECT COUNT() FROM ( SELECT",idfields,",","");
	if (result.empty())
		return result;
	result += sql_list("FROM",tables);
	result += sql_list("WHERE",clauses," AND ");
	result += sql_list("GROUP BY",idfields);
	result += ")";
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

