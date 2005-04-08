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
#include "mg_content.h"
#include "mg_tools.h"

using namespace std;

typedef list<string> strlist;

strlist& operator+=(strlist&a, strlist b);

//! \brief adds string n to string s, using string sep to separate them
string& addsep (string & s, string sep, string n);

bool iskeyGenre(mgKeyTypes kt);

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
		virtual ~mgKey() {};
		virtual mgParts Parts(mgmySql &db,bool orderby=false) const = 0;
		virtual string id() const = 0;
		virtual bool valid() const = 0;
		virtual string value () const = 0;
		//!\brief translate field into user friendly string
		virtual void set(mgListItem& item) = 0;
		virtual mgListItem& get() = 0;
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
	mgListItem& getKeyItem(unsigned int idx) const;
	void setKeys(vector<mgKeyTypes> kt);
	string Name();
	void setOrderByCount(bool orderbycount) { m_orderByCount = orderbycount;}
	bool getOrderByCount() { return m_orderByCount; }
	string GetContent(mgmySql &db,unsigned int level,vector < mgContentItem > &content) const;
private:
	bool m_orderByCount;
	bool isCollectionOrder() const;
	keyvector Keys;
	void setKey ( const mgKeyTypes kt);
	void clean();
};

bool operator==(const mgOrder& a,const mgOrder&b); //! \brief compares only the order, not the current key values

#endif               // _MG_SQL_H
