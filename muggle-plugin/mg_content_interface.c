/*! \file  mg_content_interface.c
 *  \brief  Data Objects for content (e.g. mp3 files, movies) for the vdr muggle plugin
 *
 * \version $Revision: 1.5 $
 * \date    $Date: 2004/05/28 15:29:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: lvw $
 *
 * Implements main classes of for content items and interfaces to SQL databases
 *
 * This file implements the following classes 
 * - GdTracklist    a playlist
 * - mgGdTrack       a single track (content item). e.g. an mp3 file
 * - mgSelection   a set of tracks (e.g. a database subset matching certain criteria)
 *
 */
#define DEBUG

#include "mg_content_interface.h"
#include "mg_tools.h"

#define DUMMY

/* constructor */
mgContentItem mgContentItem::UNDEFINED =  mgContentItem();

using namespace std;

/*!
 * \brief constructor
 *
 * create an empty tracklist
 */
mgTracklist::mgTracklist()
{
 
}

/*!
 * \brief destrucor
 *
 *  Deletes all items in the tracklist and removes the list itself
 */
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
    if( position >= m_list.size() ) 
      {
	return false;
      }
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
/*  class mgSelectionTreeNode                                      */
/*                                                                 */
/*=================================================================*/
mgSelectionTreeNode::mgSelectionTreeNode(MYSQL db, int view)
{
    m_db = db;
    m_parent = NULL;
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
  if (m_view < 100 || m_level > 1) {
    return  m_parent;
  } else {
    return NULL;
  }
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

/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_content_interface.c,v $
 * Revision 1.5  2004/05/28 15:29:18  lvw
 * Merged player branch back on HEAD branch.
 *
 *
 * Revision 1.4  2004/02/23 15:41:21  RaK
 * - first i18n attempt
 *
 * Revision 1.3.2.6  2004/05/27 07:58:38  lvw
 * Removed bugs in moving and removing tracks from playlists
 *
 * Revision 1.3.2.5  2004/05/25 00:10:45  lvw
 * Code cleanup and added use of real database source files
 *
 * Revision 1.3.2.4  2004/05/04 16:51:53  lvw
 * Debugging aids added.
 *
 * Revision 1.3.2.3  2004/03/08 21:42:22  lvw
 * Added count method. Some comments for further todos added.
 *
 * Revision 1.3.2.2  2004/03/08 07:14:27  lvw
 * Preliminary changes to muggle player
 *
 * Revision 1.3.2.1  2004/03/02 07:08:12  lvw
 * 118 additions
 *
 * Revision 1.4  2004/02/23 15:41:21  RaK
 * - first i18n attempt
 *
 * Revision 1.3  2004/02/10 23:47:23  RaK
 * - views konsitent gemacht. siehe FROMJOIN
 * - isLeafNode angepasst fuer neue views 4,5,100,101
 * - like '%abba%' eingebaut
 * - filter ist default mit abba gefuellt, zum leichteren testen.
 * - search results werden jetzt gleich im ROOT expanded
 *
 * Revision 1.2  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */
