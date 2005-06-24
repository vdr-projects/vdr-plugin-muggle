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
#include <assert.h>

#include "mg_selection.h"
#include "mg_setup.h"
#include "mg_tools.h"
#include "mg_thread_sync.h"

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

bool compvalue (const mgListItem* x, const mgListItem* y)
{
	return x->value()<y->value();
}

bool compid (const mgListItem* x, const mgListItem* y)
{
	return x->id()<y->id();
}

bool compidnum (const mgListItem* x, const mgListItem* y)
{
	return atol(x->id().c_str())<atol(y->id().c_str());
}

bool compcount (const mgListItem* x, const mgListItem* y)
{
	return x->count()>y->count();
}

bool compitem (const mgItem* x, const mgItem* y)
{
	const mgSelection *s = x->getSelection();
	string xval="";
	string yval="";
	int xnum;
	int ynum;
	for (unsigned int idx=s->orderlevel();idx<s->ordersize();idx++)
	{
		mgSortBy sb = s->getKeySortBy(idx);
		mgListItem *xitem = x->getKeyItem(s->getKeyType (idx));
		mgListItem *yitem = y->getKeyItem(s->getKeyType (idx));
		switch (sb) {
			case mgSortNone:
				xval="";
				yval="";
				break;
			case mgSortById:
				xval=xitem->id();
				yval=yitem->id();
				break;
			case mgSortByIdNum:
				xnum=atol(xitem->id().c_str());
				ynum=atol(yitem->id().c_str());
				if (xnum<ynum) {
					xval="0";
					yval="1";
				} else if (xnum>ynum) {
					xval="1";
					yval="0";
				} else {
					xval="0";
					yval="0";
				}
				break;
			case mgSortByValue:
				xval=xitem->value();
				yval=yitem->value();
				break;
		}
		if (xval!=yval) break;
	}
	return xval<yval;
}

void
mgSelection::mgListItems::sort(bool bycount,mgSortBy SortBy)
{
	if (SortBy==mgSortNone)
	{
		return;
	}
	if (bycount)
	{
		std::sort(m_items.begin(),m_items.end(),compcount);
	}
	else if (SortBy==mgSortById)
	{
		std::sort(m_items.begin(),m_items.end(),compid);
	}
	else if (SortBy==mgSortByIdNum)
	{
		std::sort(m_items.begin(),m_items.end(),compidnum);
	}
	else
	{
		std::sort(m_items.begin(),m_items.end(),compvalue);
	}
}

void
mgSelection::mgListItems::clear()
{
	for (unsigned int i=0;i<m_items.size();i++)
		delete m_items[i];
	m_items.clear();
}

bool
mgSelection::mgListItems::operator==(const mgListItems&x) const
{
	bool result = m_items.size()==x.m_items.size();
	if (result)
		for (unsigned int i=0;i<size();i++)
			result &= *(m_items[i])==*(x.m_items[i]);
	return result;
}

size_t
mgSelection::mgListItems::size() const
{
	if (!m_sel)
		mgError("mgListItems: m_sel is 0");
	m_sel->refreshValues();
	return m_items.size();
}

mgListItem*
mgSelection::mgListItems::operator[](unsigned int idx)
{
	if (!m_sel)
		mgError("mgListItems: m_sel is 0");
	m_sel->refreshValues();
	assert(idx<m_items.size());
	return m_items[idx];
}

void
mgSelection::mgListItems::setOwner(mgSelection* sel)
{
	m_sel = sel;
}

int
mgSelection::mgListItems::search (const string v) const
{
    if (!m_sel)
	mgError("mgListItems::index(%s): m_sel is 0",v.c_str());
    unsigned int itemsize = m_items.size();
    const char *cstr = v.c_str();
    unsigned int clen = strlen(cstr);
    int result = -1;
    for (unsigned int idx = 0 ; idx < itemsize; idx++)
	if( strncasecmp( m_items[idx]->value().c_str(), cstr, clen )>=0)
	{
		result = idx;
		break;
	}
    return result;
}

unsigned int
mgSelection::mgListItems::valindex (const string v) const
{
	return index(v,true);
}

unsigned int
mgSelection::mgListItems::idindex (const string i) const
{
	return index(i,false);
}

unsigned int
mgSelection::mgListItems::index (const string s,bool val,bool second_try) const
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
    	mgDebug(5,"index: Gibt es nicht:%s",s.c_str());
	mgDebug(5,"index: wir haben z.B.");
	if (size()>0) mgDebug(5,"%s/%s",m_items[0]->value().c_str(),m_items[0]->id().c_str());
	if (size()>1) mgDebug(5,"%s/%s",m_items[1]->value().c_str(),m_items[1]->id().c_str());
	if (size()>2) mgDebug(5,"%s/%s",m_items[2]->value().c_str(),m_items[2]->id().c_str());
	if (size()>3) mgDebug(5,"%s/%s",m_items[3]->value().c_str(),m_items[3]->id().c_str());
	if (size()>4) mgDebug(5,"%s/%s",m_items[4]->value().c_str(),m_items[4]->id().c_str());
	if (size()>5) mgDebug(5,"%s/%s",m_items[5]->value().c_str(),m_items[5]->id().c_str());
    	return 0;
    }
    else
    {
    	m_sel->clearCache();
        return index(s,val,true);
    }
}

