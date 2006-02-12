
#include "mg_image_provider.h"

#include "mg_item_gd.h"
#include "mg_tools.h"
#include "mg_setup.h"

#include <id3v2header.h>
#include <flacfile.h>
#include <vorbisfile.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>

#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include <fileref.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fts.h>
#include <regex.h>

using namespace std;

extern int alphasort();

const int FALSE = 0;
const int TRUE =  1;

int picture_select( struct dirent const *entry )
{
  if( (strcmp(entry->d_name, ".") == 0) ||
      (strcmp(entry->d_name, "..") == 0) )
    {
      return (FALSE);
    }
  else
    {
      char *ext = rindex( entry->d_name, '.' );

      if( ext )
	{
	  if( !strcmp( ext, ".jpg" ) || !strcmp( ext, ".png" ) || !strcmp( ext, ".JPG" ) || !strcmp( ext, ".PNG" ) )
	    {
	      return (TRUE);
	    }
	  else
	    {
	      return ( FALSE );
	    }
	}
      else
	{
	  return (FALSE);
	}
    }
}

std::string mgImageProvider::getImagePath( string &source )
{
  string fname;

  // check, how many images are converted at all
  Lock();
  if( m_image_index < m_converted_images.size() )
    {
      fname = m_converted_images[ m_image_index ];
      source = m_image_list[ m_image_index ];
      // wrap to beginning of list when all images are displayed
      m_image_index += 1;
      if( m_image_index >= m_converted_images.size() )
	{
	  m_image_index = 0;
	}
    }
  Unlock();

  return fname;
}

mgImageProvider::mgImageProvider( string dir )
{
  // iterate all files in dir and put them into m_image_list
  m_mode = IM_PLAYLIST;
  m_image_index = 0;
  m_delete_imgs_from_tag = false;

  fillImageList( dir );
  Start();
}

mgImageProvider::mgImageProvider( )
{
  m_mode = IM_ITEM_DIR;
  m_image_index = 0;
  m_delete_imgs_from_tag = false;
}

mgImageProvider::~mgImageProvider()
{
  deleteTemporaryImages();
}

void mgImageProvider::deleteTemporaryImages()
{
  if( m_delete_imgs_from_tag )
    {
      for( vector<string>::iterator iter = m_image_list.begin(); iter != m_image_list.end(); iter ++ )
	{
	  // remove( (*iter).c_str() );
	  cout << "Removing " << *iter << endl;
	}
      m_delete_imgs_from_tag = false;
    }
  m_image_list.clear();

  for( vector<string>::iterator iter = m_converted_images.begin(); iter != m_converted_images.end(); iter ++ )
    {
      // remove( (*iter).c_str() );
      cout << "Removing " << *iter << endl;
    }  
  m_converted_images.clear();
}

mgImageProvider* mgImageProvider::Create( string dir )
{
  return new mgImageProvider( dir );
}

mgImageProvider* mgImageProvider::Create( )
{
  return new mgImageProvider();
}

bool mgImageProvider::extractImagesFromDatabase( mgItemGd *item )
{
  /* GD entry looks like: img0x00004810-00.jpg
     and may reside in $ToplevelDir/[0-9]{2}/

     Then, all img0x00004810-??.jpg are possible matches, but not
     files such as img0x00004810-00-s200.jpg which are optimized
     for a different resoluition.     
     
     Thanks to viking (from vdrportal.de) for help with this
  */
  string file = item->getImagePath();

  if( file == "" )
    {
      return false;
    }

  bool result = false;

  char *dir[2];
  dir[0] = the_setup.ToplevelDir;
  dir[1] = NULL;
  
  FTS *fts = fts_open( dir, FTS_LOGICAL, 0);
  if( fts )
    {
      FTSENT *ftsent;
      regex_t path_rex;
      
      if( regcomp( &path_rex, "[0-9][0-9]", REG_NOSUB ) )
	{
	  mgDebug( 1, "mgImageProvider::extractImagesFromDatabase: Error compiling dir regex. Not using GD images." );
	  return false;
	}
      
      while( (ftsent = fts_read(fts)) != NULL )
	{
	  mode_t mode = ftsent->fts_statp->st_mode;
	  if( mode & S_IFDIR && ftsent->fts_info & FTS_D )
	    {
	      // a directory -- check whether name is [0-9][0-9] (GD Dir), skip otherwise
	      if( !regexec( &path_rex, ftsent->fts_name, 0, NULL, 0 ) )
		{
		  fts_set( fts, ftsent, FTS_SKIP );
		  mgDebug(1,"Skipping directory %s",ftsent->fts_path);
		}
	    }
	  if( !( mode & S_IFREG ) )
	    {
	      continue;
	    }
	  if( ftsent->fts_info & FTS_F )
	    {
	      // check against GD filename also
	      if( !strcmp( ftsent->fts_name, file.c_str() ) )
		{
		  if( access(ftsent->fts_path, R_OK) )
		    {
		      mgDebug( 1, "Ignoring unreadable file %s",
			       ftsent->fts_path);
		      continue;
		    }
		  
		  m_image_list.push_back( string( ftsent->fts_path ) );
		  mgDebug( 1, "Found image %s", ftsent->fts_path );
		  
		  result = true;
		}
	    }
	}
      fts_close(fts);
      regfree( &path_rex );
    }  
  return result;
}

