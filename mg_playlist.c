/*! 
 * \file   mg_playlist.c
 * \brief  defines functions to be executed on playlists for the vdr muggle plugin
 *
 * \version $Revision: 1.6 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * This file implements the class mgPlaylist which maintains a playlist
 * and supports editing (e.g. adding or moving tracks), navigating it
 * (e.g. obtaining arbitrary items or accessing them sequentially) or
 * making it persistent in some database.
 */

#include <stdio.h>
#include "mg_playlist.h"
#include "mg_tools.h"

#include <vector>
#include <iostream>


/* ==== constructors ==== */

mgPlaylist::mgPlaylist()
{
  m_current_idx = -1;

  char *buffer;
  asprintf( &buffer, "Playlist-%ld", random() );
  
  m_listname = buffer;
}

mgPlaylist::mgPlaylist(std::string listname)
{
  m_current_idx = -1;
  m_listname = listname;
}
     
/* ==== destructor ==== */

mgPlaylist::~mgPlaylist()
{
}

void mgPlaylist::toggleShuffle()
{
}

void mgPlaylist::toggleLoop()
{
}

void mgPlaylist::initialize()
{
  m_current_idx = 0;
}

/* ==== add/remove tracks ==== */

/* adds a song at the end of the playlist */
void mgPlaylist::append(mgContentItem* item)
{
  m_list.push_back(item);
}

/* append a list of tracks at the end of the playlist */
void mgPlaylist::appendList( std::vector<mgContentItem*> *tracks )
{
  std::vector<mgContentItem*>::iterator iter;
  
  mgDebug( 3, "Adding %d tracks to the playlist", tracks->size() ); 
  
  for( iter = tracks->begin(); iter != tracks->end(); iter++ )
    {
      m_list.push_back(*iter);
    }
  
  // TODO: why is this vector cleared? shouldn't the caller take care of this? LVW
  tracks->clear();
}

/* add a song after 'position' */
void mgPlaylist::insert( mgContentItem* item, unsigned int position )
{
  if( position >= m_list.size() )
    {
      m_list.push_back(item);
    }
  else
    {
      m_list.insert( m_list.begin() + position, item );
    }
}

bool mgPlaylist::remove( int pos )
{
  bool result = false;

  if( pos > 0 && pos != m_current_idx )
    {
      result = mgTracklist::remove( pos );
      
      if( result && pos < m_current_idx )
	{
	  m_current_idx --;
	}
    }

  return result;
}

void mgPlaylist::clear()
{
  // TODO: who takes care of memory allocation/deallocation of mgItems?
  
  std::vector<mgContentItem*>::iterator iter;
  
  for( iter = m_list.begin(); iter != m_list.end(); iter++ )
    { // delete each item in the list
      delete *iter;
    }
  
  // finally clear the list itself
  m_list.clear();

  // reset index
  m_current_idx = 0;
}

void mgPlaylist::move( int from, int to )
{
  std::vector<mgContentItem*>::iterator from_iter = m_list.begin() + from;  
  std::vector<mgContentItem*>::iterator to_iter   = m_list.begin() + to;

  m_list.insert( to_iter, *from_iter);
  m_list.erase( from_iter );

  if( from < m_current_idx )
    {
      m_current_idx--;
    }
  if( to < m_current_idx )
    {
      m_current_idx++;
    }
}

/*====  access tracks ====*/
std::string mgPlaylist::getListname() 
{ 
  return m_listname; 
}

void mgPlaylist::setListname(std::string name)
{ 
  m_listname = name;
}

// returns the count of items in the list
int mgPlaylist::getCount()
{ 
  return m_list.size();
}

// returns current index in the playlist
int mgPlaylist::getIndex() const
{ 
  return m_current_idx;
}

// returns the current item of the list
mgContentItem* mgPlaylist::getCurrent()
{
  mgContentItem *result;

  if( 0 <= m_current_idx && m_current_idx < (int) m_list.size() )
    {
      result = *( m_list.begin() + m_current_idx );
    }
  else
    {
      result = &(mgContentItem::UNDEFINED);
    }

  return result;
}

// returns the nth track from the playlist
bool mgPlaylist::gotoPosition(unsigned int position)
{
  bool result = false;

  if( position < m_list.size() )
    {
      m_current_idx = position;
      result = true;
    }

  return result;
}

// proceeds to the next item
bool mgPlaylist::skipFwd()
{
  bool result = false;

  if( m_current_idx + 1 < (int) m_list.size() ) // unless loop mode
    {
      m_current_idx ++;
      result = true;
    }

  // if we are already at the end -- just do nothing unless in loop mode
  return result;
}

// goes back to the previous item
bool mgPlaylist::skipBack()
{
  bool result = false;

  if( m_current_idx > 0 )
    {
      m_current_idx --;
      result = true;
    }

  // if we are at the beginning -- just do nothing unless in loop mode
  return result;
}

// get next track, do not update data structures
mgContentItem* mgPlaylist::sneakNext()
{
  if( m_current_idx + 1 <= (int) m_list.size() ) // unless loop mode
    {
      return *(m_list.begin() + m_current_idx + 1);
    }
  else
    {
      return &(mgContentItem::UNDEFINED);
    }
}

bool mgPlaylist::exportM3U( std::string m3u_file )
{
  std::vector<mgContentItem*>::iterator iter;
  bool result = true;

  // open a file for writing
  FILE *listfile = fopen( m3u_file.c_str(), "w" );

  if( !listfile )
    {
      return false;
    }

  fprintf( listfile, "#EXTM3U" );

  for( iter = m_list.begin(); iter != m_list.end(); iter++ )
    { //  each item in the list
      fprintf( listfile, "#EXTINF:0,%s\n", (*iter)->getLabel().c_str() );
      fprintf( listfile, "%s", (*iter)->getSourceFile().c_str() );
    }

  fclose( listfile );

  return result;
}
