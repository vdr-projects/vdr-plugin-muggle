
#include <string>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <mysql/mysql.h>

#include <taglib/tag.h>
#include <taglib/fileref.h>

#include "mg_tools.h"

MYSQL *db;

char *host, *user, *pass, *dbname;

int init_database()
{
  db = mysql_init(NULL);
  if( db == NULL )
    {	
      return -1;
    }
  
  if( mysql_real_connect( db, host, user, pass, 
			  dbname, 0, NULL, 0 ) == NULL )
    {
      return -2;
    }
  
  return 0;
}

time_t get_fs_modification_time( string filename )
{
  struct stat *buf = (struct stat*) malloc( sizeof( struct stat ) );
  
  // yes: obtain modification date for file and db entry
  int statres = stat( filename.c_str(), buf );

  time_t mod = buf->st_mtime;
  free( buf );

  return mod;
}

time_t get_db_modification_time( long uid )
{
  time_t mt;

  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT modification_time FROM tracks WHERE id=\"%d\"", uid );
  MYSQL_ROW row     = mysql_fetch_row( result );
  
  string mod_time = row[0];
  mt = (time_t) atol( mod_time.c_str() );

  return mt;
}

long find_file_in_database( string filename )
{
  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT id FROM tracks WHERE mp3file=\"%s\"", filename.c_str() );
  MYSQL_ROW row = mysql_fetch_row( result );

  // obtain ID and return
  return atol( row[0] );
}

// read tags from the mp3 file and store them into the corresponding database entry
void update_db( long uid, string filename )
{
  // char title[1024], album[1024], year[5], artist[1024], cddbid[20], trackno[5];
  TagLib::String title, album, artist, genre;
  uint trackno, year;
  string cddbid;

  //  ID3_Tag filetag( filename.c_str() );
  TagLib::FileRef f( filename.c_str() );

  if( !f.isNull() && f.tag() ) 
    {
      cout << "Evaluating " << filename << endl;
      TagLib::Tag *tag = f.tag();

      // obtain tag information
      title   = tag->title();
      album   = tag->album();
      year    = tag->year();
      artist  = tag->artist();
      trackno = tag->track();
      genre   = tag->genre();
            
      // TODO: CD identifier (?), playcounter, popularimeter?, volume adjustment
      
      // finally update the database

      // obtain associated album or create
      if( album == "" )
	{ // no album found, associate with default album
	  cddbid = "0000unknown0000";
	}
      else
	{ // album tag found, associate or create
	  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT cddbid FROM album WHERE title=\"%s\" AND artist=\"%s\"", 
					      album.toCString(), artist.toCString() );
	  MYSQL_ROW row = mysql_fetch_row( result );

	  // num rows == 0 ?
	  int nrows   = mysql_num_rows(result);
	  if( nrows == 0 )
	    {
	      // create new album entry 
	      long id = random();
	      char *buf;
	      asprintf( &buf, "%d-%s", id, album.toCString() );
	      cddbid = buf;
	      free( buf );
	  
	      mgSqlWriteQuery( db, "INSERT INTO album (artist,title,cddbid) VALUES (\"%s\", \"%s\", \"%s\")", artist.toCString(), album.toCString(), cddbid.c_str() );
	    }
	  else
	    { // use first album found as source id for the track
	      cddbid = row[0];
	    }
	}
      
      // update tracks table
      if( uid > 0 )
	{ // the entry is known to exist already, hence update it
	  mgSqlWriteQuery( db, "UPDATE tracks SET artist=\"%s\", title=\"%s\", year=\"%s\", sourceid=\"%s\", mp3file=\"%s\""
			   "WHERE id=%d", artist.toCString(), title.toCString(), year, cddbid.c_str(), filename.c_str(), uid );
	}
      else
	{ // the entry does not exist, create it
	  // int t = title.find( "'" );
	  // int a = artist.find( "'" );
	      mgSqlWriteQuery( db, 
			       "INSERT INTO tracks (artist,title,year,sourceid,tracknb,mp3file)"
			       " VALUES (\"%s\", \"%s\", %d, \"%s\", %d, \"%s\")",
			       artist.toCString(), title.toCString(), year, cddbid.c_str(), trackno, filename.c_str() );
	      /*
      	  if( !( t > 0 || a > 0 ) )
	    {
	    }
	  else
	    {
	      cout << filename << " skipped." << endl;
	      cout << "-- TAG --" << endl;
	      cout << "title   - \"" << tag->title()   << "\"" << endl;
	      cout << "artist  - \"" << tag->artist()  << "\"" << endl;
	      cout << "album   - \"" << tag->album()   << "\"" << endl;
	      cout << "year    - \"" << tag->year()    << "\"" << endl;
	      cout << "comment - \"" << tag->comment() << "\"" << endl;
	      cout << "track   - \"" << tag->track()   << "\"" << endl;
	      cout << "genre   - \"" << tag->genre()   << "\"" << endl;
	    }
	      */
	}
    }
}

void update_tags( long uid, string filename )
{
  
}

void evaluate_file( string filename )
{
  if( 0 == init_database() )
    {  
      // is filename stored in database?
      long uid = find_file_in_database( filename );
      if( uid >= 0 )
	{	  
	  // currently only update database, do not consider writing changes from the db back
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
}

int main( int argc, char *argv[] )
{
  host   = "134.130.124.222";
  user   = "root";
  dbname = "giantdisc";
  pass   = NULL;
  /*
  host   = "localhost";
  user   = "vdr";
  dbname = "GiantDisc";
  pass   = NULL;
  */

  int res = init_database();

  if( !res )
    {
      update_db( 0, string( argv[1] ) );
    }
  else
    {
      printf( "Database initialization failed. Exiting.\n" );
    }

  return res;
}

