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
#include "mg_keymaps.h"
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

//! \brief adds string n to string s, using a comma to separate them
static string comma (string & s, string n);

/*! \brief returns a random integer within some range
 */
unsigned int
randrange (const unsigned int high)
{
    unsigned int result=0;
    result = random () % high;
    return result;
}


static string
comma (string & s, string n)
{
    return addsep (s, ",", n);
}


void
mgSelection::mgListItems::clear()
{
	m_items.clear();
}

bool
mgSelection::mgListItems::operator==(const mgListItems&x) const
{
	bool result = m_items.size()==x.m_items.size();
	if (result)
		for (unsigned int i=0;i<size();i++)
			result &= m_items[i]==x.m_items[i];
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

mgListItem&
mgSelection::mgListItems::operator[](unsigned int idx)
{
	if (!m_sel)
		mgError("mgListItems: m_sel is 0");
	m_sel->refreshValues();
	if (idx>=size()) return zeroitem;
	return m_items[idx];
}

void
mgSelection::mgListItems::setOwner(mgSelection* sel)
{
	m_sel = sel;
}

unsigned int
mgSelection::mgListItems::valindex (const string v) const
{
	return index(v,true);
}

unsigned int
mgSelection::mgListItems::idindex (const string i) const
{
	return index(i,true);
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
             if (m_items[i].value() == s)
             return i;
	}
	else
	{
             if (m_items[i].id() == s)
             return i;
	}
    }
    // nochmal mit neuen Werten:
    if (second_try) {
    	mgWarning("index: Gibt es nicht:%s",s.c_str());
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
	return listitems[gotoPosition()].value();
}

mgListItem&
mgSelection::getKeyItem(const unsigned int level) const
{
       return order.getKeyItem(level);
}


mgKeyTypes
mgSelection::getKeyType (const unsigned int level) const
{
        return order.getKeyType(level);
}

