/*! \file  mg_content_interface.h
 *  \brief data Objects for content (e.g. mp3 files, movies) for the vdr muggle plugin
 *
 *  \version $Revision: 1.4 $
 *  \date    $Date$
 *  \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 *  \author  file owner: $Author$
 * 
 *  Declares generic classes of for content items and interfaces to SQL databases
 *
 *  This file defines the following classes 
 *   - mgMediaMplayer
 *   - mgContentItem
 *   - mgTracklist
 *   - mgSelectionTreeNode
 */

/* makes sure we dont parse the same declarations twice */
#ifndef _CONTENT_INTERFACE_H
#define _CONTENT_INTERFACE_H

#include <string>
#include <vector>

#include <mysql/mysql.h>

#define ILLEGAL_ID -1

class mgFilter;
class mgPlaylist;

/*! 
 * \class mgMediaPlayer
 * \brief dummy player class
 */
class mgMediaPlayer
{
 public: 

  mgMediaPlayer()
    { }

  ~mgMediaPlayer()
    { }
};

/*! 
 * \brief Generic class that represents a single content item.
 *
 * This is the parent class from which classes like mgGdTrack are derived
 *
 */
class mgContentItem
{
 public:
  typedef enum contentType
    {
      ABSTRACT =0,
      GD_AUDIO,
      EPG
    };

 protected:
  int m_uniqID;  // internal identifier to uniquely identify a content item;
  
 public:

  /*! \brief default constructor 
   */
  mgContentItem()
    : m_uniqID( -1 )
    { }

  /*! \brief constructor with explicit id
   */
  mgContentItem( int id )
    : m_uniqID( id )
    { }

  /*! \brief copy constructor 
   */
  mgContentItem( const mgContentItem& org )
    : m_uniqID( org.m_uniqID )
    { }

  /*! \brief destructor 
   */
  virtual ~mgContentItem()
    { };
  
  /*! \brief acess unique id 
   */
  int getId()
    { 
      return  m_uniqID;
    }

  /*! \brief determine what type of content are we looking at (e.g. audio, video, epg)
   */
  virtual contentType getContentType()
    {
      return ABSTRACT;
    }

  /*! \brief return a (global?) object that is used to play content items
   *  \todo What for? Interesting properties? Last state, play info, ...?
   */
  virtual mgMediaPlayer getPlayer()
    {
      return mgMediaPlayer();
    }

  //@{

  /*! \brief return a specific label
   */
  virtual std::string getLabel(int col = 0)
    {
      return "";
    }

  /*! \brief return the title
   */
  virtual std::string getTitle()
    {
      return "";
    }

  /*! \brief get the "file" (or URL) that is passed to the player
   */
  virtual std::string getSourceFile()
    {
      return "";
    }

  /*! \brief return a short textual description
   */
  virtual std::string getDescription()
    {
      return "";
    }

  /*! \brief obtain the genre to which the track belongs
   */
  virtual std::string getGenre()
    {
      return "";
    }

  /*! \brief obtain the rating of the title
   */
  virtual int getRating()
    {
      return 0;
    }

  /*! \brief obtain the samplerate of the track
   */
  virtual int getSampleRate()
    {
      return 0;
    }

  /*! \brief obtain the length of the track (in seconds)
   */
  virtual int getLength()
    {
      return 0;
    }

  /*! \brief obtain the number of audio channels of the track
   */
  virtual int getChannels()
    {
      return 0;
    }

  /*! \brief obtain the bitrate of the track
   */
  virtual std::string getBitrate()
    {
      return "";
    }

  /*! \brief obtain track information in aggregated form
   */
  virtual std::vector<mgFilter*> *getTrackInfo()
    {
      return NULL;
    }

  //@}

  virtual bool updateTrackInfo(std::vector<mgFilter*>*)
    {
      return false;
    }

  virtual bool operator == (mgContentItem o)
    {
      return  m_uniqID == o.m_uniqID;
    }

  // check, if usable
  virtual bool isValid() 
    {
      return ( m_uniqID >= 0 );
    }
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
 *  \brief represent a node in a tree of selections
 *  \ingroup muggle
 */
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
 * Revision 1.4  2004/05/28 15:29:18  lvw
 * Merged player branch back on HEAD branch.
 *
 * Revision 1.3.2.4  2004/05/25 00:10:45  lvw
 * Code cleanup and added use of real database source files
 *
 * Revision 1.3.2.3  2004/04/01 21:35:32  lvw
 * Minor corrections, some debugging aid.
 *
 * Revision 1.3.2.2  2004/03/08 21:42:22  lvw
 * Added count method. Some comments for further todos added.
 *
 * Revision 1.3.2.1  2004/03/08 07:14:28  lvw
 * Preliminary changes to muggle player
 *
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


















