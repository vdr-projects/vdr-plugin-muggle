#include <string>
using namespace std;

#include <id3.h/tag.h>
#include <mysql/mysql.h>

MYSQL *db;

int init_database()
{
  if( mysql_init(db) == NULL )
    {	
      return -1;
    }
  
  if( mysql_real_connect( db, "host", "user", "pass", 
			  "dbname", 0, NULL, 0 ) == NULL )
    {
      return -2;
    }
  
  return 0;
}

time_t get_fs_modification_time( string filename )
{
  struct stat *buf = malloc( sizeof( struct stat ) );
  
  //    yes: obtain modification date for file and db entry
  int res = stat( filename.c_str(), buf );

  time_t res = buf->st_mtime;
  free( buf );

  return res;
}

time_t get_db_modification_time( long uid )
{
  char *query;
  time_t mt;

  asprintf( &query, "SELECT modification_time FROM tracks WHERE id='%d'", uid );

  if( mysql_real_query( &db, query, strlen( query ) ) )
    { 
      printf( mysql_error(&db) );
      exit(1);
    }
  else
    {
      MYSQL_RES *result = mysql_store_result( db );
      MYSQL_ROW row     = mysql_fetch_row( result );

      string mod_time = row[0];
      mt = (time_t) atol( mod_time.c_str() );
    }  

  free( query );
  return mt;
}

long find_file_in_database( string filename )
{
  char *query;

  asprintf( &query, "SELECT id FROM tracks WHERE mp3file='%s'", filename.c_str() );

  if( mysql_real_query( &demo_db, query, strlen( query ) ) )
    { 
      printf( mysql_error(&demo_db) );
      exit(1);
    }
  else
    {
      MYSQL_RES *result = mysql_store_result( db );
      row = mysql_fetch_row( result );

      // obtain ID and return
      return atol( row[0] );
    }

  free( query );
}

void evaluate_file( string filename )
{
  if( 0 == init_database() )
    {  
      // is filename stored in database?
      long uid = find_file_in_database( filename );
      if( uid >= 0 )
	{	  
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
	}
      else
	{
	  // not in db yet: import file
	  import_file( filename );
	}
    }
}

void update_db( long uid, string filename )
{
  char title[1024], album[1024];
  ID3_Tag filetag( filename.c_str() );

  // obtain album value
  ID3_Frame* album_frame = filetag.Find( ID3FID_ALBUM );
  if( NULL != album_frame )
    {      
      album_frame->Field ( ID3FN_TEXT ).Get ( album, 1024 );
    }
  
  // obtain title value
  ID3_Frame* title_frame = filetag.Find( ID3FID_TITLE );
  if( NULL != title_frame )
    {      
      title_frame->Field ( ID3FN_TEXT ).Get ( title, 1024 ); 
    }

  // obtain year value (ID3FID_YEAR)

  // obtain artist value (ID3FID_LEADARTIST)

  // see what else may be needed later

  // finally update the database

  // update tracks table
  mgSqlWriteQuery( db, 
		   "UPDATE tracks "
		   "SET artist=\"%s\", title=\"%s\", year=%d, rating=%d "
		   "WHERE id=%d", 
		   artist, title,
		   m_year, m_rating, uid );

  // obtain associated album or create
  
  
}

void update_tags( long uid, string filename )
{
  
}

void import_file( string filename )
{

}

void main( int argc, char *argv[] )
{
  
}
