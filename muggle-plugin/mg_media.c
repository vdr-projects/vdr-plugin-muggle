/*******************************************************************/
/*! \file   mg_media.c
 *  \brief  Top level access to media in vdr plugin muggle
 *          for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.9 $
 * \date    $Date: 2004/02/09 19:27:52 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: MountainMan $
 * 
 *
 */
/*******************************************************************/

/* makes sure we dont parse the same declarations twice */
#include "mg_media.h"
#include "mg_tools.h"
#include "mg_content_interface.h"
#include "gd_content_interface.h"


using namespace std;

//-------------------------------------------------------------------
//                         mgFilter
//-------------------------------------------------------------------
mgFilter::mgFilter(const char* name)
{
  m_name = strdup(name);
}
mgFilter::~mgFilter()
{
  free(m_name);
}

const char* mgFilter::getName()
{
  return m_name;
}

mgFilter::filterType mgFilter::getType()
{
  return m_type;
}

//-------------------------------------------------------------------
//        mgFilterInt
//-------------------------------------------------------------------
mgFilterInt::mgFilterInt(const char *name, int value, int min, int max)
  : mgFilter(name)
{
  m_type = INT;
  m_intval = value;
  m_default_val = value;
  m_stored_val = value;
  m_max = max;
  m_min = min;
}
mgFilterInt::~mgFilterInt()
{
}

string mgFilterInt::getStrVal()
{
  char buffer[20];
  sprintf(buffer, "%d", m_intval);

  return (string)buffer;
}

int mgFilterInt::getIntVal()
{
  return (int) m_intval;
}

int mgFilterInt::getVal()
{
  return m_intval;
}

int mgFilterInt::getMin()
{
  return m_min;
}

int mgFilterInt::getMax()
{
  return m_max;
}

void mgFilterInt::store()
{
  m_stored_val = m_intval;
}
void mgFilterInt::restore()
{
  m_intval = m_stored_val;
}
void mgFilterInt::clear()
{
  m_stored_val = m_default_val;
  m_intval     =  m_default_val;
}

bool mgFilterInt::isSet()
{
  if(m_stored_val == m_default_val)
  {
    return false;
  }
  return true;
}

//-------------------------------------------------------------------
//       mgFilterString
//-------------------------------------------------------------------
mgFilterString::mgFilterString(const char *name, const char* value,
			       int maxlen, string allowedchar)
  : mgFilter(name)
{
  m_type = STRING;
  m_strval = strdup(value);
  m_default_val = strdup(value);
  m_stored_val  = strdup(value);
  m_allowedchar = allowedchar;
  m_maxlen = maxlen;
}
mgFilterString::~mgFilterString()
{
  if(m_strval)
    {
      free(m_strval);
    }
}
  
int mgFilterString::getMaxLength()
{
  return m_maxlen;
} 

string mgFilterString::getAllowedChars()
{
  return m_allowedchar;
}
string mgFilterString::getStrVal()
{
 
  return (string) m_strval;
}
void mgFilterString::store()
{
  if(m_stored_val) free(m_stored_val);
  m_stored_val = strdup(m_strval);
}
void mgFilterString::restore()
{
  if(m_strval) free(m_strval);
  m_strval = strdup(m_stored_val);
}
void mgFilterString::clear()
{
  if(m_stored_val) free(m_stored_val);
  if(m_strval) free(m_strval);

  m_stored_val = strdup(m_default_val);
  m_strval = strdup(m_default_val);
}

bool mgFilterString::isSet()
{
  if(strlen(m_stored_val) == 0)
  {
    return false;
  }
  return true;
}
//-------------------------------------------------------------------
//        mgFilterBool
//-------------------------------------------------------------------
mgFilterBool::mgFilterBool(const char *name, bool value,
			   string truestr, string falsestr)
  : mgFilter(name)
{
  m_type = BOOL;
  m_bval  = (int) value;
  m_default_val = value;
  m_stored_val  = value;
  m_truestr = truestr;
  m_falsestr = falsestr;
  
}

mgFilterBool::~mgFilterBool()
{
}

string mgFilterBool::getStrVal()
{
  if(m_bval)
    return "true";
  else
    return "false";
}

int mgFilterBool::getIntVal()
{
  return (int) m_bval;
}

string mgFilterBool::getTrueString()
{
  return m_truestr;
}

string mgFilterBool::getFalseString()
{
  return m_falsestr;
}

bool mgFilterBool::getVal()
{
  return (bool) m_bval;
}

void mgFilterBool::store()
{
  m_stored_val = (bool) m_bval;
}

void mgFilterBool::restore()
{
  m_bval = (int) m_stored_val;
}

void mgFilterBool::clear()
{
  m_stored_val = (int) m_default_val;
  m_bval       = (int) m_default_val;
}

bool mgFilterBool::isSet()
{
  if(m_stored_val == m_default_val )
  {
    return false;
  }
  return true;
}
//-------------------------------------------------------------------
//        mgFilterChoice
//-------------------------------------------------------------------
mgFilterChoice::mgFilterChoice(const char *name, int value, vector<string> *choices)
  : mgFilter(name)
{
  m_choices = *choices;
  m_selval = value;
  m_default_val = value;
  if( m_selval < 0 || m_selval >= (int) m_choices.size() )
  {
    mgError("mgFilterChoice::mgFilterChoice(..): Illegal index %d", m_selval);
  }
}
mgFilterChoice::~mgFilterChoice()
{
  m_choices.clear();
}

string mgFilterChoice::getStrVal()
{
  if( m_selval < 0 || m_selval >= (int) m_choices.size() )
  {
    mgError("mgFilterChoice::getStrVal(): Illegal index %d", m_selval);
  }
  return m_choices[m_selval];
}
vector<string> &mgFilterChoice::getChoices()
{
  return m_choices;
}
void mgFilterChoice::store()
{
  m_stored_val = m_selval;
  
}
void mgFilterChoice::restore()
{
  m_selval =  m_stored_val;
}
void mgFilterChoice::clear()
{
  m_stored_val =  m_default_val;
  m_selval       =  m_default_val;
}

bool mgFilterChoice::isSet()
{
  if(m_stored_val == m_default_val)
  {
    return false;
  }
  return true;
}

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
	    return "GiantDisc-mp3";
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

  switch(m_mediatype)
  {
    case GD_MP3:
      if(!m_filters)
      {
	mgError("ImplementationError: applyActiveFilter() m_filters == NULL");
      }
      m_filters->accept();
      m_sql_filter = m_filters->computeRestriction(&view);
      return new GdTreeNode(m_db, view,  m_sql_filter);
  }	 
  return NULL;
}


/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_media.c,v $
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


