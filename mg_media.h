/*! 
 * \file  mgmedia.h
 * \brief  Top level access to media in vdr plugin muggle
 *
 * \version $Revision: 1.11 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 */

// makes sure we dont use parse the same declarations twice
#ifndef _MG_MEDIA_H
#define _MG_MEDIA_H

#include <string>
#include <vector>

#include <mysql/mysql.h>

class mgPlaylist;
class mgTracklist;
class mgSelectionTreeNode;
class mgFilter;
class mgFilterSets;

/*! 
 * \class mgFilterSets
 *
 * Represents one or several sets of filters to set and memorize search constraint 
 */
class mgFilterSets 
{
 protected:
  int m_activeSetId;
  std::vector<mgFilter*> *m_activeSet;   // pointer to the active filter set
  
  // stores name-value pairs, even if a different set is active
  std::vector< std::vector<mgFilter*>*> m_sets;

  // stores the titles for all filters
  std::vector<std::string>             m_titles;

 public:

  /*!
   * \brief a  constructor
   * 
   * constracts a number >=1 of filter sets the first set (index 0 ) is active by default
   */
  mgFilterSets();

  /*! 
   * \brief the destructor
   */
  virtual ~mgFilterSets();

  /*!
   * \brief  returns the number of available sets
   */
  int numSets(); 

  /*!
   * \brief proceeds to the next one in a circular fashion
   */
  void nextSet();

  /*!
   * \brief  activates a specific set by index
   */
  void select(int n);

  /*!
   * \brief restore empty state
   *
   * Restores the default values for all filter values in the active set
   * normally, the default values represent 'no restrictions'
   */
  virtual void clear();

  /*! 
   * \brief stores the current filter values
   */
  void accept();

  /*!
   * \brief  returns the active set to the application
   *
   * The application may temporarily modify the filter values 
   * accept() needs to be called to memorize the changed values
   */
  std::vector<mgFilter*> *getFilters();

  /*!
   * \brief compute restrictions
   *
   * computes the (e.g. sql-) restrictions specified by the active filter set
   * and returns the index of the appropriate defualt view in viewPrt
   */
  virtual std::string computeRestriction(int *viewPrt) = 0;

  /*!
   * \brief returns title of active filter set
   */
  std::string getTitle();
};


/*! 
 * \class mgMedia
 *
 * \brief main class to access content in the vdr plugin muggle
 *
 * The constructor of this class should be the only point in the plugin,
 * where the data type is explicitelymentioned. 
 * The class provides a set of objects that abstract from the data
 * type and source
 */
class mgMedia
{
 
 public: 
  typedef enum contentType
    {
      GD_MP3
    } contentType;
  
 private:
    MYSQL m_db;
    contentType m_mediatype;
    std::string m_sql_filter;
    int m_defaultView;
    mgFilterSets *m_filters;

 public:
    mgMedia(contentType mediatype);
    ~mgMedia();
    
    std::string getMediaTypeName();
    
    mgSelectionTreeNode* getSelectionRoot();
    
    /*! \brief  playlist management */
    //@{
    mgPlaylist* createTemporaryPlaylist();
    mgPlaylist* loadPlaylist( std::string name );
    std::vector<std::string> *getStoredPlaylists();
    //@}
    
    std::vector<int> getDefaultCols();
    mgTracklist* getTracks();
    
    // filter management
    
    void initFilterSet(int num=0);
    // creates FiliterSetObject for the selected media type 
    // and activates set n (if available)

    std::vector<mgFilter*> *getActiveFilters();
    // returns pointer to the activen filter set to be modified by the osd
    // Note: Modifications become only active by calling applyActiveFilter()
    
    std::string getActiveFilterTitle();
    
    void nextFilterSet();
    // proceeds to the next filter set in a cirlucar fashion
    
    void clearActiveFilter();
    // clears the current filter values and restores defaults
        
    mgSelectionTreeNode *applyActiveFilter();
    // Applies the active filter set and returns a root node for the 
    // selection in the default view for this filter set

};

/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_media.h,v $
 * Revision 1.11  2004/05/28 15:29:18  lvw
 * Merged player branch back on HEAD branch.
 *
 * Revision 1.10  2004/02/12 09:15:07  LarsAC
 * Moved filter classes into separate files
 *
 * Revision 1.9.2.2  2004/05/25 00:10:45  lvw
 * Code cleanup and added use of real database source files
 *
 * Revision 1.9.2.1  2004/03/02 07:05:50  lvw
 * Initial adaptations from MP3 plugin added (untested)
 *
 * Revision 1.10  2004/02/12 09:15:07  LarsAC
 * Moved filter classes into separate files
 *
 * Revision 1.9  2004/02/09 22:07:44  RaK
 * secound filter set (album search incl. special view #101
 *
 * Revision 1.8  2004/02/09 19:27:52  MountainMan
 * filter set implemented
 *
 * Revision 1.7  2004/02/02 23:33:41  MountainMan
 * impementation of gdTrackFilters
 *
 * Revision 1.6  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */
#endif  /* END  _MG_MEDIA_H */

