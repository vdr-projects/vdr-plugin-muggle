/*!
 * \file mugglei.c
 * \brief implement a small utility for importing files
 *
 * \author  Lars von Wedel
 */

// #define VERBOSE

#include <unistd.h>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mysql/mysql.h>
#include <getopt.h>
/*extern "C"
{*/
  #include <stdarg.h>
  #include <stdio.h>
/*}
*/
#include <stdlib.h>

#include <tag.h>
#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>

#include "mg_tools.h"

MYSQL *db;

static char *server_args[] = 
{
  "this_program",       /* this string is not used */
  "--datadir=.",
  "--key_buffer_size=32M"
};

static char *server_groups[] = 
{
  "embedded",
  "server",
  "this_program_SERVER",
  (char *)NULL
};

char *db_cmds[] = 
{
  "DROP DATABASE IF EXISTS GiantDisc; CREATE DATABASE GiantDisc;",
  "grant all privileges on GiantDisc.* to vdr@localhost;",
  "use GiantDisc;",
  "drop table if exists album; CREATE TABLE album ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "cddbid varchar(20) NOT NULL default '', "
	  "coverimg varchar(255) default NULL, "
	  "covertxt mediumtext, "
	  "modified date default NULL, "
	  "genre varchar(10) default NULL, "
	  "PRIMARY KEY  (cddbid), "
	  "KEY artist (artist(10)), "
	  "KEY title (title(10)), "
	  "KEY genre (genre), "
	  "KEY modified (modified)) "
	  "TYPE=MyISAM;",
  "drop table if exists genre; CREATE TABLE genre ("
	  "id varchar(10) NOT NULL default '', "
	  "id3genre smallint(6) default NULL, "
	  "genre varchar(255) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists language; CREATE TABLE language ("
	  "id varchar(4) NOT NULL default '', "
	  "language varchar(40) default NULL, "
	  "freq int(11) default NULL, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists musictype; CREATE TABLE musictype ("
	  "musictype varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists player;CREATE TABLE player ( "
	  "ipaddr varchar(255) NOT NULL default '', "
	  "uichannel varchar(255) NOT NULL default '', "
	  "logtarget int(11) default NULL, "
	  "cdripper varchar(255) default NULL, "
	  "mp3encoder varchar(255) default NULL, "
	  "cdromdev varchar(255) default NULL, "
	  "cdrwdev varchar(255) default NULL, "
	  "id int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists playerstate;CREATE TABLE playerstate ( "
	  "playerid int(11) NOT NULL default '0', "
	  "playertype int(11) NOT NULL default '0', "
	  "snddevice varchar(255) default NULL, "
	  "playerapp varchar(255) default NULL, "
	  "playerparams varchar(255) default NULL, "
	  "ptlogger varchar(255) default NULL, "
	  "currtracknb int(11) default NULL, "
	  "state varchar(4) default NULL, "
	  "shufflepar varchar(255) default NULL, "
	  "shufflestat varchar(255) default NULL, "
	  "pauseframe int(11) default NULL, "
	  "framesplayed int(11) default NULL, "
	  "framestotal int(11) default NULL, "
	  "anchortime bigint(20) default NULL, "
	  "PRIMARY KEY  (playerid,playertype)) "
	  "TYPE=HEAP;",
  "drop table if exists playlist;CREATE TABLE playlist ( "
	  "title varchar(255) default NULL, "
	  "author varchar(255) default NULL, "
	  "note varchar(255) default NULL, "
	  "created timestamp(8) NOT NULL, "
	  "id int(10) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists playlistitem;CREATE TABLE playlistitem ( "
	  "playlist int(11) NOT NULL default '0', "
	  "tracknumber mediumint(9) NOT NULL default '0', "
	  "trackid int(11) default NULL, "
	  "PRIMARY KEY  (playlist,tracknumber)) "
	  "TYPE=MyISAM;",
  "drop table if exists playlog;CREATE TABLE playlog ( "
	  "trackid int(11) default NULL, "
	  "played date default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists recordingitem;CREATE TABLE recordingitem ( "
	  "trackid int(11) default NULL, "
	  "recdate date default NULL, "
	  "rectime time default NULL, "
	  "reclength int(11) default NULL, "
	  "enddate date default NULL, "
	  "endtime time default NULL,  "
	  "repeat varchar(10) default NULL, "
	  "initcmd varchar(255) default NULL, "
	  "parameters varchar(255) default NULL, "
	  "atqjob int(11) default NULL, "
	  "id int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists source; CREATE TABLE source ( "
	  "source varchar(40) default NULL, "
	  "id tinyint(3) unsigned NOT NULL auto_increment, "
	  "PRIMARY KEY  (id)) "
	  "TYPE=MyISAM;",
  "drop table if exists tracklistitem;CREATE TABLE tracklistitem ( "
	  "playerid int(11) NOT NULL default '0', "
	  "listtype smallint(6) NOT NULL default '0', "
	  "tracknb int(11) NOT NULL default '0', "
	  "trackid int(11) NOT NULL default '0', "
	  "PRIMARY KEY  (playerid,listtype,tracknb)) "
	  "TYPE=MyISAM;",
  "drop table if exists tracks;CREATE TABLE tracks ( "
	  "artist varchar(255) default NULL, "
	  "title varchar(255) default NULL, "
	  "genre1 varchar(10) default NULL, "
	  "genre2 varchar(10) default NULL, "
	  "year smallint(5) unsigned default NULL, "
	  "lang varchar(4) default NULL, "
	  "type tinyint(3) unsigned default NULL, "
	  "rating tinyint(3) unsigned default NULL, "
	  "length smallint(5) unsigned default NULL, "
	  "source tinyint(3) unsigned default NULL, "
	  "sourceid varchar(20) default NULL, "
	  "tracknb tinyint(3) unsigned default NULL, "
	  "mp3file varchar(255) default NULL, "
	  "condition tinyint(3) unsigned default NULL, "
	  "voladjust smallint(6) default '0', "
	  "lengthfrm mediumint(9) default '0', "
	  "startfrm mediumint(9) default '0', "
	  "bpm smallint(6) default '0', "
	  "lyrics mediumtext, "
	  "bitrate varchar(10) default NULL, "
	  "created date default NULL, "
	  "modified date default NULL, "
	  "backup tinyint(3) unsigned default NULL, "
	  "samplerate int(7) unsigned default NULL, "
	  "channels   tinyint(3) unsigned default NULL,  "
	  "id int(11) NOT NULL auto_increment, "
	  "folder1 varchar(255), "
	  "folder2 varchar(255), "
	  "folder3 varchar(255), "
	  "folder4 varchar(255), "
	  "PRIMARY KEY  (id), "
	  "KEY title (title(10)), "
	  "KEY mp3file (mp3file(10)), "
	  "KEY genre1 (genre1), "
	  "KEY genre2 (genre2), "
	  "KEY year (year), "
	  "KEY lang (lang), "
	  "KEY type (type), "
	  "KEY rating (rating), "
	  "KEY sourceid (sourceid), "
	  "KEY artist (artist(10))) "
	  "TYPE=MyISAM;"
};

