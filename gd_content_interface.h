/*! 
 * \file   gd_content_interface.h
 * \brief  Data objects for content (e.g. mp3 files, movies)
 *         for the vdr muggle plugin database
 * \ingroup giantdisc
 *          
 * \version $Revision: 1.11 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 * 
 * Declares main classes for content items and interfaces to SQL databases
 *
 * This file defines the following classes 
 *  - gdFilterSets:  filters to specifically search for GD items
 *  - mgGdTrack:     a single track (content item). e.g. an mp3 file
 *  - GdTracklist:
 *  - GdPlaylist:
 *  - GdTreeNode:
 */

#ifndef _GD_CONTENT_INTERFACE_H
#define _GD_CONTENT_INTERFACE_H

#include <string>
#include <vector>

#include <mysql/mysql.h>

#include "mg_content_interface.h"
#include "mg_media.h"

#include "mg_playlist.h"
#include "mg_filters.h"
#include "i18n.h"

/*! 
 * \brief Initialize a database for GD use
 * \ingroup giantdisc
 * \todo Should be a static member of some GD class
 */
int GdInitDatabase(MYSQL *db);

/*! 
 * \brief Obtain the playlists stored within the GD schema
 * \ingroup giantdisc
 * \todo Should be a static member of some GD class
 */
std::vector<std::string> *GdGetStoredPlaylists(MYSQL db);

/*! 
 * \brief A set of filters to search for content
 * \ingroup giantdisc
 */
class gdFilterSets : public mgFilterSets 
{

 public:

  //\@{

  /*!
   * \brief the default constructor
   *
   * Constructs a number ( >= 1 ) of filter sets where
   * the first (index 0) is active by default.
   */
  gdFilterSets();

  /*!
   * \brief the destructor
   */
  virtual ~gdFilterSets();

  //\@

  /*!
   * \brief compute restriction w.r.t active filters
   *
   * Computes the (e.g. sql) restrictions specified by
   * the active filter sets.
   *
   * \param viewPort - after call, contains the index of the appropriate default view in  
   * \return  sql string representing the restrictions
   * \todo should viewPort be a reference?
   */
  virtual std::string computeRestriction(int *viewPort);
};


/*! 
 * \brief represents a a single track
 * \ingroup giantdisc
 *
 * This may be any content item. e.g. a mp3 fileselection
 * The object is initially created with a database identifier.
 * The actual data is only read when a content field is accessed for
 * the first time. For subsequent access, cached values are used.
 *
 * \todo does each track node need a reference to the database?
 *       maybe we can use a static db handle in mgDatabase?
 */
class mgGdTrack : public mgContentItem
{
 public:
 
  //@{

  /*! 
   * \brief a constructor 
   * 
   * Creates an invalid item.
   *
   * \todo does this make sense? used anywhere?
   */
  mgGdTrack()
  { 
    m_uniqID = -1;
  }  
  
  /*!
   * \brief a constructor for a specific item
   *
   * The constructor creates a specific item in a given database
   * On creation, the object is only a wrapper without data. Actual 
   * data fields are filled when readData() is called for the first time.
   *
   * \param sqlIdentifier - a unique ID of the item which will be represented
   * \param dbase - the database in which the item exists
   */
  mgGdTrack( int sqlIdentifier, MYSQL dbase );

  /*!
   * \brief a copy constructor
   */
  mgGdTrack(const mgGdTrack&);

  /*!
   * \brief the destructor 
   */
  virtual ~mgGdTrack();

  //@}
  
  /*!
   * \brief obtain the content type of the item
   */
  virtual mgContentItem::contentType getContentType()
    {
      return mgContentItem::GD_AUDIO;
    }
  
  /*!
   * \brief obtain the associated player object
   *
   * \todo what is this used for?
   */
  virtual mgMediaPlayer getPlayer()
    {
      return mgMediaPlayer();
    }

  //\@{

  /*!
   * \brief returns a certain field of the item as a string
   *
   *    0 - title
   *    1 - artist
   *    2 - album
   *    3 - genre
   */
  virtual std::string getLabel( int col = 0 );

  /*!
   * \brief returns value for the track title
   */
  virtual std::string getTitle();  

  /*!
   * \brief returns value for the location of the track on disk
   */
  virtual std::string getSourceFile();  

  /*!
   * \brief returns the genre of the track
   */
  virtual std::string getGenre();  

