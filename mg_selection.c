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

#include "i18n.h"
#include "mg_selection.h"
#include "vdr_setup.h"
#include "mg_tools.h"

#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>


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


static string zerostring;

bool
mgSelection::mgSelStrings::operator==(const mgSelStrings&x) const
{
	bool result = strings.size()==x.strings.size();
	if (result)
		for (unsigned int i=0;i<size();i++)
			result &= strings[i]==x.strings[i];
	return result;
}

size_t
mgSelection::mgSelStrings::size() const
{
	if (!m_sel)
		mgError("mgSelStrings: m_sel is 0");
	m_sel->refreshValues();
	return strings.size();
}

string&
mgSelection::mgSelStrings::operator[](unsigned int idx)
{
	if (!m_sel)
		mgError("mgSelStrings: m_sel is 0");
	m_sel->refreshValues();
	if (idx>=strings.size()) return zerostring;
	return strings[idx];
}

void
mgSelection::mgSelStrings::setOwner(mgSelection* sel)
{
	m_sel = sel;
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
	return values[gotoPosition()];
}

string
mgSelection::getKeyValue(const unsigned int level) const
{
       return order.getKeyValue(level);
}

unsigned int
mgSelection::getKeyIndex(const unsigned int level) const
{
	return valindex(getKeyValue(level));
}


mgKeyTypes
mgSelection::getKeyType (const unsigned int level) const
{
        return order.getKeyType(level);
}