bool folderfields;

std::string host, user, pass, dbname, sck;
bool import_assorted, delete_mode, create_mode;

#define  MAX_QUERY_BUFLEN  2048
static char querybuf[MAX_QUERY_BUFLEN];

void showmessage(const char *msg)
{
}

void
init_folderfields()
{
  folderfields=false;
  mysql_query(db,"DESCRIBE tracks folder1");
  MYSQL_RES *rows = mysql_store_result(db);
  if (rows)
    {
      folderfields = mysql_num_rows(rows)>0;
      mysql_free_result(rows);
      if (!folderfields)
      {
	      folderfields = !mysql_query(db,
	              "alter table tracks add column folder1 varchar(255),"
		      "add column folder2 varchar(255),"
		      "add column folder3 varchar(255),"
		      "add column folder4 varchar(255)");

      }
    }
}

MYSQL_RES* mgSqlReadQuery(MYSQL *db, const char *fmt, ...)
{
  va_list ap;
  va_start( ap, fmt );  
  vsnprintf( querybuf, MAX_QUERY_BUFLEN-1, fmt, ap );
  
  if( mysql_query(db, querybuf) )
    {
      mgError( "SQL error in MUGGLE:\n%s: %s\n", querybuf,mysql_error(db) );
    }
  
  MYSQL_RES *result = mysql_store_result(db);
  
  va_end(ap);
  return result;
}