  /*!
   * \brief returns the artist of the track
   */
  std::string getArtist();

  /*!
   * \brief returns value the album to which the track belongs
   */
  std::string getAlbum();

  /*!
   * \brief obtain the location of the image file
   */
  std::string getImageFile();

  /*!
   * \brief obtain the year of the track
   */
  int getYear();

  /*!
   * \brief obtain the duration of the track
   */
  int getDuration();

  /*!
   * \brief obtain the rating of the track
   */
  virtual int getRating();

  /*! \brief obtain the samplerate of the track
   */
  virtual int getSampleRate();

  /*! \brief obtain the number of audio channels of the track
   */
  virtual int getChannels();

  /*! \brief obtain the bitrate of the track
   */
  virtual string getBitrate();

  /*!
   * \brief obtain the complete track information
   */
  virtual std::vector<mgFilter*> *getTrackInfo();  

  //\@}

  //\@{
  /*!
   * \brief set the title of the track
   */
  void setTitle(std::string new_title);

  /*!
   * \brief set the title of the track
   */
  void setArtist(std::string new_artist);

  /*!
   * \brief set the album name of the track
   */
  void setAlbum(std::string new_album);

  /*!
   * \brief set the genre of the track
   */
  void setGenre(std::string new_genre);

  /*!
   * \brief set the year of the track
   */
  void setYear(int new_year);

  /*!
   * \brief set the rating of the track
   */
  void setRating(int new_rating);

  /*!
   * \brief set complete information of the track
   */
  virtual bool setTrackInfo(std::vector<mgFilter*>*);

  /*!
   * \brief make changes persistent
   *
   * The changes made using the setXxx methods are not
   * stored persistently in the database until writeData
   * is called.
   *
   * \note only the tracks table in the SQL database is updated.
   * Genre and album field information is lost.
   */
  bool writeData();
  //\@}

  //! \brief a special instance denoting an undefined track
  static mgGdTrack UNDEFINED;

private:

  /*! 
   * \brief the database in which the track resides 
   */
  MYSQL m_db;
  
  /*!
   * \brief a dirty flag
   *
   * false, if content field values have not yet been retrieved
   * from the database. Set to true when contents are retrieved
   * (on demand only).
   */
  bool m_retrieved;

  /*! 
   * \brief the artist name
   */
  std::string m_artist;

  /*! 
   * \brief the track title
   */
  std::string m_title;

  /*!
   * \brief the filename
   */
  std::string m_mp3file;

  /*!
   * \brief The album to which the file belongs
   */
  std::string m_album;

  /*!
   * \brief The genre of the music
   */
  std::string m_genre; 
  
  /*!
   * \brief The year in which the track appeared
   */
  int m_year; 
  
  /*!
   * \brief The rating by the user
   */
  int m_rating;
  
  /*!
   * \brief The length of the track in seconds
   */
  int m_length;

  /*!
   * \brief The sampling rate of the track in Hz
   */
  int m_samplerate;
  
  /*!
   * \brief The number of channels of the track
   */
  int m_channels;

  /*!
   * \brief Access the data of the item from the database and fill actual data fields
   *
   * In order to avoid abundant queries to the database, the content fields
   * of the mgGdTrack object may not be filled upon creation. As soon as the 
   * first content field is needed, this private function is called to fill 
   * all content fields at once.
   */
  bool readData();

};

/* 
 * \brief a list of tracks stored in the databas
 * \ingroup giantdisc
 */
class GdTracklist : public mgTracklist
{
 public:

  GdTracklist(MYSQL db_handle, std::string restrictions);
};

/*! 
 * \class GdPlaylist
 *
 * \brief represents a playlist, i.e. an ordered collection of tracks
 */
class  GdPlaylist : public mgPlaylist
{	  
 public:
     
  //@{
  /*! 
   * \brief opens an existing or creates empty playlist by name
   *
   * If the playlist does not yet exist, an empty playlist is created.
   *
   * \param listname  - user-readable identifier of the paylist
   * \param db_handle - database which stores the playlist
   */
  GdPlaylist(std::string listname, MYSQL db_handle);

  /*! 
   * \brief destructor
   *
   * \note the destructor only destroys the in-memory footprint of the playlist,
   *       it does not modify the database.
   */
  virtual ~GdPlaylist();

  //@}
     
  /*!
   * changes the listname of the playlist (and unset the sql id)
   */
  virtual void setListname(std::string name);
  
