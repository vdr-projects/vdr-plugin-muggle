/*!
 * \file mg_sync.h
 * \brief synchronization between SQL and filesystem
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_SYNC_H
#define _MG_SYNC_H

#include <map>
#include <string.h>
#include <tag.h>

#include "mg_mysql.h"

class mgSync
{
	public:
		mgSync();
		~mgSync();
		/*! import/export tags like
	 	 * \par path can be a file or a directory. If directory, 
	 	 * sync all files within 
	 	 * \par assorted see mugglei -h
	 	 * \par delete_missing if the file does not exist, delete the
	 	 * data base entry. If the file is unreadable, do not delete.
	 	 */
		void Sync(const char * path, bool delete_missing=false);

	private:
		mgmySql m_db;
		char *sql_Cstring(TagLib::String s);
		char *lower(char *s);
		TagLib::String getlanguage(const char *filename);
		char * getAlbum(const char *c_album,const char *c_artist,const char *c_directory);
		void AddTrack(const char *filename);
		void SyncFile(const char *filename);
		map<string,string> m_Genres;
		MYSQL_RES* m_genre_rows;
};

#endif
