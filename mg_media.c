/*******************************************************************/
/*! \file   mg_media.c
 *  \brief  Top level access to media in vdr plugin muggle
 *          for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.12 $
 * \date    $Date: 2004/02/23 16:30:58 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: RaK $
 */
/*******************************************************************/

/* makes sure we dont parse the same declarations twice */
#include "mg_media.h"
#include "mg_tools.h"
#include "mg_content_interface.h"
#include "gd_content_interface.h"

using namespace std;

//-------------------------------------------------------------------
//        mgFilterSets
//-------------------------------------------------------------------
/*! 
 *******************************************************************
  * \brief constructor
  *
  * constructor of the base class
 ********************************************************************/
mgFilterSets::mgFilterSets()
{
  // nothing to be done in the base class
}
 /*! 
 *******************************************************************
 * \brief destructor
 ********************************************************************/
mgFilterSets::~mgFilterSets()
{
  vector<mgFilter*> *set;
  for(vector<vector<mgFilter*>*>::iterator iter1 = m_sets.begin();
      iter1 != m_sets.end(); iter1++)
  {
    set = *iter1;
    for(vector<mgFilter*>::iterator iter2 = set->begin();
      iter2 != set->end(); iter2++)
    {
      delete (*iter2);
    }
    set->clear();
    delete set;
  }
  m_sets.clear();
}
 /*! 
 *******************************************************************
 * \brief returns the number of available sets
 ********************************************************************/
int mgFilterSets::numSets()
{
  return m_sets.size();
}

 /*! 
 *******************************************************************
  * \brief proceeds to the next one in a circular fashion
 ********************************************************************/
void mgFilterSets::nextSet()
{
  m_activeSetId++;
  if(m_activeSetId >=  (int) m_sets.size())
  {
    m_activeSetId = 0;
  }
  m_activeSet = m_sets[m_activeSetId];
}
  // 

/*! 
 *******************************************************************
 * \brief activates a specific set by index
 *
 * \par n : index of the set to be activated
 *
 * If n is not a valid filter set, the first set (index 0 ) is activated
 ********************************************************************/
void mgFilterSets::select(int n)
{
  m_activeSetId = n ;
  if(m_activeSetId >=  (int) m_sets.size())
  {
    m_activeSetId = 0;
  }
  m_activeSet = m_sets[m_activeSetId];
  
}
 
 /*! 
 *******************************************************************
 * \brief restores the default values for all filter values in the active set
 ********************************************************************/
 void mgFilterSets::clear()
{
  for(vector<mgFilter*>::iterator iter = m_activeSet->begin();
      iter != m_activeSet->end(); iter++)
  {
    (*iter)->clear();
  }
}
 /*! 
 *******************************************************************
 * \brief stores the current filter values
 ********************************************************************/
 void mgFilterSets::accept()
{
  for(vector<mgFilter*>::iterator iter = m_activeSet->begin();
      iter != m_activeSet->end(); iter++)
  {
    (*iter)->store();
  }
}
 
/*! 
 *******************************************************************
 * \brief returns the active filter set to the application
 *
 * the application may temporarily modify the filter values 
 * accept() needs to be called to memorize the changed values
 ********************************************************************/
 vector<mgFilter*> *mgFilterSets::getFilters()
{
  for(vector<mgFilter*>::iterator iter = m_activeSet->begin();
      iter != m_activeSet->end(); iter++)
  {
    (*iter)->restore();
  }
  return m_activeSet;
}


/*! 
 *******************************************************************
 * \brief return title of the active filter set
 ********************************************************************/
string mgFilterSets::getTitle()
{
  if(m_activeSetId < (int) m_titles.size())
  {
    return m_titles[m_activeSetId];
  }
  else
  {
    mgWarning("Implementation error: No title string for filter set %d",
	      m_activeSetId);
    return "NO-TITLE";
  }
}

//-------------------------------------------------------------------
//        mgFilterSets
//-------------------------------------------------------------------
/*! 
 *******************************************************************
 * \class mgMedia
 *
 * \brief mein class to access content in the vdr plugin muggle
 ********************************************************************/
mgMedia::mgMedia(contentType mediatype)
{
    int errval = 0;
    m_filters = NULL;
    m_mediatype = mediatype;
    m_sql_filter = "1";
    m_defaultView = 1;

    // now initialize the database
    mgDebug(1, "Media Type %sselected", getMediaTypeName().c_str());
    switch(m_mediatype)
    {
	case GD_MP3:
	{
	    errval = GdInitDatabase(&m_db);
	    mgDebug(3, "Successfully conntected to sql database 'GiantDisc'"); 
	}
    }
    if(errval < 0)
    {
	mgError("Error connecting to database\n");
    }

    mgDebug(3, "Initializing track filters");
    switch(m_mediatype)
    {
	case GD_MP3:
	{
	    errval = GdInitDatabase(&m_db);
	    mgDebug(3, "Successfully conntected to sql database 'GiantDisc'"); 
	}
    }
}
	
/*! 
 *******************************************************************
 * \brief 
 ********************************************************************/
mgMedia::~mgMedia()
{
  if( m_filters )
    {        
      delete m_filters;
    }
}
  
/*! 
 *******************************************************************
 * \brief
 ********************************************************************/
string mgMedia::getMediaTypeName()
{
    switch(m_mediatype)
    {
	case GD_MP3:
	    return "GiantDisc";
    }	    
    mgError("implementation Error"); // we should never get here
    return "";
}
  
  
/*! 
 *******************************************************************
 * \brief
 ********************************************************************/