  /*!
   * returns the total duration of all songs in the list in seconds
   */
  int getPlayTime();
  
  /*! 
   * returns the duration of all remaining songs in the list in seconds 
   */
  int getPlayTimeRemaining();

  /*! 
   * \brief write data back to the database
   */
  bool storePlaylist();

 private:

  /*!
   * \brief reads the track list from the sql database into memory
   */
  int insertDataFromSQL();

  //! \brief the database identifier of the list
  int m_sqlId; 
  
  //! \brief the list type used in GiantDisc db queries
  int m_listtype;

  //! \brief the author of the playlist
  std::string m_author;
  
  //! \brief the handle to the database in which the list is stored
  MYSQL m_db;  
};

/*! 
 * \brief hierarchical representation of a set of tracks
 * \ingroup giantdisc
 *
 * The selection can be based on the whole database or a subset of it.
 * Within this selection, the data is organized in a tree hierarchy
 * The levels hof the hierarchy can be expanded dynamically by specifying
 * the database field  for the next expansion step
 * In this way, the expnasion scheme (order of the fields) is not static.
 * When a node is expanded, a list of children is created. 
 * Each child inherits the restrictions of its father and an additional
 * restriction on the recently expanded db field
 */
class GdTreeNode : public mgSelectionTreeNode
{
public:

  //@{
  /*! 
   * \brief Construct a top level tree node in a certain view with certain filters 
   */
  GdTreeNode(MYSQL db, int view, std::string filters);
  
  /*! 
   * \brief Construct a subordinate tree node with given labels and restrictions
   */
  GdTreeNode(mgSelectionTreeNode* parent,
	     std::string id, std::string label, std::string restriction); 

  /*! 
   * \brief destructor to clean up the object
   */
  virtual ~GdTreeNode();
  //@}

  //@{
  /*!
   * \brief check, whether the node is a leaf node (i.e. has no more children)
   */
  virtual bool isLeafNode();

  /*!
   * \brief expand the node and generate child nodes according to restrictions passed in the constructor
   */
  virtual bool expand();  

  /*!
   * \brief obtain the tracks in this node
   */
  virtual std::vector<mgContentItem*>* getTracks();

  /*!
   * \brief obtain a single track in this node
   */
  virtual mgContentItem* getSingleTrack();

  //@}
};

/* -------------------- begin CVS log ---------------------------------
 * $Log: gd_content_interface.h,v $
 * Revision 1.11  2004/08/30 14:31:43  LarsAC
 * Documentation added
 *
 * Revision 1.10  2004/08/27 15:19:34  LarsAC
 * Changed formatting and documentation
 *
 * Revision 1.9  2004/07/29 06:17:50  lvw
 * Added todo entries
 *
 * Revision 1.8  2004/07/06 00:20:51  MountainMan
 * loading and saving playlists
 *
 * Revision 1.7  2004/05/28 15:29:18  lvw
 * Merged player branch back on HEAD branch.
 *
 *
 * Revision 1.6  2004/02/23 15:41:21  RaK
 * - first i18n attempt
 *
 * Revision 1.5  2004/02/12 09:15:07  LarsAC
 * Moved filter classes into separate files
 *
 * Revision 1.4.2.6  2004/05/25 00:10:45  lvw
 * Code cleanup and added use of real database source files
 *
 * Revision 1.4.2.5  2004/04/01 21:35:32  lvw
 * Minor corrections, some debugging aid.
 *
 * Revision 1.4.2.4  2004/03/14 17:57:30  lvw
 * Linked against libmad. Introduced config options into code.
 *
 * Revision 1.4.2.3  2004/03/10 13:11:24  lvw
 * Added documentation
 *
 * Revision 1.4.2.2  2004/03/08 07:14:27  lvw
 * Preliminary changes to muggle player
 *
 * Revision 1.4.2.1  2004/03/02 07:05:50  lvw
 * Initial adaptations from MP3 plugin added (untested)
 *
 * Revision 1.6  2004/02/23 15:41:21  RaK
 * - first i18n attempt
 *
 * Revision 1.5  2004/02/12 09:15:07  LarsAC
 * Moved filter classes into separate files
 *
 * Revision 1.4  2004/02/09 19:27:52  MountainMan
 * filter set implemented
 *
 * Revision 1.3  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */
#endif  /* END  _GD_CONTENT_INTERFACE_H */



