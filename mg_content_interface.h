/*******************************************************************/
/*! \file  content_interface.h
 * \brief  Data Objects for content (e.g. mp3 files, movies)
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.3 $
 * \date    $Date: 2004/02/09 19:27:52 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: MountainMan $
 * 
 * Declares main classes of for content items and interfaces to SQL databases
 *
 * This file defines the following classes 
 * - mgPlaylist    a playlist
 * - mgTrack       a single track (content item). e.g. an mp3 file
 * - mgSelection   a set of tracks (e.g. a database subset matching certain criteria)
 *
 */
/*******************************************************************/

/* makes sure we dont parse the same declarations twice */
#ifndef _CONTENT_INTERFACE_H
#define _CONTENT_INTERFACE_H

#include <string>
#include <vector>

#include <mysql/mysql.h>

#define ILLEGAL_ID -1
class mgFilter;

/*! 
 *******************************************************************
 * \class mgMediaPlayer
 *
 * \brief dummy player class
 ********************************************************************/
class mgMediaPlayer
{
  public: 
  mgMediaPlayer(){;}
  ~mgMediaPlayer(){;}
  
};

/*! 
 *******************************************************************
 * \class mgContentItem
 *
 * \brief Generic class that represents a single content item.
 * This is the parent class from which classes like mgGdTrack are derived
 ********************************************************************/
class mgContentItem
{

  public:
  typedef enum contentType{
    ABSTRACT =0,
    GD_AUDIO,
    EPG
  }contentType;

  protected:
  int m_uniqID;  // internal identifier to uniquely identify a content item;
  
  public:
  /* constructor */
  mgContentItem(){ m_uniqID = -1;}
  mgContentItem(int id){m_uniqID = id;}
  mgContentItem(const mgContentItem& org){m_uniqID = org.m_uniqID;}
  /* destructor */
  virtual ~mgContentItem(){};
  
  /* data acess */
  int getId(){ return  m_uniqID;}

  // what type of content are we looking at (e.g. audio, video, epg)
  virtual contentType getContentType(){return ABSTRACT;}

  // return (global?) object that is used to play the content items
  virtual mgMediaPlayer getPlayer(){return mgMediaPlayer();}


  // get the "file" (or URL) that is passed to the player
  virtual std::string getSourceFile(){return "";}

  // ============ data access =================
  virtual std::string getTitle(){return "";}     // return the title
  virtual std::string getLabel(int col){return "";}     // return the title
  
  virtual std::string getDescription()// return a short textual description
      {return "";}
  virtual std::vector<mgFilter*> *getTrackInfo(){return NULL;}
  virtual bool updateTrackInfo(std::vector<mgFilter*>*){return false;}

  virtual std::string getGenre(){return "";}
  virtual int getRating()
    {
      return 0;
    }

  virtual bool operator == (mgContentItem o){return  m_uniqID == o.m_uniqID;}

  // check, if usable
  virtual bool isValid() {return (m_uniqID >=0);}
  static  mgContentItem UNDEFINED;
};


class mgTracklist
{
 protected:
  std::vector<mgContentItem*> m_list;
  std::vector<int> m_columns;
  int sorting;

 public:
  mgTracklist(); // creates empty tracklist;
 
  virtual ~mgTracklist();

  std::vector<mgContentItem*> *getAll();
  unsigned int getNumItems();

  virtual void shuffle();
  virtual void sortBy(int col, bool direction);

  void setDisplayColumns(std::vector<int> cols);
  unsigned int getNumColumns();
  virtual std::string getLabel(unsigned int position, const std::string separator);
  
  virtual mgContentItem* mgTracklist::getItem(unsigned int position);

  virtual int remove(mgContentItem* item); // remove all occurences of item
  virtual bool remove(unsigned int position);    // remove item at position
};

/*! 
 *******************************************************************
 * \class mgPlaylist
 *
 * Represents a generic playlist, i.e. an ordered collection of tracks
 * The derived classes take care of specifics of certain media types
 ********************************************************************/
class  mgPlaylist : public mgTracklist
{	  
  protected:
     std::string m_listname;
     std::vector<mgContentItem*>::iterator m_current;
  public:
     
     /*==== constructors ====*/
     mgPlaylist();
     mgPlaylist(std::string listname);
     
     /*==== destructor ====*/
     virtual ~mgPlaylist();
     
     /*==== add/ remove tracks ====*/

     /* adds a song at the end of the playlist */
     virtual void append(mgContentItem* item);
     virtual void appendList(std::vector<mgContentItem*> *tracks);
     /* adds a song after 'position' */
     virtual void insert(mgContentItem* item, unsigned int position);

     /*====  access tracks ====*/
     std::string getListname() ;
     void setListname(std::string name);

     // returns the first item of the list
     virtual mgContentItem* getFirst();

     // returns the  nth track from the playlist
     virtual mgContentItem* getPosition(unsigned int position);

     // proceeds to the next item
     virtual mgContentItem*  skipFwd();

     // goes back to the previous item
     virtual mgContentItem*  skipBack();

     virtual mgContentItem* sneakNext();
     
};

class  mgSelectionTreeNode
{

protected:
    MYSQL m_db;
    bool m_expanded;      // already expanded ?
    std::string m_restriction; // list of active restrictions at this level
    std::string m_id;          // ID of the node, used for further expand
    int m_level;          // depth of tree (0 = root)
    int m_view;
    std::string m_label; 
    
//    std::vector<std::string> _labels; // Labels used for interaction with the user 
    // about this node

//    vector<mgSelectionTreeNode> _children; // if expanded the links to the 
                                           // children are stopred here
    mgSelectionTreeNode* m_parent;
    std::vector <mgSelectionTreeNode*> m_children;
    
public:

    /*==== constructors ====*/
    mgSelectionTreeNode(MYSQL db, int view);

    mgSelectionTreeNode(mgSelectionTreeNode* parent, std::string id, std::string label);
  
    /*==== destructor ====*/
    virtual ~mgSelectionTreeNode();

    // compute children on the fly 
    virtual bool isLeafNode()=0;
    virtual bool expand()=0; 
    virtual void collapse(); // removes all children (recursively)

    mgSelectionTreeNode* getParent();

    // access children
    virtual std::vector<mgSelectionTreeNode*> &getChildren();

    // access data in  current node
    bool isExpanded(){return m_expanded;}
    int getLevel(){return m_level;} // for debugging
    std::string getID();
    virtual std::string getRestrictions();

    std::string getLabel(); 
    virtual std::string getLabel(int n); 
 #if 0
    virtual std::string viewTitle(int level)=0;
    virtual std::vector<std::string> viewChoices(int level, int choice);
#endif
    
    // returns all tracks below this node
    // Note: This function allocates memory for the vector and for all elements of the vector
    //       The calling function is in charge of releasing this memory 
   virtual std::vector<mgContentItem*>* getTracks()=0;
   virtual mgContentItem* getSingleTrack()=0;
};

/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_content_interface.h,v $
 * Revision 1.3  2004/02/09 19:27:52  MountainMan
 * filter set implemented
 *
 * Revision 1.2  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */

#endif  /* END  _CONTENT_INTERFACE_H */


















