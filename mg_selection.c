/*!
 * \file mg_selection.c
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fts.h>
#include <assert.h>

#include "i18n.h"
#include "mg_selection.h"
#include "vdr_setup.h"
#include "mg_tools.h"
#include "mg_thread_sync.h"

#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>

#if VDRVERSNUM >= 10307
#include <interface.h>
#include <skins.h>
#endif

/*! \brief returns a random integer within some range
 */
unsigned int
randrange (const unsigned int high)
{
    unsigned int result=0;
    result = random () % high;
    return result;
}


void
mgListItems::clear()
{
	for (unsigned int i=0;i<m_items.size();i++)
		delete m_items[i];
	m_items.clear();
}

bool
mgListItems::operator==(const mgListItems&x) const
{
	bool result = m_items.size()==x.m_items.size();
	if (result)
		for (unsigned int i=0;i<size();i++)
			result &= *(m_items[i])==*(x.m_items[i]);
	return result;
}

size_t
mgListItems::size() const
{
	if (!m_sel)
		mgError("mgListItems: m_sel is 0");
	m_sel->refreshValues();
	return m_items.size();
}

mgListItem*
mgListItems::operator[](unsigned int idx)
{
	if (!m_sel)
		mgError("mgListItems: m_sel is 0");
	m_sel->refreshValues();
	assert(idx<m_items.size());
	return m_items[idx];
}

void
mgListItems::setOwner(mgSelection* sel)
{
	m_sel = sel;
}

unsigned int
mgListItems::valindex (const string v) const
{
	return index(v,true);
}

unsigned int
mgListItems::idindex (const string i) const
{
	return index(i,true);
}

unsigned int
mgListItems::index (const string s,bool val,bool second_try) const
{
    if (!m_sel)
	mgError("mgListItems::index(%s): m_sel is 0",s.c_str());
    m_sel->refreshValues();
    for (unsigned int i = 0; i < size (); i++)
    {
        if (val)
	{
             if (m_items[i]->value() == s)
             return i;
	}
	else
	{
             if (m_items[i]->id() == s)
             return i;
	}
    }
    // nochmal mit neuen Werten:
    if (second_try) {
    	mgDebug(2,"index: Gibt es nicht:%s",s.c_str());
    	return 0;
    }
    else
    {
    	m_sel->clearCache();
        return index(s,val,true);
    }
}

void
mgSelection::clearCache() const
{
        m_current_values = "";
        m_current_tracks = "";
}

string
mgSelection::getCurrentValue()
{
	gotoPosition();
	return getValue(m_position);
}

string
mgSelection::getValue(unsigned int idx) const
{
	if (idx>=listitems.size())
		return "";
	else
		return listitems[idx]->value();
}

mgListItem*
mgSelection::getKeyItem(const unsigned int level) const
{
       return order.getKeyItem(level);
}


mgKeyTypes
mgSelection::getKeyType (const unsigned int level) const
{
        return order.getKeyType(level);
}

mgItem *
mgSelection::getItem (unsigned int position)
{
    if (position >= items().size())
        return 0;
    return m_items[position];
}


mgSelection::ShuffleMode mgSelection::toggleShuffleMode ()
{
    setShuffleMode((m_shuffle_mode == SM_PARTY) ? SM_NONE : ShuffleMode (m_shuffle_mode + 1));
    Shuffle();
    return getShuffleMode();
}

void
mgSelection::setShuffleMode (mgSelection::ShuffleMode mode)
{
    m_shuffle_mode = mode;
}