mgContentItem *
mgSelection::getTrack (unsigned int position)
{
    if (position >= getNumTracks ())
        return 0;
    return &(m_tracks[position]);
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
    unsigned int tracksize = getNumTracks();
    if (tracksize==0) return;
    switch (m_shuffle_mode)
    {
        case SM_NONE:
        {
    	    long id = m_tracks[getTrackPosition()].getId ();
            m_current_tracks = "";                // force a reload
            tracksize = getNumTracks();		  // getNumTracks also reloads
    	    for (unsigned int i = 0; i < tracksize; i++)
        	if (m_tracks[i].getId () == id)
    		{
        		setTrackPosition(i);
        		break;
    		}
        }
        break;
        case SM_PARTY:
        case SM_NORMAL:
        {
	    // play all, beginning with current track:
            mgContentItem tmp = m_tracks[getTrackPosition()];
	    m_tracks[getTrackPosition()]=m_tracks[0];
	    m_tracks[0]=tmp;
	    setTrackPosition(0);
	    // randomize all other tracks
            for (unsigned int i = 1; i < tracksize; i++)
            {
                unsigned int j = 1+randrange (tracksize-1);
                tmp = m_tracks[i];
                m_tracks[i] = m_tracks[j];
                m_tracks[j] = tmp;
            }
        } break;
/*
 * das kapiere ich nicht... (wolfgang)
 - Party mode (see iTunes)
 - initialization
 - find 15 titles according to the scheme below
 - playing
 - before entering next title perform track selection
 - track selection
 - generate a random uid
 - if file exists:
 - determine maximum playcount of all tracks
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
    unsigned int tracksize = getNumTracks ();

    // this code is rather complicated but works in a multi user
    // environment:
 
    // insert a unique trackid:
    string trackid = ltos(m_db.thread_id()+1000000);
    m_db.exec_sql("INSERT INTO playlistitem SELECT "+listid+","
	   "MAX(tracknumber)+"+ltos(tracksize)+","+trackid+
	   " FROM playlistitem WHERE playlist="+listid);
    
    // find tracknumber of the trackid we just inserted:
    string sql = string("SELECT tracknumber FROM playlistitem WHERE "
		    "playlist=")+listid+" AND trackid="+trackid;
    long first = atol(m_db.get_col0(sql).c_str()) - tracksize + 1;

    // replace the place holder trackid by the correct value:
    m_db.exec_sql("UPDATE playlistitem SET trackid="+ltos(m_tracks[tracksize-1].getId())+
		    " WHERE playlist="+listid+" AND trackid="+trackid);
    
    // insert all other tracks:
    const char *sql_prefix = "INSERT INTO playlistitem VALUES ";
    sql = "";
    for (unsigned int i = 0; i < tracksize-1; i++)
    {
	string item = "(" + listid + "," + ltos (first + i) + "," +
            ltos (m_tracks[i].getId ()) + ")";
	comma(sql, item);
	if ((i%100)==99)
	{
    		m_db.exec_sql (sql_prefix+sql);
		sql = "";
	}
    }
    if (!sql.empty()) m_db.exec_sql (sql_prefix+sql);
    if (inCollection(Name)) clearCache ();
    return tracksize;
}


unsigned int
mgSelection::RemoveFromCollection (const string Name)
{
    if (!m_db.Connected()) return 0;
    mgParts p = order.Parts(m_db,m_level,false);
    string sql = p.sql_delete_from_collection(id(keyCollection,Name));
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
    string listid = id(keyCollection,Name);
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
    unsigned int tracksize = getNumTracks ();
    for (unsigned i = 0; i < tracksize; i++)
    {
        mgContentItem& t = m_tracks[i];
        fprintf (listfile, "#EXTINF:%d,%s\n", t.getDuration (),
            t.getTitle ().c_str ());
	fprintf (listfile, "#MUGGLE:%ld\n", t.getId());
        fprintf (listfile, "%s\n", t.getSourceFile (false).c_str ());
    }
    fclose (listfile);
    return fn;
}

bool
mgSelection::empty()
{
    if (m_level>= order.size ()-1)
	return ( getNumTracks () == 0);
    else
	return ( values.size () == 0);
}

void
mgSelection::setPosition (unsigned int position)
{
    if (m_level == order.size())
	setTrackPosition(position);
    else
    	m_position = position;
}

void
mgSelection::setTrackPosition (unsigned int position) const
{
	m_tracks_position = position;
}

unsigned int
mgSelection::getPosition ()  const
{
    if (m_level == order.size())
	    return getTrackPosition();
    else
	    return m_position;
}

unsigned int
mgSelection::gotoPosition ()
{
    if (m_level == order.size ())
        return gotoTrackPosition();
    else
    {
    	unsigned int valsize = values.size();
    	if (valsize==0)
		m_position = 0;
    	else if (m_position >= valsize)
        	m_position = valsize -1;
        return m_position;
    }
}

unsigned int
mgSelection::getTrackPosition() const
{
    if (m_tracks_position>=m_tracks.size())
	if (m_tracks.size()==0)
		m_tracks_position=0;
	else
		m_tracks_position = m_tracks.size()-1;
    return m_tracks_position;
}

unsigned int
mgSelection::gotoTrackPosition()
{
    unsigned int tracksize = getNumTracks ();
    if (tracksize == 0)
	setTrackPosition(0);
    else if (m_tracks_position >= tracksize)
        setTrackPosition(tracksize -1);
    return m_tracks_position;
}

bool mgSelection::skipTracks (int steps)
{
    unsigned int tracksize = getNumTracks();
    if (tracksize == 0)
        return false;
    if (m_loop_mode == LM_SINGLE)
        return true;
    unsigned int old_pos = m_tracks_position;
    unsigned int new_pos;
    if (old_pos + steps < 0)
    {
        if (m_loop_mode == LM_NONE)
            return false;
        new_pos = tracksize - 1;
    }
    else
	new_pos = old_pos + steps;
    if (new_pos >= tracksize)
    {
        if (m_loop_mode == LM_NONE)
           	return false;
        new_pos = 0;
    }
    setTrackPosition (new_pos);
    return (new_pos == gotoTrackPosition());
}


unsigned long
mgSelection::getLength ()
{
    unsigned long result = 0;
    unsigned int tracksize = getNumTracks ();
    for (unsigned int i = 0; i < tracksize; i++)
        result += m_tracks[i].getDuration ();
    return result;
}


unsigned long
mgSelection::getCompletedLength () const
{
    unsigned long result = 0;
    tracks ();                                    // make sure they are loaded
    for (unsigned int i = 0; i < getTrackPosition(); i++)
        result += m_tracks[i].getDuration ();
    return result;
}



string mgSelection::getListname () const
{
    list<string> st;
    for (unsigned int i = 0; i < m_level; i++)
	    st.push_back(order.getKeyValue(i));
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
mgSelection::tracks () const
{
    if (!m_db.Connected()) return m_tracks;
    if (!m_current_tracks.empty()) return m_tracks;
    mgParts p = order.Parts(m_db,m_level);
    p.fields.clear();
    p.fields.push_back("tracks.id");
    p.fields.push_back("tracks.title");
    p.fields.push_back("tracks.mp3file");
    p.fields.push_back("tracks.artist");
    p.fields.push_back("album.title");
    p.fields.push_back("tracks.genre1");
    p.fields.push_back("tracks.genre2");
    p.fields.push_back("tracks.bitrate");
    p.fields.push_back("tracks.year");
    p.fields.push_back("tracks.rating");
    p.fields.push_back("tracks.length");
    p.fields.push_back("tracks.samplerate");
    p.fields.push_back("tracks.channels");
    p.tables.push_back("tracks");
    p.tables.push_back("album");
    for (unsigned int i = m_level; i<order.size(); i++)
    	p += order.Key(i)->Parts(m_db,true);
     m_current_tracks = p.sql_select(false); 
     m_tracks.clear ();
        MYSQL_RES *rows = m_db.exec_sql (m_current_tracks);
        if (rows)
        {
        	MYSQL_ROW row;
           	while ((row = mysql_fetch_row (rows)) != 0)
		  m_tracks.push_back (mgContentItem (this,row));
            	mysql_free_result (rows);
	}
    if (m_shuffle_mode!=SM_NONE)
    	Shuffle();
    return m_tracks;
}


void mgSelection::InitSelection() {
	m_Directory=".";
    	m_level = 0;
    	m_position = 0;
    	m_tracks_position = 0;
    	m_trackid = -1;
    	if (the_setup.InitShuffleMode)
    		m_shuffle_mode = SM_NORMAL;
	else
    		m_shuffle_mode = SM_NONE;
	if (the_setup.InitLoopMode)
		m_loop_mode = LM_FULL;
	else
    		m_loop_mode = LM_NONE;
    	clearCache();
	values.setOwner(this);
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
	m_trackid = nv.getlong("TrackId");
	setPosition(nv.getstr("Position"));
	if (m_level>=order.size()-1) 
		setTrackPosition(nv.getlong("TrackPosition"));
}


mgSelection::~mgSelection ()
{
}

void mgSelection::InitFrom(const mgSelection* s)
{
    InitSelection();
    m_fall_through = s->m_fall_through;
    m_Directory = s->m_Directory;
    map_values = s->map_values;
    map_ids = s->map_ids;
    order = s->order;
    m_level = s->m_level;
    m_position = s->m_position;
    m_trackid = s->m_trackid;
    m_tracks_position = s->m_tracks_position;
    setShuffleMode (s->getShuffleMode ());
    setLoopMode (s->getLoopMode ());
}

unsigned int
mgSelection::ordersize ()
{
    return order.size ();
}

unsigned int
mgSelection::valindex (const string val,const bool second_try) const
{
    for (unsigned int i = 0; i < values.size (); i++)
    {
        if (values[i] == val)
            return i;
    }
    // nochmal mit neuen Werten:
    clearCache();
    refreshValues();
    if (second_try) {
    	esyslog("valindex: Gibt es nicht:%s",val.c_str());
    	return 0;
    }
    else
        return valindex(val,true);
}

unsigned int
mgSelection::valcount (string value)
{
	assert(m_counts.size()==values.size());
	return m_counts[valindex(value)];
}

unsigned int
mgSelection::idindex (const string id,const bool second_try) const
{
    for (unsigned int i = 0; i < m_ids.size (); i++)
        if (m_ids[i] == id)
            return i;
    // nochmal mit neuen Werten:
    clearCache();
    refreshValues();
    if (second_try) {
    	mgWarning("idindex: Gibt es nicht:%s",id.c_str());
    	return 0;
    }
    else
        return idindex(id,true);
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
        values.strings.clear ();
        m_ids.clear ();
	m_counts.clear();
        MYSQL_RES *rows = m_db.exec_sql (m_current_values);
        if (rows)
        {
                unsigned int num_fields = mysql_num_fields(rows);
		MYSQL_ROW row;
            	while ((row = mysql_fetch_row (rows)) != 0)
            	{
			if (!row[0]) continue;
			string r0 = row[0];
			if (r0=="NULL") // there is a genre NULL!
				continue;
			if (num_fields==3)
			{
				if (!row[1]) continue;
				string r1 = row[1];
                		values.strings.push_back (r0);
                		m_ids.push_back (r1);
				m_counts.push_back(atol(row[2]));
			}
			else
			{
                		values.strings.push_back (value(order.Key(m_level),r0));
                		m_ids.push_back (r0);
				m_counts.push_back(atol(row[1]));
			}
            	}
            	mysql_free_result (rows);
        }
    }
}

unsigned int
mgSelection::count () const
{
    return values.size ();
}


bool mgSelection::enter (unsigned int position)
{
    if (order.empty())
	esyslog("mgSelection::enter(%u): order is empty", position);
    if (empty())
	return false;
    setPosition (position);
    position = gotoPosition();		// reload adjusted position
    if (values.size()==0)
	return false;
    string value = values[position];
    string id = m_ids[position];
    mgSelStrings prev;
    if (m_fall_through && values.size()<10)
	    prev=values;
    while (1)
    {
        if (m_level >= order.size () - 1)
            return false;
        order[m_level++]->set (value,id);
	clearCache();
	if (m_level >= order.size())
	  mgError("mgSelection::enter(%u): level greater than order.size() %u",
	      m_level,order.size());
        m_position = 0;
	refreshValues();
	if (count()==0)
		break;
	value = values[0];
        id = m_ids[0];
        if (!m_fall_through)
            break;
        if (m_level==order.size()-1)
	    break;
	if (count () > 1 && !(prev==values))
            break;
    }
    return true;
}


bool mgSelection::select (unsigned int position)
{
    if (m_level == order.size () - 1)
    {
        if (getNumTracks () <= position)
            return false;
        order[m_level]->set (values[position],m_ids[position]);
        m_level++;
        m_trackid = m_tracks[position].getId ();

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
	prevvalue=order.getKeyValue(m_level);
	order[m_level]->set("",EMPTY);
        m_trackid = -1;
	clearCache();
	setPosition(prevvalue);
        return true;
    }
    mgSelStrings prev;
    if (m_fall_through && values.size()<10)
	    prev=values;
    while (1)
    {
        if (m_level < 1)
            return false;
        if (m_level<order.size()) order[m_level]->set ("",EMPTY);
	m_level--;
	prevvalue=order.getKeyValue(m_level);
        if (m_level<order.size()) order[m_level]->set ("",EMPTY);
	clearCache();
        if (!m_fall_through)
            break;
        if (count () > 1 && !(prev==values))
            break;
    }
    setPosition(prevvalue);
    return true;
}

void
mgSelection::leave_all ()
{
	m_level=0;
	for (unsigned int i=0;i<ordersize();i++)
		order[i]->set ("",EMPTY);
	clearCache();
}

void 
mgSelection::selectfrom(mgOrder& oldorder,mgContentItem* o)
{
	leave_all();
	string selval;
	string selid;
	assert(m_level==0);
	for (unsigned int idx = 0; idx < ordersize(); idx++)
	{
		selval = EMPTY;
		selid = EMPTY;
		mgKeyTypes new_kt = getKeyType(idx);
		for (unsigned int i=0;i<oldorder.size();i++)
		{
			mgKeyTypes old_kt = oldorder.getKeyType(i);
			if (old_kt==new_kt)
			{
				selval = oldorder.getKeyValue(i);
				selid = oldorder.getKeyId(i);
			}
			else if (old_kt>new_kt
					&& iskeyGenre(old_kt)
					&& iskeyGenre(new_kt))
			{
				selid = id(new_kt,value(new_kt,oldorder.getKeyId(i)));
				selval= value(new_kt,selid);
			}
			if (selid!=EMPTY) break;
		}
		if (selid==EMPTY && o && o->getId()>=0)
		{
			selval = o->getKeyValue(new_kt);
			selid = o->getKeyId(new_kt);
		}
		if (selid==EMPTY)
			break;
		if (m_level<ordersize()-1)
		{
			order[m_level++]->set (selval, selid);
		}
		else
		{
			setPosition(selval);
			return;
		}
	}
	if (m_level>0)
	{
		m_level--;
		selval = order.getKeyValue(m_level);
		order[m_level]->set("",EMPTY);
		setPosition(selval);
		order[m_level+1]->set("",EMPTY);
	}
	assert(m_level<ordersize());
}

string
mgSelection::value(mgKeyTypes kt, string id) const
{
	if (loadvalues (kt))
	{
		map<string,string>& valmap = map_values[kt];
		map<string,string>::iterator it;
		it = valmap.find(id);
		if (it!=valmap.end())
		{
			string r = it->second;
			if (!r.empty())
				return r;
		}
		map_ids[kt].clear();
		loadvalues(kt);
		it = valmap.find(id);
		if (it!=valmap.end())
			return valmap[id];
	}
	return id;
}

string
mgSelection::value(mgKey* k, string id) const
{
	return value(k->Type(),id);
}

string
mgSelection::value(mgKey* k) const
{
	return value(k,k->id());
}

string
mgSelection::id(mgKeyTypes kt, string val) const
{
	if (loadvalues (kt))
	{
		map<string,string>& idmap = map_ids[kt];
		string v = idmap[val];
		if (kt==keyGenre1) v=v.substr(0,1);
		if (kt==keyGenre2) v=v.substr(0,2);
		if (kt==keyGenre3) v=v.substr(0,3);
		return v;
	}
	else
		return val;
}

string
mgSelection::id(mgKey* k, string val) const
{
	return id(k->Type(),val);
}

string
mgSelection::id(mgKey* k) const
{
	return k->id();
}

bool
mgSelection::UsedBefore(mgOrder *o,const mgKeyTypes kt,unsigned int level) const
{
	if (level>=o->size())
		level = o->size() -1;
	for (unsigned int lx = 0; lx < level; lx++)
		if (o->getKeyType(lx)==kt)
			return true;
	return false;
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
    	result &= (order.getKeyValue(0) == Name);
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
			nv.put(n,getKeyValue(i));
			free(n);
		}
	}
	nv.put("TrackId",m_trackid);
	nv.put("Position",values[m_position]);
	if (m_level>=order.size()-1) 
		nv.put("TrackPosition",getTrackPosition());
}

map <mgKeyTypes, string>
mgSelection::UsedKeyValues() 
{
	map <mgKeyTypes, string> result;
	for (unsigned int idx = 0 ; idx < level() ; idx++)
	{
		result[order.getKeyType(idx)] = order.getKeyValue(idx);
	}
	if (level() < order.size()-1)
	{
		mgKeyTypes ch =  order.getKeyType(level());
		result[ch] = getCurrentValue();
	}
	return result;
}

bool
mgSelection::loadvalues (mgKeyTypes kt) const
{
	if (map_ids.count(kt)>0) 
		return true;
	map<string,string>& idmap = map_ids[kt];
	mgKey* k = ktGenerate(kt);
	if (k->map_idfield().empty())
	{
		delete k;
		return false;
	}
	map<string,string>& valmap = map_values[kt];
	char *b;
	asprintf(&b,"select %s,%s from %s;",k->map_idfield().c_str(),k->map_valuefield().c_str(),k->map_valuetable().c_str());
	MYSQL_RES *rows = m_db.exec_sql (string(b));
	free(b);
	if (rows) 
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row (rows)) != 0)
		{
			if (row[0] && row[1])
			{
				valmap[row[0]] = row[1];
				idmap[row[1]] = row[0];
			}
		}
		mysql_free_result (rows);
	}
	delete k;
	return true;
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


vector <const char *>
mgSelection::choices(mgOrder *o,unsigned int level, unsigned int *current)
{
	vector<const char*> result;
	if (level>o->size())
	{
		*current = 0;
		return result;
	}
	for (unsigned int ki=int(mgKeyTypesLow);ki<=int(mgKeyTypesHigh);ki++)
	{
		mgKeyTypes kt = mgKeyTypes(ki);
		if (kt==o->getKeyType(level))
		{
			*current = result.size();
			result.push_back(ktName(kt));
			continue;
		}
		if (UsedBefore(o,kt,level))
			continue;
		if (kt==keyDecade && UsedBefore(o,keyYear,level))
			continue;
		if (kt==keyGenre1)
		{
			if (UsedBefore(o,keyGenre2,level)) continue;
			if (UsedBefore(o,keyGenre3,level)) continue;
			if (UsedBefore(o,keyGenres,level)) continue;
		}
		if (kt==keyGenre2)
		{
			if (UsedBefore(o,keyGenre3,level)) continue;
			if (UsedBefore(o,keyGenres,level)) continue;
		}
		if (kt==keyGenre3)
		{
			if (UsedBefore(o,keyGenres,level)) continue;
		}
		if (kt==keyFolder1)
		{
		 	if (UsedBefore(o,keyFolder2,level)) continue;
		 	if (UsedBefore(o,keyFolder3,level)) continue;
		 	if (UsedBefore(o,keyFolder4,level)) continue;
		}
		if (kt==keyFolder2)
		{
		 	if (UsedBefore(o,keyFolder3,level)) continue;
		 	if (UsedBefore(o,keyFolder4,level)) continue;
		}
		if (kt==keyFolder3)
		{
		 	if (UsedBefore(o,keyFolder4,level)) continue;
		}
		if (kt==keyCollection || kt==keyCollectionItem)
			result.push_back(ktName(kt));
		else if (keycount(kt)>1)
			result.push_back(ktName(kt));
	}
	return result;
}