bool
mgSelection::inItem() const
{
	return m_level==ordersize()-1;
}

bool
mgSelection::inItems() const
{
	return m_level==ordersize()-2;
}

void
mgSelection::setOrderByCount(bool orderbycount)
{
	m_orderByCount = orderbycount;
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
       assert(level<Keys.size());
       return Keys[level]->get();
}


mgKeyTypes
mgSelection::getKeyType (const unsigned int level) const
{
       assert(level<Keys.size());
       return Keys[level]->Type();
}

mgSortBy
mgSelection::getKeySortBy (const unsigned int level) const
{
       assert(level<Keys.size());
       return Keys[level]->SortBy();
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
    int result = m_db->AddToCollection(Name,items(),0);
    if (result>0)
	    if (inCollection(Name)) clearCache ();
    return result;
}


unsigned int
mgSelection::RemoveFromCollection (const string Name)
{
    mgParts p = SelParts(false,false);
    unsigned int result = m_db->RemoveFromCollection(Name,items(),&p);
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
    m_db->ClearCollection (Name);
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
    return ( listitems.size () == 0);
}

void
mgSelection::setPosition (unsigned int position)
{
    assert(m_level<ordersize());
    m_position = position;
}

void
mgSelection::setPosition(string value)
{
	setPosition (listitems.valindex (value));
}

unsigned int
mgSelection::searchPosition(string search)
{
	int res = listitems.search (search);
	if (res>=0)
		setPosition (res);
	return gotoPosition();
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
    assert(m_level<ordersize());
    return m_position;
}