void
mgSelection::Shuffle() const
{
    unsigned int numitems = items().size();
    if (numitems==0) return;
    switch (m_shuffle_mode)
    {
        case SM_NONE:
        {
    	    long id = m_items[getItemPosition()]->getItemid ();
            m_current_tracks = "";                // force a reload
            numitems = items().size();		  // also reloads
    	    for (unsigned int i = 0; i < numitems; i++)
        	if (m_items[i]->getItemid () == id)
    		{
        		m_items_position = i;
        		break;
    		}
        }
        break;
        case SM_PARTY:
        case SM_NORMAL:
        {
	    // play all, beginning with current item:
            mgItem* tmp = m_items[getItemPosition()];
	    m_items[getItemPosition()]=m_items[0];
	    m_items[0]=tmp;
	    m_items_position = 0;
	    // randomize all other items
            for (unsigned int i = 1; i < numitems; i++)
            {
                unsigned int j = 1+randrange (numitems-1);
                tmp = m_items[i];
                m_items[i] = m_items[j];
                m_items[j] = tmp;
            }
        } break;
/*
 * das kapiere ich nicht... (wolfgang)
 - Party mode (see iTunes)
 - initialization
 - find 15 titles according to the scheme below
 - playing
 - before entering next title perform item selection
 - item selection
 - generate a random uid
 - if file exists:
 - determine maximum playcount of all items
- generate a random number n
- if n < playcount / max. playcount
- add the file to the end of the list
*/
    }
}


mgSelection::LoopMode mgSelection::toggleLoopMode ()
{
    m_loop_mode = (m_loop_mode == LM_FULL) ? LM_NONE : LoopMode (m_loop_mode + 1);
    return m_loop_mode;
}


unsigned int
mgSelection::AddToCollection (const string Name)
{
    int result = m_db->AddToCollection(Name,items());
    if (result>0)
	    if (inCollection(Name)) clearCache ();
    return result;
}


unsigned int
mgSelection::RemoveFromCollection (const string Name)
{
    unsigned int result = m_db->RemoveFromCollection(Name,&order,m_level);
    if (result>0)
    	if (inCollection(Name)) clearCache ();
    return result;
}


bool mgSelection::DeleteCollection (const string Name)
{
    bool result = m_db->DeleteCollection (Name);
    if (result)
    	if (isCollectionlist()) clearCache ();
    return result;
}


void mgSelection::ClearCollection (const string Name)
{
    m_db->DeleteCollection (Name);
    if (inCollection(Name)) clearCache ();
}


bool mgSelection::CreateCollection(const string Name)
{
    bool result = m_db->CreateCollection (Name);
    if (result)
	    if (isCollectionlist()) clearCache ();
    return result;
}


string mgSelection::exportM3U ()
{

// open a file for writing
    string fn = "/tmp/" + ListFilename () + ".m3u";
    FILE * listfile = fopen (fn.c_str (), "w");
    if (!listfile)
        return "";
    fprintf (listfile, "#EXTM3U\n");
    unsigned int numitems = items().size();
    for (unsigned i = 0; i < numitems; i++)
    {
        mgItem* t = m_items[i];
        fprintf (listfile, "#EXTINF:%d,%s\n", t->getDuration (),
            t->getTitle ().c_str ());
	fprintf (listfile, "#MUGGLE:%ld\n", t->getItemid());
        fprintf (listfile, "%s\n", t->getSourceFile (false).c_str ());
    }
    fclose (listfile);
    return fn;
}

bool
mgSelection::empty()
{
    if (m_level>= order.size ()-1)
	return ( items().size() == 0);
    else
	return ( listitems.size () == 0);
}

void
mgSelection::setPosition (unsigned int position)
{
    if (m_level == order.size())
	mgError("setPosition:m_level==order.size()");
    m_position = position;
}

void
mgSelection::GotoItemPosition (unsigned int position) const
{
    m_items_position = position;
    skipItems(0);
}

unsigned int
mgSelection::getPosition ()  const
{
    if (m_level == order.size())
	mgError("getPosition:m_level==order.size()");
    return m_position;
}

unsigned int
mgSelection::gotoPosition ()
{
    if (m_level == order.size())
	mgError("gotoPosition:m_level==order.size()");
    unsigned int itemsize = listitems.size();
    if (itemsize==0)
	m_position = 0;
    else if (m_position >= itemsize)
       	m_position = itemsize -1;
    return m_position;
}

