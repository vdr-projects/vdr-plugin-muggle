#include "mg_order.h"
#include "mg_tools.h"
#include "i18n.h"

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

//! \brief adds n1=n2 to string s, using AND to separate several such items
static string
undequal (string & s, string n1, string op, string n2)
{
	if (n1.compare (n2) || op != "=")
		return addsep (s, " AND ", n1 + op + n2);
	else
		return s;
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


class mgKeyNormal : public mgKey {
	public:
		mgKeyNormal(const mgKeyNormal& k);
		mgKeyNormal(const mgKeyTypes kt, string table, string field);
		virtual mgParts Parts(bool orderby=false) const;
		string value() const;
		string id() const;
		void set(string value,string id);
		mgKeyTypes Type() const { return m_kt; }
		virtual string expr() const { return field(); }
		virtual string field() const { return m_table + "." + m_field; }
		virtual string table() const { return m_table; }
	protected:
		virtual string orderfield() const { return expr(); }
		void AddIdClause(mgParts &result,string what) const;
	private:
		mgKeyTypes m_kt;
		string m_field;
		string m_table;
		string m_value;
		string m_id;
};

class mgKeyTrack : public mgKeyNormal {
	public:
		mgKeyTrack() : mgKeyNormal(keyTrack,"tracks","tracknb") {};
		mgParts Parts(bool orderby=false) const;
};

class mgKeyGenre1 : public mgKeyNormal {
	public:
		mgKeyGenre1() : mgKeyNormal(keyGenre1,"tracks","genre1") {};
		mgParts Parts(bool orderby=false) const;
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "genre"; }
		string map_valuetable() const { return "genre"; }
};

class mgKeyGenre2 : public mgKeyNormal {
	public:
		mgKeyGenre2() : mgKeyNormal(keyGenre2,"tracks","genre2") {};
		mgParts Parts(bool orderby=false) const;
		string map_idfield() const { return "id"; }
		string map_valuefield() const { return "genre"; }
		string map_valuetable() const { return "genre"; }
};

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
       		result.orders.push_back(orderfield());
	}
	return result;
}

void
mgKeyNormal::AddIdClause(mgParts &result,string what) const
{
	assert(strlen(m_db->host));
	if (id() != EMPTY)
	{
       		string op;
       		string xid;
       		if (id() == "'NULL'")
       		{
	       		op = "is";
	       		xid = "NULL";
       		}
       		else
       		{
	       		op = "=";
	       		xid = sql_string(m_db,id());
       		}
       		string clause = "";
       		result.clauses.push_back(undequal(clause,what,op,xid));
	}
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
mgKeyGenre1::Parts(bool orderby) const
{
	mgParts result;
	AddIdClause(result,"tracks.genre1");
	result.tables.push_back("tracks");
	if (orderby)
	{
		result.fields.push_back("genre.genre");
		result.fields.push_back("tracks.genre1");
		result.tables.push_back("genre");
       		result.orders.push_back("genre.genre");
	}
	return result;
}

mgParts
mgKeyGenre2::Parts(bool orderby) const
{
	mgParts result;
	AddIdClause(result,"tracks.genre2");
	result.tables.push_back("tracks");
	if (orderby)
	{
		result.fields.push_back("genre.genre");
		result.fields.push_back("tracks.genre2");
		result.tables.push_back("genre");
       		result.orders.push_back("genre.genre");
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
sql_list (string prefix,list < string > v,string sep=",",string postfix="")
{
	string result = "";
	for (list < string >::iterator it = v.begin (); it != v.end (); it++)
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
	for (it = tables.rbegin(); it != tables.rend(); it++)
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
	for (list < string >::iterator it = tables.begin (); it != tables.end (); it++)
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

const mgOrder&
mgOrder::operator=(const mgOrder& from)
{
	Name = from.Name;
	Keys.clear();
    	for (unsigned int i = 0; i < from.size();i++)
    	{
        	mgKey *k = ktGenerate(from.getKeyType(i),m_db);
		k->set(from.getKeyValue(i),from.getKeyId(i));
		Keys.push_back(k);
    	}
	setDB(from.m_db);
	return *this;
}

mgOrder&
mgOrder::operator+=(mgKey* k) {
	k->setdb(m_db);
	Keys.push_back(k);
	return *this;
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
	for (i = Keys.begin () ; i != Keys.end (); i++)
	{
		(*i)->setdb(db);
	}
}

mgKey*
mgOrder::find(const mgKeyTypes kt) 
{
	keyvector::iterator i;
	for (i = Keys.begin () ; i != Keys.end (); i++)
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
	for (i = Keys.begin () ; i != Keys.end (); i++)
	{
		mgKeyNormal* k = dynamic_cast<mgKeyNormal*>(*i);
		album_found |= (k->Type()==keyAlbum);
		tracknb_found |= (k->Type()==keyTrack);
		title_found |= (k->Type()==keyTitle);
		if (tracknb_found || (album_found && title_found))
		{
			for (j = i+1 ; j !=Keys.end(); j++)
				delete *j;
			Keys.erase(i+1,Keys.end ());
			break;
		}
		for (j = i+1 ; j != Keys.end(); j++)
			if (*i == *j) {
				delete *j;
				Keys.erase(j);
			}
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
		result += k->Parts(orderby && (i==level));
	}
	return result;
}

mgReferences::mgReferences()
{
	push_back(mgReference ("tracks","id","playlistitem","trackid"));
	push_back(mgReference ("playlist","id","playlistitem","playlist"));
	push_back(mgReference ("tracks","sourceid","album","cddbid"));
	push_back(mgReference ("tracks","genre1","genre","id"));
	push_back(mgReference ("tracks","genre2","genre","id"));
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
		case keyGenre1:	result = new mgKeyGenre1;break;
		case keyGenre2:	result = new mgKeyGenre2;break;
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
		case keyGenre1: result = "Genre";break;
		case keyGenre2: result = "Genre 2";break;
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

