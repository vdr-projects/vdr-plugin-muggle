#ifndef _MG_SQL_H
#define _MG_SQL_H
#include <stdlib.h>
#include <typeinfo>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include "mg_valmap.h"
#include "mg_mysql.h"

using namespace std;

typedef list<string> strlist;

strlist& operator+=(strlist&a, strlist b);

//! \brief adds string n to string s, using string sep to separate them
string& addsep (string & s, string sep, string n);

//! \brief all key types
enum mgKeyTypes {
	keyGenre1=1, // the genre types must have exactly this order!
	keyGenre2,
	keyGenre3,
	keyGenres,
	keyDecade,
	keyYear,
	keyArtist,
	keyAlbum,
	keyTitle,
	keyTrack,
	keyLanguage,
	keyRating,
	keyFolder1,
	keyFolder2,
	keyFolder3,
	keyFolder4,
	keyCreated,
	keyModified,
	keyArtistABC,
	keyTitleABC,
	keyCollection,
	keyCollectionItem,
};
const mgKeyTypes mgKeyTypesLow = keyGenre1;
const mgKeyTypes mgKeyTypesHigh = keyCollectionItem;
const unsigned int mgKeyTypesNr = keyCollectionItem;

//! \brief returns true if kt is one of the genre key types
bool iskeyGenre(mgKeyTypes kt);

class mgParts;

//! \brief defines a foreign key
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

//! \brief a list of all foreign keys
class mgReferences : public vector<mgReference> {
public:
	//! \todo memory leak for vector ref?
	mgReferences();
	//! \brief returns the code needed to add a foreign key to the SQL statement
	mgParts Connect(string c1, string c2) const;
private:
	bool Equal(unsigned int i,string table1, string table2) const;
	mgParts FindConnectionBetween(string table1, string table2) const;
	mgParts ConnectToTracks(string table) const;
};

class mgSelItem
{
	public:
		//! \brief constructs an invalid item
		mgSelItem();
		//! \brief constructs a valid item
		mgSelItem(string v,string i,unsigned int c=0);
		//! \brief sets the item values making the item valid
		void set(string v,string i,unsigned int c=0);
		//! \brief assignment operator
		void operator=(const mgSelItem& from);
		//! \brief assignment operator
		void operator=(const mgSelItem* from);
		//! \brief equal operator
		bool operator==(const mgSelItem& other) const;
		//! \brief the value of the item (user friendly)
		string value() const { return m_value; } 
		//! \brief the id of the item (computer friendly)
		string id() const { return m_id; } 
		//! \brief the number of tracks associated with this item
		unsigned int count() const { return m_count; } 
		//! \brief true if the item is valid/set
		bool valid() const { return m_valid; }
	private:
		bool m_valid;
		string m_value;
		string m_id;
		unsigned int m_count;
};

class mgKey {
	public:
		virtual ~mgKey() {};
		virtual mgParts Parts(mgmySql &db,bool orderby=false) const = 0;
		virtual string id() const = 0;
		virtual bool valid() const = 0;
		virtual string value () const = 0;
		//!\brief translate field into user friendly string
		virtual void set(mgSelItem& item) = 0;
		virtual mgSelItem& get() = 0;
		virtual mgKeyTypes Type() const = 0;
		virtual string map_idfield() const { return ""; }
		virtual string map_valuefield() const { return ""; }
		virtual string map_valuetable() const { return ""; }
		virtual bool Enabled(mgmySql &db) { return true; }
};


mgKey*
ktGenerate(const mgKeyTypes kt);

const char * const ktName(const mgKeyTypes kt);
mgKeyTypes ktValue(const char * name);
vector < const char*> ktNames();
 
typedef vector<mgKey*> keyvector;

class mgParts {
public:
	mgParts();
	~mgParts();
	strlist fields;
	strlist tables;
	strlist clauses;
	strlist groupby;
	strlist orders;
	mgParts& operator+=(mgParts a);
	void Prepare();
	string sql_count();
	string sql_select(bool distinct=true);
	string sql_delete_from_collection(string pid);
	string sql_update(strlist new_values);
	bool empty() const { return tables.size()==0;}
	string m_sql_select;
	bool orderByCount;
private:
	bool UsesTracks();
	mgReferences ref;
};

//! \brief converts long to string
string itos (int i);

//! \brief convert long to string
string ltos (long l);


const unsigned int MaxKeys = 20;

class mgOrder {
public:
	mgOrder();
	mgOrder(const mgOrder &from);
	mgOrder(mgValmap& nv, char *prefix);
	mgOrder(vector<mgKeyTypes> kt);
	~mgOrder();
	void InitFrom(const mgOrder &from);
        void DumpState(mgValmap& nv, char *prefix) const;
	mgParts Parts(mgmySql &db,const unsigned int level,bool orderby=true) const;
	const mgOrder& operator=(const mgOrder& from);
	mgKey*& operator[](unsigned int idx);
	unsigned int size() const { return Keys.size(); }
	void truncate(unsigned int i);
	bool empty() const { return Keys.empty(); }
	void clear();
	mgKey* Key(unsigned int idx) const;
	mgKeyTypes getKeyType(unsigned int idx) const;
	mgSelItem& getKeyItem(unsigned int idx) const;
	void setKeys(vector<mgKeyTypes> kt);
	string Name();
	void setOrderByCount(bool orderbycount) { m_orderByCount = orderbycount;}
	bool getOrderByCount() { return m_orderByCount; }
private:
	bool m_orderByCount;
	bool isCollectionOrder() const;
	keyvector Keys;
	void setKey ( const mgKeyTypes kt);
	void clean();
};

bool operator==(const mgOrder& a,const mgOrder&b); //! \brief compares only the order, not the current key values

extern mgSelItem zeroitem;

#endif               // _MG_SQL_H