void mgSqlWriteQuery(MYSQL *db, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(querybuf, MAX_QUERY_BUFLEN-1, fmt, ap);
  
  if( mysql_query(db, querybuf) )
    {
      mgError( "SQL error in MUGGLE:\n%s %s\n", querybuf,mysql_error(db) );
    }
  
  va_end(ap);
}

int init_database()
{
  db = mysql_init(0); // NULL?

  if( db == NULL )
    {	
      std::cout << "mysql_init failed." << std::endl;
      return -1;
    }

  // check for use of sockets
  if( sck != "" )
    {
      if( mysql_real_connect( db, NULL, user.c_str(), pass.c_str(), dbname.c_str(),
			      0, sck.c_str(), 0 ) == NULL )

	{
	  std::cout << "mysql_real_connect using sockets failed." << std::endl;
	  return -2;
	}
    }
  else
    {
      if( mysql_real_connect( db, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(),
			      0, NULL, 0 ) == NULL )
	{
	  std::cout << "mysql_real_connect via TCP failed." << std::endl;
	  return -2;
	}
    }
  
  return 0;
}

MYSQL_RES *
exec_sql( std::string query )
{
  if( query.empty() )
    return 0;
  mgDebug( 3, "exec_sql(%X,%s)", db, query.c_str() );
  if (mysql_query (db, (query + ';').c_str ()))
    {
      mgError("SQL Error in %s: %s",query.c_str(),mysql_error (db));
      std::cout << "ERROR in " << query << ":" << mysql_error(db) << std::endl;
      return 0;
    }
  return mysql_store_result(db);
}

int create_database()
{
  // create database and tables
  int len = sizeof( db_cmds ) / sizeof( char* );
  for( int i=0; i < len; i ++ )
    {
      exec_sql( std::string( db_cmds[i] ) );
    }
  return 0;
}

time_t get_fs_modification_time( std::string filename )
{
  struct stat *buf = (struct stat*) malloc( sizeof( struct stat ) );
  time_t mod = 0;

  // yes: obtain modification date for file and db entry
  if( !stat( filename.c_str(), buf ) )
    {
      mod = buf->st_mtime;
      free( buf );
    }

  return mod;
}

time_t get_db_modification_time( long uid )
{
  time_t mt = 0;

  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT UNIX_TIMESTAMP(modification_time) "
				          "FROM tracks WHERE id=\"%d\"", uid );
  if( mysql_num_rows(result) )
    {
      MYSQL_ROW row     = mysql_fetch_row( result );
  
      std::string mod_time = row[0];
      mt = (time_t) atol( mod_time.c_str() );
    }
  
  return mt;
}

TagLib::String escape_string( MYSQL *db, TagLib::String s )
{
  char *buf = strdup( s.toCString() );
  char *escbuf = (char *) malloc( 2*strlen( buf ) + 1 );

  mysql_real_escape_string( db, escbuf, s.toCString(), s.size() );
  TagLib::String r = TagLib::String( escbuf );

  free( escbuf );
  free( buf);

  return r;
}

long find_file_in_database( MYSQL *db, std::string filename )
{
  long uid = -1;
 
  TagLib::String file = TagLib::String( filename.c_str() );
  file = escape_string( db, file );

  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT id FROM tracks WHERE mp3file=\"%s\"", file.toCString() );
  if( mysql_num_rows(result) )
    {
      MYSQL_ROW row = mysql_fetch_row( result );
      uid = atol( row[0] );
    }

  // obtain ID and return
  return uid;
}

TagLib::String find_genre_id( TagLib::String genre )
{
  TagLib::String id = "";
  
  if( genre.size() )
    {
      MYSQL_RES *result = mgSqlReadQuery( db, "SELECT id FROM genre WHERE "
					  "genre=\"%s\"", genre.toCString() );
      
      if( mysql_num_rows(result) )
	{
	  MYSQL_ROW row = mysql_fetch_row( result );
	  
	  id = row[0];
	}
    }

  return id;
} 