unsigned int
mgSelection::getItemPosition() const
{
    if (m_items_position>=m_items.size())
	if (m_items.size()==0)
		m_items_position=0;
	else
		m_items_position = m_items.size()-1;
    return m_items_position;
}

unsigned int
mgSelection::gotoItemPosition()
{
    unsigned int numitems = items().size();
    if (numitems == 0)
    {
	m_items_position = 0;
	return 0;
    }
    if (m_items_position >= numitems)
        m_items_position = numitems -1;
    return m_items_position;
}

bool mgSelection::skipItems (int steps) const
{
    unsigned int numitems = items().size();
    if (numitems == 0)
    {
	m_items_position=0;
        return false;
    }
    int old_pos = m_items_position;
    unsigned int new_pos;
    if (m_loop_mode == LM_SINGLE)
    	steps = 0;
    if (old_pos + steps < 0)
    {
        if (m_loop_mode == LM_NONE)
            return false;
        new_pos = numitems - 1;
    }
    else
	new_pos = old_pos + steps;
    if (new_pos >= numitems)
    {
        if (m_loop_mode == LM_NONE)
           	return false;
        new_pos = 0;
    }
    m_items_position = new_pos;
    while (true)
    {
	if (m_items[m_items_position]->Valid())
		break;
	delete m_items[m_items_position];
	m_items.erase(m_items.begin()+m_items_position);
	if (m_items.size()==0)
	{
		m_items_position = 0;
		return false;
	}
	if (steps<0 && m_items.size()>0 && m_items_position>0)
		m_items_position--;
	if (m_items_position==m_items.size())
		m_items_position--;
    }
    return true;
}

unsigned long
mgSelection::getLength ()
{
    unsigned long result = 0;
    unsigned int numitems = items().size();
    for (unsigned int i = 0; i < numitems; i++)
        result += m_items[i]->getDuration ();
    return result;
}


unsigned long
mgSelection::getCompletedLength () const
{
    unsigned long result = 0;
    items ();                                    // make sure they are loaded
    for (unsigned int i = 0; i < getItemPosition(); i++)
        result += m_items[i]->getDuration ();
    return result;
}



string mgSelection::getListname () const
{
    list<string> st;
    for (unsigned int i = 0; i < m_level; i++)
	    st.push_back(order.getKeyItem(i)->value());
    st.unique();
    string result="";
    for (list < string >::iterator it = st.begin (); it != st.end (); ++it)
    {
	addsep (result, ":", *it);
    }
    if (result.empty ())
	if (order.size()>0)
        	result = string(ktName(order.getKeyType(0)));
    return result;
}

string mgSelection::ListFilename ()
{
    string res = getListname ();
    // convert char set ?
    string::iterator it;
    for (it=res.begin();it!=res.end();it++)
    {
	    char& c = *it;
	    switch (c)
	    {
		    case '\'':
		    case '/':
		    case '\\':
		    case ' ':
		    case ')':
	    	    case '(': c = '_';break;
    	    }
    }
    return res;
}

const vector < mgItem* > &
mgSelection::items () const
{
    if (!m_current_tracks.empty())
	return m_items;
    m_current_tracks=order.GetContent(dynamic_cast<mgDbGd*>(m_db),m_level,m_items);
    // \todo remove dynamic_cast
    if (m_shuffle_mode!=SM_NONE)
    	Shuffle();
    return m_items;
}


void mgSelection::InitSelection() {
	m_db = new mgDbGd;
    	m_level = 0;
    	m_position = 0;
    	m_items_position = 0;
    	if (the_setup.InitShuffleMode)
    		m_shuffle_mode = SM_NORMAL;
	else
    		m_shuffle_mode = SM_NONE;
	if (the_setup.InitLoopMode)
		m_loop_mode = LM_FULL;
	else
    		m_loop_mode = LM_NONE;
    	clearCache();
	listitems.setOwner(this);
}


