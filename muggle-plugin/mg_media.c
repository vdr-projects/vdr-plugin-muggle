/*******************************************************************/
/*! \file   mg_media.c
 *  \brief  Top level access to media in vdr plugin muggle
 *          for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/01 18:22:53 $
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

mgFilters::mgFilters()
{
}

mgFilters::~mgFilters()
{
}

int mgFilters::getNumFilters()
{
    return 0;
}

string mgFilters::getName(int filter)
{
    return 0;
}

int mgFilters::getValue(int filter)
{
    return 0;
}

mgFilters::filterType mgFilters::getType(int filter)
{
    return NUMBER;
}

// for NUMBER filters

int mgFilters::getMin(int filter)
{
    return 0;
}

int mgFilters::getMax(int filter)
{
    return 0;
}

// for CHOICE

vector<string> mgFilters::getChoices()
{
    return vector<string>();
}

int mgFilters::getCurrent(int filter)
{
    return 0;
}

    // check, if a value is correct
bool mgFilters::checkValue(int filter, string value)
{
    return false;
}

bool mgFilters::checkValue(int filter, int value)
{
    return false;
}
    
// finally set the values
bool mgFilters::setValue(int filter, string value)
{
    return false;
}

bool mgFilters::setValue(int filter, int value)
{
    return false;
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

    m_mediatype = mediatype;
    m_filter = "1";
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
	    return new GdTreeNode(m_db, m_defaultView, m_filter);
    }	    
    mgError("implementation Error"); // we should never get here
    return NULL;
}

mgFilters mgMedia::getActiveFilters()
{
    switch(m_mediatype)
    {
	case DUMMY:
	    return mgFilters();
	case GD_MP3:
	    return mgFilters();
    }	 
    mgError("implementation Error"); // we should never get here
    return mgFilters();
}

void mgMedia::setFilters(mgFilters filters)
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

vector<string> mgMedia::getStoredPlaylists()
{
    switch(m_mediatype)
    {
	case DUMMY:
	    return  DummyGetStoredPlaylists(m_db);
	case GD_MP3:
	    return GdGetStoredPlaylists(m_db);
    }	 
    mgError("implementation Error"); // we should never get here
    return vector<string>();
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
	    
	    tracks = new DummyTracklist(m_db, m_filter);
	    tracks->setDisplayColumns(getDefaultCols());
	    return tracks;
	case GD_MP3:
	    tracks = new GdTracklist(m_db, m_filter);
	    tracks->setDisplayColumns(getDefaultCols());
	    return tracks;
    }	 
    mgError("implementation Error"); // we should never get here
    
    return NULL;
}















