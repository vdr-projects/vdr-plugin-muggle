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
#include <vector>
#include <map>

using namespace std;

class mgItem;
class mgParts;
#include "mg_listitem.h"
#include "mg_tools.h"
#include "mg_order.h"

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
  	virtual unsigned long exec_count (string query) = 0;
  	virtual bool ServerConnect() = 0;
  	virtual bool Connect() = 0;
  	bool HasFolderFields() const { return m_hasfolderfields;}
  	virtual bool Create() = 0;
	virtual void ServerEnd() =0;		// must be done explicitly
	virtual int AddToCollection( const string Name,const vector<mgItem*>&items) =0;
	virtual int RemoveFromCollection( const string Name,mgParts& what) =0;
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
   protected:
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
};

mgDb* GenerateDB(bool SeparateThread=false);

/*! \brief if the SQL command works on only 1 table, remove all table
* qualifiers. Example: SELECT X.title FROM X becomes SELECT title
* FROM X
* \param spar the sql command. It will be edited in place
* \return the new sql command is also returned
*/
extern string optimize(string& spar);
#endif
