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
#include <libpq-fe.h>

using namespace std;

#include "mg_db.h"

class mgDbServerPG : public mgDbServerImp {
};

class mgSQLStringPG : public mgSQLStringImp {
	public:
		mgSQLStringPG(const char* s);
		~mgSQLStringPG();
		char *unquoted() const;
	private:
		mutable char* m_unquoted;
};

class mgQueryPG : public mgQueryImp {
	public:
		mgQueryPG(void* db,string sql,mgQueryNoise noise);
		~mgQueryPG();
		char ** Next();
	private:
		PGresult *m_table;
		PGconn *m_db;
		char *m_rowpointers[100];
};

class mgDbGd : public mgDb {
   public:
	mgDbGd (bool SeparateThread=false);
	~mgDbGd();
	bool ServerConnect();
	bool ConnectDatabase();
  	bool Create();
	
	bool NeedGenre2();
	long thread_id() { return -1; }
	bool FieldExists(string table, string field);
	bool Threadsafe();
	const char* Options() const;
	const char* HelpText() const;
	const char *DecadeExpr();
	string Now() const { return "CURRENT_TIMESTAMP";}
	string Directory() const { return "substring(tracks.mp3file from '.*/(.*)')"; }
   protected:
	void SyncFile(const char *filename);
	void StartTransaction();
	void Commit();
	void *ImplDbHandle() const { return (void*)m_db;}
   private:
	bool myCreate();
	PGconn *m_db;

};
#endif