void mgImageProvider::updateFromItemDirectory( mgItemGd *item )
{
  // no images in tags, find images in the directory of the file itself
  string dir = dirname( (char *) (item->getSourceFile().c_str()) );
  
  // do something, when there are no images here, either:
  // simply go up one step in the directory hierarchy, until we reach top level directory      
  bool toplevel_reached = false;
  while( !m_image_list.size() && !toplevel_reached )
    {
      if( samedir( dir.c_str(), the_setup.ToplevelDir ) )
	{
	  toplevel_reached = true;
	}
      
      fillImageList( dir );
      
      if( !m_image_list.size() )
	{
	  // nothing found, turn up one directory level
	  dir = dirname( (char *)dir.c_str() );
	}
    }
}

bool mgImageProvider::updateItem( mgItemGd *item )
{
  // clean up stuff from previous item ?

  string filename = item->getSourceFile();

  if( m_mode == IM_ITEM_DIR )
    { // do not try to acquire new images when we are playing back a separate directory      
      deleteTemporaryImages();
      
      // check whether the item has cover images defined in the database ?
      bool has_db_images = extractImagesFromDatabase( item );

      if( !has_db_images )
	{
	  // no. check whether the item has cover images defined in tags ?
	  string dir = extractImagesFromTag( filename );

	  if( dir == "" )
	    {
	      // no. use anything from the directory
	      updateFromItemDirectory( item );
	    }
	  else
	    {
	      fillImageList( dir );
	    }
	}		

      // start a thread to convert all images in 'dir' into .mpg format in the background
      Start();
      
      m_image_index = 0;
    }
  // else: nothing todo when changing the item currently being played

  Lock();
  bool result = m_image_list.size() > 0;
  Unlock();

  return result;
}

void mgImageProvider::Action()
{
  // convert all images
  Lock();
  vector<string> images( m_image_list );
  Unlock();

  for( unsigned i=0; i < images.size(); i++ )
    {
      FILE *fp;
      string filename = images[i];
      
      if( (fp = fopen( filename.c_str(), "rb" )) )
	{
	  // filename can be opened
	  fclose (fp);
	  
	  // determine the filename of a (to be) cached .mpg file
	  string bname = basename( (char *)filename.c_str() );
	  unsigned dotpos = bname.rfind( ".", filename.length() );
	  
	  // assemble path from relative paths
	  string tmpFile = string( the_setup.ImageCacheDir ) + string( "/" ) + bname.substr( 0, dotpos ) + string( ".mpg" );

	  // cout << "Converting " << filename << " to " << tmpFile << endl << flush;

	  char *tmp;
	  asprintf( &tmp, "image_convert.sh \"%s\" \"%s\"", filename.c_str(), tmpFile.c_str() );
	  system( (const char*) tmp );
	  free(tmp);
	 
	  // if file can be read by process, add to the list of converted images
	  if( !access( tmpFile.c_str(), R_OK ) )
	    {
	      Lock();
	      m_converted_images.push_back( tmpFile );
	      Unlock();
	    }
	}

      // Check whether we need to continue this!? Next song may be playing already...
    }
  // cout << "Image conversion thread ending." << endl << flush;
}

void mgImageProvider::fillImageList( string dir )
{
  // obtain all .png, .jpg in dir and gather them in m_image_list  

  struct dirent **files;
  int count = scandir( dir.c_str(), &files, picture_select, alphasort );

  if( count )
    {
      for ( int i=0; i < count; i++ )
	{
	  string fname = dir + "/" + string( files[i]->d_name );
	  m_image_list.push_back( fname );
	  // cout << "Added " << fname << endl;
	  free( files[i] );
	}
      free( files );
    }
}

void mgImageProvider::writeImage( TagLib::ByteVector &image, int num, string &image_cache )
{
  char* image_data = image.data();
  int len = image.size();
  
  // save image_data to temporary file  
  char *buf;
  asprintf( &buf, "%s/image-%d.jpg", image_cache.c_str(), num );

  FILE *f = fopen( buf, "w+" );
  fwrite( image_data, sizeof(char), len, f );
  fclose( f );
  free( buf );
}

string mgImageProvider::treatFrameList( TagLib::ID3v2::FrameList &l, string &image_cache )
{
  string result;

  if( !l.isEmpty() )
    {
      m_delete_imgs_from_tag = true;
      TagLib::ID3v2::FrameList::ConstIterator it = l.begin();
  
      int num = 0;
      while( !(it == l.end()) )
	{
	  TagLib::ID3v2::AttachedPictureFrame *picframe = (TagLib::ID3v2::AttachedPictureFrame*) (*it);

	  TagLib::ByteVector image = picframe->picture();
	  writeImage( image, num, image_cache );
	  
	  it++;
	  num++;
	}
      result = image_cache;
    }
  else
    {
      result = "";
    }
  
  return result;
}

string mgImageProvider::extractImagesFromTag( string f )
{
  TagLib::ID3v2::FrameList l;
  const char *filename = f.c_str();
  string image_cache = string( the_setup.ImageCacheDir );
  string dir = "";
  
  if( !strcasecmp(extension(filename), "flac") )
    {
      TagLib::FLAC::File f(filename);
      if (f.ID3v2Tag())
        {
          l = f.ID3v2Tag()->frameListMap()["APIC"];
          dir = treatFrameList( l, image_cache );
        }
    }
  else if( !strcasecmp(extension(filename), "mp3") )
    {
      TagLib::MPEG::File f(filename);
      if (f.ID3v2Tag())
        {
          l = f.ID3v2Tag()->frameListMap()["APIC"];
          dir = treatFrameList( l, image_cache );
        }
    }
  else if( !strcasecmp(extension(filename), "ogg") )
    {
      // what to do here?

      TagLib::Vorbis::File f(filename);
      /*
      if (f.ID3v2Tag())
        {
          l = f.ID3v2Tag()->frameListMap()["APIC"];
          dir = treatFrameList( l, image_cache );
        }
      */
    }

  // returns empty if no images were found in tags
  return dir;
}

