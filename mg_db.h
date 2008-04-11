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
#include <tag.h>
#include <id3v2tag.h>
#include <fileref.h>
#include <xiphcomment.h>

#ifndef trdb
#define trdb(a) (a)
#endif

using namespace std;

class mgItem;

#include "mg_listitem.h"
#include "mg_keytypes.h"

typedef list<string> strlist;

strlist& operator+=(strlist&a, strlist b);

string sql_list (string prefix,strlist v,string sep=",",string postfix="");

class mgSQLStringImp
{
	public:
		mgSQLStringImp();
		virtual ~mgSQLStringImp();
		char *quoted() const;
		virtual char *unquoted() const = 0 ;
		const char *original() const { return m_original; }
	protected:
		const char *m_original;
	private:
		mutable char *m_quoted;
};

class mgSQLString
{
	public:
		mgSQLString(const char*s);
		mgSQLString(string s);
		mgSQLString(TagLib::String s);
		mgSQLString(const mgSQLString& s);
		~mgSQLString();
		char *quoted() const;
		char *unquoted() const;
		const char *original() const;
		void operator=(const mgSQLString& b);
		bool operator==(const mgSQLString& b) const;
		bool operator==(const char* b) const;
		bool operator==(string b) const;
		bool operator!=(const mgSQLString& b) const;
		bool operator!=(const char* b) const;
		bool operator!=(string b) const;
		void operator=(const char* b);
	private:
		void Init(const char* s);
		mgSQLStringImp* m_str;
		char* m_original;
};

enum mgQueryNoise {mgQueryNormal, mgQueryWarnOnly, mgQuerySilent};

class mgQueryImp
{
	public:
		mgQueryImp(void *db,string sql,mgQueryNoise noise);
		virtual ~mgQueryImp() {};
		virtual char ** Next() = 0;
		int Rows() { return m_rows; }
		int Columns() { return m_columns; }
		string ErrorMessage() { if (!m_errormessage) return "";else return m_errormessage; }
	protected:
		void HandleErrors();
		string m_sql;
		int m_rows;
		int m_columns;
		int m_rc;
		const char* m_errormessage;
		int m_cursor;
		const char* m_optsql;
		mgQueryNoise m_noise;
		void *m_db_handle;
};

class mgQuery
{
	public:
		mgQuery(void *db,const char*s,mgQueryNoise noise = mgQueryNormal);
		mgQuery(void *db,string s,mgQueryNoise noise = mgQueryNormal);
		~mgQuery();
		int Rows() { return m_q->Rows(); }
		int Columns() { return m_q->Columns(); }
		char ** Next() { return m_q->Next(); }
		string ErrorMessage() { return m_q->ErrorMessage(); }
	private:
		void Init(void *db,const char*s,mgQueryNoise noise);
		mgQueryImp* m_q;
};

class mgReference
{
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

class mgReferences : public vector<mgReference*>
{
	public:
		void InitReferences();
		unsigned int CountTable(string table) const;
};

class mgParts
{
	public:
		mgParts();
		~mgParts();
		strlist valuefields;	 // if idfield and valuefield are identical, define idfield only
		strlist idfields;
		strlist tables;
		strlist clauses;
		mgParts& operator+=(mgParts a);
		void Prepare();
		void ConnectAllTables();
		string sql_count();
		string sql_select(bool distinct);
		string sql_selectitems();
		bool empty() const { return tables.size()==0;}
		string special_statement;
		bool orderByCount;
		void Dump(string where) const;
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
class mgDb
{
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
		bool Connect();
		virtual bool ConnectDatabase() = 0;
		bool HasFolderFields() const { return m_hasfolderfields;}
		virtual int AddToCollection( const string Name,const vector<mgItem*>&items,mgParts* what=0);
		virtual int RemoveFromCollection( const string Name,const vector<mgItem*>&items,mgParts* what=0);
		virtual bool DeleteCollection( const string Name);
		virtual void ClearCollection( const string Name);
		virtual bool CreateCollection( const string Name);