mgContentItem *
mgSelection::getItem (unsigned int position)
{
    if (position >= getNumItems ())
        return 0;
    return &(m_items[position]);
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
    unsigned int numitems = getNumItems();
    if (numitems==0) return;
    switch (m_shuffle_mode)
    {
        case SM_NONE:
        {
    	    long id = m_items[getItemPosition()].getItemid ();
            m_current_tracks = "";                // force a reload
            numitems = getNumItems();		  // getNumItems also reloads
    	    for (unsigned int i = 0; i < numitems; i++)
        	if (m_items[i].getItemid () == id)
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
            mgContentItem tmp = m_items[getItemPosition()];
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
    if (!m_db.Connected()) return 0;
    CreateCollection(Name);
    string listid = m_db.sql_string (m_db.get_col0
        ("SELECT id FROM playlist WHERE title=" + m_db.sql_string (Name)));
    unsigned int numitems = getNumItems ();
    if (numitems==0)
	    return 0;

    // this code is rather complicated but works in a multi user
    // environment:
 
    // insert a unique trackid:
    string trackid = ltos(m_db.thread_id()+1000000);
    m_db.exec_sql("INSERT INTO playlistitem SELECT "+listid+","
	   "MAX(tracknumber)+"+ltos(numitems)+","+trackid+
	   " FROM playlistitem WHERE playlist="+listid);
    
    // find tracknumber of the trackid we just inserted:
    string sql = string("SELECT tracknumber FROM playlistitem WHERE "
		    "playlist=")+listid+" AND trackid="+trackid;
    long first = atol(m_db.get_col0(sql).c_str()) - numitems + 1;

    // replace the place holder trackid by the correct value:
    m_db.exec_sql("UPDATE playlistitem SET trackid="+ltos(m_items[numitems-1].getItemid())+
		    " WHERE playlist="+listid+" AND trackid="+trackid);
    
    // insert all other items:
    const char *sql_prefix = "INSERT INTO playlistitem VALUES ";
    sql = "";
    for (unsigned int i = 0; i < numitems-1; i++)
    {
	string item = "(" + listid + "," + ltos (first + i) + "," +
            ltos (m_items[i].getItemid ()) + ")";
	comma(sql, item);
	if ((i%100)==99)
	{
    		m_db.exec_sql (sql_prefix+sql);
		sql = "";
	}
    }
    if (!sql.empty()) m_db.exec_sql (sql_prefix+sql);
    if (inCollection(Name)) clearCache ();
    return numitems;
}


unsigned int
mgSelection::RemoveFromCollection (const string Name)
{
    if (!m_db.Connected()) return 0;
    mgParts p = order.Parts(m_db,m_level,false);
    string sql = p.sql_delete_from_collection(KeyMaps.id(keyCollection,Name));
    m_db.exec_sql (sql);
    unsigned int removed = m_db.affected_rows ();
    if (inCollection(Name)) clearCache ();
    return removed;
}


bool mgSelection::DeleteCollection (const string Name)
{
    if (!m_db.Connected()) return false;
    ClearCollection(Name);
    m_db.exec_sql ("DELETE FROM playlist WHERE title=" + m_db.sql_string (Name));
    if (isCollectionlist()) clearCache ();
    return (m_db.affected_rows () == 1);
}


void mgSelection::ClearCollection (const string Name)
{
    if (!m_db.Connected()) return;
    string listid = KeyMaps.id(keyCollection,Name);
    m_db.exec_sql ("DELETE FROM playlistitem WHERE playlist="+m_db.sql_string(listid));
    if (inCollection(Name)) clearCache ();
}


bool mgSelection::CreateCollection(const string Name)
{
    if (!m_db.Connected()) return false;
    string name = m_db.sql_string(Name);
    if (m_db.exec_count("SELECT count(title) FROM playlist WHERE title = " + name)>0) 
	return false;
    m_db.exec_sql ("INSERT playlist VALUES(" + name + ",'VDR',NULL,NULL,NULL)");
    if (isCollectionlist()) clearCache ();
    return true;
}


string mgSelection::exportM3U ()
{

// open a file for writing
    string fn = "/tmp/" + ListFilename () + ".m3u";
    FILE * listfile = fopen (fn.c_str (), "w");
    if (!listfile)
        return "";
    fprintf (listfile, "#EXTM3U\n");
    unsigned int numitems = getNumItems ();
    for (unsigned i = 0; i < numitems; i++)
    {
        mgContentItem& t = m_items[i];
        fprintf (listfile, "#EXTINF:%d,%s\n", t.getDuration (),
            t.getTitle ().c_str ());
	fprintf (listfile, "#MUGGLE:%ld\n", t.getItemid());
        fprintf (listfile, "%s\n", t.getSourceFile (false).c_str ());
    }
    fclose (listfile);
    return fn;
}

bool
mgSelection::empty()
{
    if (m_level>= order.size ()-1)
	return ( getNumItems () == 0);
    else
	return ( listitems.size () == 0);
}

void
mgSelection::setPosition (unsigned int position)
{
    if (m_level == order.size())
	m_items_position = position;
    else
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
	    return getItemPosition();
    else
	    return m_position;
}

unsigned int
mgSelection::gotoPosition ()
{
    assert (m_level<order.size());
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
    unsigned int numitems = getNumItems ();
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
    unsigned int numitems = getNumItems();
    if (numitems == 0)
    {
	m_items_position=0;
        return false;
    }
    unsigned int old_pos = m_items_position;
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
	if (m_items[m_items_position].Valid())
		break;
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
    unsigned int numitems = getNumItems ();
    for (unsigned int i = 0; i < numitems; i++)
        result += m_items[i].getDuration ();
    return result;
}


unsigned long
mgSelection::getCompletedLength () const
{
    unsigned long result = 0;
    items ();                                    // make sure they are loaded
    for (unsigned int i = 0; i < getItemPosition(); i++)
        result += m_items[i].getDuration ();
    return result;
}



string mgSelection::getListname () const
{
    list<string> st;
    for (unsigned int i = 0; i < m_level; i++)
	    st.push_back(order.getKeyItem(i).value());
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

const vector < mgContentItem > &
mgSelection::items () const
{
    if (!m_db.Connected()) return m_items;
    if (!m_current_tracks.empty()) return m_items;
    m_current_tracks=order.GetContent(m_db,m_level,m_items);
    if (m_shuffle_mode!=SM_NONE)
    	Shuffle();
    return m_items;
}


void mgSelection::InitSelection() {
	m_Directory=".";
    	m_level = 0;
    	m_position = 0;
    	m_items_position = 0;
    	m_itemid = -1;
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
mgSelection::setOrder(mgOrder* o)
{
	if (o)
 	{
		order = *o;
	}
	else
		mgWarning("mgSelection::setOrder(0)");
}


void
mgSelection::InitFrom(mgValmap& nv)
{
	extern time_t createtime;
	if (m_db.ServerConnected() && !m_db.Connected() 
		&& (time(0)>createtime+10))
	{
	    char *b;
	    asprintf(&b,tr("Create database %s?"),the_setup.DbName);
	    if (Interface->Confirm(b))
	    {
		    char *argv[2];
		    argv[0]=".";
		    argv[1]=0;
		    m_db.Create();
	            if (Interface->Confirm(tr("Import items?")))
		    {
		        mgThreadSync *s = mgThreadSync::get_instance();
		        if (s)
		        {
				extern char *sync_args[];
				s->Sync(sync_args,(bool)the_setup.DeleteStaleReferences);
			}
		    }
	    }
	    free(b);
	}
	InitSelection();
	m_fall_through = nv.getbool("FallThrough");
    	m_Directory = nv.getstr("Directory");
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
	m_itemid = nv.getlong("ItemId");
	setPosition(nv.getstr("Position"));
	if (m_level==order.size()) 
		m_items_position = nv.getlong("ItemPosition");
}


mgSelection::~mgSelection ()
{
}

void mgSelection::InitFrom(const mgSelection* s)
{
    InitSelection();
    m_fall_through = s->m_fall_through;
    m_Directory = s->m_Directory;
    order = s->order;
    m_level = s->m_level;
    m_position = s->m_position;
    m_itemid = s->m_itemid;
    m_items_position = s->m_items_position;
    setShuffleMode (s->getShuffleMode ());
    setLoopMode (s->getLoopMode ());
}

unsigned int
mgSelection::valcount (string value)
{
	return listitems[listitems.valindex(value)].count();
}

void
mgSelection::refreshValues ()  const
{
    assert(this);
    if (!m_db.Connected())
	return;
    if (m_current_values.empty())
    {
	mgParts p =  order.Parts(m_db,m_level);
        m_current_values = p.sql_select();
        listitems.clear ();
        MYSQL_RES *rows = m_db.exec_sql (m_current_values);
        if (rows)
        {
                unsigned int num_fields = mysql_num_fields(rows);
		MYSQL_ROW row;
            	while ((row = mysql_fetch_row (rows)) != 0)
            	{
			if (!row[0]) continue;
			string r0 = row[0];
			if (!strcmp(row[0],"NULL")) // there is a genre NULL!
				continue;
			mgListItem n;
			if (num_fields==3)
			{
				if (!row[1]) continue;
				n.set(row[0],row[1],atol(row[2]));
			}
			else
                		n.set(value(order.Key(m_level),r0),r0,atol(row[1]));
			listitems.push_back(n);
            	}
            	mysql_free_result (rows);
        }
    }
}

unsigned int
mgSelection::count () const
{
    return listitems.size ();
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
    mgListItem item = listitems[position];
    mgListItems prev;
    if (m_fall_through && listitems.size()<10)
	    prev=listitems;
    while (1)
    {
        if (m_level >= order.size () - 1)
            return false;
        order[m_level++]->set (item);
	clearCache();
        m_position = 0;
	refreshValues();
	if (count()==0)
		break;
	item=listitems[0];
        if (!m_fall_through)
            break;
        if (m_level==order.size()-1)
	    break;
	if (count () > 1 && !(prev==listitems))
            break;
    }
    return true;
}


bool mgSelection::select (unsigned int position)
{
    if (m_level == order.size () - 1)
    {
        if (getNumItems () <= position)
            return false;
        order[m_level]->set (listitems[position]);
        m_level++;
        m_itemid = m_items[position].getItemid ();

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
        m_level--;
	prevvalue=order.getKeyItem(m_level).value();
	order[m_level]->set(zeroitem);
        m_itemid = -1;
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
        if (m_level<order.size()) order[m_level]->set (zeroitem);
	m_level--;
	prevvalue=order.getKeyItem(m_level).value();
        if (m_level<order.size()) order[m_level]->set (zeroitem);
	clearCache();
        if (!m_fall_through)
            break;
        if (count () > 1 && !(prev==listitems))
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
		order[i]->set (zeroitem);
	clearCache();
}

void 
mgSelection::selectfrom(mgOrder& oldorder,mgContentItem* o)
{
	leave_all();
	mgListItem selitem;
	assert(m_level==0);
	for (unsigned int idx = 0; idx < order.size(); idx++)
	{
		selitem = zeroitem;
		mgKeyTypes new_kt = getKeyType(idx);
		for (unsigned int i=0;i<oldorder.size();i++)
		{
			mgKeyTypes old_kt = oldorder.getKeyType(i);
			if (old_kt==new_kt)
				selitem = oldorder.getKeyItem(i);
			else if (old_kt>new_kt
					&& iskeyGenre(old_kt)
					&& iskeyGenre(new_kt))
			{
				string selid=KeyMaps.id(new_kt,KeyMaps.value(new_kt,oldorder.getKeyItem(i).id()));
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
			order[m_level++]->set (selitem);
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
		order[m_level]->set(zeroitem);
		setPosition(selitem.value());
		order[m_level+1]->set(zeroitem);
	}
	assert(m_level<order.size());
}


string
mgSelection::value(mgKey* k, string idstr) const
{
	return KeyMaps.value(k->Type(),idstr);
}

string
mgSelection::value(mgKey* k) const
{
	return value(k,k->id());
}


string
mgSelection::id(mgKey* k, string val) const
{
	return KeyMaps.id(k->Type(),val);
}

string
mgSelection::id(mgKey* k) const
{
	return k->id();
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
    if (order.size()==0) return false;
    bool result = (order.getKeyType(0) == keyCollection && m_level == 1);
    if (result)
	    if (order.getKeyType(1) != keyCollectionItem)
		    mgError("inCollection: key[1] is not keyCollectionItem");
    if (!Name.empty())
    	result &= (order.getKeyItem(0).value() == Name);
    return result;
}


void mgSelection::DumpState(mgValmap& nv) const
{
	nv.put("FallThrough",m_fall_through);
	nv.put("Directory",m_Directory);
	nv.put("Level",int(m_level));
    	for (unsigned int i=0;i<order.size();i++)
    	{
		if (i<m_level) {
			char *n;
			asprintf(&n,"order.Keys.%u.Position",i);
			nv.put(n,getKeyItem(i).value());
			free(n);
		}
	}
	nv.put("ItemId",m_itemid);
	nv.put("Position",listitems[m_position].value());
	if (m_level>=order.size()-1) 
		nv.put("ItemPosition",getItemPosition());
}

map <mgKeyTypes, string>
mgSelection::UsedKeyValues() 
{
	map <mgKeyTypes, string> result;
	for (unsigned int idx = 0 ; idx < m_level ; idx++)
	{
		result[order.getKeyType(idx)] = order.getKeyItem(idx).value();
	}
	if (m_level < order.size()-1)
	{
		mgKeyTypes ch =  order.getKeyType(m_level);
		result[ch] = getCurrentValue();
	}
	return result;
}

static vector<int> keycounts;

unsigned int
mgSelection::keycount(mgKeyTypes kt)
{
	if (keycounts.size()==0)
	{
		for (unsigned int ki=int(mgKeyTypesLow);ki<=int(mgKeyTypesHigh);ki++)
		{
			keycounts.push_back(-1);
		}
	}
	int& count = keycounts[int(kt-mgKeyTypesLow)];
	if (count==-1)
	{
		mgKey* k = ktGenerate(kt);
		if (k->Enabled(m_db))
			count = m_db.exec_count(k->Parts(m_db,true).sql_count());
		else
			count = 0;
		delete k;
	}
	return count;
}


