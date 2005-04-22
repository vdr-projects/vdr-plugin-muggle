#include "mg_order.h"
#include "mg_item_gd.h"
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

class mgKeyFolder1 : public mgKeyFolder {
	public:
		mgKeyFolder1() : mgKeyFolder(keyFolder1,"folder1") {};
};
class mgKeyFolder2 : public mgKeyFolder {
	public:
		mgKeyFolder2() : mgKeyFolder(keyFolder2,"folder2") {};
};
class mgKeyFolder3 : public mgKeyFolder {
	public:
		mgKeyFolder3() : mgKeyFolder(keyFolder3,"folder3") {};
};
class mgKeyFolder4 : public mgKeyFolder {
	public:
		mgKeyFolder4() : mgKeyFolder(keyFolder4,"folder4") {};
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
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "genre"; }
		string map_table() const { return "genre"; }
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
		result.fields.push_back("genre.id");
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
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "language"; }
		string map_table() const { return "language"; }
};

class mgKeyCollection: public mgKeyNormal {
	public:
  	  mgKeyCollection() : mgKeyNormal(keyCollection,"playlist","id") {};
	  mgParts Parts(mgDb *db,bool orderby=false) const;
	protected:
	 string map_idfield() const { return "id"; }
	 string map_valuefield() const { return "title"; }
	 string map_table() const { return "playlist"; }
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
	m_sql_select="";
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
			mgDebug(5,"Prepare:Connect %s with %s",prevtable.c_str(),(*it).c_str());
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
	mgDebug(5,"Prepare done");
}