// read tags from the mp3 file and store them into the corresponding database entry
void update_db( long uid, std::string filename )
{
  TagLib::String title, album, artist, genre, cddbid, language;
  uint trackno, year;

  //  ID3_Tag filetag( filename.c_str() );
  TagLib::FileRef f( filename.c_str() );

  if( !f.isNull() && f.tag() ) 
    {
      //      std::cout << "Evaluating " << filename << std::endl;
      TagLib::Tag *tag = f.tag();

      // obtain tag information
      title   = tag->title();
      album   = tag->album();
      year    = tag->year();
      artist  = tag->artist();
      trackno = tag->track();
      genre   = tag->genre();
      language = "";
      TagLib::ID3v2::Tag * id3v2tag=0;
      if (filename.substr(filename.size()-5)==".flac")
      {
      	TagLib::FLAC::File f(filename.c_str());
	id3v2tag = f.ID3v2Tag();
      	if (id3v2tag)
      	{
      	  TagLib::ID3v2::FrameList l = id3v2tag->frameListMap()["TLAN"];
      	  if (!l.isEmpty())
  	    language = l.front()->toString();
        }
      }
      else if (filename.substr(filename.size()-4)==".mp3")
      {
      	TagLib::MPEG::File f(filename.c_str());
	id3v2tag = f.ID3v2Tag();
        if (id3v2tag)
        {
      	  TagLib::ID3v2::FrameList l = id3v2tag->frameListMap()["TLAN"];
      	  if (!l.isEmpty())
  	    language = l.front()->toString();
        }
      }

      TagLib::String gid = find_genre_id( genre );

      TagLib::AudioProperties *ap = f.audioProperties();
      int len      = ap->length();     // tracks.length
      int bitrate  = ap->bitrate();    // tracks.bitrate
      int sample   = ap->sampleRate(); //tracks.samplerate
      int channels = ap->channels();   //tracks.channels

      title  = escape_string( db, title );
      album  = escape_string( db, album );
      artist = escape_string( db, artist );
            
      // TODO: CD identifier (if it exists), playcounter, popularimeter (rating?), volume adjustment, lyrics, cover
      
      // finally update the database

      // obtain associated album or create
      if( album == "" )
	{ // no album found, create default album for artist
	  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT cddbid FROM album WHERE title=\"Unassigned\" AND artist=\"%s\"", artist.toCString() );
	  MYSQL_ROW row = mysql_fetch_row( result );
	  
	  // Default album does not yet exist (num rows == 0)
	  int nrows = mysql_num_rows(result);
	  if( nrows == 0 )
	    {
	      // create new album entry "Unassigned" for this artist
	      long id = random();
	      char *buf;
	      asprintf( &buf, "%ld-%s", id, tag->artist().toCString() );
	      cddbid = TagLib::String( buf ).substr( 0, 20 );
	      cddbid = escape_string( db, cddbid );
	      free( buf );
	  
	      mgSqlWriteQuery( db,
			       "INSERT INTO album (artist, title,          cddbid) "
			       "VALUES            (\"%s\", \"Unassigned\", \"%s\")", 
			       artist.toCString(), cddbid.toCString() );
	    }
	  else
	    { // use first album found as source id for the track
	      cddbid = escape_string(db,row[0]);
	    }
	}
      else
	{ // album tag found, associate or create
	  MYSQL_RES *result;
	  if( import_assorted )
	    { // lookup an existing album by title only (artist should be "Various Artists"
	      result = mgSqlReadQuery( db, "SELECT cddbid FROM album WHERE title=\"%s\" AND artist=\"Various Artists\"", 
				       album.toCString(), artist.toCString() );	      
	    }
	  else
	    {
	      result = mgSqlReadQuery( db, "SELECT cddbid FROM album WHERE title=\"%s\" AND artist=\"%s\"", 
				       album.toCString(), artist.toCString() );
	    }
	  MYSQL_ROW row = mysql_fetch_row( result );

	  // num rows == 0 ?
	  int nrows   = mysql_num_rows(result);
	  if( nrows == 0 )
	    {
	      // create new album entry 
	      long id = random();
	      char *buf;
	      asprintf( &buf, "%ld-%s", id, tag->album().toCString() );
	      cddbid = TagLib::String( buf ).substr( 0, 20 );
	      cddbid = escape_string( db, cddbid );
	      free( buf );

	      if( import_assorted )
		{ // in this case, the album author is "Various Artists"
		  mgSqlWriteQuery( db, 
				   "INSERT INTO album (artist,title,cddbid) "
				   "VALUES (\"Various Artists\", \"%s\", \"%s\")", 
				   album.toCString(), cddbid.toCString() );		
		}
	      else
		{
		  mgSqlWriteQuery( db, 
				   "INSERT INTO album (artist,title,cddbid) "
				   "VALUES (\"%s\", \"%s\", \"%s\")", 
				   artist.toCString(), album.toCString(), cddbid.toCString() );		
		}
	    }
	  else
	    { // use first album found as source id for the track
	      cddbid = escape_string(db,row[0]);
	    }
	}
      
      // update tracks table
      if( uid > 0 )
	{ // the entry is known to exist already, hence update it
	  
	  mgSqlWriteQuery( db,	"UPDATE tracks SET artist=\"%s\", title=\"%s\", year=\"%d\","
			   "sourceid=\"%s\", mp3file=\"%s\", length=%d, bitrate=\"%d\","
			   "samplerate=%d, channels=%d, genre1=\"%s\", lang=\"%s\" WHERE id=%d", 
			   artist.toCString(), title.toCString(), year, 
			   cddbid.toCString(), filename.c_str(), len, bitrate,
			   sample, channels, gid.toCString(), language.toCString(), uid );
	}
      else
	{ // the entry does not exist, create it
	  if (folderfields)
	  {
	    char *path = strdup(filename.c_str());
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
  	    TagLib::String f1 = escape_string( db, folder1 );
  	    TagLib::String f2 = escape_string( db, folder2 );
  	    TagLib::String f3 = escape_string( db, folder3 );
  	    TagLib::String f4 = escape_string( db, folder4 );
	    mgSqlWriteQuery( db,
			   "INSERT INTO tracks "
			   "(artist, title,  year,sourceid,tracknb,mp3file,length,bitrate,samplerate,channels,genre1,genre2,lang,folder1,folder2,folder3,folder4) VALUES"
			   "(\"%s\", \"%s\", %d,  \"%s\",  %d,     \"%s\", %d,    \"%d\", %d,        %d,      \"%s\",\"\",\"%s\","
			   	"\"%s\",\"%s\",\"%s\",\"%s\")",
			   artist.toCString(), title.toCString(), year, cddbid.toCString(), 
			   trackno, filename.c_str(), len, bitrate, sample, channels, gid.toCString(),
			   language.toCString(),f1.toCString(),f2.toCString(),f3.toCString(),f4.toCString());
	    free(path);
	  }
	  else
	    mgSqlWriteQuery( db,
			   "INSERT INTO tracks "
			   "(artist, title,  year,sourceid,tracknb,mp3file,length,bitrate,samplerate,channels,genre1,genre2,lang) VALUES"
			   "(\"%s\", \"%s\", %d,  \"%s\",  %d,     \"%s\", %d,    \"%d\", %d,        %d,      \"%s\",\"\",\"%s\")",
			   artist.toCString(), title.toCString(), year, cddbid.toCString(), 
			   trackno, filename.c_str(), len, bitrate, sample, channels, gid.toCString(),
			   language.toCString());

#ifdef VERBOSE
	    std::cout << "-- TAG --" << std::endl;
	    std::cout << "title   - '" << tag->title()   << "'" << std::endl;
	    std::cout << "artist  - '" << tag->artist()  << "'" << std::endl;
	    std::cout << "album   - '" << tag->album()   << "'" << std::endl;
	    std::cout << "year    - '" << tag->year()    << "'" << std::endl;
	    std::cout << "comment - '" << tag->comment() << "'" << std::endl;
	    std::cout << "track   - '" << tag->track()   << "'" << std::endl;
	    std::cout << "genre   - '" << tag->genre()   << "'" << std::endl;
	    std::cout << "language- '" << language   << "'" << std::endl;
#endif
	}
    }
}

