#ifndef _MG_SQL_H
#define _MG_SQL_H
#include <stdlib.h>
#include <mysql/mysql.h>
#include <typeinfo>
#include <string>
#include <assert.h>
#include <list>
#include <vector>
#include <iostream>
#include <istream>
#include <sstream>
#include <ostream>

using namespace std;

typedef list<string> strlist;

strlist& operator+=(strlist&a, strlist b);

static const string EMPTY = "XNICHTGESETZTX";

//! \brief adds string n to string s, using string sep to separate them
string& addsep (string & s, string sep, string n);

enum mgKeyTypes {
	keyGenre1 = 1,
	keyGenre2,
	keyArtist,
	keyTitle,
	keyTrack,
	keyDecade,
	keyCollection,
	keyCollectionItem,
	keyAlbum,
	keyLanguage,
	keyRating,
	keyYear,
};
const mgKeyTypes mgKeyTypesHigh = keyYear;

class mgParts;

class mgReference {
	public:
		mgReference(string t1,string f1,string t2,string f2);
                string t1() const { return m_t1; }
                string t2() const { return m_t2; }
                string f1() const { return m_f1; }
                string f2() const { return m_f2; }
	private:
		string m_t1;
		string m_t2;
		string m_f1;
		string m_f2;
};

class mgReferences : public vector<mgReference> {
public:
	// \todo memory leak for vector ref?
	mgReferences();
	mgParts Connect(string c1, string c2) const;
private:
	bool Equal(unsigned int i,string table1, string table2) const;
	mgParts FindConnectionBetween(string table1, string table2) const;
	mgParts ConnectToTracks(string table) const;
};

class mgKey {
	public:
		mgKey();
		virtual ~mgKey();
		virtual mgParts Parts(bool orderby=false) const = 0;
		virtual string id() const = 0;
		virtual string value () const = 0;
		//!\brief translate field into user friendly string
		virtual void set(string value, string id) = 0;
		virtual mgKeyTypes Type() const = 0;
		virtual string map_idfield() const { return ""; }
		virtual string map_valuefield() const { return ""; }
		virtual string map_valuetable() const { return ""; }
		void setdb(MYSQL *db) { m_db = db; }
	protected:
		MYSQL *m_db;
};


mgKey*
ktGenerate(const mgKeyTypes kt,MYSQL *db);

const char * const
ktName(const mgKeyTypes kt);

typedef vector<mgKey*> keyvector;

class mgParts {
public:
	mgParts();
	~mgParts();
	strlist fields;
	strlist tables;
	strlist clauses;
	strlist orders;
	mgParts& operator+=(mgParts a);
	void Prepare();
	string sql_count();
	string sql_select(bool distinct=true);
	string sql_delete_from_collection(string pid);
	string sql_update(strlist new_values);
	bool empty() const { return tables.size()==0;}
private:
	bool UsesTracks();
	mgReferences ref;
};

string
sql_string (MYSQL *db, const string s);

MYSQL_RES * exec_sql (MYSQL *db,string query);

//! \brief converts long to string
string itos (int i);

//! \brief convert long to string
string ltos (long l);


const unsigned int MaxKeys = 20;

class mgOrder {
public:
	mgOrder();
	void setDB(MYSQL *db);
	mgParts Parts(unsigned int level,bool orderby=true) const;
	string Name;
	const mgOrder& operator=(const mgOrder& from);
	mgOrder& operator+=(mgKey* k);
	mgKey*& operator[](unsigned int idx);
	unsigned int size() const { return Keys.size(); }
	void truncate(unsigned int i);
	bool empty() const { return Keys.empty(); }
	void clear() { Keys.clear();}
	void clean();
	mgKey* Key(unsigned int idx) const;
	mgKey* find(const mgKeyTypes kt) ;
	mgKeyTypes getKeyType(unsigned int idx) const;
	string getKeyValue(unsigned int idx) const;
	string getKeyId(unsigned int idx) const;
private:
	MYSQL *m_db;
	keyvector Keys;
};

#endif               // _MG_SQL_H
