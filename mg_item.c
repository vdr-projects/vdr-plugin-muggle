/*!
 * \file mg_item.c
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include <unistd.h>

#include "mg_item.h"

bool
mgItem::Valid() const
{
    if (!m_validated)
    {
	    getSourceFile();	// sets m_valid as a side effect
	    m_validated=true;
    }
    return m_valid;
}

bool
mgItem::readable(string filename) const
{
	return !access(filename.c_str(),R_OK);
}

mgItem::mgItem()
{
    m_valid = false;
    m_validated = false;
    m_itemid = -1;
    m_year = 0;
    m_rating = 0;
    m_duration = 0;
}

mgItem*
mgItem::Clone ()
{
    if (!this)
	    return 0;
    mgItem* result = new mgItem();
    result->InitFrom(this);
    return result;
}

void
mgItem::InitFrom(const mgItem* c)
{
    m_valid = c->m_valid;
    m_validated = c->m_validated;
    m_itemid = c->m_itemid;
    m_title = c->m_title;
    m_realfile = c->m_realfile;
    m_genre_id = c->m_genre_id;
    m_genre = c->m_genre;
    m_language = c->m_language;
    m_language_id = c->m_language_id;
    m_year = c->m_year;
    m_rating = c->m_rating;
    m_duration = c->m_duration;
}