void update_tags( long uid )
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  if( uid >= 0 )
    {
      result = mgSqlReadQuery( db, "SELECT artist,title,year,tracknb,mp3file,genre1,id FROM tracks where id=%d", uid );
    }
  else
    {
      result = mgSqlReadQuery( db, "SELECT artist,title,year,tracknb,mp3file,genre1,id FROM tracks" );
    }

  // loop all results
  char* cwd = getcwd( NULL, 0 );
  std::string wdir = std::string( cwd );
  free( cwd );

  struct stat *buf = (struct stat*) malloc( sizeof( struct stat ) );
  while(( row = mysql_fetch_row(result) ) != NULL )
    {

      std::string file   = wdir + "/" + std::string( row[4] );
      
      if( !stat( file.c_str(), buf ) )
	{
	  // set tags?
	  /*
	  std::string artist = row[0];
	  std::string title  = row[1];
	  int year           = atoi( row[2] );
	  int track          = atoi( row[3] );
	  std::string genre  = row[5];
	  */
        }
      else
        {
          if( delete_mode )
	    {
#ifdef VERBOSE
	      std::cout << "Deleting entry " << row[6] << " from database because the file no longer exists." << std::endl;
#endif
	      mgSqlWriteQuery( db, "DELETE FROM tracks where id=%s", row[6] );
	    }
        }
    }
  free( buf );
}

