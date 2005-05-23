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
	bool DeleteCollection( const string Name);
	void ClearCollection( const string Name);
	bool CreateCollection( const string Name);
	
	bool NeedGenre2();
	long thread_id() { return mysql_thread_id(m_db); }
	bool FieldExists(string table, string field);
	void LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap);
	string LoadItemsInto(mgParts& what,vector<mgItem*>& items);
	string LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems,bool distinct);
	void ServerEnd();
	bool Threadsafe();
	void Execute(const string sql);
	const char* HelpText() const;
	const char *Options() const;
   protected:
	char* sql_Cstring(const char *s,char *buf);
	bool SyncStart();
	void SyncFile(const char *filename);
   private:
	MYSQL *m_db;
	void CreateFolderFields();
	MYSQL_RES* Query( const string sql);
  	bool sql_query(string sql);
  	string get_col0( const string sql);
	char *sql_Cstring(TagLib::String s,char *buf=0);
	TagLib::String getlanguage(const char *filename);
	char * getAlbum(const char *filename,const char *c_album,const char *c_artist);
	map<string,string> m_Genres;

};

#endif