string
mgParts::sql_select(bool distinct)
{
	if (!m_sql_select.empty())
		return m_sql_select;
	Prepare();
	string result;
	if (distinct)
	{
		fields.push_back("COUNT(*) AS mgcount");
		result = sql_list("SELECT",fields);
		fields.pop_back();
	}
	else
		result = sql_list("SELECT",fields);
	if (result.empty())
		return result;
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

mgOrder::mgOrder()
{
       	clear();
	setKey (keyArtist);
       	setKey (keyAlbum);
       	setKey (keyTrack);
	m_orderByCount = false;
}

mgOrder::~mgOrder()
{
	truncate(0);
}

mgKey*
mgOrder::Key(unsigned int idx) const
{
	return Keys[idx];
}

mgKey*&
mgOrder::operator[](unsigned int idx) 
{
	assert(idx<size());
	return Keys[idx];
}

bool
operator==(const mgOrder& a, const mgOrder &b)
{
    bool result =  a.size()==b.size();
    if (result)
    	for (unsigned int i=0; i<a.size();i++)
    	{
    		result &= a.Key(i)->Type()==b.Key(i)->Type();
		if (!result) break;
    	}
    return result;
}

const mgOrder&
mgOrder::operator=(const mgOrder& from)
{
	clear();
	InitFrom(from);
	return *this;
}

mgOrder::mgOrder(const mgOrder &from)
{
	InitFrom(from);
}

void
mgOrder::InitFrom(const mgOrder &from)
{
    	for (unsigned int i = 0; i < from.size();i++)
    	{
        	mgKey *k = ktGenerate(from.getKeyType(i));
		k->set(from.getKeyItem(i));
		Keys.push_back(k);
    	}
	m_orderByCount=from.m_orderByCount;
	m_level=from.m_level;
}

string
mgOrder::Name()
{
	string result="";
	for (unsigned int idx=0;idx<size();idx++)
	{
		if (!result.empty()) result += ":";
		result += ktName(Keys[idx]->Type());
	}
	return result;
}

void
mgOrder::setKey (const mgKeyTypes kt)
{
    mgKey *newkey = ktGenerate(kt);
    if (newkey)
    	Keys.push_back(newkey);
}

mgOrder::mgOrder(mgValmap& nv,char *prefix)
{
	char *idx;
	asprintf(&idx,"%s.OrderByCount",prefix);
	m_orderByCount = nv.getbool(idx);
	free(idx);
	clear();
	for (unsigned int i = 0; i < 999 ; i++)
	{
		asprintf(&idx,"%s.Keys.%u.Type",prefix,i);
		unsigned int v = nv.getuint(idx);
		free(idx);
		if (v==0) break;
        	setKey (mgKeyTypes(v) );
	}
	if (size()>0)
		clean();
}

void
mgOrder::DumpState(mgValmap& nv, char *prefix) const
{
	char n[100];
	sprintf(n,"%s.OrderByCount",prefix);
	nv.put(n,m_orderByCount);
    	for (unsigned int i=0;i<size();i++)
	{
		sprintf(n,"%s.Keys.%d.Type",prefix,i);
		nv.put(n,int(Key(i)->Type()));
	}
}

mgOrder::mgOrder(vector<mgKeyTypes> kt)
{
	m_orderByCount = false;
	m_level=0;
	setKeys(kt);
}

void
mgOrder::setKeys(vector<mgKeyTypes> kt)
{
	clear();
	for (unsigned int i=0;i<kt.size();i++)
		setKey(kt[i]);
        clean();
}


mgKeyTypes
mgOrder::getKeyType(unsigned int idx) const
{
	assert(idx<Keys.size());
	return Keys[idx]->Type();
}

mgListItem*
mgOrder::getKeyItem(unsigned int idx) const
{
	assert(idx<Keys.size());
	return Keys[idx]->get();
}

void
mgOrder::truncate(unsigned int i)
{
	while (size()>i)
	{
		delete Keys.back();
		Keys.pop_back();
	}
}

void
mgOrder::clear()
{
	truncate(0);
	m_level=0;
}

void
mgOrder::clean()
{
	// remove double entries:
	keyvector::iterator i;
	keyvector::iterator j;
	bool collection_found = false;	
	bool collitem_found = false;	
	bool album_found = false;	
	bool tracknb_found = false;	
	bool title_found = false;	
	bool is_unique = false;
	for (i = Keys.begin () ; i != Keys.end (); ++i)
	{
		mgKeyNormal* k = dynamic_cast<mgKeyNormal*>(*i);
		collection_found |= (k->Type()==keyCollection);
		collitem_found |= (k->Type()==keyCollectionItem);
		album_found |= (k->Type()==keyAlbum);
		tracknb_found |= (k->Type()==keyTrack);
		title_found |= (k->Type()==keyTitle);
		is_unique = tracknb_found || (album_found && title_found)
			|| (collection_found && collitem_found);
		if (is_unique)
		{
			for (j = i+1 ; j !=Keys.end(); ++j)
				delete *j;
			Keys.erase(i+1,Keys.end ());
			break;
		}
		if (k->Type()==keyYear)
		{
			for (j = i+1 ; j != Keys.end(); ++j)
				if ((*j)->Type() == keyDecade)
				{
					delete *j;
					Keys.erase(j);
					break;
				}
		}
cleanagain:
		for (j = i+1 ; j != Keys.end(); ++j)
			if ((*i)->Type() == (*j)->Type())
			{
				delete *j;
				Keys.erase(j);
				goto cleanagain;
			}
	}
	if (!is_unique)
	{
		if (!album_found)
			Keys.push_back(ktGenerate(keyAlbum));
		if (!title_found)
			Keys.push_back(ktGenerate(keyTitle));
	}
}

bool
mgOrder::isCollectionOrder() const
{
	return (size()==2
		&& (Keys[0]->Type()==keyCollection)
		&& (Keys[1]->Type()==keyCollectionItem));
}

mgParts
mgOrder::Parts(mgDb *db,unsigned int level,bool orderby) const
{
	mgParts result;
	result.orderByCount = m_orderByCount;
	if (level==0 &&  isCollectionOrder())
	{
		// sql command contributed by jarny
		result.m_sql_select = string("select playlist.title,playlist.id, "
				"count(*) * (playlistitem.playlist is not null) from playlist "
				"left join playlistitem on playlist.id = playlistitem.playlist "
				"group by playlist.title");
		return result;
	}
	for (unsigned int i=0;i<=level;i++)
	{
		if (i==Keys.size()) break;
		mgKeyNormal *k = dynamic_cast<mgKeyNormal*>(Keys[i]);
		mgKeyTypes kt = k->Type();
		if (iskeyGenre(kt))
		{
			for (unsigned int j=i+1;j<=level;j++)
			{
				if (j>=Keys.size())
					break;
				mgKeyNormal *kn = dynamic_cast<mgKeyNormal*>(Keys[j]);
				if (kn)
				{
					mgKeyTypes knt = kn->Type();
					if (iskeyGenre(knt) && knt>kt && !kn->id().empty())
						goto next;
				}
			}
		}
		result += k->Parts(db,orderby && (i==level));
next:
		continue;
	}
	return result;
}

string
mgOrder::GetContent(mgDbGd *db,unsigned int level,vector < mgItem* > &content) const
{
    mgParts p = Parts(db,level);
    p.fields.clear();
    p.fields.push_back("tracks.id");
    p.fields.push_back("tracks.title");
    p.fields.push_back("tracks.mp3file");
    p.fields.push_back("tracks.artist");
    p.fields.push_back("album.title");
    p.fields.push_back("tracks.genre1");
    p.fields.push_back("tracks.genre2");
    p.fields.push_back("tracks.bitrate");
    p.fields.push_back("tracks.year");
    p.fields.push_back("tracks.rating");
    p.fields.push_back("tracks.length");
    p.fields.push_back("tracks.samplerate");
    p.fields.push_back("tracks.channels");
    p.fields.push_back("tracks.lang");
    p.tables.push_back("tracks");
    p.tables.push_back("album");
    for (unsigned int i = level; i<size(); i++)
    	p += Key(i)->Parts(db,true);
     string result = p.sql_select(false); 
     MYSQL_RES *rows = db->exec_sql (result);
     if (rows)
     {
        for (unsigned int idx=0;idx<content.size();idx++)
		delete content[idx];
	content.clear ();
     	MYSQL_ROW row;
      	while ((row = mysql_fetch_row (rows)) != 0)
		content.push_back (new mgItemGd (row));
        mysql_free_result (rows);
     }
     return result;
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
	mgDebug(5,"Connecting %s with %s",table1.c_str(),table2.c_str());
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

	mgDebug(5,"generic:rest.size()=%d",rest.size());
	// now the generic part:
	for (unsigned int i=0 ; i<rest.size(); i++ )
	{
		mgReference* r = rest[i];
		if (r->Equal(table1,table2))
		{
			rest.erase(rest.begin()+i);
			positives.push_back(r);
			mgDebug(5,"use:%s-%s",r->t1().c_str(),r->t2().c_str());
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
					mgDebug(5,"use:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
					return;
				}
				else
				{
					mgDebug(5,"throw away:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
					delete r;
					continue;
				}
			}
			else if (ct1==1)
			{
				if (r->t1()==table1)
				{
					positives.push_back(r);
					mgDebug(5,"use:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
					ConnectTables(r->t2(),table2);
				}
				else if (r->t1()==table2)
				{
					positives.push_back(r);
					mgDebug(5,"use:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
					ConnectTables(table1,r->t2());
				}
				else
				{
					mgDebug(5,"throw away:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
					delete r;
					goto again;
				}
			}
			else
			{
				if (r->t2()==table1)
				{
					positives.push_back(r);
					mgDebug(5,"use:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
					ConnectTables(r->t1(),table2);
				}
				else if (r->t2()==table2)
				{
					positives.push_back(r);
					mgDebug(5,"use:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
					ConnectTables(table1,r->t1());
				}
				else
				{
					mgDebug(5,"throw away:%s(%d)-%s(%d)",r->t1().c_str(),ct1,r->t2().c_str(),ct2);
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
		case keyFolder1:result = new mgKeyFolder1;break;
		case keyFolder2:result = new mgKeyFolder2;break;
		case keyFolder3:result = new mgKeyFolder3;break;
		case keyFolder4:result = new mgKeyFolder4;break;
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

const char * const
ktName(const mgKeyTypes kt) 
{
	const char * result = "";
	switch (kt)
	{
		case keyGenres: result = "Genre";break;
		case keyGenre1: result = "Genre1";break;
		case keyGenre2: result = "Genre2";break;
		case keyGenre3: result = "Genre3";break;
		case keyFolder1: result = "Folder1";break;
		case keyFolder2: result = "Folder2";break;
		case keyFolder3: result = "Folder3";break;
		case keyFolder4: result = "Folder4";break;
		case keyArtist: result = "Artist";break;
		case keyArtistABC: result = "ArtistABC";break;
		case keyTitle: result = "Title";break;
		case keyTitleABC: result = "TitleABC";break;
		case keyTrack: result = "Track";break;
		case keyDecade: result = "Decade";break;
		case keyAlbum: result = "Album";break;
		case keyCreated: result = "Created";break;
		case keyModified: result = "Modified";break;
		case keyCollection: result = "Collection";break;
		case keyCollectionItem: result = "Collection item";break;
		case keyLanguage: result = "Language";break;
		case keyRating: result = "Rating";break;
		case keyYear: result = "Year";break;
	}
	return tr(result);
}

mgKeyTypes
ktValue(const char * name)
{
	for (int kt=int(mgKeyTypesLow);kt<=int(mgKeyTypesHigh);kt++)
		if (!strcmp(name,ktName(mgKeyTypes(kt))))
				return mgKeyTypes(kt);
	mgError("ktValue(%s): unknown name",name);
	return mgKeyTypes(0);
}

static vector<int> keycounts;

unsigned int
mgOrder::keycount(mgKeyTypes kt) const
{
	if (keycounts.size()==0)
	{
		for (unsigned int ki=int(mgKeyTypesLow);ki<=int(mgKeyTypesHigh);ki++)
		{
			keycounts.push_back(-1);
		}
	}
	int& kcount = keycounts[int(kt-mgKeyTypesLow)];
	if (kcount==-1)
	{
		mgKey* k = ktGenerate(kt);
		struct mgDb *db = GenerateDB();
		if (k->Enabled(db))
			kcount = db->exec_count(k->Parts(db,true).sql_count());
		else
			kcount = 0;
		delete k;
		delete db;
	}
	return kcount;
}


bool
mgOrder::UsedBefore(const mgKeyTypes kt,unsigned int level) const
{
	if (level>=size())
		level = size() -1;
	for (unsigned int lx = 0; lx < level; lx++)
		if (getKeyType(lx)==kt)
			return true;
	return false;
}

vector <const char *>
mgOrder::Choices(unsigned int level, unsigned int *current) const
{
	vector<const char*> result;
	if (level>size())
	{
		*current = 0;
		return result;
	}
	for (unsigned int ki=int(mgKeyTypesLow);ki<=int(mgKeyTypesHigh);ki++)
	{
		mgKeyTypes kt = mgKeyTypes(ki);
		if (kt==getKeyType(level))
		{
			*current = result.size();
			result.push_back(ktName(kt));
			continue;
		}
		if (UsedBefore(kt,level))
			continue;
		if (kt==keyDecade && UsedBefore(keyYear,level))
			continue;
		if (kt==keyGenre1)
		{
			if (UsedBefore(keyGenre2,level)) continue;
			if (UsedBefore(keyGenre3,level)) continue;
			if (UsedBefore(keyGenres,level)) continue;
		}
		if (kt==keyGenre2)
		{
			if (UsedBefore(keyGenre3,level)) continue;
			if (UsedBefore(keyGenres,level)) continue;
		}
		if (kt==keyGenre3)
		{
			if (UsedBefore(keyGenres,level)) continue;
		}
		if (kt==keyFolder1)
		{
		 	if (UsedBefore(keyFolder2,level)) continue;
		 	if (UsedBefore(keyFolder3,level)) continue;
		 	if (UsedBefore(keyFolder4,level)) continue;
		}
		if (kt==keyFolder2)
		{
		 	if (UsedBefore(keyFolder3,level)) continue;
		 	if (UsedBefore(keyFolder4,level)) continue;
		}
		if (kt==keyFolder3)
		{
		 	if (UsedBefore(keyFolder4,level)) continue;
		}
		if (kt==keyCollectionItem)
		{
		 	if (!UsedBefore(keyCollection,level)) continue;
		}
		if (kt==keyCollection)
			result.push_back(ktName(kt));
		else if (keycount(kt)>1)
			result.push_back(ktName(kt));
	}
	return result;
}

bool
mgKey::LoadMap() const
{
	if (map_idfield().empty())
		return false;
	mgDb *db = GenerateDB();
	char *b;
	asprintf(&b,"select %s,%s from %s;",map_idfield().c_str(),map_valuefield().c_str(),map_table().c_str());
	db->LoadMapInto(b, map_ids[Type()], map_values[Type()]);
	free(b);
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
		string v = idmap[valstr];
		if (kt==keyGenre1) v=v.substr(0,1);
		if (kt==keyGenre2) v=v.substr(0,2);
		if (kt==keyGenre3) v=v.substr(0,3);
		return v;
	}
	return valstr;
}
