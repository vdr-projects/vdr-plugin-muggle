/*!
 * \file mg_listitem.h
 * \brief an item as delivered by mgSelection
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_LISTITEM_H
#define _MG_LISTITEM_H
#include <stdlib.h>
#include <string>

using namespace std;


class mgListItem
{
	public:
		mgListItem();
		mgListItem(string v,string i,unsigned int c=0);
		mgListItem* Clone();
		void set(string v,string i,unsigned int c=0);
		void operator=(const mgListItem& from);
		void operator=(const mgListItem* from);
		bool operator==(const mgListItem& other) const;
		string value() const { return m_value; } 
		string id() const { return m_id; } 
		unsigned int count() const { return m_count; } 
		bool valid() const { return m_valid; }
	private:
		mgListItem(const mgListItem *from);
		bool m_valid;
		string m_value;
		string m_id;
		unsigned int m_count;
};

extern mgListItem zeroitem;

#endif
