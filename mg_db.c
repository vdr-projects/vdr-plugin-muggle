/*!
 * \file mg_db.c
 * \brief A database interface to the GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include "mg_db.h"
#include "vdr_setup.h"
#include "mg_tools.h"

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

size_t
mgSelection::mgSelStrings::size()
{
	if (!m_sel)
		mgError("mgSelStrings: m_sel is NULL");
	m_sel->refreshValues();
	return strings.size();
}

string&
mgSelection::mgSelStrings::operator[](unsigned int idx)
{
	if (!m_sel)
		mgError("mgSelStrings: m_sel is NULL");
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

mgKeyTypes
mgSelection::getKeyType (const unsigned int level) const
{
        return order.getKeyType(level);
}

MYSQL_RES*
mgSelection::exec_sql(string query) const
{
	return ::exec_sql(m_db, query);
}

/*! \brief executes a query and returns the first columnu of the
 * first row.
 * \param query the SQL query string to be executed
 */
string mgSelection::get_col0 (string query) const
{
    MYSQL_RES * sql_result = exec_sql (query);
    if (!sql_result)
	    return "NULL";
    MYSQL_ROW row = mysql_fetch_row (sql_result);
    string result;
    if (row == NULL)
        result = "NULL";
    else if (row[0] == NULL)
        result = "NULL";
    else
        result = row[0];
    mysql_free_result (sql_result);
    return result;
}


unsigned long
mgSelection::exec_count (string query) const
{
    return atol (get_col0 (query).c_str ());
}



string
mgSelection::sql_string (const string s) const
{
    char *buf = (char *) malloc (s.size () * 2 + 1);
    mysql_real_escape_string (m_db, buf, s.c_str (), s.size ());
    string result = "'" + std::string (buf) + "'";
    free (buf);
    return result;
}

string
mgContentItem::getKeyValue(mgKeyTypes kt)
{
	if (m_id<0) 
		return "";
	switch (kt) {
		case keyGenre1: return getGenre1();
		case keyGenre2: return getGenre2();
		case keyArtist: return getArtist();
		case keyAlbum: return getAlbum();
		case keyYear: return string(ltos(getYear()));
		case keyDecade: return string(ltos(int((getYear() % 100) / 10) * 10));
		case keyTitle: return getTitle();
		case keyTrack: return getTitle();
		default: return "";
	}
}

mgContentItem *
mgSelection::getTrack (unsigned int position)
{
    if (position >= getNumTracks ())
        return NULL;
    return &(m_tracks[position]);
}


string mgContentItem::getGenre () const
{
    return m_genre1;
}


string mgContentItem::getGenre1 () const
{
    return m_genre1;
}


string mgContentItem::getGenre2 () const
{
    return m_genre2;
}


string mgContentItem::getBitrate () const
{
    return m_bitrate;
}


string mgContentItem::getImageFile () const
{
    return "Name of Imagefile";
}


string mgContentItem::getAlbum () const
{
    return m_albumtitle;
}


int mgContentItem::getYear () const
{
    return m_year;
}


int mgContentItem::getRating () const
{
    return m_rating;
}


int mgContentItem::getDuration () const
{
    return m_duration;
}


int mgContentItem::getSampleRate () const
{
    return m_samplerate;
}


int mgContentItem::getChannels () const
{
    return m_channels;
}


mgSelection::ShuffleMode mgSelection::toggleShuffleMode ()
{
    m_shuffle_mode = (m_shuffle_mode == SM_PARTY) ? SM_NONE : ShuffleMode (m_shuffle_mode + 1);
    unsigned int tracksize = getNumTracks();
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
    return m_shuffle_mode;
}


mgSelection::LoopMode mgSelection::toggleLoopMode ()
{
    m_loop_mode = (m_loop_mode == LM_FULL) ? LM_NONE : LoopMode (m_loop_mode + 1);
    return m_loop_mode;
}


