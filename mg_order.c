#include "mg_order.h"
#include "mg_tools.h"
#include "i18n.h"
#include <stdio.h>
#include <assert.h>

const char * EMPTY = "XNICHTGESETZTX";

bool iskeyGenre(mgKeyTypes kt)
{
	return kt>=keyGenre1  && kt <= keyGenres;
}

class mgRefParts : public mgParts {
	public:
		mgRefParts(const mgReference& r);
};


strlist& operator+=(strlist&a, strlist b) 
{
	a.insert(a.end(), b.begin(),b.end());
	return a;
}


/*! \brief if the SQL command works on only 1 table, remove all table
* qualifiers. Example: SELECT tracks.title FROM tracks becomes SELECT title
* FROM tracks
* \param spar the sql command. It will be edited in place
* \return the new sql command is also returned
*/
static string
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
		string::size_type track;
		while ((track = spar.find (from)) != string::npos)
		{
    			spar.erase (track, from.size ());
		}
	}
	return spar;
}

string&
addsep (string & s, string sep, string n)
{
	if (!n.empty ())
	{
		if (!s.empty ())
    		s.append (sep);
		s.append (n);
	}
	return s;
}


static string
sql_list (string prefix,strlist v,string sep=",",string postfix="")
{
	string result = "";
	for (list < string >::iterator it = v.begin (); it != v.end (); ++it)
	{
		addsep (result, sep, *it);
	}
	if (!result.empty())
	{
    		result.insert(0," "+prefix+" ");
    	result += postfix;
	}
	return result;
}

//! \brief converts long to string
string
itos (int i)
{
	stringstream s;
	s << i;
	return s.str ();
}

//! \brief convert long to string
string
ltos (long l)
{
	stringstream s;
	s << l;
	return s.str ();
}

class mgKeyNormal : public mgKey {
	public:
		mgKeyNormal(const mgKeyNormal& k);
			mgKeyNormal(const mgKeyTypes kt, string table, string field);
		virtual mgParts Parts(mgmySql &db,bool orderby=false) const;
		string value() const;
		string id() const;
		void set(string value,string id);
		mgKeyTypes Type() const { return m_kt; }
		virtual string expr() const { return m_table + "." + m_field; }
		virtual string table() const { return m_table; }
	protected:
		string IdClause(mgmySql &db,string what,string::size_type start=0,string::size_type len=string::npos) const;
		void AddIdClause(mgmySql &db,mgParts &result,string what) const;
		string m_id;
		string m_field;
	private:
		mgKeyTypes m_kt;
		string m_table;
		string m_value;
};

class mgKeyDate : public mgKeyNormal {
	public:
		mgKeyDate(mgKeyTypes kt,string table, string field) : mgKeyNormal(kt,table,field) {}
};

class mgKeyTrack : public mgKeyNormal {
	public:
		mgKeyTrack() : mgKeyNormal(keyTrack,"tracks","tracknb") {};
		mgParts Parts(mgmySql &db,bool orderby=false) const;
};

class mgKeyAlbum : public mgKeyNormal {
	public:
		mgKeyAlbum() : mgKeyNormal(keyAlbum,"album","title") {};
		mgParts Parts(mgmySql &db,bool orderby=false) const;
};

mgParts
mgKeyAlbum::Parts(mgmySql &db,bool orderby) const
{
	mgParts result = mgKeyNormal::Parts(db,orderby);
	result.tables.push_back("tracks");
	return result;
}

class mgKeyFolder : public mgKeyNormal {
	public:
		mgKeyFolder(mgKeyTypes kt,const char *fname)
			: mgKeyNormal(kt,"tracks",fname) { m_enabled=-1;};
		bool Enabled(mgmySql &db);
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
mgKeyFolder::Enabled(mgmySql &db)
{
    if (m_enabled<0)
    {
    	if (!db.Connected()) 
		return false;
    	char *b;
    	asprintf(&b,"DESCRIBE tracks %s",m_field.c_str());
    	MYSQL_RES * rows = db.exec_sql (b);
    	free(b);
    	if (rows)
	{
		m_enabled = mysql_num_rows(rows);
    		mysql_free_result (rows);
	}
    }
    return (m_enabled==1);
}

class mgKeyGenres : public mgKeyNormal {
	public:
		mgKeyGenres() : mgKeyNormal(keyGenres,"tracks","genre1") {};
		mgKeyGenres(mgKeyTypes kt) : mgKeyNormal(kt,"tracks","genre1") {};
		mgParts Parts(mgmySql &db,bool orderby=false) const;
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "genre"; }
		string map_valuetable() const { return "genre"; }
	protected:
		virtual unsigned int genrelevel() const { return 4; }
	private:
		string GenreClauses(mgmySql &db,bool orderby) const;
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
mgKeyGenres::GenreClauses(mgmySql &db,bool orderby) const
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

