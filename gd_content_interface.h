/*! 
 * \file   gd_content_interface.h
 * \brief  Data objects for content (e.g. mp3 files, movies)
 *         for the vdr muggle plugin database
 *          
 * \version $Revision: 1.9 $
 * \date    $Date: 2004/07/29 06:17:50 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: lvw $
 * 
 * Declares main classes for content items and interfaces to SQL databases
 *
 * This file defines the following classes 
 *  - gdFilterSets
 *  - mgGdTrack     a single track (content item). e.g. an mp3 file
 *  - mgSelection   a set of tracks (e.g. a database subset matching certain criteria)
 *
 */

/* makes sure we dont use the same declarations twice */
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

// non-member functions
int GdInitDatabase(MYSQL *db);
std::vector<std::string> *GdGetStoredPlaylists(MYSQL db);

/*! 
 * \brief A set of filters to search for content
 */
class gdFilterSets : public mgFilterSets 
{

 public:

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

  /*!
   * \brief compute restriction w.r.t active filters
   *
   * Computes the (e.g. sql) restrictions specified by
   * the active filter sets.
   *
   * \param viewPort - after call, contains the index of the appropriate default view in  
   * \todo should viewPort be a reference?
   */
  virtual std::string computeRestriction(int *viewPrt);
};


/*! 
 * \brief represents a a single track 
 
 * This may be any content item. e.g. a mp3 fileselection
 * The object is initially created with a database identifier.
 * The actual data is only read when a content field is accessed for
 * the first time. For subsequent access, cached values are used.
 *
 * \todo does each track node need a reference to the database?
 *       maybe we can use a static db handle in mgDatabase?
 * \
 */
class  mgGdTrack : public mgContentItem
{

private:

  /*! 
   * \brief the database in which the track resides 
   * */
  MYSQL m_db;
  
  /*!
   * \brief a dirty flag
   *
   * false, if content field values have not yet been retrieved
   * from the database. Set to true when contents are retrieved
   * (on demand only).
   */
  bool m_retrieved;

  // content fields
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
   * \brief The length of the file in bytes.
   * \todo TODO: is this true? Or what length are we talking about?
   */
  int m_length;

  /*!
   * \brief Access the data of the item from the database
   */
  bool readData();

 public:
 
  /*!
   * \brief a constructor 
   * 
   * Creates an invalid item.
   *
   * \todo does this make sense?
   */
  mgGdTrack()
  { 
    m_uniqID = -1;
  }  
  
  /*!
   * \brief a constructor for a specific item
   *
   * The constructor creates a specific item in a given database
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
  
  virtual mgContentItem::contentType getContentType(){return mgContentItem::GD_AUDIO;}
  
  virtual mgMediaPlayer getPlayer()
    {
      return mgMediaPlayer();
    }

  /* data acess */
  //virtual functions of the base class 
  virtual std::string getSourceFile();
  
  virtual std::string getTitle();
  
  virtual std::string getLabel(int col = 0);

  virtual std::vector<mgFilter*> *getTrackInfo();
  
  virtual bool setTrackInfo(std::vector<mgFilter*>*);

  virtual std::string getGenre();
  
  virtual int getRating();

  // additional class-specific functions
  std::string getArtist();
  std::string getAlbum();
  int getYear();
  int getDuration();
  std::string getImageFile();
 
  /* data manipulation */
  void setTitle(std::string new_title);
  void setArtist(std::string new_artist);
  void setAlbum(std::string new_album);
  void setGenre(std::string new_genre);
  void setYear(int new_rating);
  void setRating(int new_rating);
  
  bool writeData();
  static mgGdTrack UNDEFINED;

};

class GdTracklist : public mgTracklist
{
 public:
    GdTracklist(MYSQL db_handle, std::string restrictions);
};

/*! 
 *******************************************************************
 * \class GdPlaylist
 *
 * \brief represents a playlist, i.e. an ordered collection of tracks
 ********************************************************************/
class  GdPlaylist : public mgPlaylist
{	  
  private:
     int m_sqlId; /* -1 means: not valid */
     int m_listtype; // used in GiantDisc db queries
     std::string m_author;
     MYSQL m_db;
     
  private:
     int insertDataFromSQL();

  public:
     
     
     /*==== constructors ====*/
     GdPlaylist(std::string listname, MYSQL db_handle);
     /* opens existing or creates empty playlist */

     
     /*==== destructor ====*/
     virtual ~GdPlaylist();
     
     virtual void setListname(std::string name);
     /* changes the listname of the playlist (and unset the sql id */

     int getPlayTime();
     /* returns the total duration of all songs in the list in seconds */

     int getPlayTimeRemaining();
     /* returns the duration of all remaining songs in the list in seconds */


     bool storePlaylist();
};
/*! 
 *******************************************************************
 * \class mgSelectionTreeNode
 *
 * \brief hierarchical representation of a set of tracks
 * The selection can be based on the whole database or a subset of it.
 * Within this selection, the data is organized in a tree hierarchy
 * The levels hof the hierarchy can be expanded dynamically by specifying
 * the database field  for the next expansion step
 * In this way, the expnasion scheme (order of the fields) is not static.
 * When a node is expanded, a list of children is created. 
 * Each child inherits the restrictions of its father and an additional
 * restriction on the recently expanded db field
 ********************************************************************/
class  GdTreeNode : public mgSelectionTreeNode{

private:
    // everything is in the base class
public:

    /*==== constructors ====*/
    GdTreeNode(MYSQL db, int view, std::string filters); // top level
    GdTreeNode(mgSelectionTreeNode* parent,
	       std::string id, std::string label, std::string restriction); 
  
    /*==== destructor ====*/
    virtual ~GdTreeNode();

    // compute children on the fly 
    virtual bool isLeafNode();
    virtual bool expand();  

    // access data in  current node
    virtual std::vector<mgContentItem*>* getTracks();
    virtual mgContentItem* getSingleTrack();
};

/* -------------------- begin CVS log ---------------------------------
 * $Log: gd_content_interface.h,v $
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
>>>>>>> 1.4.2.6
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



