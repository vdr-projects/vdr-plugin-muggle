/*! 
 * \file   mg_db_gd.h
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

class mgDbGd : public mgDb {
   public:
	mgDbGd (bool SeparateThread=false);
	~mgDbGd();
	bool Connect();
  	bool Create();
	int AddToCollection( const string Name,const vector<mgItem*>&items,mgParts* what);
	int  RemoveFromCollection( const string Name,const vector<mgItem*>&items,mgParts* what);
	bool DeleteCollection( const string Name);
	void ClearCollection( const string Name);
	bool CreateCollection( const string Name);
	
	bool NeedGenre2();
	long thread_id() { return -1; }
	bool FieldExists(string table, string field);
	void LoadMapInto(string sql,map<string,string>*idmap,map<string,string>*valmap);
	string LoadItemsInto(mgParts& what,vector<mgItem*>& items);
	string LoadValuesInto(mgParts& what,mgKeyTypes tp,vector<mgListItem*>& listitems);
	bool Threadsafe();
   protected:
	char* sql_Cstring(const char *s,char *buf);
	bool SyncStart();
	void SyncFile(const char *filename);
	void SyncEnd();
	void Execute(const string sql);
   private:
	char ** Query(const string sql);
	sqlite3 *m_db;
	char ** query(const string sql);
	int silent_execute( const string sql);
	void execute( const string sql);
  	string get_col0( const string sql);
	char *sql_Cstring(TagLib::String s,char *buf=0);
	TagLib::String getlanguage(const char *filename);
	char * getAlbum(const char *filename,const char *c_album,const char *c_artist);
	map<string,string> m_Genres;
	char *m_errmsg;

};

#endif
