/*******************************************************************/
/*! \file   mg_media.c
 *  \brief  Top level access to media in vdr plugin muggle
 *          for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.5 $
 * \date    $Date: 2004/02/02 19:42:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 * 
 *
 */
/*******************************************************************/

/* makes sure we dont parse the same declarations twice */
#include "mg_media.h"
#include "mg_tools.h"
#include "mg_content_interface.h"
#include "gd_content_interface.h"

#include "sh_dummy_content.h"

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
  m_min = min;
  m_max = max;
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
int mgFilterInt::getMin()
{
  return m_min;
}
int mgFilterInt::getMax()
{
  return m_max;
}

//-------------------------------------------------------------------
//       mgFilterString
//-------------------------------------------------------------------
mgFilterString::mgFilterString(const char *name, const char* value)
  : mgFilter(name)
{
  m_type = STRING;
  m_strval = strdup(value);
}
mgFilterString::~mgFilterString()
{
  if(m_strval)
    {
      free(m_strval);
    }
}

string mgFilterString::getStrVal()
{
 
  return (string) m_strval;
}

//-------------------------------------------------------------------
//        mgFilterBool
//-------------------------------------------------------------------
mgFilterBool::mgFilterBool(const char *name, bool value)
  : mgFilter(name)
{
  m_type = BOOL;
  m_bval = value;
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

//-------------------------------------------------------------------
//        mgFilterChoice
//-------------------------------------------------------------------
mgFilterChoice::mgFilterChoice(const char *name, int val, vector<char*> *choices)
  : mgFilter(name)
{
  m_choices = *choices;
  m_selval = val;
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
vector<char*> &mgFilterChoice::getChoices()
{
  return m_choices;
}
//-------------------------------------------------------------------
//        mgTrackFilters
//-------------------------------------------------------------------
mgTrackFilters::mgTrackFilters()
{
}
mgTrackFilters::~mgTrackFilters()
{
  for(vector<mgFilter*>::iterator iter = m_filters.begin();
      iter != m_filters.end(); iter++)
    {
      delete (*iter);
    }
  m_filters.clear();
}

vector<mgFilter*> *mgTrackFilters::getFilters()
{
  return &m_filters;
}

/*! 
 *******************************************************************
 * \class mgMedia
 *
 * \brief mein class to access content in the vdr plugin muggle
 ********************************************************************/
mgMedia::mgMedia(contentType mediatype)
{
    int errval = 0;
    mgTrackFilters *m_trackfilter;
    m_mediatype = mediatype;
    m_sql_trackfilter = "1";
    m_defaultView = 1;

    // now initialize the database
    mgDebug(1, "Media Type %sselected", getMediaTypeName().c_str());
    switch(m_mediatype)
    {
	case DUMMY:
	{
	    errval = DummyInitDatabase(&m_db);
	    break;
	}
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
	case DUMMY:
	{
	    errval = DummyInitDatabase(&m_db);
	    break;
	}
	case GD_MP3:
	{
	    errval = GdInitDatabase(&m_db);
	    mgDebug(3, "Successfully conntected to sql database 'GiantDisc'"); 
	}
    }
}
	
mgMedia::~mgMedia()
{
}
  
string mgMedia::getMediaTypeName()
{
    switch(m_mediatype)
    {
	case DUMMY:
	    return "DUMMY";
	case GD_MP3:
	    return "GiantDisc-mp3";
    }	    
    mgError("implementation Error"); // we should never get here
    return "";
}
  
  
mgSelectionTreeNode* mgMedia::getSelectionRoot()
{
    switch(m_mediatype)
    {
	case DUMMY:
	    return new DummyTreeNode(m_db, m_defaultView);
	case GD_MP3:
	    return new GdTreeNode(m_db, m_defaultView, m_sql_trackfilter);
    }	    
    mgError("implementation Error"); // we should never get here
    return NULL;
}

vector<mgFilter*> *mgMedia::getTrackFilters()
{
    switch(m_mediatype)
    {
	case DUMMY:
	  //return mgFilters();
	case GD_MP3:
	  // return m_trackfilters;
        default:
	  break;
    }	 
    mgError("implementation Error"); // we should never get here
    return NULL;
}

void mgMedia::applyTrackFilters(vector<mgFilter*> *filters)
{
}

mgPlaylist* mgMedia::createTemporaryPlaylist()
{
    string tmpname = "current";
    return loadPlaylist(tmpname);
}

mgPlaylist* mgMedia::loadPlaylist(string name)
{
    mgPlaylist *list;
    switch(m_mediatype)
    {
	case DUMMY:
	    list = new DummyPlaylist(name, m_db);
	    list->setDisplayColumns(getDefaultCols());
	    return list;
	case GD_MP3:
	    list =  new GdPlaylist(name, m_db);
	    list->setDisplayColumns(getDefaultCols());
	    return list;
    }	 
    mgError("implementation Error"); // we should never get here
    return NULL;
}

vector<string> *mgMedia::getStoredPlaylists()
{
    switch(m_mediatype)
    {
	case DUMMY:
	    return  DummyGetStoredPlaylists(m_db);
	case GD_MP3:
	    return GdGetStoredPlaylists(m_db);
    }	 
    mgError("implementation Error"); // we should never get here
    return new vector<string>();
}

vector<int> mgMedia::getDefaultCols()
{
    vector<int> cols;
    switch(m_mediatype)
    {
	case DUMMY:
	    cols.push_back(1);  // artist
	    cols.push_back(0); // track
	    return cols;
	case GD_MP3:
	    cols.push_back(1); // artist
	    cols.push_back(0); // track
	    return cols;
    }	 
    mgError("implementation Error"); // we should never get here
    
    return cols;
}

mgTracklist* mgMedia::getTracks()
{
    mgTracklist *tracks;
    switch(m_mediatype)
    {
	case DUMMY:
	    
	    tracks = new DummyTracklist(m_db, m_sql_trackfilter);
	    tracks->setDisplayColumns(getDefaultCols());
	    return tracks;
	case GD_MP3:
	    tracks = new GdTracklist(m_db, m_sql_trackfilter);
	    tracks->setDisplayColumns(getDefaultCols());
	    return tracks;
    }	 
    mgError("implementation Error"); // we should never get here
    
    return NULL;
}















