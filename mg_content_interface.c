/*******************************************************************/
/*! \file  content_interface.c
 * \brief  Data Objects for content (e.g. mp3 files, movies)
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/01 18:22:53 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 *
 * DUMMY
 * Implements main classes of for content items and interfaces to SQL databases
 *
 * This file implements the following classes 
 * - GdTracklist    a playlist
 * - mgGdTrack       a single track (content item). e.g. an mp3 file
 * - mgSelection   a set of tracks (e.g. a database subset matching certain criteria)
 *
 */
/*******************************************************************/
#define DEBUG

#include "mg_content_interface.h"
#include "mg_tools.h"

#define DUMMY

/* constructor */
mgContentItem mgContentItem::UNDEFINED =  mgContentItem();

using namespace std;

/*!
 *****************************************************************************
 * \brief construcor
 *
 * creates empty tracklist
 ****************************************************************************/
mgTracklist::mgTracklist()
{
 
}

/*!
 *****************************************************************************
 * \brief destrucor
 *
 *  Deletes all items in the tracklist and removes the list itself
 ****************************************************************************/
mgTracklist::~mgTracklist()
{
  mgContentItem* ptr;
  vector<mgContentItem*>::iterator iter;
  
  for(iter = m_list.begin(); iter != m_list.end();iter++)
  {
    ptr = *iter;
    delete ptr;
  }
  m_list.clear();
}

/*!
 *****************************************************************************
 * \brief returns a pointer to the list of elements
 *
 ****************************************************************************/
vector<mgContentItem*> *mgTracklist::getAll()
{
  return &m_list;
}

/*!
 *****************************************************************************
 * \brief returns the number of elements in the list
 *
 ****************************************************************************/
unsigned int mgTracklist::getNumItems()
{
  return m_list.size();
}

/*!
 *****************************************************************************
 * \brief randomizes the order of the elements in the list 
 * 
 ****************************************************************************/
void mgTracklist::shuffle()
{
  random_shuffle(m_list.begin(),m_list.end ());
}

/*!
 *****************************************************************************
 * \brief sorts the elements in the list by the nth column
 * 
 ****************************************************************************/
void mgTracklist::sortBy(int col, bool direction)
{
}

/*!
 *****************************************************************************
 * \brief stores the ids of columns to be used in label creation
 * 
 * The list can create a label with different fields (columns) using the
 * function getLabel()
 * This function defines the fields of the contentItems to be used 
 * in the label and their order
 ****************************************************************************/
void mgTracklist::setDisplayColumns(vector<int> cols)
{    

  m_columns = cols;
}

/*!
 *****************************************************************************
 * \brief returns the number of dsplay coulmns 
 * 
 ****************************************************************************/
unsigned int mgTracklist::getNumColumns()
{
  return m_columns.size();
}


/*!
 *****************************************************************************
 * \brief creates the label string for an item
 * 
 * The list can create a label with different fields (columns).
 * The fields used in the list and their order is set by the function
  using the function setDisplayColumns
 * function getLabel().
 * This function creates a string from these columns, separated by the string
 * 'separator'
 * in the label and their order
 ****************************************************************************/
string mgTracklist::getLabel(unsigned int position, const string separator)
{
    string label ="";
    mgContentItem* item;

    if(position >= m_list.size())
	return "";

    else 
    {
	item = *(m_list.begin()+position);
    }
     
   
    for( vector<int>::iterator iter = m_columns.begin();
	 iter != m_columns.end(); iter++ )
    {
	if( iter != m_columns.begin() )
	{
	    label += separator;
	}
	label += item->getLabel(*iter);
    }
    return label;
}
  

/*!
 *****************************************************************************
 * \brief returns an item from the list at the specified position
 * 
 ****************************************************************************/
mgContentItem* mgTracklist::getItem(unsigned int position)
{
    if(position >= m_list.size())
	return &(mgContentItem::UNDEFINED); //invalid
    return *(m_list.begin()+position);
}

/*!
 *****************************************************************************
 * \brief remove item at position
 * 
 ****************************************************************************/
bool mgTracklist::remove(unsigned int position)       
{
    if(position >= m_list.size()) return false;
    vector<mgContentItem*>::iterator iter;
    
    iter = m_list.begin()+ position;
    m_list.erase(iter);
    return true;
}

/*!
 *****************************************************************************
 * \brief remove all occurences of item
 * 
 ****************************************************************************/
int mgTracklist::remove(mgContentItem* item) 
{
    int retval = 0;
    vector<mgContentItem*>::iterator iter;
    for(iter=m_list.begin(); iter != m_list.end (); iter++)
    {
	     if(*iter == item) 
	     {
		 m_list.erase(iter);
		 retval++;
		 break;
	     }
    }
    return retval;
}