void evaluate_file( std::string filename )
{
  // is filename stored in database?
  long uid = find_file_in_database( db, filename );
  if( uid >= 0 )
    {	  
      // currently only update database, do not consider writing changes from the db back to tags
      /*
      // determine modification times in database and on filesystem
      time_t db_time = get_db_modification_time( uid );
      time_t fs_time = get_fs_modification_time( filename );
      
      if( db_time > fs_time )
      {
      // db is newer: update id3 tags from db
      update_tags( uid, filename );
      }
      else
      {
      // file is newer: update db from id3 tags
      update_db( uid, filename );
      }
      */
      
      update_db( uid, filename );
    }
  else
    {
      // not in db yet: import file
      update_db( -1, filename );
    }
}

int main( int argc, char *argv[] )
{
  if( mysql_server_init(sizeof(server_args) / sizeof(char *),
                        server_args, server_groups) )
    {
      exit(1);
    }
  std::string filename;
      
  if( argc < 2 )
    { // we need at least a filename!
      std::cout << "mugglei -- import helper for Muggle VDR plugin" << std::endl;
      std::cout << "(C) Lars von Wedel" << std::endl;
      std::cout << "This is free software; see the source for copying conditions." << std::endl;
      std::cout << "" << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  -h <hostname>       - specify host of mySql database server (default is 'localhost')" << std::endl;
      std::cout << "  -s <socket>         - specify a socket for mySQL communication (default is TCP)" << std::endl;
      std::cout << "  -n <database>       - specify database name (default is 'GiantDisc')" << std::endl;
      std::cout << "  -u <username>       - specify user of mySql database (default is empty)" << std::endl;
      std::cout << "  -p <password>       - specify password of user (default is empty password)" << std::endl;
      std::cout << "  -f <filename>       - name of music file to import or update" << std::endl;
      std::cout << "  -a                  - import track as if it was on an assorted album" << std::endl;
      std::cout << "  -z                  - scan all database entries and delete entries for files not found" << std::endl;
      std::cout << "  -c                  - create a new database entry deleting existing entries" << std::endl;

      exit( 1 );
    }

  // option defaults
  host   = "localhost";
  dbname = "GiantDisc";
  user   = "";
  pass   = "";
  sck = "";
  import_assorted = false;
  delete_mode = false;
  create_mode = false;
  filename = "";

  // parse command line options
  while( 1 )
    {
      int c = getopt(argc, argv, "h:u:p:n:af:s:z");

      if (c == -1)
	break;
      
      switch (c) 
	{
	case 0:
	  { // long option
	    
	  } break;
	case 'h':
	  {
	    host = optarg;
	  } break;
	case 'n':
	  {
	    dbname = optarg;
	  } break;
	case 'u':
	  {
	    user = optarg;
	  } break;
	case 'p':
	  {
	    pass = optarg;
	  } break;
	case 'a':
	  {
	    import_assorted = true;
	  } break;
	case 'f':
	  {
	    filename = optarg;
	  } break;
        case 's':
          {
            sck = optarg;
          } break;
        case 'z':
          {
            delete_mode = true;
          } break;
        case 'c':
          {
            create_mode = true;
          } break;
	}
    }

  if( filename.length() > 255 )
    {
      std::cerr << "Warning: length of file exceeds database field capacity: " << filename << std::endl;
    }

  // init random number generator
  struct timeval tv;
  struct timezone tz;
  gettimeofday( &tv, &tz );
  srandom( tv.tv_usec );  

  if( 0 == init_database() )
    {  
      init_folderfields();
      if( delete_mode )
	{
	  update_tags( -1 );
	}
      else if( create_mode )
	{
	  create_database();
	}
      if( filename.size() )
	{
	  evaluate_file( filename );
	}
    }

  mysql_server_end();

  return 0;
}