unsigned int
mgSelection::AddToCollection (const string Name)
{
    if (!m_db) return 0;
    CreateCollection(Name);
    string listid = sql_string (get_col0
        ("SELECT id FROM playlist WHERE title=" + sql_string (Name)));
    string tmp =
        get_col0 ("SELECT MAX(tracknumber) FROM playlistitem WHERE playlist=" +
        listid);
    int high;
    if (tmp == "NULL")
        high = 0;
    else
        high = atol (tmp.c_str ());
    unsigned int tracksize = getNumTracks ();
    const char *sql_prefix = "INSERT INTO playlistitem VALUES ";
    string sql = "";
    for (unsigned int i = 0; i < tracksize; i++)
    {
	string item = "(" + listid + "," + ltos (high + 1 + i) + "," +
            ltos (m_tracks[i].getId ()) + ")";
	comma(sql, item);
	if ((i%100)==99)
	{
    		exec_sql (sql_prefix+sql);
		sql = "";
	}
    }
    if (!sql.empty()) exec_sql (sql_prefix+sql);
    if (inCollection(Name)) clearCache ();
    return tracksize;
}


unsigned int
mgSelection::RemoveFromCollection (const string Name)
{
    if (!m_db) return 0;
    mgParts p = order.Parts(m_level,false);
    string sql = p.sql_delete_from_collection(id(keyCollection,Name));
    exec_sql (sql);
    unsigned int removed = mysql_affected_rows (m_db);
    if (inCollection(Name)) clearCache ();
    return removed;
}


bool mgSelection::DeleteCollection (const string Name)
{
    if (!m_db) return false;
    ClearCollection(Name);
    exec_sql ("DELETE FROM playlist WHERE title=" + sql_string (Name));
    if (isCollectionlist()) clearCache ();
    return (mysql_affected_rows (m_db) == 1);
}


void mgSelection::ClearCollection (const string Name)
{
    if (!m_db) return;
    string listid = id(keyCollection,Name);
    exec_sql ("DELETE FROM playlistitem WHERE playlist="+listid);
    if (inCollection(Name)) clearCache ();
}


bool mgSelection::CreateCollection(const string Name)
{
    if (!m_db) return false;
    string name = sql_string(Name);
    if (exec_count("SELECT count(title) FROM playlist WHERE title = " + name)>0) 
	return false;
    exec_sql ("INSERT playlist VALUES(" + name + ",'VDR',NULL,NULL,NULL)");
    if (isCollectionlist()) clearCache ();
    return true;
}


