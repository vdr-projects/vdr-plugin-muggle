#include "mg_order.h"
#include "mg_tools.h"
#include "i18n.h"


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


string
sql_string (MYSQL *db, const string s) 
{
	if (!db)
		return "";
	char *buf = (char *) malloc (s.size () * 2 + 1);
	mysql_real_escape_string (db, buf, s.c_str (), s.size ());
	string result = "'" + std::string (buf) + "'";
	free (buf);
	return result;
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

MYSQL_RES *
exec_sql (MYSQL *db,string query)
{
	if (!db)
		return 0;
	if (query.empty())
		return 0;
	mgDebug(3,"exec_sql(%X,%s)",db,query.c_str());
	if (mysql_query (db, (query + ';').c_str ()))
	{
	mgError("SQL Error in %s: %s",query.c_str(),mysql_error (db));
    		std::cout<<"ERROR in " << query << ":" << mysql_error(db)<<std::endl;
		return 0;
	}
	return mysql_store_result (db);
}

string
get_col0(MYSQL *db, string query)
{
    MYSQL_RES * sql_result = exec_sql (db, query);
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

int
exec_count(MYSQL *db, string query)
{
    return atol (get_col0 (db, query).c_str ());
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
		virtual mgParts Parts(bool orderby=false) const;
		string value() const;
		string id() const;
		void set(string value,string id);
		mgKeyTypes Type() const { return m_kt; }
		virtual string expr() const { return m_table + "." + m_field; }
		virtual string table() const { return m_table; }
	protected:
		string IdClause(string what,string::size_type start=0,string::size_type len=string::npos) const;
		void AddIdClause(mgParts &result,string what) const;
		string m_id;
	private:
		mgKeyTypes m_kt;
		string m_field;
		string m_table;
		string m_value;
};

class mgKeyTrack : public mgKeyNormal {
	public:
		mgKeyTrack() : mgKeyNormal(keyTrack,"tracks","tracknb") {};
		mgParts Parts(bool orderby=false) const;
};

class mgKeyGenres : public mgKeyNormal {
	public:
		mgKeyGenres() : mgKeyNormal(keyGenres,"tracks","genre1") {};
		mgKeyGenres(mgKeyTypes kt) : mgKeyNormal(kt,"tracks","genre1") {};
		mgParts Parts(bool orderby=false) const;
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "genre"; }
		string map_valuetable() const { return "genre"; }
		string GenreClauses(bool orderby) const;
		virtual unsigned int genrelevel() const { return 4; }
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
mgKeyGenres::GenreClauses(bool orderby) const
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
      		g1.push_back(IdClause("tracks.genre1",0,genrelevel()));
      		g2.push_back(IdClause("tracks.genre2",0,genrelevel()));
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
mgKeyGenres::Parts(bool orderby) const
{
	mgParts result;
   	result.clauses.push_back(GenreClauses(orderby));
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
		mgParts Parts(bool orderby=false) const;
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "language"; }
		string map_valuetable() const { return "language"; }
};

class mgKeyCollection: public mgKeyNormal {
	public:
  	  mgKeyCollection() : mgKeyNormal(keyCollection,"playlist","id") {};
	  mgParts Parts(bool orderby=false) const;
	 string map_idfield() const { return "id"; }
	 string map_valuefield() const { return "title"; }
	 string map_valuetable() const { return "playlist"; }
};
class mgKeyCollectionItem : public mgKeyNormal {
	public:
		mgKeyCollectionItem() : mgKeyNormal(keyCollectionItem,"playlistitem","tracknumber") {};
		mgParts Parts(bool orderby=false) const;
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


mgKey::mgKey()
{
	m_db = 0;
}

mgKey::~mgKey()
{
}

void
mgKey::setdb(MYSQL *db)
{
	m_db = db;
}

mgKeyNormal::mgKeyNormal(const mgKeyNormal& k)
{
	m_kt = k.m_kt;
	m_table = k.m_table;
	m_field = k.m_field;
	m_id = k.m_id;
	m_db = k.m_db;
}

mgKeyNormal::mgKeyNormal(const mgKeyTypes kt, string table, string field)
{
	m_kt = kt;
	m_table = table;
	m_field = field;
	m_id = EMPTY;
	m_db = 0;
}

void
mgKeyNormal::set(string value, string id)
{
	m_value = value;
	m_id = id;
}

mgParts::mgParts()
{
}

mgParts::~mgParts()
{
}

mgParts
mgKeyNormal::Parts(bool orderby) const
{
	assert(strlen(m_db->host));
	mgParts result;
	result.tables.push_back(table());
	AddIdClause(result,expr());
	if (orderby)
	{
		result.fields.push_back(expr());
       		result.orders.push_back(expr());
	}
	return result;
}

string
mgKeyNormal::IdClause(string what,string::size_type start,string::size_type len) const
{
	assert(strlen(m_db->host));
	if (len==0)
		len=string::npos;
       	if (id() == "'NULL'")
		return what + " is NULL";
       	else if (len==string::npos)
		return what + "=" + sql_string(m_db,id());
	else
	{
		return "substring("+what + ","+ltos(start+1)+","+ltos(len)+")="
			+ sql_string(m_db,id().substr(start,len));
	}
}

void
mgKeyNormal::AddIdClause(mgParts &result,string what) const
{
	if (id() != EMPTY)
       		result.clauses.push_back(IdClause(what));
}

mgParts
mgKeyTrack::Parts(bool orderby) const
{
	mgParts result;
	result.tables.push_back("tracks");
	AddIdClause(result,"tracks.title");
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
mgKeyLanguage::Parts(bool orderby) const
{
	mgParts result;
	AddIdClause(result,"tracks.lang");
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
mgKeyCollection::Parts(bool orderby) const
{
	mgParts result;
	if (orderby)
	{
		result.tables.push_back("playlist");
		AddIdClause(result,"playlist.id");
		result.fields.push_back("playlist.title");
		result.fields.push_back("playlist.id");
       		result.orders.push_back("playlist.title");
	}
	else
	{
		result.tables.push_back("playlistitem");
		AddIdClause(result,"playlistitem.playlist");
	}
	return result;
}

mgParts
mgKeyCollectionItem::Parts(bool orderby) const
{
	assert(strlen(m_db->host));
	mgParts result;
	result.tables.push_back("playlistitem");
	AddIdClause(result,"playlistitem.tracknumber");
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
	Prepare();
	string result = "";
	if (distinct)
		result += sql_list("SELECT DISTINCT",fields);
	else
		result += sql_list("SELECT",fields);
	if (result.empty())
		return result;
	result += sql_list("FROM",tables);
	result += sql_list("WHERE",clauses," AND ");
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
	setDB(0);
       	setKey (0,keyArtist);
       	setKey (1,keyAlbum);
       	setKey (2,keyTrack);
}

mgKey*
mgOrder::Key(unsigned int idx) const
{
	return Keys[idx];
}

mgKey*&
mgOrder::operator[](unsigned int idx) 
{
	assert(idx<Keys.size());
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
	Keys.clear();
    	for (unsigned int i = 0; i < from.size();i++)
    	{
        	mgKey *k = ktGenerate(from.getKeyType(i),m_db);
		k->set(from.getKeyValue(i),from.getKeyId(i));
		Keys.push_back(k);
    	}
	if (from.m_db) setDB(from.m_db);
	return *this;
}

mgOrder&
mgOrder::operator+=(mgKey* k) {
	k->setdb(m_db);
	Keys.push_back(k);
	return *this;
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
mgOrder::setKey (const unsigned int level, const mgKeyTypes kt)
{
    mgKey *newkey = ktGenerate(kt,m_db);
    if (level == 0 && kt == keyCollection)
    {
        clear ();
	Keys.push_back(newkey);
	Keys.push_back(ktGenerate(keyCollectionItem,m_db));
        return;
    }
    if (level == size ())
    {
        Keys.push_back(newkey);
    }
    else
    {
	if (level >= Keys.size())
	  mgError("mgOrder::setKey(%u,%s): level greater than size() %u",
	      level,ktName(kt),Keys.size());
        delete Keys[level];
	Keys[level] = newkey;
    }

// clear values for this and following levels (needed for copy constructor)
    for (unsigned int i = level; i < Keys.size (); i++)
        Keys[i]->set ("",EMPTY);
}

mgOrder::mgOrder(mgValmap& nv,char *prefix)
{
	setDB(0);
	for (unsigned int i = 0; i < 999 ; i++)
	{
		char *idx;
		asprintf(&idx,"%s.Keys.%u.Type",prefix,i);
		unsigned int v = nv.getuint(idx);
		free(idx);
		if (v==0) break;
        	setKey (i,mgKeyTypes(v) );
	}
}

mgOrder::mgOrder(vector<mgKeyTypes> kt)
{
	setDB(0);
	setKeys(kt);
}

void
mgOrder::setKeys(vector<mgKeyTypes> kt)
{
	clear();
	for (unsigned int i=0;i<kt.size();i++)
		setKey(size(),kt[i]);
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
mgOrder::setDB(MYSQL *db)
{
	m_db = db;
	keyvector::iterator i;
	for (i = Keys.begin () ; i != Keys.end (); ++i)
	{
		(*i)->setdb(db);
	}
}

mgKey*
mgOrder::find(const mgKeyTypes kt) 
{
	keyvector::iterator i;
	for (i = Keys.begin () ; i != Keys.end (); ++i)
	{
		if ((*i)->Type() == kt)
			return *i;
	}
	return 0;
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
mgOrder::clean()
{
	// remove double entries:
	keyvector::iterator i;
	keyvector::iterator j;
	bool album_found = false;	
	bool tracknb_found = false;	
	bool title_found = false;	
	bool is_unique = false;
	for (i = Keys.begin () ; i != Keys.end (); ++i)
	{
		mgKeyNormal* k = dynamic_cast<mgKeyNormal*>(*i);
		album_found |= (k->Type()==keyAlbum);
		tracknb_found |= (k->Type()==keyTrack);
		title_found |= (k->Type()==keyTitle);
		is_unique = tracknb_found || (album_found && title_found);
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
			Keys.push_back(ktGenerate(keyAlbum,m_db));
		if (!title_found)
			Keys.push_back(ktGenerate(keyTitle,m_db));
	}
}


mgParts
mgOrder::Parts(unsigned int level,bool orderby) const
{
	assert(strlen(m_db->host));
	mgParts result;
	for (unsigned int i=0;i<=level;i++)
	{
		if (i==Keys.size()) break;
		mgKeyNormal *k = dynamic_cast<mgKeyNormal*>(Keys[i]);
		k->setdb(m_db);
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
		result += k->Parts(orderby && (i==level));
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
	return (((at(i).t1()==table1) && (at(i).t2()==table2))
	   || ((at(i).t1()==table2) && (at(i).t2()==table1)));
}

mgParts
mgReferences::FindConnectionBetween(string table1, string table2) const
{
	for (unsigned int i=0 ; i<size(); i++ )
		if (Equal(i,table1,table2)) 
			return mgRefParts(at(i));
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
ktGenerate(const mgKeyTypes kt,MYSQL* db)
{
	mgKey* result = 0;
	switch (kt)
	{
		case keyGenres:	result = new mgKeyGenres;break;
		case keyGenre1:	result = new mgKeyGenre1;break;
		case keyGenre2:	result = new mgKeyGenre2;break;
		case keyGenre3:	result = new mgKeyGenre3;break;
		case keyArtist: result = new mgKeyNormal(kt,"tracks","artist");break;
		case keyTitle: result = new mgKeyNormal(kt,"tracks","title");break;
		case keyTrack: result = new mgKeyTrack;break;
		case keyDecade: result = new mgKeyDecade;break;
		case keyAlbum: result = new mgKeyNormal(kt,"album","title");break;
		case keyCollection: result = new mgKeyCollection;break;
		case keyCollectionItem: result = new mgKeyCollectionItem;break;
		case keyLanguage: result = new mgKeyLanguage;break;
		case keyRating: result = new mgKeyNormal(kt,"tracks","rating");break;
		case keyYear: result = new mgKeyNormal(kt,"tracks","year");break;
	}
	if (result) result->setdb(db);
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
		case keyArtist: result = "Artist";break;
		case keyTitle: result = "Title";break;
		case keyTrack: result = "Track";break;
		case keyDecade: result = "Decade";break;
		case keyAlbum: result = "Album";break;
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
	for (int kt=int(mgKeyTypesLow);kt<int(mgKeyTypesHigh);kt++)
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

