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
 * \brief dummy player class
 * \ingroup muggle
 *
 * \todo what to do with this
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
 * This is the parent class from which classes like mgGdTrack are derived.
 * 
 */
class mgContentItem
{
 public:
  
  /*! 
   * \brief defines the content type of the item
   * \todo rethink this mechanism because adding new subclasses
   * breaks existing ones (makes recompile cycle necessary).
   */
  typedef enum contentType
    {
      ABSTRACT =0, //< an abstract item which cannot be used
      GD_AUDIO,    //< a GiantDisc audio track
      EPG          //< an EPG item (i.e. a TV show)
    };

 protected:

  /*! 
   * \brief internal identifier to uniquely identify a content item;
   */
  int m_uniqID;  
  
 public:
  
  /*! 
   * \brief default constructor 
   */
  mgContentItem()
    : m_uniqID( -1 )
    { }

  /*! 
   * \brief constructor with explicit id
   */
  mgContentItem( int id )
    : m_uniqID( id )
    { }

  /*! 
   * \brief copy constructor 
   */
  mgContentItem( const mgContentItem& org )
    : m_uniqID( org.m_uniqID )
    { }

  /*! 
   * \brief the destructor 
   */
  virtual ~mgContentItem()
    { };
  
  /*! 
   * \brief acess unique id 
   */
  int getId()
    { 
      return  m_uniqID;
    }
  
  /*! 
   * \brief determine what type of content are we looking at (e.g. audio, video, epg)
   *
   * The method should be overriden for concrete subclasses to return concrete a contentType.
   */
  virtual contentType getContentType()
    {
      return ABSTRACT;
    }
  
  /*! 
   * \brief return a (global?) object that is used to play content items
   * \todo What for? Interesting properties? Last state, play info, ...?
   */
  virtual mgMediaPlayer getPlayer()
    {
      return mgMediaPlayer();
    }

  //! \brief Access item data
  //@{

  /*! 
   * \brief return a specific label
   */
  virtual std::string getLabel(int col = 0)
    {
      return "";
    }

  /*! 
   * \brief return the title
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


/*!
 * \brief a list of content items
 */
class mgTracklist
{
 protected:
  std::vector<mgContentItem*> m_list;
  std::vector<int> m_columns;
  int sorting;

 public:

  /*!
   * \brief constructor
   *
   * create an empty tracklist
   */
  mgTracklist();
 
  /*!
   * \brief the destructor
   *
   * Deletes all items in the tracklist and removes the list itself
   */
  virtual ~mgTracklist();

  /*!
   * \brief returns a pointer to the list of elements
   */
  std::vector<mgContentItem*> *getAll();

  /*!
   * \brief returns the number of elements in the list
   */
  unsigned int getNumItems();

  /*!
   * \brief randomizes the order of the elements in the list 
   */
  virtual void shuffle();

  /*!
   * \brief sorts the elements in the list by the nth column
   */
  virtual void sortBy(int col, bool direction);

  /*!
   * \brief stores the ids of columns to be used in label creation
   * 
   * The list can create a label with different fields (columns) using the
   * function getLabel(). This function defines the fields of the contentItems
   * to be used in the label and their order.
   */
  void setDisplayColumns(std::vector<int> cols);

  /*!
   * \brief returns the number of display columns 
   */
  unsigned int getNumColumns();

  /*!
   * \brief creates the label string for an item
   * 
   * The list can create a label with different fields (columns).
   * The fields used in the list and their order is set using the function setDisplayColumns.
   * 
   * This function creates a string from these columns, separated by the string
   * 'separator' in the label and their order.
   */
  virtual std::string getLabel(unsigned int position, const std::string separator);
  
  /*!
   * \brief returns an item from the list at the specified position
   */
  virtual mgContentItem* mgTracklist::getItem(unsigned int position);

  /*!
   * \brief remove item at position
   */
  virtual int remove(mgContentItem* item); // remove all occurences of item

  /*!
   * \brief remove all occurences of item
   */
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

#endif
