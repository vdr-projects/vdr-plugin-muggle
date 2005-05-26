/*! 
 * \file   mg_db_gd_mysql.h
 * \brief  A capsule around giantdisc database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-04-13 17:42:54 +0100 (Thu, 13 Apr 2005) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wolfgang61 $
 */

#ifndef __MG_DB_GD_H
#define __MG_DB_GD_H

#include <string>
#include <tag.h>
#include <map>
#include <mysql/mysql.h>

using namespace std;

#include "mg_db.h"

class mgDbGd : public mgDb {
   public:
	mgDbGd (bool SeparateThread=false);
	~mgDbGd();
	bool ServerConnect();
	bool Connect();
  	bool Create();
	int AddToCollection( const string Name,const vector<mgItem*>&items,mgParts* what);
	int  RemoveFromCollection( const string Name,const vector<mgItem*>&items,mgParts* what);
	
	bool NeedGenre2();
	long thread_id() { return mysql_thread_id(m_db); }
	bool FieldExists(string table, string field);
	void ServerEnd();
	bool Threadsafe();
	const char* HelpText() const;
	const char *Options() const;
	void *DbHandle() const { return (void*)m_db; }
	const char *DecadeExpr();
	string Now() const { return "CURRENT_TIMESTAMP"; }
	string Directory() const { return "substring(tracks.mp3file,1,length(tracks.mp3file)"
		                        "-instr(reverse(tracks.mp3file),'/'))"; }
   protected:
	void SyncFile(const char *filename);
	void StartTransaction();
	void Commit();
   private:
	MYSQL *m_db;
	void CreateFolderFields();
	MYSQL_RES* Query( const string sql);
  	bool sql_query(string sql);

};

class mgSQLStringMySQL : public mgSQLStringImp {
	public:
		mgSQLStringMySQL(const char* s);
		~mgSQLStringMySQL();
		char *unquoted() const;
	private:
		mutable char* m_unquoted;
};

class mgQueryMySQL : public mgQueryImp {
	public:
		mgQueryMySQL(void* db,string sql,mgQueryNoise noise);
		~mgQueryMySQL();
		char ** Next();
	private:
		MYSQL_RES *m_table;
		int m_rc;
		MYSQL *m_db;
};
#endif
