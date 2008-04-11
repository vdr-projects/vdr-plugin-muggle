/*!
 * \file   mg_db_gd_sqlite.h
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
#include <sqlite3.h>

using namespace std;

#include "mg_db.h"

class mgDbServerSQLite : public mgDbServerImp
{
};

class mgSQLStringSQLite : public mgSQLStringImp
{
	public:
		mgSQLStringSQLite(const char* s);
		~mgSQLStringSQLite();
		char *unquoted() const;
	private:
		mutable char* m_unquoted;
};

class mgQuerySQLite : public mgQueryImp
{
	public:
		mgQuerySQLite(void *db,string sql,mgQueryNoise noise);
		~mgQuerySQLite();
		char ** Next();
	private:
		char **m_table;
		int m_rc;
};

class mgDbGd : public mgDb
{
	public:
		mgDbGd (bool SeparateThread=false);
		~mgDbGd();
		bool ConnectDatabase();
		bool Creatable();
		bool Create();
		bool Clear();

		bool NeedGenre2();
		long thread_id() { return -1; }
		bool FieldExists(string table, string field);
		bool Threadsafe();
		const char* Options() const;
		const char* HelpText() const;
		void *ImplDbHandle() const { return m_db; }
		const char *DecadeExpr();
		string Now() const { return "strftime('%s','now')"; }
		string Directory() const { return "mgDirectory(mp3file)"; }
	protected:
		void StartTransaction();
		void Commit();
		bool SyncStart();
		sqlite3 *m_db;
};
#endif
