/*! \file  mg_content_interface.c
 *  \brief  Data Objects for content (e.g. mp3 files, movies) for the vdr muggle plugin
 *
 * \version $Revision: 1.6 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * Implements main classes of for content items and abstract interfaces to media access
 *
 * This file implements the following classes 
 *   - mgTracklist
 *   - mgSelectionTreeNode
 */
#define DEBUG

#include <algorithm>
#include "mg_content_interface.h"
#include "mg_tools.h"


//! \brief a special item representing an undefined state
mgContentItem mgContentItem::UNDEFINED = mgContentItem();

mgTracklist::mgTracklist()
{
 
}

mgTracklist::~mgTracklist()
{
  mgContentItem* ptr;
  std::vector<mgContentItem*>::iterator iter;
  
  for( iter = m_list.begin(); iter != m_list.end(); iter++ )
  {
    ptr = *iter;
    delete ptr;
  }
  m_list.clear();
}

std::vector<mgContentItem*> *mgTracklist::getAll()
{
  return &m_list;
}

unsigned int mgTracklist::getNumItems()
{
  return m_list.size();
}

unsigned long mgTracklist::getLength()
{
  unsigned long result = 0;
  std::vector<mgContentItem*>::iterator iter;

  for( iter = m_list.begin(); iter != m_list.end (); iter++ )
    {
      result += (*iter)->getLength();
    }

  return result;
}

void mgTracklist::shuffle()
{
  random_shuffle( m_list.begin(), m_list.end () );
}

void mgTracklist::sortBy(int col, bool direction)
{
}

void mgTracklist::setDisplayColumns(std::vector<int> cols)
{    
  m_columns = cols;
}

unsigned int mgTracklist::getNumColumns()
{
  return m_columns.size();
}

std::string mgTracklist::getLabel(unsigned int position, const std::string separator)
{
    std::string label = "";
    mgContentItem* item;

    if( position >= m_list.size() )
      {
	return "";
      }
    else 
      {
	item = *( m_list.begin() + position );
      }     

    for( std::vector<int>::iterator iter = m_columns.begin();
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
  
mgContentItem* mgTracklist::getItem(unsigned int position)
{
  if( position >= m_list.size() )
    {
      return &(mgContentItem::UNDEFINED); //invalid
    }
  return *( m_list.begin() + position);
}

bool mgTracklist::remove(unsigned position)       
{
  bool result = false;

  if( position < (int)m_list.size() ) 
    {
      std::vector<mgContentItem*>::iterator iter;
      
      iter = m_list.begin() + position;
      m_list.erase(iter);
      
      result = true;
    }
  
  return result;
}

int mgTracklist::removeItem(mgContentItem* item) 
{
  int retval = 0;
  std::vector<mgContentItem*>::iterator iter;
  
  for( iter = m_list.begin(); iter != m_list.end (); iter++ )
    {
      if( *iter == item ) 
	{
	  m_list.erase(iter);
	  retval++;
	  break;
	}
    }
  return retval;
}

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
mgSelectionTreeNode::mgSelectionTreeNode(mgSelectionTreeNode* parent, std::string id, std::string label)
{
    m_parent = parent;
    m_level  = m_parent->m_level+1;
    m_view   = m_parent->m_view;
    m_db     = m_parent->m_db;
    m_id = id;
    m_label = label;
    m_expanded = false;
}

mgSelectionTreeNode::~mgSelectionTreeNode() 
{
  collapse();
  // TODO - lvw
  //    _children.clear();
}

mgSelectionTreeNode*  mgSelectionTreeNode::getParent()
{
  if (m_view < 100 || m_level > 1) 
    {
      return  m_parent;
    } 
  else 
    {
      return NULL;
    }
}

void mgSelectionTreeNode::collapse() // removes all children (recursively)
{
  std::vector <mgSelectionTreeNode*>::iterator iter;
  mgSelectionTreeNode* ptr;

  for(iter = m_children.begin(); iter != m_children.end();iter++)
    {
      ptr = *iter;
      delete ptr;
    }
  m_expanded = false;
  m_children.clear();
}

std::vector<mgSelectionTreeNode*> &mgSelectionTreeNode::getChildren()
{
  // mgDebug(5," returning %d children", m_children.size());
  return m_children;
}

std::string mgSelectionTreeNode::getID()
{
  return m_id;
}

std::string mgSelectionTreeNode::getLabel()
{
  return m_label;
}

std::string mgSelectionTreeNode::getLabel(int n)
{
  mgSelectionTreeNode* node = this;
  int d = m_level;
  while(n < d)
    {
      // TODO: check for NULL
      node = node->m_parent;
      d--;
    }
  
  return node->m_label;
} 

std::string mgSelectionTreeNode::getRestrictions()
{
  return m_restriction;
}