mgSelection::mgSelection (const bool fall_through)
{
    InitSelection ();
    m_fall_through = fall_through;
}

mgSelection::mgSelection (const mgSelection &s)
{
    InitFrom(&s);
}

mgSelection::mgSelection (const mgSelection* s)
{
    InitFrom(s);
}

mgSelection::mgSelection (mgValmap& nv)
{
    InitFrom(nv);
}

void
mgSelection::setOrder(mgOrder* n)
{
	if (n)
 	{
		mgOrder oldorder = order;
		mgItem *o=0;
		select();
		if (items().size()==1)
			o = getItem(0)->Clone();
		order = *n;
		selectfrom(oldorder,o);
		delete o;
	}
	else
		mgWarning("mgSelection::setOrder(0)");
}


void
mgSelection::InitFrom(mgValmap& nv)
{
	InitSelection();
	m_fall_through = nv.getbool("FallThrough");
	while (m_level < nv.getuint("Level"))
	{
		char *idx;
		asprintf(&idx,"order.Keys.%u.Position",m_level);
        	string newval = nv.getstr(idx);
		free(idx);
        	if (!enter (newval))
            		if (!select (newval)) break;
	}
	assert(m_level<=order.size());
	if (m_level==order.size()) 
		m_items_position = nv.getlong("ItemPosition");
	else
		setPosition(nv.getstr("Position"));
}


mgSelection::~mgSelection ()
{
    delete m_db;
}

void mgSelection::InitFrom(const mgSelection* s)
{
    InitSelection();
    m_fall_through = s->m_fall_through;
    order = s->order;
    m_level = s->m_level;
    m_position = s->m_position;
    m_items_position = s->m_items_position;
    setShuffleMode (s->getShuffleMode ());
    setLoopMode (s->getLoopMode ());
}


void
mgSelection::refreshValues ()  const
{
    assert(this);
    assert(m_db);
    if (!m_current_values.empty())
        return;
    mgParts p =  order.Parts(m_db,m_level);
    m_current_values = p.sql_select();
    if (!m_db->LoadValuesInto(&order,m_level,listitems.items()))
	m_current_values = "";
}


bool mgSelection::enter (unsigned int position)
{
    if (order.empty())
	mgWarning("mgSelection::enter(%u): order is empty", position);
    if (empty())
	return false;
    if (m_level == order.size ())
        return false;
    setPosition (position);
    position = gotoPosition();		// reload adjusted position
    if (listitems.size()==0)
	return false;
    mgListItem item = *listitems[position];
    mgListItems prev;
    if (m_fall_through && listitems.size()<10)
	    prev=listitems;// \todo deep copy?
    while (1)
    {
        if (m_level >= order.size () - 1)
            return false;
        order[m_level++]->set (&item);
	clearCache();
        m_position = 0;
	refreshValues();
	if (listitems.size()==0)
		break;
	item=*listitems[0];
        if (!m_fall_through)
            break;
        if (m_level==order.size()-1)
	    break;
	if (listitems.size () > 1 && !(prev==listitems))
            break;
    }
    return true;
}


bool mgSelection::select (unsigned int position)
{
    if (m_level == order.size () - 1)
    {
        if (items().size() <= position)
            return false;
        order[m_level++]->set (listitems[position]);

	clearCache();
        return true;
    }
    else
        return enter (position);
}

bool
mgSelection::leave ()
{
    string prevvalue;
    if (order.empty())
    {
	mgWarning("mgSelection::leave(): order is empty");
	return false;
    }
    if (m_level == order.size ())
    {
	if (m_level<order.size())
		order[m_level]->set(0);
	m_level--;
	prevvalue=order.getKeyItem(m_level)->value();
	clearCache();
	setPosition(prevvalue);
        return true;
    }
    mgListItems prev;
    if (m_fall_through && listitems.size()<10)
	    prev=listitems;
    while (1)
    {
        if (m_level < 1)
            return false;
        order[m_level--]->set (0);
	prevvalue=order.getKeyItem(m_level)->value();
        if (m_level<order.size()) order[m_level]->set (0);
	clearCache();
        if (!m_fall_through)
            break;
        if (listitems.size () > 1 && !(prev==listitems))
            break;
    }
    setPosition(prevvalue);
    return true;
}

