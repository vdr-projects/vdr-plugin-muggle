/*! 
 * \file   mg_playlist.c
 * \brief  defines functions to be executed on playlists for the vdr muggle plugindatabase
 *
 * \version $Revision: 1.4 $
 * \date    $Date: 2004/07/09 12:22:00 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: LarsAC $
 *
 * This file implements the class mgPlaylist which maintains a playlist
 * and supports editing (e.g. adding or moving tracks), navigating it
 * (e.g. obtaining arbitrary items or accessing them sequentially) or
 * making it persistent in some database.
 */
#ifndef __MG_PLAYLIST
#define __MG_PLAYLIST

#include "mg_content_interface.h"

/*! 
 * \class mgPlaylist
 *
 * \brief Represents a generic playlist, i.e. an ordered collection of tracks
 * Derived classes take care of specifics of certain media types
 */
class mgPlaylist : public mgTracklist
{	  

private:
  //! \brief current index in the playlist
  // TODO: should be a property of the player?
  int m_current_idx;

protected:

  // TODO: Why not make these private? Subclasses should use access functions. LVW
  
  //! \brief the name of the playlist
  std::string m_listname;
  
  //! \brief the current item as an iterator
  std::vector<mgContentItem*>::iterator m_current;
  
public:
     
  /* ==== constructors and initialization ==== */
  
  //! \brief the default constructor (random listname)
  mgPlaylist();

  /*! \brief constructor with a specified listname
   *
   *  \param listname - initial name of the list
   */
  mgPlaylist( std::string listname );

  void initialize();
  
  /* ==== destructor ==== */
  //! \brief the destructor
  virtual ~mgPlaylist();

  /* === control behavior */

  //! \brief toggle the loop mode. TODO.
  void toggleLoop();

  //! \brief toggle the shuffle mode. TODO.
  void toggleShuffle();
  
  /* ==== add/ remove tracks ==== */
  
  /*! \brief adds a song at the end of the playlist
   *
   *  \param item - the item to be appended
   */
  virtual void append(mgContentItem* item);

  /*! \brief add a vector of songs at the end of the playlist
   *
   *  \param tracks - the vector of tracks to be added
   */
  virtual void appendList(std::vector<mgContentItem*> *tracks);

  /*! \brief add a song after a specified position
   *
   *  Might be merged with append by using a default argument for the position
   *
   *  \param item     - the item to be added
   *  \param position - the position where the item should be added
   */
  virtual void insert(mgContentItem* item, unsigned int position);

  //! \brief clear all tracks
  virtual void clear();

  /*! \brief move tracks within playlist
   *
   *  \param from - the item of the index to be moved
   *  \param to   - the target index of the item
   */
  void move( int from, int to );

  /*! \brief remove a track from the playlist
   *
   *  \param pos - the index of the track to be removed
   */
  //  bool remove( int pos );

  /* ====  access tracks ==== */
  
  //! \brief obtain the listname
  std::string getListname() ;

  /*! \brief set the listname
   *
   *  \param name - the new name of this list
   */
  virtual void setListname(std::string name);
 
  //! \brief returns the count of items in the list
  int count();

  //! \brief returns the first item of the list
  virtual mgContentItem* getFirst();

  /*! \brief returns the nth track from the playlist
   *
   *  \param position - the position to skip to
   */
  virtual mgContentItem* getPosition(unsigned int position);
  
  //! \brief proceeds to the next item
  virtual mgContentItem* skipFwd();

  //! \brief goes back to the previous item
  virtual mgContentItem* skipBack();
 
  //! \brief obtain the next item without skipping the current position
  virtual mgContentItem* sneakNext(); 
  virtual bool storePlaylist()=0;

  //! \brief export the playlist in m3u format
  virtual void exportM3U( const char *m3u_file );
};

#endif
