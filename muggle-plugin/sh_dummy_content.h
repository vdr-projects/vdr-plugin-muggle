/*******************************************************************/
/*! \file  dummy_content.h
 * \brief  Dummy Data Objects for testing Muggle
 ******************************************************************** 
 * \version $Revision: 1.3 $
 * \date    $Date: 2004/02/02 22:48:04 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: MountainMan $
 * 
 * Declares main classes of for content items and interfaces to SQL databases
 *
 *******************************************************************/
/* makes sur we dont use parse the same declarations twice */
#ifndef _DUMMY_CONTENT_H
#define _DUMMY_CONTENT_H
using namespace std;
#include <string>
#include <list>
#include <vector>

#include "mg_content_interface.h"

// non-member function
int DummyInitDatabase(MYSQL *db);
vector<string> *DummyGetStoredPlaylists(MYSQL db);
/*! 
 *******************************************************************
 * \class DummyTrack
 *
 * \brief represents a a single track 
 * DUMMY
 ********************************************************************/
class  DummyTrack : public mgContentItem
{

private:
     MYSQL m_db;        

  // content fields
  string m_artist;
  string m_title;
  string m_mp3file;
  string m_album;
  string m_genre; 
  int m_year; 
  int m_rating;
  int m_length;

 
 public:
 
   /* constructor */
  DummyTrack(){ m_uniqID = -1;} // creates invalid item
  DummyTrack(int sqlIdentifier, MYSQL dbase);

  DummyTrack(const DummyTrack&);

  /* destructor */
  ~DummyTrack();
  
  virtual mgContentItem::contentType getContentType()
    {return mgContentItem::GD_AUDIO;}
  virtual mgMediaPlayer getPlayer(){return mgMediaPlayer();}

  /* data acess */
  //virtual functions of the base class 
  virtual string getSourceFile();
  virtual string getTitle();
  virtual string getLabel(int col);
  virtual string getDescription();
  virtual string getGenre();
  virtual int getRating();

  // additional class-specific functions
  string getArtist();
  string getAlbum();
  int getYear();
  int getDuration();

  /* data manipulation */
  void setTitle(string new_title);
  void setArtist(string new_artist);
  void setAlbum(string new_album);
  void setGenre(string new_genre);
  void setYear(int new_rating);
  void setRating(int new_rating);
  
  bool writeData();
  static DummyTrack UNDEFINED;
 
};

class DummyTracklist : public mgTracklist
{
 public:
    DummyTracklist(MYSQL db_handle, string restrictions);
};

/*! 
 *******************************************************************
 * \class GdTracklist
 *
 * \brief represents a playlist, i.e. an ordered collection of tracks
 ********************************************************************/
class  DummyPlaylist : public mgPlaylist
{	  
  private:
     int m_sqlId; /* -1 means: not valid */
     string m_author;
     MYSQL m_db;        
  private:
     void createDummyPlaylist(int strt);
  public:
     
     
     /*==== constructors ====*/
     DummyPlaylist(string listname, MYSQL db_handle);
     /* opens existing or creates empty playlist */
     
     DummyPlaylist(unsigned int sql_identifier, MYSQL db_handle);
     /* construct  from the db by internal id*/
  
     /*==== destructor ====*/
     ~DummyPlaylist();
     
     int getPlayTime();
     /* returns the total duration of all songs in the list in seconds */

     int getPlayTimeRemaining();
     /* returns the duration of all remaining songs in the list in seconds */
     
     bool storePlaylist();
};
/*! 
 *******************************************************************
 * \class mgSelectionTreeNode
 ********************************************************************/
class  DummyTreeNode : public mgSelectionTreeNode {

private:
    MYSQL m_db;           // underlying db
    
public:

    /*==== constructors ====*/
    DummyTreeNode(MYSQL db, int view);
    DummyTreeNode(mgSelectionTreeNode* parent, 
		  string id, string label, string restriction); 

    /*==== destructor ====*/
    ~DummyTreeNode();

    // compute children o^xn the fly 
    virtual bool isLeafNode();  
    virtual bool expand();  

    virtual vector<mgContentItem*>* getTracks();
    virtual mgContentItem* getSingleTrack();
};

/* -------------------- begin CVS log ---------------------------------
 * $Log: sh_dummy_content.h,v $
 * Revision 1.3  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */

#endif  /* END  _CONTENT_INTERFACE_H */


















