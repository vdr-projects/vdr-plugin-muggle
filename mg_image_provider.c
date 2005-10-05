
#include "mg_image_provider.h"

#include "mg_item_gd.h"
#include "mg_tools.h"
#include "mg_setup.h"

#include <id3v2tag.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <flacfile.h>
#include <vorbisfile.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <tbytevector.h>

#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/types.h>

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

std::string mgImageProvider::getImagePath()
{
  string fname;

  // check, how many images are converted at all
  Lock();
  if( m_image_index < m_converted_images.size() )
    {
      fname = m_converted_images[ m_image_index ];
      
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

  fillImageList( dir );
  Start();
}

mgImageProvider::mgImageProvider( )
{
  m_mode = IM_ITEM_DIR;
  m_image_index = 0;
}

mgImageProvider* mgImageProvider::Create( string dir )
{
  return new mgImageProvider( dir );
}

mgImageProvider* mgImageProvider::Create( )
{
  return new mgImageProvider();
}

void mgImageProvider::updateItem( mgItemGd *item )
{
  // clean up stuff from previous item ?

  string filename = item->getSourceFile();

  if( m_mode == IM_ITEM_DIR )
    {
      m_image_list.clear();
      // clear temporary image directory
      
      // check whether the item has cover images in tags?
      string dir = extractImagesFromTag( filename );
      if( dir == "" )
	{
	  // no images in tags, find images in the directory itself
	  dir = dirname( (char *) (item->getSourceFile().c_str()) );
	}

      // finally put all image filenames here
      fillImageList( dir );
      
      // start a thread to convert all images in 'dir into .mpg format in the background
      Start();

      m_image_index = 0;
    }
  // else: nothing todo when changing the item currently being played
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

	  cout << "Converting " << filename << " to " << tmpFile << endl << flush;

	  char *tmp;
	  asprintf( &tmp, "image_convert.sh \"%s\" \"%s\"", filename.c_str(), tmpFile.c_str() );
	  system( (const char*) tmp );
	  delete tmp;

	  // add to the list of converted images
	  Lock();
	  m_converted_images.push_back( tmpFile );
	  Unlock();
	}

      // Check whether we need to continue this!? Next song may be playing already...
    }
  cout << "Image conversion thread ending." << endl << flush;
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
	  cout << "Adding to image list " << fname << endl << flush;
	  m_image_list.push_back( fname );

	  free( files[i] );
	}
      free( files );
    }
  else
    {
      m_image_list.push_back( string(the_setup.ToplevelDir) + "/cover.jpg" );
    }
}

string mgImageProvider::extractImagesFromTag( string f )
{
  // currently not working due to bug #113770 in taglib
  // see http://bugs.kde.org/show_bug.cgi?id=113770
  return "";
  
  TagLib::ID3v2::FrameList l;
  const char *filename = f.c_str();

  if( !strcasecmp(extension(filename), "flac") )
    {
      TagLib::FLAC::File f(filename);
      l = f.ID3v2Tag()->frameListMap()["APIC"];
    }
  else if ( !strcasecmp(extension(filename), "mp3") )
    {
      TagLib::MPEG::File f(filename);
      l = f.ID3v2Tag()->frameListMap()["APIC"];
    }
  else if( !strcasecmp(extension(filename), "ogg") )
    {
      TagLib::Vorbis::File f(filename);
      // l = f.ID3v2Tag()->frameListMap()["APIC"];
    }
  
  string image_cache = string( the_setup.ImageCacheDir ); // TODO: obtain from setup!
  if( !l.isEmpty() )
    {
      TagLib::ID3v2::FrameList::ConstIterator it = l.begin();
      int num = 0;
      for(; it != l.end(); it++)
	{
	  TagLib::ID3v2::AttachedPictureFrame *picframe = (TagLib::ID3v2::AttachedPictureFrame*) (*it);
	  TagLib::ByteVector image = picframe->picture();
	  char* image_data = image.data();
	  
	  // save image_data to temporary file  
	  char *buf;
	  asprintf( &buf, "%s/image-%d.jpg", image_cache.c_str(), num );
	  FILE *f = fopen( buf, "w+" );
	  fprintf( f, "%s", image_data );
	  fclose( f );
	  free( buf );
	    
	  num ++;
	}
      return image_cache;
    }
  else
    {
      return "";
    }
}

string mgImageProvider::getDefaultImage()
{
  return string(the_setup.ToplevelDir) + "/cover.jpg";
}