unsigned int
mgSelection::gotoPosition ()
{
    assert(m_level<ordersize());
    unsigned int itemsize = listitems.size();
    if (itemsize==0)
	m_position = 0;
    else if (m_position >= itemsize)
       	m_position = itemsize -1;
    if (itemsize==0)
	Key(m_level)->set (0);
    else
	Key(m_level)->set (listitems[m_position]);
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
	    st.push_back(getKeyItem(i)->value());
    st.unique();
    string result="";
    for (list < string >::iterator it = st.begin (); it != st.end (); ++it)
	addsep (result, ":", *it);
    if (result.empty ())
	if (ordersize()>0)
		result = string(ktName(getKeyType(0)));
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

mgParts
mgSelection::SelParts(bool distinct, bool deepsort) const
{
	assert(m_level<ordersize());
	mgKey *high = Keys[m_level];
	mgListItem* highitem = 0;
	if (high->Type()!=keyGdUnique)
	{
		highitem = high->get();
		high->set(0);
	}
	mgParts result;
	result.orderByCount = m_orderByCount;
	for (unsigned int i=0;i<ordersize();i++)
	{
		if (!deepsort && i>m_level)
			break;
		if (NeedKey(i))
		{
			Keys[i]->Parts(m_db,distinct&&i==m_level).Dump("SelPart");
			result += Keys[i]->Parts(m_db,distinct&&i==m_level);
		}
	}
	if (highitem)
	{
		high->set(highitem);
		delete highitem;
	}
	return result;
}

const vector < mgItem* > &
mgSelection::items () const
{
    if (m_current_tracks.empty())
    {
	mgParts p = SelParts(false,true);
    	m_current_tracks =  m_db->LoadItemsInto(p,m_items);
    	if (m_shuffle_mode==SM_NONE)
	{
    		if (!inCollection(""))
		{
			for (unsigned int i=0;i<m_items.size();i++)
				m_items[i]->setSelection(this);
			std::sort(m_items.begin(),m_items.end(),compitem);
		}
	}
	else
    			Shuffle();
    }
    return m_items;
}


void mgSelection::InitSelection() {
	m_active = false;
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
       	clear();
	m_orderByCount = false;
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


void mgSelection::DumpState(mgValmap& nv, const char *prefix) const
{
	nv.put(m_fall_through,"%s.FallThrough",prefix);
	nv.put(m_orderByCount,"%s.OrderByCount",prefix);
    	for (unsigned int i=0;i<ordersize();i++)
    	{
		nv.put(int(Key(i)->Type()),"%s.Keys.%d.Type",prefix,i);
		if (i<=m_level)
			nv.put(getKeyItem(i)->value(),
				"%s.Keys.%u.Position",prefix,i);
	}
}

void
mgSelection::InitFrom(const char *prefix,mgValmap& nv)
{
	InitSelection();
	clear();
	m_fall_through = true;
	setOrderByCount(nv.getbool("%s.OrderByCount",prefix));
	for (unsigned int idx = 0 ; idx < 999 ; idx++)
	{
		unsigned int type = nv.getuint("%s.Keys.%u.Type",prefix,idx);
		if (type==0) break;
		setKey(mgKeyTypes(type));
	}
	vector<mgListItem> items;
	for (unsigned int idx = 0 ; idx < ordersize() ; idx++)
	{
        	char b[100];
		sprintf(b,"%s.Keys.%u.Position",prefix,idx);
		if (!nv.count(b)) break;
		string newval = nv.getstr(b);
		items.push_back(mgListItem(newval,KeyMaps.id(Keys[idx]->Type(),newval),0));
	}
	if (ordersize() && Keys[ordersize()-1]->Type()!=keyGdUnique)
			setKey(keyGdUnique);
	InitOrder(items);
}


void 
mgSelection::CopyKeyValues(mgSelection* s)
{
	if (!s)
		mgError("mgSelection::CopyKeyValues(0)");
	if (s==this)
		return;
	mgItem *o=0;
	s->enter();
	if (s->items().size()==1)
		o = s->getItem(0)->Clone();
	SetLevel(0);
	vector<mgListItem> items;
	for (unsigned int idx = 0; idx < ordersize(); idx++)
	{
		bool found = false;
		mgKeyTypes new_kt = getKeyType(idx);
		if (o && o->getItemid()>=0)
		{
			items.push_back(o->getKeyItem(new_kt));
			found = true;
			continue;
		}
		if (s) for (unsigned int i=0;i<s->ordersize();i++)
		{
			mgKeyTypes old_kt = s->getKeyType(i);
			if (old_kt==new_kt && s->getKeyItem(i))
			{
				items.push_back(s->getKeyItem(i));
				found = true;
				break;
			}
		}
		if (found)
			continue;
		if (!DeduceKeyValue(new_kt,s,items))
			break;
	}
	delete o;
	assert(items.size()<=ordersize());
	InitOrder(items);
}

void
mgSelection::InitOrder(vector<mgListItem>& items)
{
	mgDebug(5,"InitOrder:");
	for (unsigned int idx = 0; idx < items.size(); idx++)
		mgDebug(5,"%d:%s/%s",idx,items[idx].value().c_str(),items[idx].id().c_str());
	if (ordersize()==0)
		return;
	for (unsigned int idx = 0; idx < ordersize(); idx++)
		Key(idx)->set(0);	
	for (unsigned int idx = 0; idx < items.size(); idx++)
		Key(idx)->set (&items[idx]);
	m_active = false;
}

void 
mgSelection::Activate()
{
	assert(ordersize());
    if (m_level)
	    assert(m_level<ordersize());
	if (m_active)
		return;
	m_active = true;
	for (m_level = 0; m_level < ordersize(); m_level++)
		if (!getKeyItem(m_level))
			break;
	if (m_level>=ordersize())
		m_level = ordersize()-1;
    assert(m_level<ordersize());
	mgListItem* item = getKeyItem(0);
	if (item)
		setPosition(item->value());
	else
		setPosition(0);
	leave(); // we are ininItem()
	while (listitems.size()==0 && m_level>0)
		leave();
	assert(m_level<ordersize());
}

mgSelection::~mgSelection ()
{
    m_level=0;
    truncate(0);
    delete m_db;
}

void mgSelection::InitFrom(const mgSelection* s)
{
    InitSelection();
    if (!s)
	return;
    for (unsigned int i = 0; i < s->ordersize();i++)
    {
       	mgKey *k = ktGenerate(s->getKeyType(i));
	k->set(s->getKeyItem(i));
	Keys.push_back(k);
    }
    m_active = s->m_active;
    SetLevel(s->m_level);
    if (m_level)
    	assert(m_level<ordersize());
    m_fall_through = s->m_fall_through;
    m_orderByCount = s->m_orderByCount;
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
    mgParts p =  SelParts(true,false);
    m_current_values = m_db->LoadValuesInto(
		    p,getKeyType(m_level),listitems.items(),m_level<ordersize()-2);
    if (!inCollection(""))
	    listitems.sort(m_orderByCount,Keys[m_level]->SortBy());
}


void
mgSelection::DecLevel()
{
	m_level--;
	clearCache();
}

void
mgSelection::IncLevel()
{
	m_level++;
	clearCache();
}

void
mgSelection::SetLevel(unsigned int level)
{
	m_level=level;
	clearCache();
}

bool mgSelection::enter (unsigned int position)
{
    assert(!Keys.empty());
    if (inItem())
        return false;
    if (empty())
	refreshValues();
    if (empty())
	return false;
    mgDebug(5,"%X:level %d:enter(%d)",this,m_level,position);
    if (inCollection())
    {
	mgListItem *item=Key(m_level)->get();
	IncLevel();
	Key(m_level)->set(item);
	setPosition(0);
	gotoPosition();
	return true;
    }
    mgListItems prev;
    if (m_level<ordersize()-2 && m_fall_through && listitems.size()<100)
	prev=listitems;
    while (1)
    {
    	setPosition(position);
    	position = gotoPosition();		// reload adjusted position
        Key(m_level)->set (listitems[position]);
	IncLevel();
	refreshValues();
        position = 0;
	if (empty())
	    break;
        if (!m_fall_through)
            break;
        if (inItem())
	    break;
	if (listitems.size () > 1 && !(prev==listitems))
            break;
    }
    setPosition(position);
    position = gotoPosition();
    return true;
}

bool
mgSelection::leave ()
{
    unsigned int position=m_position;
    assert(!Keys.empty());
    mgListItems prev;
    if (m_level>1 && m_fall_through && listitems.size()<100)
	prev=listitems;
    while (1)
    {
    	setPosition(position);
    	position = gotoPosition();		// reload adjusted position
	Key(m_level)->set(0);
    	if (m_level==0)
            return false;
	DecLevel();
	refreshValues();
	position = listitems.valindex (getKeyItem(m_level)->value());
        if (!m_fall_through)
            break;
        if (m_level==0)
	    break;
	if (listitems.size () > 1 && !(prev==listitems))
            break;
    }
    setPosition(position);
    return true;
}

void
mgSelection::leave_all ()
{
	SetLevel(0);
	for (unsigned int i=0;i<ordersize();i++)
		Key(i)->set (0);
}


void
mgSelection::truncate(unsigned int i)
{
	while (ordersize()>i)
	{
		delete Keys.back();
		Keys.pop_back();
	}
}

void
mgSelection::setKey (const mgKeyTypes kt)
{
    mgKey *newkey = ktGenerate(kt);
    if (newkey)
    	Keys.push_back(newkey);
}

void
mgSelection::setKeys(vector<const char *>& kt)
{
	clear();
	for (unsigned int i=0;i<kt.size();i++)
	{
		setKey(ktValue(kt[i]));
	}
        clean();
}

void
mgSelection::clear()
{
	m_level=0;
	clearCache();
	truncate(0);
}

void
mgSelection::clean()
{
	// remove double entries:
	keyvector::iterator a;
	keyvector::iterator b;
	for (a = Keys.begin () ; a != Keys.end (); ++a)
	{
cleanagain:
		for (b = a+1 ; b != Keys.end(); ++b)
			if ((*a)->Type() == (*b)->Type())
			{
				delete *b;
				Keys.erase(b);
				goto cleanagain;
			}
	}
}

string
mgSelection::Name()
{
	string result="";
	if (ordersize()>0)
		for (unsigned int idx=0;idx<ordersize()-1;idx++)
		{
			if (!result.empty()) result += ":";
			result += ktName(Keys[idx]->Type());
		}
	return result;
}

bool
mgSelection::SameOrder(const mgSelection* other)
{
    bool result =  ordersize()==other->ordersize() && m_orderByCount == other->m_orderByCount;
    if (result)
    	for (unsigned int i=0; i<ordersize();i++)
    	{
    		result &= Key(i)->Type()==other->Key(i)->Type();
		if (!result) break;
    	}
    return result;
}

mgKey* 
mgSelection::Key(unsigned int idx) const
{
	assert(idx<ordersize());
	return Keys[idx];
}

bool
mgSelection::UsedBefore(const mgKeyTypes kt,unsigned int level) const
{
	for (unsigned int lx = 0; lx < level; lx++)
		if (getKeyType(lx)==kt)
			return true;
	return false;
}

static vector<int> keycounts;

unsigned int
mgSelection::keycount(mgKeyTypes kt) const
{
	if (keycounts.size()==0)
	{
		for (unsigned int ki=(unsigned int)(ktLow());ki<=(unsigned int)(ktHigh());ki++)
		{
			keycounts.push_back(-1);
		}
	}
	int& kcount = keycounts[int(kt-ktLow())];
	if (kcount==-1)
	{
		mgKey* k = ktGenerate(kt);
		if (k->Enabled(m_db))
			kcount = m_db->exec_count(k->Parts(m_db,true).sql_count());
		else
			kcount = 0;
		delete k;
	}
	return kcount;
}

mgKeyTypes
mgSelection::ktValue(const char * name) const
{
	for (int kt=int(ktLow());kt<=int(ktHigh());kt++)
		if (!strcmp(name,ktName(mgKeyTypes(kt))))
				return mgKeyTypes(kt);
	mgError("ktValue(%s): unknown name",name);
	return mgKeyTypes(0);
}

