/*!
 * \file mg_listitem.c
 * \brief An item as delivered by mgSelection
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include "mg_listitem.h"

mgListItem zeroitem;

mgListItem::mgListItem()
{
	m_valid=false;
	m_count=0;
}

mgListItem::mgListItem(string v,string i,unsigned int c)
{
	set(v,i,c);
}

void
mgListItem::set(string v,string i,unsigned int c)
{
	m_valid=true;
	m_value=v;
	m_id=i;
	m_count=c;
}

void 
mgListItem::operator=(const mgListItem& from)
{
	m_valid=from.m_valid;
	m_value=from.m_value;
	m_id=from.m_id;
	m_count=from.m_count;
}

void
mgListItem::operator=(const mgListItem* from)
{
	m_valid=from->m_valid;
	m_value=from->m_value;
	m_id=from->m_id;
	m_count=from->m_count;
}

bool
mgListItem::operator==(const mgListItem& other) const
{
	return m_value == other.m_value
		&& m_id == other.m_id;
}
