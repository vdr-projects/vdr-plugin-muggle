/*! 
 * \file   mg_playlist.c
 * \brief  defines functions to be executed on playlists for the vdr muggle plugindatabase
 *
 * \version $Revision: 1.4 $
 * \date    $Date: 2004/07/25 21:33:35 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: lvw $
 *
 * This file implements the class mgPlaylist which maintains a playlist
 * and supports editing (e.g. adding or moving tracks), navigating it
 * (e.g. obtaining arbitrary items or accessing them sequentially) or
 * making it persistent in some database.
 */

#include "mg_playlist.h"
#include "mg_tools.h"

#include <vector>

using namespace std;

/* ==== constructors ==== */

mgPlaylist::mgPlaylist()
{
  m_current_idx = 0;

  char *buffer;
  asprintf( &buffer, "Playlist-%ld", random() );
  
  m_listname = buffer;
}

mgPlaylist::mgPlaylist(string listname)
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
  m_current_idx = -1;
}

/* ==== add/remove tracks ==== */

/* adds a song at the end of the playlist */
void mgPlaylist::append(mgContentItem* item)
{
  m_list.push_back(item);
}

/* append a list of tracks at the end of the playlist */
void mgPlaylist::appendList(vector<mgContentItem*> *tracks)
{
  vector<mgContentItem*>::iterator iter;
  
  mgDebug( 3, "Adding %d tracks to the playlist", tracks->size() ); 
  
  for( iter = tracks->begin(); iter != tracks->end(); iter++ )
    {
      m_list.push_back(*iter);
    }
  
  // TODO: why is this vector cleared? shouldn't the caller take care of this? LVW
  tracks->clear();
}

/* add a song after 'position' */
void mgPlaylist::insert(mgContentItem* item, unsigned int position)
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

void mgPlaylist::clear()
{
  // TODO: who takes care of memory allocation/deallocation of mgItems?
  
  vector<mgContentItem*>::iterator iter;
  
  for( iter = m_list.begin(); iter != m_list.end(); iter++ )
    { // delete each item in the list
      delete *iter;
    }
  
  // finally clear the list itself
  m_list.clear();
}

void mgPlaylist::move( int from, int to )
{
  vector<mgContentItem*>::iterator from_iter = m_list.begin() + from;  
  vector<mgContentItem*>::iterator to_iter   = m_list.begin() + to;

  m_list.insert( to_iter, *from_iter);
  m_list.erase( from_iter );
}

/*====  access tracks ====*/
string mgPlaylist::getListname() 
{ 
  return m_listname; 
}

void mgPlaylist::setListname(string name)
{ 
  m_listname = name;
}

// returns the count of items in the list
int mgPlaylist::count()
{ 
  return m_list.size();
}

// returns the first item of the list
mgContentItem* mgPlaylist::getFirst()
{
    m_current = m_list.begin();
    m_current_idx = 0;

    return *m_current;
}

// returns the  nth track from the playlist
mgContentItem* mgPlaylist::getPosition(unsigned int position)
{
    if( position >= m_list.size() )
      {
	// TODO: why not return a NULL pointer? LVW
	return &(mgContentItem::UNDEFINED); //invalid
      }
    m_current = m_list.begin() + position;

    return *m_current;
}

// proceeds to the next item
mgContentItem*  mgPlaylist::skipFwd()
{
  mgContentItem* next;

  if( m_current_idx < 0 )
    {
      return getFirst();
    }

  if( m_current + 1 == m_list.end() ) // unless loop mode
    {
      // TODO: why not return a NULL pointer? LVW
      next = &(mgContentItem::UNDEFINED); //invalid item
    }
  else
    {
      return * (++ m_current);
    }
  
  return next;
}

// goes back to the previous item
mgContentItem*  mgPlaylist::skipBack()
{
  mgContentItem* prev;

  if( m_current == m_list.begin() )
    {
      // TODO: why not return a NULL pointer? LVW
      prev = &(mgContentItem::UNDEFINED); //invalid
    }
  else
    {
      prev = * (--m_current);
    }

  return prev;
}

// get next track, do not update data structures
mgContentItem* mgPlaylist::sneakNext()
{
    if( m_current+1 == m_list.end() )
      {
	// TODO: why not return a NULL pointer? LVW
	return &(mgContentItem::UNDEFINED); //invalid
      }
    else
      {
	return * (m_current+1);
      }
}

bool mgPlaylist::exportM3U( string m3u_file )
{
  vector<mgContentItem*>::iterator iter;
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
      fprintf( listfile, "#EXTINF:0,%s", (*iter)->getLabel().c_str() );
      fprintf( listfile, "%s", (*iter)->getSourceFile().c_str() );
    }

  fclose( listfile );

  return result;
}