void
mgSelection::leave_all ()
{
	m_level=0;
	for (unsigned int i=0;i<order.size();i++)
		order[i]->set (0);
	clearCache();
}

void 
mgSelection::selectfrom(mgOrder& oldorder,mgItem* o)
{
	leave_all();
	mgListItem selitem;
	mgListItem zeroitem;
	assert(m_level==0);
	for (unsigned int idx = 0; idx < order.size(); idx++)
	{
		selitem = zeroitem;
		mgKeyTypes new_kt = getKeyType(idx);
		for (unsigned int i=0;i<oldorder.size();i++)
		{
			mgKeyTypes old_kt = oldorder.getKeyType(i);
			if (old_kt==new_kt && oldorder.getKeyItem(i))
				selitem = *oldorder.getKeyItem(i);
			else if (old_kt>new_kt
					&& iskeyGenre(old_kt)
					&& iskeyGenre(new_kt))
			{
				string selid=KeyMaps.id(new_kt,KeyMaps.value(new_kt,oldorder.getKeyItem(i)->id()));
				selitem=mgListItem (KeyMaps.value(new_kt,selid),selid);
			}
			if (selitem.valid()) break;
		}
		if (!selitem.valid() && o && o->getItemid()>=0)
			selitem = o->getKeyItem(new_kt);
		if (!selitem.valid())
			break;
		if (m_level<order.size()-1)
		{
			order[m_level++]->set (&selitem);
		}
		else
		{
			setPosition(selitem.value());
			return;
		}
	}
	if (m_level>0)
	{
		m_level--;
		selitem = order.getKeyItem(m_level);
		order[m_level]->set(0);
		setPosition(selitem.value());
		order[m_level+1]->set(0);
	}
	assert(m_level<order.size());
}

bool mgSelection::isLanguagelist() const
{
    return (order.getKeyType(0) == keyLanguage);
}

bool mgSelection::isCollectionlist () const
{
    if (order.size()==0) return false;
    return (order.getKeyType(0) == keyCollection && m_level == 0);
}

bool
mgSelection::inCollection(const string Name) const
{
    bool result = false;
    for (unsigned int idx = 0 ; idx <= m_level; idx++)
    {
	    if (idx==order.size()) break;
	    if (order.getKeyType(idx) == keyCollection)
		    if (order.getKeyItem(idx)->value() != Name) break;
	    if (order.getKeyType(idx) == keyCollectionItem)
	    {
		    result = true;
		    break;
	    }
    }
    return result;
}


void mgSelection::DumpState(mgValmap& nv) const
{
	nv.put("FallThrough",m_fall_through);
	nv.put("Level",int(m_level));
    	for (unsigned int i=0;i<order.size();i++)
    	{
		if (i<m_level) {
			char *n;
			asprintf(&n,"order.Keys.%u.Position",i);
			nv.put(n,getKeyItem(i)->value());
			free(n);
		}
	}
	nv.put("Position",getValue(m_position));
	if (m_level>=order.size()-1) 
		nv.put("ItemPosition",getItemPosition());
}

map <mgKeyTypes, string>
mgSelection::UsedKeyValues() 
{
	map <mgKeyTypes, string> result;
	for (unsigned int idx = 0 ; idx < m_level ; idx++)
	{
		result[order.getKeyType(idx)] = order.getKeyItem(idx)->value();
	}
	if (m_level < order.size()-1)
	{
		mgKeyTypes ch =  order.getKeyType(m_level);
		result[ch] = getCurrentValue();
	}
	return result;
}
