/*!
 * \file mg_sync.c
 * \brief synchronization between SQL and filesystem
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include "mg_sync.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fts.h>
#include <sys/time.h>
#include <time.h>

#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>

#include "mg_tools.h"
#include "mg_mysql.h"
#include "mg_setup.h"

char *
mgSync::sql_Cstring(TagLib::String s,char *buf)
{
	return m_db.sql_Cstring(s.toCString(),buf);
}

char *
mgSync::lower(char *s)
{
	char *p=s;
	while (*p)
	{
		int i=(int)(*p);
		(*p)=(char)tolower(i);
		p++;
	}
	return s;
}

TagLib::String
mgSync::getlanguage(const char *filename)
{
      TagLib::String result = "";
      TagLib::ID3v2::Tag * id3v2tag=0;
      if (!strcmp(c_extension,"flac"))
      {
      	TagLib::FLAC::File f(filename);
	id3v2tag = f.ID3v2Tag();
      	if (id3v2tag)
      	{
      	  TagLib::ID3v2::FrameList l = id3v2tag->frameListMap()["TLAN"];
      	  if (!l.isEmpty())
  	    result = l.front()->toString();
        }
      }
      else if (!strcmp(c_extension,"mp3"))
      {
      	TagLib::MPEG::File f(filename);
	id3v2tag = f.ID3v2Tag();
        if (id3v2tag)
        {
      	  TagLib::ID3v2::FrameList l = id3v2tag->frameListMap()["TLAN"];
      	  if (!l.isEmpty())
  	    result = l.front()->toString();
        }
      }
      return result;
}

char *
mgSync::getAlbum(const char *c_album,const char *c_artist,const char *c_directory)
{
	char * result;
	char *b;
	asprintf(&b,"SELECT cddbid FROM album"
			" WHERE title=%s AND artist=%s",c_album,c_artist);
	result=m_db.sql_Cstring(m_db.get_col0(b));
	free(b);
	if (!strcmp(result,"'NULL'"))
	{
		const char *directory="substring(tracks.mp3file,1,length(tracks.mp3file)"
			"-instr(reverse(tracks.mp3file),'/'))";
		char *where;
		asprintf(&where,"WHERE tracks.sourceid=album.cddbid "
			"AND %s=%s "
			"AND album.title=%s",
			directory,c_directory,
			c_album);

		// how many artists will the album have after adding this one?
		asprintf(&b,"SELECT distinct album.artist FROM tracks, album %s "
				" union select %s",where,c_artist);
        	MYSQL_RES *rows = m_db.exec_sql (b);
		free(b);
		long new_album_artists = m_db.affected_rows();
		mysql_free_result(rows);	
		if (new_album_artists>1)	// is the album multi artist?
		{
			asprintf(&b,"SELECT album.cddbid FROM tracks, album %s",where);
			free(result);
			result=m_db.sql_Cstring(m_db.get_col0(b));
			free(b);
			asprintf(&b,"UPDATE album SET artist='Various Artists' WHERE cddbid=%s",result);
			m_db.exec_sql(b);
			// here we could change all tracks.sourceid to result and delete
			// the other album entries for this album, but that should only 
			// be needed if a pre 0.1.4 import has been done incorrectly, so we
			// don't bother
		}
		else
		{				// no usable album found
			free(result);
			asprintf(&result,"'%ld-%9s",random(),c_artist+1);
			char *p=strchr(result,0)-1;
			if (*p!='\'')
				*p='\'';
			asprintf(&b,"INSERT INTO album SET title=%s,artist=%s,cddbid=%s",
				c_album,c_artist,result);
			m_db.exec_sql(b);
			free(b);
		}
		free(where);
	}
	return result;
}

mgSync::mgSync()
{
      	m_genre_rows=0;
	if (!m_db.Connected())
		return;
	m_genre_rows = m_db.exec_sql ("SELECT id,genre from genre");
 	MYSQL_ROW rx;
	while ((rx = mysql_fetch_row (m_genre_rows)) != 0)
		m_Genres[rx[1]]=rx[0];
	// init random number generator
	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv, &tz );
	srandom( tv.tv_usec );
}

mgSync::~mgSync()
{
	if (m_genre_rows) mysql_free_result(m_genre_rows);
}

void
mgSync::UpdateTrack(long trackid)
{
	char sql[7000];
	char *c_cddbid=getAlbum(c_album,c_artist,c_directory);
	sprintf(sql,"UPDATE tracks SET artist=%s, title=%s,year=%d,sourceid=%s,"
			"tracknb=%d,length=%d,bitrate=%d,samplerate=%d,"
			"channels=%d,genre1=%s,lang=%s WHERE id=%ld",
			c_artist,c_title,year,c_cddbid,
			trackno,len,bitrate,sample,
			channels,c_genre1,c_lang,trackid);
	free(c_cddbid);
	m_db.exec_sql(sql);
}

void
mgSync::AddTrack()
{
	char sql[7000];
	char *c_cddbid=getAlbum(c_album,c_artist,c_directory);
	sprintf(sql,"INSERT INTO tracks SET artist=%s,title=%s,year=%u,sourceid=%s,"
			"tracknb=%u,mp3file=%s,length=%d,bitrate=%d,samplerate=%d,"
			"channels=%d,genre1=%s,genre2='',lang=%s,"
			"folder1=%s,folder2=%s,folder3=%s,folder4=%s",
			c_artist,c_title,year,c_cddbid,
			trackno,c_mp3file,len,bitrate,sample,
			channels,c_genre1,c_lang,
			c_folder1,c_folder2,c_folder3,c_folder4);
	free(c_cddbid);
	m_db.exec_sql(sql);
}

bool
mgSync::GetFileInfo(const char *filename)
{
	TagLib::FileRef f( filename) ;
	if (f.isNull())
		return false;
	TagLib::Tag *tag = f.tag();
	if (!f.tag())
		return false;
	if (tag->album()=="")
		strcpy(c_album,"'Unassigned'");
	else
		sql_Cstring(tag->album(),c_album);
        sql_Cstring(tag->artist(),c_artist);
	sql_Cstring(tag->title(),c_title);
	sql_Cstring(filename,c_directory);
	char *slash=strrchr(c_directory,'/');
	if (slash)
	{
		*slash='\'';
		*(slash+1)=0;
	}
	const char *genrename=tag->genre().toCString();
	const char *genreid=m_Genres[genrename].c_str();
	sql_Cstring(genreid,c_genre1);
	sql_Cstring(getlanguage(filename),c_lang);
	trackno=tag->track();
	year=tag->year();
        TagLib::AudioProperties *ap = f.audioProperties();
        len      = ap->length();     // tracks.length
        bitrate  = ap->bitrate();    // tracks.bitrate
        sample   = ap->sampleRate(); //tracks.samplerate
        channels = ap->channels();   //tracks.channels
	if (m_db.HasFolderFields())
	{
	    char *path = strdup(filename);
	    char *folder1="";
	    char *folder2="";
	    char *folder3="";
	    char *folder4="";
	    char *p=path;
	    char *slash;
	    slash=strchr(p,'/');
	    if (slash)
	    {
	    	folder1=p;
		*slash=0;
		p=slash+1;
	    	slash=strchr(p,'/');
	    	if (slash)
	    	{
	    		folder2=p;
	    		*slash=0;
			p=slash+1;
	    		slash=strchr(p,'/');
	    		if (slash)
	    		{
	    			folder3=p;
	    			*slash=0;
				p=slash+1;
	    			slash=strchr(p,'/');
	    			if (slash)
	    			{
	    				folder4=p;
	    				*slash=0;
				}
			}
		}
	    }
		sql_Cstring(folder1,c_folder1);
		sql_Cstring(folder2,c_folder2);
		sql_Cstring(folder3,c_folder3);
		sql_Cstring(folder4,c_folder4);
		free(path);
	}
	return true;
}

void
mgSync::SyncFile(const char *filename)
{
	if (!strncmp(filename,"./",2))	// strip leading ./
		filename += 2;
	if (strlen(filename)>255)
	{
		mgWarning("Length of file exceeds database field capacity: %s", filename);
		return;
	}
	mgDebug(3,"Importing %s",filename);
	sql_Cstring(filename,c_mp3file);
	char sql[600];
	sprintf(sql,"SELECT id from tracks WHERE mp3file=%s",c_mp3file);
	string s = m_db.get_col0(sql);
	if (s!="NULL")
	{
		if (GetFileInfo(filename))
			UpdateTrack(atol(s.c_str()));
	}
	else
	{
		if (GetFileInfo(filename))
			AddTrack();
	}
}

void
mgSync::Sync(char * const * path_argv, bool delete_missing)
{
	if (!m_db.Connected())
		return;
	unsigned int count=0;
	m_db.CreateFolderFields();
	chdir(the_setup.ToplevelDir);
	FTS *fts;
	FTSENT *ftsent;
	fts = fts_open( path_argv, FTS_LOGICAL, 0);
	if (fts)
	{
		while ( (ftsent = fts_read(fts)) != NULL)
		{
			if (!((ftsent->fts_statp->st_mode)||S_IFREG))
				continue;
      			char *extension = strrchr(ftsent->fts_path,'.');
      			if (!extension)
	      			continue;
			strcpy(c_extension,extension+1);
      			lower(c_extension);
			if (!strcmp(c_extension,"flac") || !strcmp(c_extension,"ogg") || !strcmp(c_extension,"mp3"))
			{
				SyncFile(ftsent->fts_path);
				count++;
				if (count%1000==0)
				{
					extern void showimportcount(unsigned int);
					showimportcount(count);
				}
			}
		}
		fts_close(fts);
	}
}

void
mgSync::Create()
{
	m_db.Create();
}