string mgSelection::exportM3U ()
{

// open a file for writing
    string fn = m_Directory + '/' + ListFilename () + ".m3u";
    FILE * listfile = fopen (fn.c_str (), "w");
    if (!listfile)
        return "";
    fprintf (listfile, "#EXTM3U");
    unsigned int tracksize = getNumTracks ();
    for (unsigned i = 0; i < tracksize; i++)
    {
        mgContentItem& t = m_tracks[i];
        fprintf (listfile, "#EXTINF:%d,%s\n", t.getDuration (),
            t.getTitle ().c_str ());
        fprintf (listfile, "%s", t.getSourceFile ().c_str ());
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
    if (m_level < order.size ())
        m_position[m_level] = position;
    if (m_level >= order.size ()-1)
        setTrackPosition(position);
}


void
mgSelection::setTrackPosition (unsigned int position)
{
    m_tracks_position = position;
}


unsigned int
mgSelection::getPosition (unsigned int level)  const
{
    if (level == order.size ())
        return getTrackPosition();
    else
        return m_position[m_level];
}

unsigned int
mgSelection::gotoPosition (unsigned int level)
{
    if (level>order.size()) 
	    mgError("mgSelection::gotoPosition: level %u > order.size %u",
	 	level,order.size());
    if (level == order.size ())
        return gotoTrackPosition();
    else
    {
    	unsigned int valsize = values.size();
    	if (valsize==0)
		m_position[m_level] = 0;
    	else if (m_position[m_level] >= valsize)
        	m_position[m_level] = valsize -1;
        return m_position[m_level];
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
    string
        result = "";
    for (unsigned int i = 0; i < m_level; i++)
        addsep (result, ":", getKeyValue(i));
    if (result.empty ())
	if (order.size()>0)
        	result = string(ktName(order.Key(0)->Type ()));
    return result;
}

string mgSelection::ListFilename ()
{
    string res = getListname ();
    // convert char set ?
    return res;
}

const vector < mgContentItem > &
mgSelection::tracks () const
{
    if (!m_db) return m_tracks;
    if (!m_current_tracks.empty()) return m_tracks;
    mgParts p = order.Parts(m_level);
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
    	p.orders += order.Key(i)->Parts(true).orders;
     m_current_tracks = p.sql_select(false); 
     m_tracks.clear ();
        MYSQL_RES *rows = exec_sql (m_current_tracks);
        if (rows)
        {
        	MYSQL_ROW row;
           	while ((row = mysql_fetch_row (rows)) != NULL)
           	{
		  m_tracks.push_back (mgContentItem (this,row));
            	}
            	mysql_free_result (rows);
	}
    return m_tracks;
}


mgContentItem::mgContentItem (const mgContentItem* c)
{
    m_id = c->m_id;
    m_title = c->m_title;
    m_mp3file = c->m_mp3file;
    m_artist = c->m_artist;
    m_albumtitle = c->m_albumtitle;
    m_genre1 = c->m_genre1;
    m_genre2 = c->m_genre2;
    m_bitrate = c->m_bitrate;
    m_year = c->m_year;
    m_rating = c->m_rating;
    m_duration = c->m_duration;
    m_samplerate = c->m_samplerate;
    m_channels = c->m_channels;
}

mgContentItem::mgContentItem (const mgSelection* sel,const MYSQL_ROW row)
{
    m_id = atol (row[0]);
    if (row[1])
	m_title = row[1];
    else
	m_title = "NULL";
    if (row[2])
    	m_mp3file = row[2];
    else
    	m_mp3file = "NULL";
    if (row[3])
    	m_artist = row[3];
    else
    	m_artist = row[3];
    if (row[4])
    	m_albumtitle = row[4];
    else
    	m_albumtitle = "NULL";
    if (row[5])
    	m_genre1 = sel->value(keyGenre1,row[5]);
    else
    	m_genre1 = "NULL";
    if (row[6])
    	m_genre2 = sel->value(keyGenre2,row[5]);
    else
    	m_genre2 = "NULL";
    if (row[7])
    	m_bitrate = row[7];
    else
    	m_bitrate = "NULL";
    if (row[8])
    	m_year = atol (row[8]);
    else
    	m_year = 0;
    if (row[9])
        m_rating = atol (row[9]);
    else
        m_rating = 0;
    if (row[10])
    	m_duration = atol (row[10]);
    else
    	m_duration = 0;
    if (row[11])
    	m_samplerate = atol (row[11]);
    else
    	m_samplerate = 0;
    if (row[12])
    	m_channels = atol (row[12]);
    else
    	m_channels = 0;
};

void mgSelection::InitSelection() {
	m_Directory=".";
	InitDatabase();
    	m_level = 0;
        m_position.reserve (20);
    	m_tracks_position = 0;
    	m_trackid = -1;
    	m_shuffle_mode = SM_NONE;
    	m_loop_mode = LM_NONE;
    	clearCache();
	values.setOwner(this);
}

mgSelection::mgSelection()
{
    setDB(0);
    m_Host = "";
    m_User = "";
    m_Password = "";
    InitSelection ();
    m_fall_through = false;
}

mgSelection::mgSelection (const string Host, const string User, const string Password, const bool fall_through)
{
    setDB(0);
    m_Host = Host;
    m_User = User;
    m_Password = Password;
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
mgSelection::setDB(MYSQL *db)
{
	m_db = db;
	order.setDB(db);
}

void
mgSelection::InitFrom(mgValmap& nv)
{
        setDB(0);
	m_Host = nv.getstr("Host");
	m_User = nv.getstr("User");
	m_Password = nv.getstr("Password");
	InitSelection();
	m_fall_through = nv.getbool("FallThrough");
    	m_Directory = nv.getstr("Directory");
	for (unsigned int i = 0; i < 99 ; i++)
	{
		char *idx;
		asprintf(&idx,"Keys.%u.Choice",i);
		unsigned int v = nv.getuint(idx);
		free(idx);
		if (v==0) break;
        	setKey (i,mgKeyTypes(v) );
	}
	while (m_level < nv.getuint("Level"))
	{
		char *idx;
		asprintf(&idx,"Keys.%u.Position",m_level);
        	unsigned int newpos = nv.getuint(idx);
		free(idx);
        	if (!enter (newpos))
            		if (!select (newpos)) break;
	}
	m_trackid = nv.getlong("TrackId");
	// TODO do we really need Position AND TrackPosition in muggle.state?
	setPosition(nv.getlong("Position"));
	if (m_level>=order.size()-1) 
		setTrackPosition(nv.getlong("TrackPosition"));
	setShuffleMode(ShuffleMode(nv.getuint("ShuffleMode")));
	setLoopMode(LoopMode(nv.getuint("LoopMode")));
}


mgSelection::~mgSelection ()
{
    if (m_db)
    {
	mgDebug(3,"%X: closing m_db %X",this,m_db);
    	mysql_close (m_db);
    }
}

void mgSelection::InitFrom(const mgSelection* s)
{
    setDB(0);
    m_Host = s->m_Host;	 
    m_User = s->m_User;	 
    m_Password = s->m_Password;	 
    InitSelection();
    m_fall_through = s->m_fall_through;
    m_Directory = s->m_Directory;
    map_values = s->map_values;
    map_ids = s->map_ids;
    order = s->order;
    m_level = s->m_level;
    m_position.reserve (s->m_position.capacity());
    for (unsigned int i = 0; i < s->m_position.capacity(); i++)
    	m_position[i] = s->m_position[i];
    m_trackid = s->m_trackid;
    m_tracks_position = s->m_tracks_position;
    setShuffleMode (s->getShuffleMode ());
    setLoopMode (s->getLoopMode ());
}

const mgSelection& mgSelection::operator=(const mgSelection &s)
{
    if ((&m_Host)==&(s.m_Host)) {	// prevent s = s
	    return *this;
    }
    InitFrom(&s);
    return *this;
}


unsigned int
mgSelection::ordersize ()
{
    return order.size ();
}

unsigned int
mgSelection::valindex (const string val,const bool second_try)
{
    for (unsigned int i = 0; i < values.size (); i++)
    {
        if (values[i] == val)
            return i;
    }
    // nochmal mit neuen Werten:
    clearCache();
    if (second_try) {
    	mgWarning("valindex: Gibt es nicht:%s",val.c_str());
    	return 0;
    }
    else
        return valindex(val,true);
}


void
mgSelection::refreshValues ()  const
{
    if (!m_db) return;
    mgOrder o1 = order;
    if (m_current_values.empty())
    {
	mgParts p =  order.Parts(m_level);
        m_current_values = p.sql_select();
        values.strings.clear ();
        m_ids.clear ();
        MYSQL_RES *rows = exec_sql (m_current_values);
        if (rows)
        {
                unsigned int num_fields = mysql_num_fields(rows);
		MYSQL_ROW row;
            	while ((row = mysql_fetch_row (rows)) != NULL)
            	{
			string r0 = "NULL";
			if (row[0])
				r0 = row[0];
			if (num_fields==2)
			{
				string r1 = "NULL";
				if (row[1])
					r1 = row[1];
                		values.strings.push_back (r0);
                		m_ids.push_back (r1);
			}
			else
			{
                		values.strings.push_back (value(order.Key(m_level),r0));
                		m_ids.push_back (r0);
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

void
mgSelection::InitDatabase ()
{
    if (m_db) 
    {
       mgDebug(3,"%X: InitDatabase closes %X",this,m_db);
       mysql_close (m_db);
       setDB(0);
    }
    if (m_Host == "") return;
    setDB(mysql_init (0));
    mgDebug(3,"%X: InitDatabase opens %X",this, m_db);
    if (!m_db)
        return;
    if (mysql_real_connect (m_db, m_Host.c_str (), m_User.c_str (), m_Password.c_str (),
        "GiantDisc", 0, NULL, 0) == NULL) {
	    mgWarning("Failed to connect to host '%s' as User '%s', Password '%s': Error: %s",
			    m_Host.c_str(),m_User.c_str(),m_Password.c_str(),mysql_error(m_db));
        mysql_close (m_db);
	setDB(0);
	return;
    }
    return;
}


void
mgSelection::setKey (const unsigned int level, const mgKeyTypes kt)
{
    mgKey *newkey = ktGenerate(kt,m_db);
    if (level == 0 && kt == keyCollection)
    {
        order.clear ();
        order += newkey;
	order += ktGenerate(keyCollectionItem,m_db);
        return;
    }
    if (level == order.size ())
    {
        order += newkey;
    }
    else
    {
	if (level >= order.size())
	  mgError("mgSelection::setKey(%u,%s): level greater than order.size() %u",
	      level,ktName(kt),order.size());
        delete order[level];
	order[level] = newkey;
    }

    order.clean();

// clear values for this and following levels (needed for copy constructor)
    for (unsigned int i = level; i < order.size (); i++)
        order[i]->set ("",EMPTY);

    if (m_level > level)
        m_level = level;
    if (m_level == level) setPosition(0);
}


bool mgSelection::enter (unsigned int position)
{
    if (order.empty())
	mgWarning("mgSelection::enter(%u): order is empty", position);
    if (empty())
	return false;
    setPosition (position);
    position = gotoPosition();		// reload adjusted position
    string value = values[position];
    string id = m_ids[position];
    while (1)
    {
        mgDebug(3,"enter(level=%u,pos=%u, id=%s)",m_level,position,id.c_str());
        if (m_level >= order.size () - 1)
            return false;
        order[m_level++]->set (value,id);
	clearCache();
	if (m_level >= order.size())
	  mgError("mgSelection::enter(%u): level greater than order.size() %u",
	      m_level,order.size());
        if (m_position.capacity () == m_position.size ())
            m_position.reserve (m_position.capacity () + 10);
        m_position[m_level] = 0;
        if (!m_fall_through)
            break;
        if (count () > 1)
            break;
        if (count () == 1)
	{
	    value = values[0];
            id = m_ids[0];
	}
    }
    return true;
}


bool mgSelection::select (unsigned int position)
{
    mgDebug(3,"select(%u) on Level %d",position,m_level);
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


bool mgSelection::leave ()
{
    if (order.empty())
    {
	mgWarning("mgSelection::leave(): order is empty");
	return false;
    }
    if (m_level == order.size ())
    {
        m_level--;
        m_trackid = -1;
	clearCache();
        return true;
    }
    while (1)
    {
        if (m_level < 1)
            return false;
        order[m_level]->set ("",EMPTY);
        order[--m_level]->set ("",EMPTY);
	clearCache();
        if (!m_fall_through)
            break;
        if (count () > 1)
            break;
    }
    return true;
}

string
mgSelection::value(mgKeyTypes kt, string id) const
{
	if (kt==keyGenre2) kt = keyGenre1;
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
	if (kt==keyGenre2) kt = keyGenre1;
	if (loadvalues (kt))
	{
		map<string,string>& idmap = map_ids[kt];
		return idmap[val];
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
mgSelection::UsedBefore(const mgKeyTypes kt,unsigned int level) const
{
	if (level>=order.size())
		level = order.size() -1;
	for (unsigned int lx = 0; lx < level; lx++)
		if (order.Key(lx)->Type()==kt)
			return true;
	return false;
}


bool mgSelection::isCollectionlist () const
{
    if (order.size()==0) return false;
    return (order.Key(0)->Type() == keyCollection && m_level == 0);
}

bool
mgSelection::inCollection(const string Name) const
{
    if (order.size()==0) return false;
    bool result = (order.Key(0)->Type() == keyCollection && m_level == 1);
    if (result)
	    if (order.Key(1)->Type() != keyCollectionItem)
		    mgError("inCollection: key[1] is not keyCollectionItem");
    if (!Name.empty())
    	result &= (order.getKeyValue(0) == Name);
    return result;
}


#if 0
void
keychoice(mgOrder& o,const unsigned int level)
{
    if (level > o.size ())
        return;
    std::cout<<"possible choices:";
    for (mgKeyTypes kt = mgKeyTypes(1); kt <= mgKeyTypesHigh; kt = mgKeyTypes(int(kt)+1))
    {
	if (level !=0 && kt == keyCollection)
		continue;
	if (level !=1 && kt == keyCollectionItem)
		continue;
	if (level == 1 && o[0]->Type() != keyCollection && kt == keyCollectionItem)
		continue;
	if (level == 1 && o[0]->Type() == keyCollection && kt != keyCollectionItem)
		continue;
	if (level >1 && o[0]->Type() == keyCollection)
		break;
	if (kt == keyDecade && UsedBefore(o,keyYear,level))
		continue;
	if (o[0]->Type() == keyCollection)
	{
		std::cout<<" "<<ktName(kt);
	}
	else if (!UsedBefore(o,kt,level))
	{
		if (keycounts[kt]==-1)
		{
			mgOrder oc(db);
			oc += ktGenerate(kt,db);
			keycounts[kt]=atol(get_col0(oc.Parts(0).sql_count()).c_str());
		}
		if (keycounts[kt]>1)
			std::cout<<" "<<ktName(kt)<<"("<<keycounts[kt]<<")";
	}
    }
    std::cout<<endl;
}
#endif

void mgSelection::DumpState(mgValmap& nv) const
{
	nv.put("Host",m_Host);
	nv.put("User",m_User);
	nv.put("Password",m_Password);
	nv.put("FallThrough",m_fall_through);
	nv.put("ShuffleMode",int(m_shuffle_mode));
	nv.put("LoopMode",int(m_loop_mode));
	nv.put("Directory",m_Directory);
	nv.put("Level",int(m_level));
    	for (unsigned int i=0;i<order.size();i++)
    	{
		char *n;
		asprintf(&n,"Keys.%d.Choice",i);
		nv.put(n,int(order.Key(i)->Type()));
		free(n);
		if (i<m_level) {
			asprintf(&n,"Keys.%d.Position",i);
			nv.put(n,m_position[i]);
			free(n);
		}
	}
	nv.put("TrackId",m_trackid);
    	if (m_level == order.size ())
		nv.put("Position",getTrackPosition());
	else
		nv.put("Position",m_position[m_level]);
	nv.put("TrackPosition",getTrackPosition());
}

map <mgKeyTypes, string> *
mgSelection::UsedKeyValues() 
{
	map <mgKeyTypes, string> *result = new map<mgKeyTypes, string>;
	for (unsigned int idx = 0 ; idx < level() ; idx++)
	{
		(*result)[order.Key(idx)->Type()] = order.getKeyValue(idx);
	}
	if (level() < order.size()-1)
	{
		mgKeyTypes ch =  order.getKeyType(level());
		(*result)[ch] = getCurrentValue();
	}
	return result;
}

bool
mgSelection::loadvalues (mgKeyTypes kt) const
{
	mgKey* k = ktGenerate(kt,m_db);
	if (k->map_idfield().empty())
	{
		delete k;
		return false;
	}
	map<string,string>& idmap = map_ids[kt];
	if (!idmap.empty())
	{
		delete k;
		return true;
	}
	map<string,string>& valmap = map_values[kt];
	char *b;
	asprintf(&b,"select %s,%s from %s;",k->map_idfield().c_str(),k->map_valuefield().c_str(),k->map_valuetable().c_str());
	MYSQL_RES *rows = exec_sql (string(b));
	free(b);
	if (rows) 
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row (rows)) != NULL)
		{
			if (row[0] && row[1])
				valmap[row[0]] = row[1];
				idmap[row[1]] = row[0];
		}
		mysql_free_result (rows);
	}
	delete k;
	return true;
}
