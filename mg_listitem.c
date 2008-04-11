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
#include <assert.h>

mgListItem::mgListItem() {
	m_valid=false;
	m_unique_id="0";
	m_count=0;
}

mgListItem::mgListItem(const mgListItem* from) {
	assert(from);
	m_valid=from->m_valid;
	m_value=from->m_value;
	m_id=from->m_id;
	m_unique_id=from->m_unique_id;
	m_count=from->m_count;
}

mgListItem::mgListItem(string v,string i,unsigned int c) {
	set(v,i,c);
}

mgListItem*
mgListItem::Clone() {
	if (!this)
		return 0;
	else
		return new mgListItem(this);
}

void
mgListItem::set(string v,string i,unsigned int c) {
	assert(this);
	m_valid=true;
	m_value=v;
	m_id=i;
	m_count=c;
}

void
mgListItem::set_unique_id(string uid) {
	assert(this);
	m_unique_id=uid;
}

void
mgListItem::operator=(const mgListItem& from) {
	assert(this);
	m_valid=from.m_valid;
	m_value=from.m_value;
	m_id=from.m_id;
	m_unique_id=from.m_unique_id;
	m_count=from.m_count;
}

void
mgListItem::operator=(const mgListItem* from) {
	assert(this);
	m_valid=from->valid();
	m_value=from->value();
	m_id=from->id();
	m_unique_id=from->unique_id();
	m_count=from->count();
}

bool
mgListItem::operator==(const mgListItem& other) const
{
	if (!this)
		return false;
	return m_value == other.m_value
		&& m_id == other.m_id
		&& m_unique_id == other.m_unique_id;
}

string
mgListItem::value() const
{
	if (!this)
		return "";
	else
		return m_value;
}

string
mgListItem::id() const
{
	if (!this)
		return "";
	else
		return m_id;
}

string
mgListItem::unique_id() const
{
	if (!this)
		return "";
	else
		return m_unique_id;
}

unsigned int
mgListItem::count() const
{
	if (!this)
		return 0;
	else
		return m_count;
}

bool
mgListItem::valid() const
{
	return this && m_valid;
}
