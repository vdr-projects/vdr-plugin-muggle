
#include <string>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <mysql/mysql.h>

#include <getopt.h>

#include <tag.h>
#include <fileref.h>

#include "mg_tools.h"

MYSQL *db;

string host, user, pass, dbname;
bool import_assorted;

int init_database()
{
  db = mysql_init(NULL);
  if( db == NULL )
    {	
      return -1;
    }
  
  if( mysql_real_connect( db, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(),
			  0, NULL, 0 ) == NULL )
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

TagLib::String escape_string( MYSQL *db, TagLib::String s )
{
  char *buf = strdup( s.toCString() );
  char *escbuf = (char *) malloc( 2*strlen( buf ) + 1 );

  int len = mysql_real_escape_string( db, escbuf, buf, strlen( buf ) );

  return TagLib::String( escbuf );
}

long find_file_in_database( MYSQL *db, string filename )
{
  TagLib::String file = TagLib::String( filename.c_str() );
  file = escape_string( db, file );

  MYSQL_RES *result = mgSqlReadQuery( db, "SELECT id FROM tracks WHERE mp3file=\"%s\"", file.toCString() );
  MYSQL_ROW row = mysql_fetch_row( result );

  // obtain ID and return
  return atol( row[0] );
}

// read tags from the mp3 file and store them into the corresponding database entry
void update_db( long uid, string filename )
{
  TagLib::String title, album, artist, genre, cddbid;
  uint trackno, year;

  //  ID3_Tag filetag( filename.c_str() );
  TagLib::FileRef f( filename.c_str() );

  if( !f.isNull() && f.tag() ) 
    {
      //      cout << "Evaluating " << filename << endl;
      TagLib::Tag *tag = f.tag();

      // obtain tag information
      title   = tag->title();
      album   = tag->album();
      year    = tag->year();
      artist  = tag->artist();
      trackno = tag->track();
      genre   = tag->genre();

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
	      asprintf( &buf, "%d-%s", id, tag->artist().toCString() );
	      cddbid = TagLib::String( buf ).substr( 0, 20 );
	      cddbid = escape_string( db, cddbid );
	      free( buf );
	  
	      mgSqlWriteQuery( db, "INSERT INTO album (artist,title,cddbid) VALUES (\"%s\", \"Unassigned\", \"%s\")", artist.toCString(), cddbid.toCString() );
	    }
	  else
	    { // use first album found as source id for the track
	      cddbid = row[0];
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
	      asprintf( &buf, "%d-%s", id, tag->album().toCString() );
	      cddbid = TagLib::String( buf ).substr( 0, 20 );
	      cddbid = escape_string( db, cddbid );
	      free( buf );

	      if( import_assorted )
		{ // in this case, the album author is "Various Artists"
		  mgSqlWriteQuery( db, "INSERT INTO album (artist,title,cddbid) VALUES (\"Various Artists\", \"%s\", \"%s\")", album.toCString(), cddbid.toCString() );		}
	      else
		{
		  mgSqlWriteQuery( db, "INSERT INTO album (artist,title,cddbid) VALUES (\"%s\", \"%s\", \"%s\")", artist.toCString(), album.toCString(), cddbid.toCString() );		}
	    }
	  else
	    { // use first album found as source id for the track
	      cddbid = row[0];
	    }
	}
      
     // update tracks table
      if( uid > 0 )
	{ // the entry is known to exist already, hence update it

	  mgSqlWriteQuery( db,	"UPDATE tracks SET artist=\"%s\", title=\"%s\", year=\"%s\","
							"sourceid=\"%s\", mp3file=\"%s\", length=%d, bitrate=\"%d\","
							"samplerate=%d, channels=%d WHERE id=%d", 
							artist.toCString(), title.toCString(), year, 
							cddbid.toCString(), filename.c_str(), len, bitrate,
							sample, channels, uid );
	}
      else
	{ // the entry does not exist, create it
		mgSqlWriteQuery( db,"INSERT INTO tracks (artist,title,genre1,genre2,year,"
							"sourceid,tracknb,mp3file,length,bitrate,samplerate,channels)"
							" VALUES (\"%s\", \"%s\", \"\", \"\", %d, \"%s\", %d, \"%s\", %d, \"%d\", %d, %d)",
							artist.toCString(), title.toCString(), year, cddbid.toCString(), 
							trackno, filename.c_str(), len, bitrate, sample, channels );
	  /*
	  cout << "-- TAG --" << endl;
	  cout << "title   - \"" << tag->title()   << "\"" << endl;
	  cout << "artist  - \"" << tag->artist()  << "\"" << endl;
	  cout << "album   - \"" << tag->album()   << "\"" << endl;
	  cout << "year    - \"" << tag->year()    << "\"" << endl;
	  cout << "comment - \"" << tag->comment() << "\"" << endl;
	  cout << "track   - \"" << tag->track()   << "\"" << endl;
	  cout << "genre   - \"" << tag->genre()   << "\"" << endl;
	  */
	}
    }
}

void update_tags( long uid, string filename )
{
  
}

void evaluate_file( MYSQL *db, string filename )
{
  if( 0 == init_database() )
    {  
      // is filename stored in database?
      long uid = find_file_in_database( db, filename );
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
  int option_index;
  string filename;

  if( argc < 2 )
    { // we need at least a filename!
      cout << "mugglei -- import helper for Muggle VDR plugin" << endl;
      cout << "(C) Lars von Wedel" << endl;
      cout << "This is free software; see the source for copying conditions." << endl;
      cout << "" << endl;
      cout << "Options:" << endl;
      cout << "  -h <hostname>       - specify host of mySql database server (default is 'localhost')" << endl;
      cout << "  -n <database>       - specify database name (default is 'GiantDisc')" << endl;
      cout << "  -u <username>       - specify user of mySql database (default is empty)" << endl;
      cout << "  -p <password>       - specify password of user (default is empty password)" << endl;
      cout << "  -f <filename>       - name of music file to import" << endl;
      cout << "  -a                  - import track as if it was on an assorted album" << endl;

      exit( 1 );
    }

  // option defaults
  host   = "localhost";
  dbname = "GiantDisc";
  user   = "";
  pass   = "";
  import_assorted = false;

  // parse command line options
  while( 1 )
    {
      int c = getopt(argc, argv, "h:u:p:n:af:");

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
	case 'u':
	  {
	    user = optarg;
	  } break;
	case 'p':
	  {
	    pass = optarg;
	  } break;
	case 'd':
	  {
	    dbname = optarg;
	  } break;
	case 'a':
	  {
	    import_assorted = true;
	  } break;
	case 'f':
	  {
	    filename = optarg;
	  } break;
	}
    }

  // init random number generator
  struct timeval tv;
  struct timezone tz;
  gettimeofday( &tv, &tz );
  srandom( tv.tv_usec );  

  int res = init_database();

  if( !res )
    {
      update_db( 0, filename );
    }
  else
    {
      cout << "Database initialization failed. Exiting.\n" << endl;
    }
  
  return res;
}

