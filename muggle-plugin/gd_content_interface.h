/*******************************************************************/
/*! \file   gd_content_interface.h
 *  \brief  Data Objects for content (e.g. mp3 files, movies)
 *          for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.6 $
 * \date    $Date: 2004/02/23 15:41:21 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: RaK $
 * 
 * Declares main classes of for content items and interfaces to SQL databases
 *
 * This file defines the following classes 
 * - mgPlaylist    a playlist
 * - mgGdTrack     a single track (content item). e.g. an mp3 file
 * - mgSelection   a set of tracks (e.g. a database subset matching certain criteria)
 *
 */
/*******************************************************************/

/* makes sure we dont use parse the same declarations twice */
#ifndef _GD_CONTENT_INTERFACE_H
#define _GD_CONTENT_INTERFACE_H

#include <string>
#include <vector>

#include <mysql/mysql.h>

#include "mg_content_interface.h"
#include "mg_media.h"
#include "mg_filters.h"

#include "i18n.h"

// non-member function
int GdInitDatabase(MYSQL *db);
std::vector<std::string> *GdGetStoredPlaylists(MYSQL db);

class gdFilterSets : public mgFilterSets 
{

 public:
  gdFilterSets();
  // constructor, constracts a number >=1 of filter sets
  // the first set (index 0 ) is active by default

  virtual ~gdFilterSets();
  // destructor

  virtual  std::string computeRestriction(int *viewPrt);
  // computes the (e.g. sql-) restrictions specified by the active filter set
  // and returns the index of the appropriate defualt view in viewPrt

};


/*! 
 *******************************************************************
 * \class mgGdTrack
 *
 * \brief represents a a single track 
 * This may be any content item. e.g. a mp3 fileselection
 * 
 * The object is initially created with a database identifier.
 * The actual data is only read when a content field is accessed for
 * The first time
 ********************************************************************/
class  mgGdTrack : public mgContentItem
{

private:
  MYSQL m_db;
  bool m_retrieved; // false if content field values have not yet been 
                   // retrieved from database. This is done on demand

  // content fields
  std::string m_artist;
  std::string m_title;
  std::string m_mp3file;
  std::string m_album;
  std::string m_genre; 
  int m_year; 
  int m_rating;
  int m_length;

  bool readData();

 public:

 
   /* constructor */
  mgGdTrack(){ m_uniqID = -1;} // creates invalid item
  mgGdTrack(int sqlIdentifier, MYSQL dbase);
  mgGdTrack(const mgGdTrack&);

  /* destructor */
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
  virtual std::string getLabel(int col);

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
     
     GdPlaylist(unsigned int sql_identifier, MYSQL db_handle);
     /* construct  from the db by internal id*/
  
     
     /*==== destructor ====*/
     virtual ~GdPlaylist();
     
      
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



