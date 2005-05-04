#include "mg_order.h"
#include "mg_tools.h"
#include "i18n.h"
#include <stdio.h>
#include <assert.h>


static map <mgKeyTypes, map<string,string> > map_values;
static map <mgKeyTypes, map<string,string> > map_ids;

mgKeyMaps KeyMaps;

bool iskeyGenre(mgKeyTypes kt)
{
	return kt>=keyGenre1  && kt <= keyGenres;
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


class mgKeyNormal : public mgKey {
	public:
		mgKeyNormal(const mgKeyNormal& k);
		mgKeyNormal(const mgKeyTypes kt, string table, string field);
		virtual mgParts Parts(mgDb *db,bool orderby=false) const;
		string value() const;
		string id() const;
		bool valid() const;
		void set(mgListItem* item);
		mgListItem* get();
		mgKeyTypes Type() const { return m_kt; }
		virtual string expr() const { return m_table + "." + m_field; }
		virtual string table() const { return m_table; }
	protected:
		string IdClause(mgDb *db,string what,string::size_type start=0,string::size_type len=string::npos) const;
		void AddIdClause(mgDb *db,mgParts &result,string what) const;
		mgListItem *m_item;
		string m_field;
	private:
		mgKeyTypes m_kt;
		string m_table;
};

class mgKeyABC : public mgKeyNormal {
	public:
		mgKeyABC(const mgKeyNormal& k) : mgKeyNormal(k) {}
		mgKeyABC(const mgKeyTypes kt, string table, string field) : mgKeyNormal(kt,table,field) {}
		virtual string expr() const { return "substring("+mgKeyNormal::expr()+",1,1)"; }
	protected:
		//void AddIdClause(mgDb *db,mgParts &result,string what) const;
};

class mgKeyDate : public mgKeyNormal {
	public:
		mgKeyDate(mgKeyTypes kt,string table, string field) : mgKeyNormal(kt,table,field) {}
};

class mgKeyTrack : public mgKeyNormal {
	public:
		mgKeyTrack() : mgKeyNormal(keyTrack,"tracks","tracknb") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
};

class mgKeyAlbum : public mgKeyNormal {
	public:
		mgKeyAlbum() : mgKeyNormal(keyAlbum,"album","title") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
};

mgParts
mgKeyAlbum::Parts(mgDb *db,bool orderby) const
{
	mgParts result = mgKeyNormal::Parts(db,orderby);
	result.tables.push_back("tracks");
	return result;
}

class mgKeyFolder : public mgKeyNormal {
	public:
		mgKeyFolder(mgKeyTypes kt,const char *fname)
			: mgKeyNormal(kt,"tracks",fname) { m_enabled=-1;};
		bool Enabled(mgDb *db);
	private:
		int m_enabled;
};


bool
mgKeyFolder::Enabled(mgDb *db)
{
    if (m_enabled<0)
	m_enabled = db->FieldExists("tracks", m_field);
    return (m_enabled==1);
}

class mgKeyGenres : public mgKeyNormal {
	public:
		mgKeyGenres() : mgKeyNormal(keyGenres,"tracks","genre1") {};
		mgKeyGenres(mgKeyTypes kt) : mgKeyNormal(kt,"tracks","genre1") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
	protected:
		string map_sql() const;
		virtual unsigned int genrelevel() const { return 4; }
	private:
		string GenreClauses(mgDb *db,bool orderby) const;
};

class mgKeyGenre1 : public mgKeyGenres
{
	public:
		mgKeyGenre1() : mgKeyGenres(keyGenre1) {}
		unsigned int genrelevel() const { return 1; }
};

class mgKeyGenre2 : public mgKeyGenres
{
	public:
		mgKeyGenre2() : mgKeyGenres(keyGenre2) {}
		unsigned int genrelevel() const { return 2; }
};

class mgKeyGenre3 : public mgKeyGenres
{
	public:
		mgKeyGenre3() : mgKeyGenres(keyGenre3) {}
		unsigned int genrelevel() const { return 3; }
};

string
mgKeyGenres::map_sql() const
{
	if (genrelevel()==4)
		return "select id,genre from genre;";
	else
		return string("select id,genre from genre where length(id)="+ltos(genrelevel())+";");
}

string
mgKeyGenres::GenreClauses(mgDb *db,bool orderby) const
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
mgKeyGenres::Parts(mgDb *db,bool orderby) const
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


class mgKeyLanguage : public mgKeyNormal {
	public:
		mgKeyLanguage() : mgKeyNormal(keyLanguage,"tracks","lang") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
	protected:
		string map_sql() const { return "select id,language from language;"; }
};

class mgKeyCollection: public mgKeyNormal {
	public:
  	  mgKeyCollection() : mgKeyNormal(keyCollection,"playlist","id") {};
	  mgParts Parts(mgDb *db,bool orderby=false) const;
	protected:
	 string map_sql() const { return "select id,title from playlist;"; }
};
class mgKeyCollectionItem : public mgKeyNormal {
	public:
		mgKeyCollectionItem() : mgKeyNormal(keyCollectionItem,"playlistitem","tracknumber") {};
		mgParts Parts(mgDb *db,bool orderby=false) const;
};

class mgKeyDecade : public mgKeyNormal {
	public:
		mgKeyDecade() : mgKeyNormal(keyDecade,"tracks","year") {}
		string expr() const { return "substring(convert(10 * floor(tracks.year/10), char),3)"; }
};

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

mgParts::mgParts()
{
	special_statement="";
	orderByCount = false;
}

mgParts::~mgParts()
{
}

mgParts
mgKeyNormal::Parts(mgDb *db, bool orderby) const
{
	mgParts result;
	result.tables.push_back(table());
	AddIdClause(db,result,expr());
	if (orderby)
	{
		result.fields.push_back(expr());
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

mgParts
mgKeyTrack::Parts(mgDb *db,bool orderby) const
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
mgKeyLanguage::Parts(mgDb *db,bool orderby) const
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
mgKeyCollection::Parts(mgDb *db,bool orderby) const
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
mgKeyCollectionItem::Parts(mgDb *db,bool orderby) const
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

mgParts&
mgParts::operator+=(mgParts a)
{
	fields += a.fields;
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
	strlist::reverse_iterator it;
	string prevtable = "";
	rest.InitReferences();
	positives.clear();
	for (it = tables.rbegin(); it != tables.rend(); ++it)
	{
		if (!prevtable.empty())
		{
			rest.InitReferences();
			ConnectTables(prevtable,*it);
		}
		prevtable = *it;
	}
	for (unsigned int i = 0 ; i < positives.size(); i++)
	{
		*this += mgRefParts(positives[i]);
	}
	tables.sort();
	tables.unique();
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
	if (fields.empty())
		return "";
	string result;
	if (distinct)
	{
		fields.push_back("COUNT(*) AS mgcount");
		result = sql_list("SELECT",fields);
		fields.pop_back();
	}
	else
		result = sql_list("SELECT",fields);
	result += sql_list("FROM",tables);
	result += sql_list("WHERE",clauses," AND ");
	if (distinct)
	{
		result += sql_list("GROUP BY",fields);
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
	string result = sql_list("SELECT COUNT(DISTINCT",fields,",",")");
	if (result.empty())
		return result;
	result += sql_list("FROM",tables);
	result += sql_list("WHERE",clauses," AND ");
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


mgKey*
ktGenerate(const mgKeyTypes kt)
{
	mgKey* result = 0;
	switch (kt)
	{
		case keyGenres:	result = new mgKeyGenres;break;
		case keyGenre1:	result = new mgKeyGenre1;break;
		case keyGenre2:	result = new mgKeyGenre2;break;
		case keyGenre3:	result = new mgKeyGenre3;break;
		case keyFolder1:result = new mgKeyFolder(keyFolder1,"folder1");break;
		case keyFolder2:result = new mgKeyFolder(keyFolder2,"folder2");break;
		case keyFolder3:result = new mgKeyFolder(keyFolder3,"folder3");break;
		case keyFolder4:result = new mgKeyFolder(keyFolder4,"folder4");break;
		case keyArtist: result = new mgKeyNormal(kt,"tracks","artist");break;
		case keyArtistABC: result = new mgKeyABC(kt,"tracks","artist");break;
		case keyTitle: result = new mgKeyNormal(kt,"tracks","title");break;
		case keyTitleABC: result = new mgKeyABC(kt,"tracks","title");break;
		case keyTrack: result = new mgKeyTrack;break;
		case keyDecade: result = new mgKeyDecade;break;
		case keyAlbum: result = new mgKeyAlbum;break;
		case keyCreated: result = new mgKeyDate(kt,"tracks","created");break;
		case keyModified: result = new mgKeyDate(kt,"tracks","modified");break;
		case keyCollection: result = new mgKeyCollection;break;
		case keyCollectionItem: result = new mgKeyCollectionItem;break;
		case keyLanguage: result = new mgKeyLanguage;break;
		case keyRating: result = new mgKeyNormal(kt,"tracks","rating");break;
		case keyYear: result = new mgKeyNormal(kt,"tracks","year");break;
	}
	return result;
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