	if (id() != EMPTY)
	{
		unsigned int len=genrelevel();
		if (len==4) len=0;
      		g1.push_back(IdClause(db,"tracks.genre1",0,genrelevel()));
      		g2.push_back(IdClause(db,"tracks.genre2",0,genrelevel()));
	}

	extern bool needGenre2;
	if (needGenre2)
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
mgKeyGenres::Parts(mgmySql &db,bool orderby) const
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
		mgParts Parts(mgmySql &db,bool orderby=false) const;
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "language"; }
		string map_valuetable() const { return "language"; }
};

class mgKeyCollection: public mgKeyNormal {
	public:
  	  mgKeyCollection() : mgKeyNormal(keyCollection,"playlist","id") {};
	  mgParts Parts(mgmySql &db,bool orderby=false) const;
	 string map_idfield() const { return "id"; }
	 string map_valuefield() const { return "title"; }
	 string map_valuetable() const { return "playlist"; }
};
class mgKeyCollectionItem : public mgKeyNormal {
	public:
		mgKeyCollectionItem() : mgKeyNormal(keyCollectionItem,"playlistitem","tracknumber") {};
		mgParts Parts(mgmySql &db,bool orderby=false) const;
};

class mgKeyDecade : public mgKeyNormal {
	public:
		mgKeyDecade() : mgKeyNormal(keyDecade,"tracks","year") {}
		string expr() const { return "substring(convert(10 * floor(tracks.year/10), char),3)"; }
};

string
mgKeyNormal::id() const
{
	return m_id;
}

string
mgKeyNormal::value() const
{
	return m_value;
}


mgKeyNormal::mgKeyNormal(const mgKeyNormal& k)
{
	m_kt = k.m_kt;
	m_table = k.m_table;
	m_field = k.m_field;
	m_id = k.m_id;
}

mgKeyNormal::mgKeyNormal(const mgKeyTypes kt, string table, string field)
{
	m_kt = kt;
	m_table = table;
	m_field = field;
	m_id = EMPTY;
}

