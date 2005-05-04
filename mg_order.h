#ifndef _MG_ORDER_H
#define _MG_ORDER_H
#include <stdlib.h>
#include <typeinfo>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <sstream>
using namespace std;
#include "mg_db.h"
#include "mg_item.h"
#include "mg_tools.h"

using namespace std;

typedef list<string> strlist;

strlist& operator+=(strlist&a, strlist b);


extern string sql_list (string prefix,strlist v,string sep=",",string postfix="");

bool iskeyGenre(mgKeyTypes kt);

class mgParts;
class mgDb;

class mgReference {
	public:
		mgReference(string t1,string f1,string t2,string f2);
                string t1() const { return m_t1; }
                string t2() const { return m_t2; }
                string f1() const { return m_f1; }
                string f2() const { return m_f2; }
		bool Equal(string table1, string table2) const;
	private:
		string m_t1;
		string m_t2;
		string m_f1;
		string m_f2;
};

class mgReferences : public vector<mgReference*> {
public:
	void InitReferences();
	unsigned int CountTable(string table) const;
};

class mgKeyMaps {
	public:
		string value(mgKeyTypes kt, string idstr) const;
		string id(mgKeyTypes kt, string valstr) const;
	private:
		bool loadvalues (mgKeyTypes kt) const;
};

extern mgKeyMaps KeyMaps;

class mgKey {
	public:
		virtual ~mgKey() {};
		virtual mgParts Parts(mgDb *db,bool orderby=false) const = 0;
		virtual string id() const = 0;
		virtual bool valid() const = 0;
		virtual string value () const = 0;
		//!\brief translate field into user friendly string
		virtual void set(mgListItem* item) = 0;
		virtual mgListItem* get() = 0;
		virtual mgKeyTypes Type() const = 0;
		virtual bool Enabled(mgDb *db) { return true; }
		virtual bool LoadMap() const;
	protected:
		virtual string map_sql() const { return ""; }
};


mgKey*
ktGenerate(const mgKeyTypes kt);

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
	bool empty() const { return tables.size()==0;}
	string special_statement;
	bool orderByCount;
private:
	mgReferences rest;
	mgReferences positives;
	void ConnectTables(string c1, string c2);
};

#endif
