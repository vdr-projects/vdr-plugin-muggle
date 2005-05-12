/*! 
 * \file   mg_db.h
 * \brief  A generic capsule around database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-04-13 17:42:54 +0100 (Thu, 13 Apr 2005) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wolfgang61 $
 */

#ifndef __MG_DB_H
#define __MG_DB_H

#include <sys/types.h>
#include <sys/time.h>
#include <string>
#include <list>
#include <vector>
#include <map>

using namespace std;

class mgItem;

#include "mg_listitem.h"
#include "mg_keytypes.h"

typedef list<string> strlist;

strlist& operator+=(strlist&a, strlist b);


string sql_list (string prefix,strlist v,string sep=",",string postfix="");

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


class mgParts {
public:
	mgParts();
	~mgParts();
	strlist valuefields; 	// if idfield and valuefield are identical, define idfield only
	strlist idfields;
	strlist tables;
	strlist clauses;
	strlist groupby;
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
	void push_table_to_front(string table);
};

/*!
 * \brief an abstract database class
 * 
 */
class mgDb {
   public:
	mgDb (bool SeparateThread=false);
	virtual ~mgDb ();
	/*! \brief executes a query and returns the integer value from
 	 * the first column in the first row. The query shold be a COUNT query
 	 * returning only one row.
 	 * \param query the SQL query to be executed
 	 */
	unsigned long exec_count(const string sql); 
  	virtual bool ServerConnect() { return true; }
  	virtual bool Connect() = 0;
  	bool HasFolderFields() const { return m_hasfolderfields;}
  	virtual bool Create() = 0;
	virtual void ServerEnd() =0;		// must be done explicitly
	virtual int AddToCollection( const string Name,const vector<mgItem*>&items,mgParts* what=0) =0;
	virtual int RemoveFromCollection( const string Name,const vector<mgItem*>&items,mgParts* what=0) =0;
	virtual bool DeleteCollection( const string Name) =0;
	virtual void ClearCollection( const string Name) =0;
	virtual bool CreateCollection( const string Name) =0;

	void Sync(char * const * path_argv = 0);
	virtual bool FieldExists(string table, string field)=0;
	virtual void LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap)=0;
	virtual string LoadItemsInto(mgParts& what,vector<mgItem*>& items) = 0;
	virtual string LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems)=0;
	string sql_string(const string s); // \todo does it need to be public?
	virtual bool NeedGenre2() = 0;
	virtual bool Threadsafe() { return false; }
	virtual void Execute(const string sql)=0;
   protected:
	int m_rows;
	int m_cols;
	virtual bool SyncStart() { return true; }
	virtual void SyncEnd() {}
	virtual void SyncFile(const char *filename) {}
  	bool m_database_found;
  	bool m_hasfolderfields;
	char* sql_Cstring(const string s,char *buf=0);
	virtual char* sql_Cstring(const char *s,char *buf)=0;
	bool m_separate_thread;
	time_t m_connect_time;
	time_t m_create_time;
	char* Build_cddbid(const char* artist);
	virtual string get_col0(const string sql) =0;
	void FillTables();
};

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
mgKey*
ktGenerate(const mgKeyTypes kt);

mgDb* GenerateDB(bool SeparateThread=false);

/*! \brief if the SQL command works on only 1 table, remove all table
* qualifiers. Example: SELECT X.title FROM X becomes SELECT title
* FROM X
* \param spar the sql command. It will be edited in place
* \return the new sql command is also returned
*/
extern string optimize(string& spar);
class mgKeyMaps {
	public:
		string value(mgKeyTypes kt, string idstr) const;
		string id(mgKeyTypes kt, string valstr) const;
	private:
		bool loadvalues (mgKeyTypes kt) const;
};

extern mgKeyMaps KeyMaps;

#endif