		void Sync(const char * const * path_argv);
		virtual bool FieldExists(string table, string field)=0;
		void LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap);
		string LoadItemsInto(mgParts& what,vector<mgItem*>& items);
		string LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems,bool groupby);
		virtual bool NeedGenre2() = 0;
		virtual bool Threadsafe() { return false; }
		int Execute(const string sql);
		virtual const char* Options() const =0;
		virtual const char* HelpText() const =0;
		void* DbHandle();
		virtual const char* DecadeExpr()=0;
		virtual string Now() const =0;
		virtual string Directory() const =0;
	protected:
		int m_rows;
		int m_cols;
		virtual void SyncEnd() {}
		bool SyncFile(const char *filename);
		bool m_hasfolderfields;
		bool m_separate_thread;
		time_t m_connect_time;
		time_t m_create_time;
		string get_col0(const string sql);
		virtual bool Creatable() { return true; }
		virtual bool Create() = 0;
		virtual bool Clear() = 0;
		virtual void StartTransaction() {};
		virtual void Commit() {};
		virtual bool SyncStart();
		virtual void CreateFolderFields() {};
		virtual void* ImplDbHandle() const = 0;
	private:
		TagLib::String m_TLAN;
		TagLib::String m_TCON;
		TagLib::String getId3v2Tag(TagLib::ID3v2::Tag *id3v2tags,const char *name) const;
		void get_ID3v2_Tags(const char *filename);
		TagLib::String getVorbisComment(const TagLib::Ogg::FieldListMap& vorbiscomments,const char* key);
		void get_vorbis_tags(const TagLib::Ogg::FieldListMap& vorbiscomments);
		void get_id3_tags(TagLib::ID3v2::Tag *tags);
		void DefineGenre(const string genre);
		mgSQLString getGenre1(TagLib::FileRef& f);
		mgSQLString Build_cddbid(const mgSQLString& artist) const;
		mgSQLString getAlbum(const char *filename,const mgSQLString& c_album,
			const mgSQLString& c_artist);
		map<string,string> m_Genres;
		map<string,string> m_GenreIds;
		bool m_database_found;
		void FillTables();
};

class mgKey
{
	public:
		virtual ~mgKey() {};
		virtual mgParts Parts(mgDb *db,bool groupby) const = 0;
		virtual string id() const = 0;
		virtual bool valid() const = 0;
		virtual string value () const = 0;
		//!\brief translate field into user friendly string
		virtual void set(mgListItem* item) = 0;
		virtual mgListItem* get() = 0;
		virtual mgKeyTypes Type() const = 0;
		virtual mgSortBy SortBy() const { return mgSortByValue; }
		virtual bool Enabled(mgDb *db) { return true; }
		virtual bool LoadMap() const;
	protected:
		virtual string map_sql() const { return ""; }
};

class mgKeyNormal : public mgKey
{
	public:
		mgKeyNormal(const mgKeyNormal& k);
		mgKeyNormal(const mgKeyTypes kt, string table, string field);
		virtual mgParts Parts(mgDb *db,bool groupby) const;
		string value() const;
		string id() const;
		bool valid() const;
		void set(mgListItem* item);
		mgListItem* get();
		mgKeyTypes Type() const { return m_kt; }
		virtual string table() const { return m_table; }
	protected:
		virtual string expr(mgDb *db) const { return m_table + "." + m_field; }
		string IdClause(mgDb *db,string what,string::size_type start=0,string::size_type len=string::npos) const;
		void AddIdClause(mgDb *db,mgParts &result,string what) const;
		mgListItem *m_item;
		string m_field;
	private:
		mgKeyTypes m_kt;
		string m_table;
};

class mgKeyABC : public mgKeyNormal
{
	public:
		mgKeyABC(const mgKeyNormal& k) : mgKeyNormal(k) {}
		mgKeyABC(const mgKeyTypes kt, string table, string field) : mgKeyNormal(kt,table,field) {}
		virtual string expr(mgDb*db) const { return "substr("+mgKeyNormal::expr(db)+",1,1)"; }
	protected:
		//void AddIdClause(mgDb *db,mgParts &result,string what) const;
};

class mgKeyDate : public mgKeyNormal
{
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
class mgKeyMaps
{
	public:
		string value(mgKeyTypes kt, string idstr) const;
		string id(mgKeyTypes kt, string valstr) const;
	private:
		bool loadvalues (mgKeyTypes kt) const;
};

extern mgKeyMaps KeyMaps;

class mgDbServerImp
{
	public:
		mgDbServerImp() {m_escape_db = 0;}
		virtual ~mgDbServerImp() {delete m_escape_db;}
		mgDb* EscapeDb() const { return m_escape_db;}
	protected:
		mgDb* m_escape_db;
};

class mgDbServer
{
	private:
		mgDbServerImp *m_server;
	public:
		mgDbServer();
		~mgDbServer();
		mgDb* EscapeDb() { return m_server->EscapeDb(); }
};

extern mgDbServer* DbServer;
#endif