mgSelectionTreeNode* mgMedia::getSelectionRoot()
{
    switch(m_mediatype)
    {
	case GD_MP3:
	    return new GdTreeNode(m_db, m_defaultView, m_sql_filter);
    }	    
    mgError("implementation Error"); // we should never get here
    return NULL;
}


/*! 
 *******************************************************************
 * \brief
 ********************************************************************/
mgPlaylist* mgMedia::createTemporaryPlaylist()
{
    string tmpname = "current";
    return loadPlaylist(tmpname);
}

/*! 
 *******************************************************************
 * \brief
 ********************************************************************/
mgPlaylist* mgMedia::loadPlaylist(string name)
{
    mgPlaylist *list;
    switch(m_mediatype)
    {
	case GD_MP3:
	    list =  new GdPlaylist(name, m_db);
	    list->setDisplayColumns(getDefaultCols());
	    return list;
    }	 
    mgError("implementation Error"); // we should never get here
    return NULL;
}

/*! 
 *******************************************************************
 * \brief
 ********************************************************************/
vector<string> *mgMedia::getStoredPlaylists()
{
    switch(m_mediatype)
    {
	case GD_MP3:
	    return GdGetStoredPlaylists(m_db);
    }	 
    mgError("implementation Error"); // we should never get here
    return new vector<string>();
}

/*! 
 *******************************************************************
 * \brief
 ********************************************************************/
vector<int> mgMedia::getDefaultCols()
{
    vector<int> cols;
    switch(m_mediatype)
    {
	case GD_MP3:
	    cols.push_back(1); // artist
	    cols.push_back(0); // track
	    return cols;
    }	 
    mgError("implementation Error"); // we should never get here
    
    return cols;
}

/*! 
 *******************************************************************
 * \brief
 ********************************************************************/
mgTracklist* mgMedia::getTracks()
{
    mgTracklist *tracks;
    switch(m_mediatype)
    {
	case GD_MP3:
	    tracks = new GdTracklist(m_db, m_sql_filter);
	    tracks->setDisplayColumns(getDefaultCols());
	    return tracks;
    }	 
    mgError("implementation Error"); // we should never get here
    
    return NULL;
}

/*! 
 *******************************************************************
 * \brief creates FiliterSetObject for the selected media type 
 *        and activates set n (if available)
 ********************************************************************/
void mgMedia::initFilterSet(int num)
{
    switch(m_mediatype)
    {
	case GD_MP3:
	  m_filters = new gdFilterSets();
	  m_filters->select(num);
	  break;
    }	 
}

/*! 
 *******************************************************************
 * \brief returns pointer to the activen filter set to be modified by the osd
 *
 *  Note: Modifications become only effective by calling applyActiveFilter()
 ********************************************************************/
vector<mgFilter*> *mgMedia::getActiveFilters()
{
  if(!m_filters)
  {
      mgError("ImplementationError: getActiveFilters m_filters == NULL");
  }
  return m_filters->getFilters();
}

/*! 
 *******************************************************************
 * \brief returns title of the active filter set
 ********************************************************************/
string mgMedia::getActiveFilterTitle()
{

    switch(m_mediatype)
    {
     case GD_MP3:
       if(!m_filters)
       {
	 mgError("ImplementationError:getActiveFilterTitle m_filters == NULL");
       }
       return m_filters->getTitle();
    }	 
    return "";
}

/*! 
 *******************************************************************
 * \brief proceeds to the next filter set in a cirlucar fashion
 ********************************************************************/
void mgMedia::nextFilterSet()
{
  if(!m_filters)
  {
    mgError("ImplementationError: nextFilterSet  m_filters == NULL");
  }
  m_filters->nextSet();
}

/*! 
 *******************************************************************
 * \brief clears the current filter values and restores defaults
 ********************************************************************/
void mgMedia::clearActiveFilter()
{
  if(!m_filters)
  {
    mgError("ImplementationError: clearActiveFilter m_filters == NULL");
  }
  m_filters->clear();
}

/*! 
 *******************************************************************
 * \brief Applies the active filter set and returns a root node for the 
 *        selection in the default view for this filter set
 ********************************************************************/
mgSelectionTreeNode *mgMedia::applyActiveFilter()
{
  int view;
  GdTreeNode* node;

  switch(m_mediatype)
  {
    case GD_MP3:
      if(!m_filters)
      {
	mgError("ImplementationError: applyActiveFilter() m_filters == NULL");
      }
      m_filters->accept();
      m_sql_filter = m_filters->computeRestriction(&view);
      node = new GdTreeNode(m_db, view,  m_sql_filter);
      node->expand();
      return node->getChildren()[0];
  }	 
  return NULL;
}


/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_media.c,v $
 * Revision 1.12  2004/02/23 16:30:58  RaK
 * - album search error because of i18n corrected
 *
 * Revision 1.11  2004/02/12 09:15:07  LarsAC
 * Moved filter classes into separate files
 *
 * Revision 1.10  2004/02/10 23:47:23  RaK
 * - views konsitent gemacht. siehe FROMJOIN
 * - isLeafNode angepasst fuer neue views 4,5,100,101
 * - like '%abba%' eingebaut
 * - filter ist default mit abba gefuellt, zum leichteren testen.
 * - search results werden jetzt gleich im ROOT expanded
 *
 * Revision 1.9  2004/02/09 19:27:52  MountainMan
 * filter set implemented
 *
 * Revision 1.8  2004/02/02 23:33:41  MountainMan
 * impementation of gdTrackFilters
 *
 * Revision 1.7  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */


