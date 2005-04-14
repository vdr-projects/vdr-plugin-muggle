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

class mgDbGd
{
	public:
                mgDbGd();
		~mgDbGd();
		//! \brief drop and create the data base GiantDisc
		void Create();

		/*! \brief import/export tags like
	 	 * \par path can be a file or a directory. If directory, 
	 	 * sync all files within 
	 	 * \par assorted see mugglei -h
	 	 * \par delete_missing if the file does not exist, delete the
	 	 * data base entry. If the file is unreadable, do not delete.
	 	 */
		void Sync(char * const * path_argv, bool delete_missing = false);
		
	private:
		mgmySql m_db;
		char *sql_Cstring(TagLib::String s,char *buf=0);
		char *lower(char *s);
		TagLib::String getlanguage(const char *filename);
		char * getAlbum(const char *c_album,const char *c_artist,const char *c_directory);
		bool GetFileInfo(const char *filename);
		void AddTrack();
		void UpdateTrack(long trackid);
		void SyncFile(const char *filename);
		map<string,string> m_Genres;
		MYSQL_RES* m_genre_rows;


		char c_album[520]; // at least 256 * 2 + 2 for VARCHAR(255), see sql_string()
		char c_artist[520];
		char c_title[520];
		char c_directory[520];
		char c_mp3file[520];
		char c_genre1[520];
		char c_lang[520];
		char c_folder1[520];
		char c_folder2[520];
		char c_folder3[520];
		char c_folder4[520];
		char c_extension[300];
		unsigned int trackno;
		unsigned int year;
		int len;
		int bitrate;
		int sample;
		int channels;
};

#endif
