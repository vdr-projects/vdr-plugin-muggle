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
#ifndef __MG_PLAYLIST
#define __MG_PLAYLIST

#include <string>

#include "mg_content_interface.h"

/*! 
 * \class mgPlaylist
 *
 * \brief Represents a generic playlist, i.e. an ordered collection of tracks
 *        Derived classes may take care of specifics of certain media types
 *
 * \todo Are loop mode, shuffle mode, and party mode implemented here?
 */
class mgPlaylist : public mgTracklist
{	  

public:

  //! \brief define various ways to play music in random order
  enum ShuffleMode
    {
      SM_NONE,    //!< \brief play normal sequence
      SM_NORMAL,  //!< \brief a shuffle with a fair distribution
      SM_PARTY    //!< \brief select the next few songs randomly, continue forever
    };

  //! \brief define various ways to play music in a neverending loop
  enum LoopMode
    {
      LM_NONE,     //!< \brief do not loop
      LM_SINGLE,   //!< \brief loop a single track
      LM_FULL      //!< \brief loop the whole playlist
    };

  //! \brief object construction and destruction
  //@{

  //! \brief the default constructor (random listname)
  mgPlaylist();

  /*! \brief constructor with a specified listname
   *
   *  \param listname - initial name of the list
   */
  mgPlaylist( std::string listname );

  void initialize();
  
  //! \brief the destructor
  virtual ~mgPlaylist();

  //@}

  //! \brief control behavior
  //@{

  //! \brief toggle the loop mode.
  LoopMode toggleLoopMode( );

  //! \brief toggle the shuffle mode.
  ShuffleMode toggleShuffleMode( );

  //! \brief report the loop mode.
  LoopMode getLoopMode( ) const { return m_loop_mode; }

  //! \brief report the shuffle mode.
  ShuffleMode getShuffleMode( ) const { return m_shuffle_mode; }
  //@}
  
  //! \brief modify playlist items
  //@{
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
  void move( unsigned from, unsigned to );

  /*! \brief remove a track from the playlist
   *
   *  \param pos - the index of the track to be removed
   */
  bool remove( unsigned pos );

  //@}

  //! \brief obtain the listname
  std::string getListname() ;

  /*! \brief set the listname
   *
   *  \param name - the new name of this list
   */
  virtual void setListname(std::string name);
 
  //! \brief access playlist items
  //@{

  //! \brief returns current index in the playlist
  unsigned getIndex() const;

  //! \brief make playlist persistent
  virtual bool storePlaylist() = 0;

  //! \brief make playlist persistent under a different name
  virtual bool storeAs( std::string name ) = 0;

  //! \brief obtain length of content already played
  unsigned long getCompletedLength();

  //! \brief export the playlist in m3u format
  virtual bool exportM3U( std::string m3u_file );

  /*! 
   * \brief returns the current item of the list
   *
   * \todo Return null in case of an empty list or invalid index
   */
  virtual mgContentItem* getCurrent();
    
  /*! \brief returns the nth track from the playlist
   *
   *  \param position - the position to skip to
   *  \return true if position was okay and changed, false otherwise
   */
  virtual bool gotoPosition(unsigned int position);
  
  /*!
   * \brief proceeds to the next item
   * 
   * \return true if position was okay and changed, false otherwise
   * \todo Handle play modes
   */
  virtual bool skipFwd();

  /*! 
   * \brief goes back to the previous item
   *
   * \return true if position was okay and changed, false otherwise
   * \todo Handle play modes
   */
  virtual bool skipBack();

  //! \brief obtain the next item without skipping the current position
  virtual mgContentItem* sneakNext(); 
  //@}

private:

  //! \brief current index in the playlist
  // TODO: should be a property of the player?
  unsigned m_current_idx;

  //! \brief the current loop mode
  LoopMode m_loop_mode;

  //! \brief the current shuffle mode
  ShuffleMode m_shuffle_mode;

protected:

  // TODO: Why not make these private? Subclasses should use access functions. LVW
  
  //! \brief the name of the playlist
  std::string m_listname;
   
};

#endif
