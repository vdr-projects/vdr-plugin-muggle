/* \file mg_item.h
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_ITEM_H
#define _MG_ITEM_H
#include <string>

using namespace std;

#include "mg_listitem.h"
#include "mg_keytypes.h"
#include "mg_selection.h"

class mgSelection;

//! \brief represents a content item 
class mgItem
{
    public:
//! \brief copy constructor
//	mgItem (const mgItem* c);
	mgItem();
	virtual mgItem* Clone();
        virtual ~mgItem() {};
	bool Valid(bool Silent=false) const;
	virtual long getItemid() const { return m_itemid; }
	virtual mgListItem* getKeyItem(mgKeyTypes kt) const { return new mgListItem; }
//! \brief returns filename
        virtual string getSourceFile (bool AbsolutePath=true,bool Silent=false) const { return m_realfile; }
//! \brief returns title
        string getTitle () const { return m_title; }
//! \brief returns the name of the language
        string getLanguage () const { return m_language; }
//! \brief returns year
        int getYear () const { return m_year; }
//! \brief returns rating
        int getRating () const { return m_rating; }
//! \brief returns duration
        int getDuration () const { return m_duration; }
	void setSelection(const mgSelection* sel) { m_sel=sel; }
	const mgSelection* getSelection() const { return m_sel; }

    protected:
	mutable bool m_valid;
	mutable bool m_validated;
        long m_itemid;
	string m_title;
	string m_realfile;
        int m_year;
        int m_rating;
        int m_duration;
        string m_genre_id;
        string m_genre;
        string m_language_id;
        string m_language;
	bool readable(string filename) const;
	void analyze_failure(string file) const;
	void InitFrom(const mgItem* c);
	const mgSelection* m_sel;
};


#endif
