#include <string>
using namespace std;

#include <stdlib.h>

#include <sys/stat.h>
#include <id3/tag.h>
#include <mysql/mysql.h>

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

  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT modification_time FROM tracks WHERE id='%d'", uid );
  MYSQL_ROW row     = mysql_fetch_row( result );
  
  string mod_time = row[0];
  mt = (time_t) atol( mod_time.c_str() );

  return mt;
}

long find_file_in_database( string filename )
{
  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT id FROM tracks WHERE mp3file='%s'", filename.c_str() );
  MYSQL_ROW row = mysql_fetch_row( result );

  // obtain ID and return
  return atol( row[0] );
}

// read tags from the mp3 file and store them into the corresponding database entry
void update_db( long uid, string filename )
{
  char title[1024], album[1024], year[5], artist[1024], cddbid[20], trackno[5];
  ID3_Tag filetag( filename.c_str() );

  printf( "ID3 tag created.\n" );  

  // obtain album value
  ID3_Frame* album_frame = filetag.Find( ID3FID_ALBUM );
  printf( "Album frame obtained.\n" );  
  if( NULL != album_frame )
    {      
      album_frame->Field( ID3FN_TEXT ).Get( album, 1024 );
      printf( "Field obtained: %s\n", album );  
    }
  else
    {
      printf( "No album info found.\n" );  
      strncpy( album, "Unassigned", 1023 );
    }

  printf( "Album frame evaluated.\n" );  

  // obtain title value
  ID3_Frame* title_frame = filetag.Find( ID3FID_TITLE );
  if( NULL != title_frame )
    {      
      title_frame->Field ( ID3FN_TEXT ).Get ( title, 1024 ); 
      printf( "Field obtained: %s\n", title);  
    }
  else
    {
      printf( "No title info found.\n" );  
      strncpy( title, "Unknown title", 1023);
    }

  printf( "Title frame evaluated.\n" );  

  // obtain year value (ID3FID_YEAR)
  ID3_Frame* year_frame = filetag.Find( ID3FID_YEAR );
  if( NULL != year_frame )
    {      
      year_frame->Field ( ID3FN_TEXT ).Get ( year, 5 ); 
      printf( "Field obtained: %s\n", year );  
    }
  else
    {
      printf( "No year info found.\n" );  
      strncpy( title, "0", 1023);
    }

  printf( "Year frame evaluated.\n" );  

  // obtain artist value (ID3FID_LEADARTIST)
  ID3_Frame* artist_frame = filetag.Find( ID3FID_LEADARTIST );
  if( NULL != artist_frame )
    {      
      artist_frame->Field ( ID3FN_TEXT ).Get ( artist, 1023 ); 
      printf( "Field obtained: \n" );  
    }
  else
    {
      printf( "No artist info found.\n" );  
      strncpy( artist, "Unknown artist", 1023);
    }

  printf( "Artist frame evaluated.\n" );  

  // obtain track number ID3FID_TRACKNUM
  ID3_Frame* trackno_frame = filetag.Find( ID3FID_TRACKNUM );
  if( NULL != trackno_frame )
    {      
      trackno_frame->Field ( ID3FN_TEXT ).Get ( trackno, 5 ); 
      printf( "Field obtained: %s\n", trackno );  
    }
  else
    {
      strncpy( trackno, "0", 5);
    }

  printf( "Trackno frame evaluated.\n" );  

  printf( "ID3 frames/fields read.\n" );  

  // obtain associated album or create
  if( NULL == album_frame )
    { // no album found, associate with default album
      strcpy( cddbid, "0000unknown0000" );
    }
  else
    { // album tag found, associate or create
      MYSQL_RES *result = mgSqlReadQuery( db, "SELECT cddbid FROM album WHERE title='%s' AND artist='%s'", 
					  album, artist );
      MYSQL_ROW row = mysql_fetch_row( result );
      printf( "\nAlbum query set.\n" );  

      // num rows == 0 ?
      int nrows   = mysql_num_rows(result);
      if( nrows == 0 )
	{
	  // create new album entry 
	  long id = random();
	  snprintf( cddbid, 19, "%d-%s", id, album );      
	  
	  mgSqlWriteQuery( db, "INSERT INTO album (artist,title,cddbid) VALUES ('%s', '%s', '%s')", artist, title, cddbid );
	}
      else
	{ // use first album found as source id for the track
	  strncpy( cddbid, row[0], 19 );
	}
    }
  
  // TODO: genre(s), CD identifier (?), playcounter, popularimeter?, volume adjustment

  // finally update the database

  // update tracks table
  if( uid > 0 )
    { // the entry is known to exist already, hence update it
      mgSqlWriteQuery( db, "UPDATE tracks SET artist='%s', title='%s', year='%s', sourceid='%s'"
		       "WHERE id=%d", artist, title, year, cddbid, uid );
    }
  else
    { // the entry does not exist, create it
      mgSqlWriteQuery( db, "INSERT INTO tracks (artist,title,year,sourceid,tracknb,mp3file) VALUES ('%s', '%s', '%s', '%s', '%s', '%s')", artist, title, year, cddbid, trackno, filename.c_str() );
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
  /*
  host   = "134.130.124.222";
  user   = "root";
  dbname = "giantdisc";
  pass   = NULL;
  */
  host   = "localhost";
  user   = "vdr";
  dbname = "GiantDisc";
  pass   = NULL;

  int res = init_database();

  printf( "Database initialized.\n" );

  if( !res )
    {
      printf( "Evaluating %s\n", argv[1] );
      update_db( 0, string( argv[1] ) );
    }
  else
    {
      printf( "Database initialization failed. Exiting.\n" );
    }

  return res;
}