void
mgKeyNormal::set(string value, string id)
{
	m_value = value;
	m_id = id;
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
mgKeyNormal::Parts(mgmySql &db, bool orderby) const
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
mgKeyNormal::IdClause(mgmySql &db,string what,string::size_type start,string::size_type len) const
{
	if (len==0)
		len=string::npos;
       	if (id() == "'NULL'")
		return what + " is NULL";
       	else if (len==string::npos)
		return what + "=" + db.sql_string(id());
	else
	{
		return "substring("+what + ","+ltos(start+1)+","+ltos(len)+")="
			+ db.sql_string(id().substr(start,len));
	}
}

void
mgKeyNormal::AddIdClause(mgmySql &db,mgParts &result,string what) const
{
	if (id() != EMPTY)
       		result.clauses.push_back(IdClause(db,what));
}

mgParts
mgKeyTrack::Parts(mgmySql &db,bool orderby) const
{
	mgParts result;
	result.tables.push_back("tracks");
	AddIdClause(db,result,"tracks.title");
	if (orderby)
	{
		// if you change tracks.title, please also
		// change mgContentItem::getKeyValue()
		result.fields.push_back("tracks.title");
       		result.orders.push_back("tracks.tracknb");
	}
	return result;
}

mgParts
mgKeyLanguage::Parts(mgmySql &db,bool orderby) const
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
mgKeyCollection::Parts(mgmySql &db,bool orderby) const
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
mgKeyCollectionItem::Parts(mgmySql &db,bool orderby) const
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

mgRefParts::mgRefParts(const mgReference& r)
{
	tables.push_back(r.t1());
	tables.push_back(r.t2());
	clauses.push_back(r.t1() + '.' + r.f1() + '=' + r.t2() + '.' + r.f2());
}

void
mgParts::Prepare()
{
	tables.sort();
	tables.unique();
	strlist::reverse_iterator it;
	string prevtable = "";
	for (it = tables.rbegin(); it != tables.rend(); ++it)
	{
		if (!prevtable.empty())
			*this += ref.Connect(prevtable,*it);
		prevtable = *it;
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
	optimize(result);
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
	optimize(result);
	return result;
}

bool
mgParts::UsesTracks()
{
	for (list < string >::iterator it = tables.begin (); it != tables.end (); ++it)
		if (*it == "tracks") return true;
	return false;
}

string
mgParts::sql_delete_from_collection(string pid)
{
	if (pid.empty())
		return "";
	Prepare();
	// del nach vorne, weil DELETE playlistitem die erste Table nimmt,
	// die passt, egal ob alias oder nicht.
	tables.push_front("playlistitem as del");
	clauses.push_back("del.playlist="+pid);
	// todo geht so nicht fuer andere selections
	if (UsesTracks())
		clauses.push_back("del.trackid=tracks.id");
	else
		clauses.push_back("del.trackid=playlistitem.trackid");
	string result = "DELETE playlistitem";
	result += sql_list(" FROM",tables);
	result += sql_list(" WHERE",clauses," AND ");
	optimize(result);
	return result;
}

string
mgParts::sql_update(strlist new_values)
{
	Prepare();
	assert(fields.size()==new_values.size());
	string result = sql_list("UPDATE",fields);
	result += sql_list(" FROM",tables);
	result += sql_list(" WHERE",clauses," AND ");
	result += sql_list("VALUES(",new_values,",",")");
	optimize(result);
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
		k->set(from.getKeyValue(i),from.getKeyId(i));
		Keys.push_back(k);
    	}
	m_orderByCount=from.m_orderByCount;
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

string
mgOrder::getKeyValue(unsigned int idx) const
{
	assert(idx<Keys.size());
	return Keys[idx]->value();
}

string
mgOrder::getKeyId(unsigned int idx) const
{
	assert(idx<Keys.size());
	return Keys[idx]->id();
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
mgOrder::Parts(mgmySql &db,unsigned int level,bool orderby) const
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
					if (iskeyGenre(knt) 
						&& knt>kt && kn->id()!=EMPTY)
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

//! \brief right now thread locking should not be needed here
mgReferences::mgReferences()
{
	push_back(mgReference ("tracks","id","playlistitem","trackid"));
	push_back(mgReference ("playlist","id","playlistitem","playlist"));
	push_back(mgReference ("tracks","sourceid","album","cddbid"));
	push_back(mgReference ("tracks","lang","language","id"));
}

bool
mgReferences::Equal(unsigned int i,string table1, string table2) const
{
	const mgReference& r = operator[](i);
	string s1 = r.t1();
	string s2 = r.t2();
	return ((s1==table1) && (s2==table2))
	   || ((s1==table2) && (s2==table1));
}

mgParts
mgReferences::FindConnectionBetween(string table1, string table2) const
{
	for (unsigned int i=0 ; i<size(); i++ )
		if (Equal(i,table1,table2)) 
			return mgRefParts(operator[](i));
	return mgParts();
}

mgParts
mgReferences::ConnectToTracks(string table) const
{
	mgParts result;
	if (table=="tracks")
		return result;
	result += FindConnectionBetween(table,"tracks");
	if (result.empty())
	{
		result += FindConnectionBetween(table,"playlistitem");
		if (!result.empty()) 
		{
			result += FindConnectionBetween("playlistitem","tracks");
		}
		else
			assert(false);
	}
	return result;
}

mgParts
mgReferences::Connect(string table1, string table2) const
{
	mgParts result;
	// same table?
	if (table1 == table2) return result;
	if (table1=="genre") return ConnectToTracks(table2);
	if (table2=="genre") return ConnectToTracks(table1);
	// do not connect aliases. See sql_delete_from_collection
	if (table1.find(" as ")!=string::npos) return result;
	if (table2.find(" as ")!=string::npos) return result;
	if (table1.find(" AS ")!=string::npos) return result;
	if (table2.find(" AS ")!=string::npos) return result;
	// direct connection?
	result += FindConnectionBetween(table1,table2);
	if (result.empty())
	{
		// indirect connection? try connecting via tracks
		result +=  ConnectToTracks(table1);
		result +=  ConnectToTracks(table2);
	}
	return result;
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
		case keyTitle: result = new mgKeyNormal(kt,"tracks","title");break;
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
		case keyTitle: result = "Title";break;
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


vector<const char*>
ktNames()
{
	static vector<const char*> result;
	for (unsigned int i = int(mgKeyTypesLow); i <= int(mgKeyTypesHigh); i++)
		result.push_back(ktName(mgKeyTypes(i)));
	return result;
}