/*=================================================================*/
/*                                                                 */
/*  class mgPlaylist                                               */
/*                                                                 */
/*=================================================================*/
mgPlaylist::mgPlaylist()
{
}
mgPlaylist::mgPlaylist(string listname)
{
    m_listname = listname;
}
     
     /*==== destructor ====*/
mgPlaylist::~mgPlaylist()
{

}
/*==== add/ remove tracks ====*/

/* adds a song at the end of the playlist */
void mgPlaylist::append(mgContentItem* item)
{
    m_list.push_back(item);
}

void mgPlaylist::appendList(vector<mgContentItem*> *tracks)
{
    vector<mgContentItem*>::iterator iter;
    mgDebug(3, "Adding %d tracks to the playlist",tracks->size()); 
    for(iter = tracks->begin(); iter != tracks->end(); iter++)
    {
	m_list.push_back(*iter);
    }
    tracks->clear();
}


/* adds a song after 'position' */
void mgPlaylist::insert(mgContentItem* item, unsigned int position)
{
    if(position >= m_list.size())
	m_list.push_back(item);
    else
	m_list.insert(m_list.begin()+position, item);
}



/*====  access tracks ====*/
string mgPlaylist::getListname() { return m_listname; }
void mgPlaylist::setListname(string name){ m_listname = name;}



// returns the first item of the list
mgContentItem* mgPlaylist::getFirst()
{
    m_current = m_list.begin();
    return *m_current;
}

// returns the  nth track from the playlist
mgContentItem* mgPlaylist::getPosition(unsigned int position)
{
    if(position >= m_list.size())
	return &(mgContentItem::UNDEFINED); //invalid
    m_current = m_list.begin()+position;
    return *m_current;
}

// proceeds to the next item
mgContentItem*  mgPlaylist::skipFwd()
{
    if(m_current+1 == m_list.end())
	return &(mgContentItem::UNDEFINED); //invalid
    else
	return * (++m_current);
}

// goes back to the previous item
mgContentItem*  mgPlaylist::skipBack()
{
    if(m_current == m_list.begin())
	return &(mgContentItem::UNDEFINED); //invalid
    else
	return * (--m_current);
}

// get next track, do not update data structures
mgContentItem* mgPlaylist::sneakNext()
{
    if(m_current+1 == m_list.end())
	return &(mgContentItem::UNDEFINED); //invalid
    else
	return * (m_current+1);
}


/*=================================================================*/
/*                                                                 */
/*  class mgSelectionTreeNode                                      */
/*                                                                 */
/*=================================================================*/
mgSelectionTreeNode::mgSelectionTreeNode(MYSQL db, int view)
{
    m_db = db;
    m_parent=NULL;
    m_level = 0;
    m_view = view;
    m_id = "";
    m_label = "ROOT";
    m_expanded = false;
}
mgSelectionTreeNode::mgSelectionTreeNode(mgSelectionTreeNode* parent, string id, string label)
{
    m_parent = parent;
    m_level  = m_parent->m_level+1;
    m_view   = m_parent->m_view;
    m_db     = m_parent->m_db;
    m_id = id;
    m_label = label;
    m_expanded = false;
}

    /*==== destructor ====*/
mgSelectionTreeNode::~mgSelectionTreeNode() 
{
  collapse();
//    _children.clear();
}

mgSelectionTreeNode*  mgSelectionTreeNode::getParent()
{
    return  m_parent;
}

void mgSelectionTreeNode::collapse() // removes all children (recursively)
{
  vector <mgSelectionTreeNode*>::iterator iter;
  mgSelectionTreeNode* ptr;
  
  for(iter = m_children.begin(); iter != m_children.end();iter++)
  {
    ptr = *iter;
    delete ptr;
  }
  m_expanded = false;
  m_children.clear();
}
// access children
vector<mgSelectionTreeNode*> &mgSelectionTreeNode::getChildren()
{
  mgDebug(5," returning %d children", m_children.size());
  return m_children;
}

// access data in  current node
string mgSelectionTreeNode::getID()
{
    return m_id;
}
string mgSelectionTreeNode::getLabel()
{
    return m_label;
}
string mgSelectionTreeNode::getLabel(int n)
{
    mgSelectionTreeNode* node = this;
    int d = m_level;
    while(n < d)
    {
	node = node->m_parent;
	d--;
    }
    return node->m_label;
} 

string mgSelectionTreeNode::getRestrictions()
{
    return m_restriction;
}
