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

/*! \brief a RAM copy of the genres table for faster access
 * and in order to avoid having to use genre as genre 1 etc.
 */
static map < string, string > genres;

//! \brief adds string n to string s, using string sep to separate them
static string addsep (string & s, string sep, string n);

//! \brief adds string n to string s, using a comma to separate them
static string comma (string & s, string n);

//! \brief adds string n to string s, using AND to separate them
static string und (string & s, string n);

/*! \brief returns a random integer within some range
 */
unsigned int
randrange (const unsigned int high)
{
    unsigned int result=0;
    result = random () % high;
    return result;
}


//! \brief adds n1=n2 to string s, using AND to separate several such items
static string
undequal (string & s, string n1, string op, string n2)
{
    if (n1.compare (n2) || op != "=")
        return addsep (s, " AND ", n1 + op + n2);
    else
        return s;
}

static string
comma (string & s, string n)
{
    return addsep (s, ",", n);
}


static string
und (string & s, string n)
{
    return addsep (s, " AND ", n);
}


static string
commalist (string prefix,list < string > v,bool sort=true)
{
    string result = "";
    if (sort) v.sort ();
    v.unique ();
    for (list < string >::iterator it = v.begin (); it != v.end (); it++)
    {
        comma (result, *it);
    }
    if (!result.empty())
	    result.insert(0," "+prefix+" ");
    return result;
}

//! \brief converts long to string
string
itos (int i)
{
    stringstream s;
    s << i;
    return s.str ();
}

//! \brief convert long to string
string
ltos (long l)
{
    stringstream s;
    s << l;
    return s.str ();
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

mgValmap::mgValmap(const char *key) {
	m_key = key;
}

void mgValmap::Read(FILE *f) {
	char *line=(char*)malloc(1000);
	char *prefix=(char*)malloc(strlen(m_key)+2);
	strcpy(prefix,m_key);
	strcat(prefix,".");
	rewind(f);
	while (fgets(line,1000,f)) {
		if (strncmp(line,prefix,strlen(prefix))) continue;
		if (line[strlen(line)-1]=='\n')
				line[strlen(line)-1]=0;
		char *name = line + strlen(prefix);
		char *eq = strchr(name,'=');
		if (!eq) continue;
		*(eq-1)=0;
		char *value = eq + 2;
		(*this)[string(name)]=string(value);
	}
	free(prefix);
	free(line);
}

void mgValmap::Write(FILE *f) {
	for (mgValmap::const_iterator it=begin();it!=end();++it) {
		char b[1000];
		sprintf(b,"%s.%s = %s\n",
			m_key,it->first.c_str(),
			it->second.c_str());
		fputs(b,f);
	}
}

void mgValmap::put(const char* name, const string value) {
	if (value.empty() || value==EMPTY) return;
	(*this)[string(name)] = value;
}

void mgValmap::put(const char* name, const char* value) {
	if (!value || *value==0) return;
	(*this)[string(name)] = value;
}

void mgValmap::put(const char* name, const int value) {
	put(name,ltos(value));
}

void mgValmap::put(const char* name, const unsigned int value) {
	put(name,ltos(value));
}

void mgValmap::put(const char* name, const long value) {
	put(name,ltos(value));
}

void mgValmap::put(const char* name, const bool value) {
	string s;
	if (value)
		s = "true";
	else
		s = "false";
	put(name,s);
}


void
mgSelection::clearCache()
{
        m_current_values = "";
        m_current_tracks = "";
}

string
mgSelection::getCurrentValue()
{
	return values[gotoPosition()];
}

MYSQL_RES *
mgSelection::exec_sql (string query)
{
    mgDebug(3,query.c_str());
    if (!m_db) return NULL;
    if (mysql_query (m_db, (query + ';').c_str ()))
    {
        mgError("SQL Error in %s: %s",query.c_str(),mysql_error (m_db));
        return NULL;
    }
    return mysql_store_result (m_db);
}


/*! \brief executes a query and returns the first columnu of the
 * first row.
 * \param query the SQL query string to be executed
 */
string mgSelection::get_col0 (string query)
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
mgSelection::exec_count (string query)
{
    return atol (get_col0 (query).c_str ());
}


/*! \brief extract table names. All words preceding a . are supposed to be
 * table names. Table names are supposed to only contain letters. That is
 * sufficient for GiantDisc
 * \par spar the SQL command
 * \return a list of table names
 * \todo is this thread safe?
 */
static list < string >
tables (const string spar)
{
    list < string > result;
    string s = spar;
    string::size_type dot;
    while ((dot = s.rfind ('.')) != string::npos)
    {
        s.erase (dot, string::npos);              // cut the rest
        string::size_type lword = s.size ();
        while (strchr
            ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
            s[lword - 1]))
        {
            lword--;
            if (lword == 0)
                break;
        }
        result.push_back (s.substr (lword));
    }
    return result;
}


/*! \brief if the SQL command works on only 1 table, remove all table
 * qualifiers. Example: SELECT tracks.title FROM tracks becomes SELECT title
 * FROM tracks
 * \param spar the sql command. It will be edited in place
 * \return the new sql command is also returned
 */
static string
optimize (string & spar)
{
    string s = spar;
    string::size_type tmp = s.find (" WHERE");
    if (tmp != string::npos)
        s.erase (tmp, 9999);
    tmp = s.find (" ORDER");
    if (tmp != string::npos)
        s.erase (tmp, 9999);
    string::size_type frompos = s.find (" FROM ") + 6;
    if (s.substr (frompos).find (",") == string::npos)
    {
        string from = s.substr (frompos, 999) + '.';
        string::size_type track;
        while ((track = spar.find (from)) != string::npos)
        {
            spar.erase (track, from.size ());
        }
    }
    return spar;
}

string keyfield::sql_string(const string s) const
{
	return selection->sql_string(s);
}

keyfield::keyfield (const string choice)
{
    if (choice.empty()) 
	    mgError("keyfield::keyfield: choice is empty");
    m_choice = choice;
    m_id = EMPTY;
    m_value = "";
    m_filter = "";
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


string keyfield::restrict (string & result) const
{
    string id = "";
    string op;
    if (m_id == EMPTY)
        return result;
    if (m_id == "NULL")
    {
        op = " is ";
        id = "NULL";
    }
    else
    {
        op = "=";
        id = sql_string (m_id);
    }
    if (idfield () == valuefield ())
        undequal (result, idfield (), op, id);
    else
        undequal (result, basefield (), op, id);
    return result;
}


string keyfield::join () const
{
    string result;
    if (need_join ())
        return undequal (result, basefield (), "=", idfield ());
    else
        return "";
}


bool keyfield::need_join () const
{
    return lookup;
}

void
keyfield::set(const string id,const string value)
{
    if (id != EMPTY)
	if (m_id == id && m_value == value) return;
    m_id = id;
    m_value = value;
    if (selection) 
	    selection->clearCache();
}

void
keyfield::writeAt (ostream & s) const
{
    if (m_id == EMPTY)
        s << choice () << '/';
    else
        s << choice () << '=' << m_id;
}


const char *
toCString (mgSelection::ShuffleMode m)
{
    static const char *modes[] =
    {
        "SM_NONE", "SM_NORMAL", "SM_PARTY"
    };
    return modes[m];
}


string
toString (mgSelection::ShuffleMode m)
{
    return toCString (m);
}

//! \brief dump keyfield
ostream & operator<< (ostream & s, keyfield & k)
{
    k.writeAt (s);
    return s;
}


string
addsep (string & s, string sep, string n)
{
    if (!n.empty ())
    {
        if (!s.empty ())
            s.append (sep);
        s.append (n);
    }
    return s;
}


mgContentItem *
mgSelection::getTrack (unsigned int position)
{
    if (position >= getNumTracks ())
        return NULL;
    return &(m_tracks[position]);
}


void
mgSelection::loadgenres ()
{
    MYSQL_RES *rows = exec_sql ("select id,genre from genre;");
    if (rows) 
    {
    	MYSQL_ROW row;
    	while ((row = mysql_fetch_row (rows)) != NULL)
    	{
        	if (row[0] && row[1])
			genres[row[0]] = row[1];
    	}
    	mysql_free_result (rows);
    }
}


string mgContentItem::getGenre1 ()
{
    return genres[m_genre1];
}


string mgContentItem::getGenre2 ()
{
    return genres[m_genre2];
}


mgSelection::ShuffleMode mgSelection::toggleShuffleMode ()
{
    m_shuffle_mode = (m_shuffle_mode == SM_PARTY) ? SM_NONE : ShuffleMode (m_shuffle_mode + 1);
    unsigned int tracksize = getNumTracks();
    switch (m_shuffle_mode)
    {
        case SM_NONE:
        {
    	    long id = m_tracks[m_tracks_position].getId ();
            m_current_tracks = "";                // force a reload
            tracksize = getNumTracks();		  // getNumTracks also reloads
    	    for (unsigned int i = 0; i < tracksize; i++)
        	if (m_tracks[i].getId () == id)
    		{
        		m_tracks_position = i;
        		break;
    		}
        }
        break;
        case SM_PARTY:
        case SM_NORMAL:
        {
	    // play all, beginning with current track:
            mgContentItem tmp = m_tracks[m_tracks_position];
	    m_tracks[m_tracks_position]=m_tracks[0];
	    m_tracks[0]=tmp;
	    m_tracks_position=0;
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
	string value = "(" + listid + "," + ltos (high + 1 + i) + "," +
            ltos (m_tracks[i].getId ()) + ")";
	comma(sql, value);
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
    string listid = get_col0
        ("SELECT id FROM playlist WHERE title=" + sql_string (Name));
    where();
    m_fromtables.push_back("playlistitem");
    m_fromtables.push_back("playlist");
    string sql = "DELETE playlistitem" + commalist("FROM",m_fromtables)
	    + m_where + " AND tracks.id = playlistitem.trackid "
	    " AND playlistitem.playlist = " + listid;
    exec_sql (sql);
    unsigned int removed = mysql_affected_rows (m_db);
    if (inCollection(Name)) clearCache ();
    return removed;
}


bool mgSelection::DeleteCollection (const string Name)
{
    if (!m_db) return false;
    exec_sql ("DELETE FROM playlist WHERE title=" + sql_string (Name));
    if (isCollectionlist()) clearCache ();
    return (mysql_affected_rows (m_db) == 1);
}


void mgSelection::ClearCollection (const string Name)
{
    if (!m_db) return;
    exec_sql ("DELETE playlistitem FROM playlist,playlistitem "
	      "WHERE playlistitem.playlist=playlist.id "
	      " AND playlist.title=" + sql_string (Name));
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
        mgContentItem* t = &m_tracks[i];
        fprintf (listfile, "#EXTINF:%d,%s\n", t->getDuration (),
            t->getTitle ().c_str ());
        fprintf (listfile, "%s", t->getSourceFile ().c_str ());
    }
    fclose (listfile);
    return fn;
}

bool
mgSelection::empty()
{
    if (m_level>= keys.size ()-1)
	return ( getNumTracks () == 0);
    else
	return ( values.size () == 0);
}

void
mgSelection::setPosition (unsigned int position)
{
    if (m_level < keys.size ())
        m_position[m_level] = position;
    if (m_level >= keys.size ()-1)
        m_tracks_position = position;
}


void
mgSelection::setTrack (unsigned int position)
{
    m_tracks_position = position;
}


unsigned int
mgSelection::getPosition (unsigned int level) const
{
    if (level == keys.size ())
        return getTrackPosition();
    else
        return m_position[m_level];
}

unsigned int
mgSelection::gotoPosition (unsigned int level)
{
    if (level>keys.size()) 
	    mgError("mgSelection::gotoPosition: level %u > keys.size %u",
	 	level,keys.size());
    if (level == keys.size ())
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
    return m_tracks_position;
}

unsigned int
mgSelection::gotoTrackPosition()
{
    unsigned int tracksize = getNumTracks ();
    if (tracksize == 0)
	m_tracks_position = 0;
    else if (m_tracks_position >= tracksize)
        m_tracks_position = tracksize -1;
    return m_tracks_position;
}

bool mgSelection::skipTracks (int steps)
{
    unsigned int tracksize = getNumTracks();
    if (tracksize == 0)
        return false;
    if (m_loop_mode == LM_SINGLE)
        return true;
    unsigned int old_pos = getTrackPosition();
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
	clearCache();
        tracksize = getNumTracks();
    	if (new_pos >= tracksize)
    	{
        	if (m_loop_mode == LM_NONE)
            	return false;
        	new_pos = 0;
	}
    }
    setTrack (new_pos);
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
mgSelection::getCompletedLength ()
{
    unsigned long result = 0;
    tracks ();                                    // make sure they are loaded
    for (unsigned int i = 0; i < m_tracks_position; i++)
        result += m_tracks[i].getDuration ();
    return result;
}


string mgSelection::getListname ()
{
    string
        result = "";
    for (unsigned int i = 0; i < m_level; i++)
        addsep (result, ":", keys[i]->value ());
    if (m_level==keys.size())
	addsep (result,":",getCurrentValue());
    if (result.empty ())
        result = string(tr(keys[0]->choice ().c_str()));
    return result;
}


string mgSelection::ListFilename ()
{
    string res = getListname ();
#if 0
    geht so noch gar
        nicht ... while (string::size_type p = res.find (" "))
    res.replace (p, "");
    while (string::size_type p = res.find ("/"))
        res.replace (p, '-');
    while (string::size_type p = res.find ("\\"))
        res.replace (p, '-');
#endif
    return res;
}

void
mgSelection::AddOrder(const string sql,list<string>& orderlist, const string item)
{
    string::size_type dot = item.rfind ('.');
    string itemtable = item.substr(0,dot);
    if (sql.find(itemtable) != string::npos)
	    orderlist.push_back(item);
}

const vector < mgContentItem > &
mgSelection::tracks ()
{
    list < string > orderby;
    orderby.clear();
    if (keys.empty())
	mgError("mgSelection::tracks(): keys is empty");
    if (genres.size () == 0)
        loadgenres ();
    string sql = "SELECT tracks.id, tracks.title, tracks.mp3file, "
        "tracks.artist, album.title, tracks.genre1, tracks.genre2, "
        "tracks.bitrate, tracks.year, tracks.rating, "
        "tracks.length, tracks.samplerate, tracks.channels ";
    sql += where (true);
    for (unsigned int i = m_level; i<keys.size(); i++)
    {
	AddOrder(sql,orderby,keys[i]->order ());
}
    if (m_level>= keys.size ()-1)
        if (inCollection())
		AddOrder(sql,orderby,"playlistitem.tracknumber");
        else
		AddOrder(sql,orderby,"tracks.title");
   
    
    sql += commalist("ORDER BY",orderby,false);

    optimize (sql);
    if (m_current_tracks != sql)
    {
        m_current_tracks = sql;
        m_tracks.clear ();
        MYSQL_RES *rows = exec_sql (sql);
        if (rows)
        {
        	MYSQL_ROW row;
           	while ((row = mysql_fetch_row (rows)) != NULL)
           	{
               		m_tracks.push_back (mgContentItem (row, m_ToplevelDir));
            	}
            	mysql_free_result (rows);
	}
	if (m_tracks_position>=m_tracks.size())
		if (m_tracks.size()==0)
			m_tracks_position=0;
		else
			m_tracks_position = m_tracks.size()-1;
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

mgContentItem::mgContentItem (const MYSQL_ROW row, const string ToplevelDir)
{
    m_id = atol (row[0]);
    if (row[1])
	m_title = row[1];
    else
	m_title = "NULL";
    if (row[2])
    	m_mp3file = ToplevelDir + row[2];
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
    	m_genre1 = row[5];
    else
    	m_genre1 = "NULL";
    if (row[6])
    	m_genre2 = row[6];
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

string mgContentItem::getAlbum ()
{
    return m_albumtitle;
}


string mgContentItem::getImageFile ()
{
    return "Name of Imagefile";
}


void
mgSelection::initkey (keyfield & f)
{
    f.setOwner(this);
    all_keys[f.choice ()] = &f;
    trall_keys[string(tr(f.choice ().c_str()))] = &f;
}

void mgSelection::InitSelection() {
	m_Directory=".";
    	m_ToplevelDir = string("/");
	InitDatabase();
    	m_level = 0;
        m_position.reserve (20);
    	m_tracks_position = 0;
    	m_trackid = -1;
    	m_shuffle_mode = SM_NONE;
    	m_loop_mode = LM_NONE;
    	clearCache();
    	initkey (kartist);
    	initkey (kgenre1);
    	initkey (kgenre2);
    	initkey (klanguage);
    	initkey (krating);
    	initkey (kyear);
    	initkey (kdecade);
    	initkey (ktitle);
    	initkey (ktrack);
    	initkey (kalbum);
    	initkey (kcollection);
    	initkey (kcollectionitem);
	keys.clear();
    	keys.push_back (&kartist);
    	keys.push_back (&kalbum);
    	keys.push_back (&ktitle);
	values.setOwner(this);
}

mgSelection::mgSelection()
{
    m_db = NULL;
    m_Host = "";
    m_User = "";
    m_Password = "";
    InitSelection ();
    m_fall_through = false;
}

mgSelection::mgSelection (const string Host, const string User, const string Password, const bool fall_through)
{
    m_db = NULL;
    m_Host = Host;
    m_User = User;
    m_Password = Password;
    InitSelection ();
    m_fall_through = fall_through;
}

mgSelection::mgSelection (const mgSelection &s)
{
    m_db = NULL;
    InitFrom(&s);
}

mgSelection::mgSelection (const mgSelection* s)
{
    m_db = NULL;
    InitFrom(s);
}

mgSelection::mgSelection (mgValmap& nv)
{
	// this is analog to the copy constructor, please keep in sync.
	
    m_db = NULL;
    InitFrom(nv);
}

void
mgSelection::InitFrom(mgValmap& nv)
{
	m_Host = nv.getstr("Host");
	m_User = nv.getstr("User");
	m_Password = nv.getstr("Password");
	InitSelection();
	m_fall_through = nv.getbool("FallThrough");
    	m_Directory = nv.getstr("Directory");
    	m_ToplevelDir = nv.getstr("ToplevelDir");
	for (unsigned int i = 0; i < 99 ; i++)
	{
		char *idx;
		asprintf(&idx,"Keys.%u.Choice",i);
		string v = nv.getstr(idx);
		free(idx);
		if (v.empty()) break;
        	setKey (i,v );
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
	if (m_level>=keys.size()-1) 
		setTrack(nv.getlong("TrackPosition"));
	setShuffleMode(ShuffleMode(nv.getuint("ShuffleMode")));
	setLoopMode(LoopMode(nv.getuint("LoopMode")));
}


mgSelection::~mgSelection ()
{
    mysql_close (m_db);
}

void mgSelection::InitFrom(const mgSelection* s)
{
    m_Host = s->m_Host;	 
    m_User = s->m_User;	 
    m_Password = s->m_Password;	 
    InitSelection();
    m_fall_through = s->m_fall_through;
    m_Directory = s->m_Directory;
    m_ToplevelDir = s->m_ToplevelDir;
    keys.clear();
    for (unsigned int i = 0; i < s->keys.size (); i++)
    {
        keys.push_back(findKey(s->keys[i]->choice()));
	keys[i]->set(s->keys[i]->id(),s->keys[i]->value());
    }
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


void
mgSelection::writeAt (ostream & s)
{
    for (unsigned int i = 0; i < keys.size (); i++)
    {
        if (i == level ())
            s << '*';
        s << *keys[i] << ' ';
        if (i == level ())
        {
            for (unsigned int j = 0; j < values.size (); j++)
            {
                s << values[j];
                if (values[j] != m_ids[j])
                    s << '(' << m_ids[j] << ")";
                s << ", ";
                if (j == 7)
                {
                    s << "(von " << values.size () << ") ";
                    break;
                }
            }
        }
    }
    s << endl;
}


ostream & operator<< (ostream & s, mgSelection & sl)
{
    sl.writeAt (s);
    return s;
}


unsigned int
mgSelection::size ()
{
    return keys.size ();
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


string mgSelection::where (bool want_trackinfo)
{
    m_from = "";
    m_where = "";
    m_fromtables.clear();
    if (m_level < keys.size ())
    {
        for (unsigned int i = 0; i <= m_level; i++)
        {
            keyfield * k = keys[i];
            k->lookup = want_trackinfo || (i == m_level);
            list < string > l = tables (k->join () + ' ' + k->basefield ());
            m_fromtables.merge (l);
            und (m_where, k->join ());
            k->restrict (m_where);
        }
    }
    else
    {
        m_fromtables.push_back ("tracks");
        m_where = "tracks.id='" + ltos (m_trackid) + "'";
    }
    if (want_trackinfo)
    {
        if (m_level == keys.size () || !UsedBefore (&kalbum, m_level + 1))
        {
            kalbum.lookup = false;
            list < string > l =
                tables (kalbum.join () + ' ' + kalbum.basefield ());
            m_fromtables.merge (l);
            und (m_where, kalbum.join ());
        }
    }
    m_from = commalist ("FROM",m_fromtables);
    if (!m_where.empty ())
        m_where.insert (0, " WHERE ");
    return m_from + m_where;
}


void
mgSelection::refreshValues ()
{
    if (m_current_values.empty())
    {
        m_current_values = sql_values();
        values.strings.clear ();
        m_ids.clear ();
        MYSQL_RES *rows = exec_sql (m_current_values);
        if (rows)
        {
            	unsigned int num_fields = mysql_num_fields(rows);
		MYSQL_ROW row;
            	while ((row = mysql_fetch_row (rows)) != NULL)
            	{
			string r0,r1;
			if (row[0])
				r0 = row[0];
			else
				r0 = "NULL";
			if (row[1])
				r1 = row[1];
			else
				r1 = "NULL";
                	values.strings.push_back (r0);
			if (num_fields==2)
                		m_ids.push_back (r1);
			else
                		m_ids.push_back (r0);
            	}
            	mysql_free_result (rows);
        }
	if (m_position[m_level]>=values.size())
		if (values.size()==0)
			m_position[m_level]=0;
		else
			m_position[m_level] = values.size()-1;
    }
}


string mgSelection::sql_values ()
{
    if (keys.empty())
	mgError("mgSelection::sql_values(): keys is empty");
    string result;
    if (m_level < keys.size ())
    {
        keyfield * last = keys[m_level];
	result = "SELECT ";
	if (m_level<keys.size()-1) result += "DISTINCT ";
        result += last->valuefield ();
	if (last->valuefield() != last->idfield())
		result += ',' + last->idfield ();
	result += where (false);
        result += " ORDER BY " + last->order ();
    }
    else
    {
        result = "SELECT title,id from tracks where id='" + ltos (m_trackid) + "'";
    }
    optimize (result);
    return result;
}


unsigned int
mgSelection::count ()
{
    return values.size ();
}


void
mgSelection::InitDatabase ()
{
    if (m_db) 
    {
       mysql_close (m_db);
       m_db = NULL;
    }
    if (m_Host == "") return;
    m_db = mysql_init (0);
    if (m_db == NULL)
        return;
    if (mysql_real_connect (m_db, m_Host.c_str (), m_User.c_str (), m_Password.c_str (),
        "GiantDisc", 0, NULL, 0) == NULL) {
	    mgWarning("Failed to connect to host '%s' as User '%s', Password '%s': Error: %s",
			    m_Host.c_str(),m_User.c_str(),m_Password.c_str(),mysql_error(m_db));
        mysql_close (m_db);
	m_db = NULL;
	return;
    }
    return;
}


string keyfield::KeyCountquery ()
{
    lookup = false;
    string from;
    from = commalist ("FROM",tables (countfield () + ' ' + countjoin ()));
    string query = "SELECT COUNT(DISTINCT " + countfield () + ") " + from;
    if (!countjoin ().empty ())
        query += " WHERE " + countjoin ();
    optimize (query);
    return query;
}

keyfield* mgSelection::findKey(const string name) 
{
	if (all_keys.find(name) != all_keys.end())
		return all_keys.find(name)->second;
	if (trall_keys.find(name) != trall_keys.end())
		return trall_keys.find(name)->second;
	return NULL;
}

void
mgSelection::setKey (const unsigned int level, const string name)
{
    keyfield *newkey = findKey(name);
    if (newkey == NULL) 
	mgError("mgSelection::setKey(%u,%s): keyname wrong",
	      level,name.c_str());
    if (level == 0 && newkey == &kcollection)
    {
        keys.clear ();
        keys.push_back (&kcollection);
        keys.push_back (&kcollectionitem);
        return;
    }
    if (level == keys.size ())
    {
        keys.push_back (newkey);
    }
    else
    {
	if (level >= keys.size())
	  mgError("mgSelection::setKey(%u,%s): level greater than keys.size() %u",
	      level,name.c_str(),keys.size());
        keys[level] = newkey;
// remove this key from following lines:
        for (unsigned int i = level + 1; i < keys.size (); i++)
            if (keys[i] == keys[level])
                keys.erase (keys.begin () + i);
    }

// remove redundant lines:
    bool album_found = false;
    bool track_found = false;
    bool title_found = false;
    for (unsigned int i = 0; i < keys.size (); i++)
    {
        album_found |= (keys[i] == &kalbum);
        track_found |= (keys[i] == &ktrack);
        title_found |= (keys[i] == &ktitle);
        if (track_found || (album_found && title_found))
        {
            keys.erase (keys.begin () + i + 1, keys.end ());
            break;
        }
    }

// clear values for this and following levels (needed for copy constructor)
    for (unsigned int i = level; i < keys.size (); i++)
        keys[i]->set (EMPTY, "");

    if (m_level > level)
        m_level = level;
    if (m_level == level) setPosition(0);
}


bool mgSelection::enter (unsigned int position)
{
    if (keys.empty())
	mgError("mgSelection::enter(%u): keys is empty", position);
    if (empty())
	return false;
    setPosition (position);
    position = gotoPosition();		// reload adjusted position
    string value = values[position];
    string id = m_ids[position];
    while (1)
    {
        mgDebug(2,"enter(level=%u,pos=%u, value=%s)",m_level,position,value.c_str());
        if (m_level >= keys.size () - 1)
            return false;
        keys[m_level++]->set (id, value);
	if (m_level >= keys.size())
	  mgError("mgSelection::enter(%u): level greater than keys.size() %u",
	      m_level,keys.size());
        if (m_position.capacity () == m_position.size ())
            m_position.reserve (m_position.capacity () + 10);
        m_position[m_level] = 0;
        if (!m_fall_through)
            break;
        if (count () > 1)
            break;
        if (count () == 1)
        {
            id = m_ids[0];
            value = values[0];
        }
    }
    return true;
}


bool mgSelection::select (unsigned int position)
{
    mgDebug(2,"select(pos=%u)",position);
    if (m_level == keys.size () - 1)
    {
        if (getNumTracks () <= position)
            return false;
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
    if (keys.empty())
	mgError("mgSelection::leave(): keys is empty");
    if (m_level == keys.size ())
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
        keys[--m_level]->set (EMPTY, "");
        if (!m_fall_through)
            break;
        if (count () > 1)
            break;
    }
    return true;
}


bool mgSelection::UsedBefore (keyfield const *k, unsigned int level)
{
    if (level >= keys.size ())
        level = keys.size () - 1;
    for (unsigned int i = 0; i < level; i++)
        if (keys[i] == k)
            return true;
    return false;
}


bool mgSelection::isCollectionlist ()
{
    return (keys[0] == &kcollection && m_level == 0);
}

bool
mgSelection::inCollection(const string Name)
{
    bool result = (keys[0] == &kcollection && m_level == 1);
    if (result)
	    if (keys[1] != &kcollectionitem)
		    mgError("inCollection: key[1] is not kcollectionitem");
    if (!Name.empty())
    	result &= (keys[0]->value() == Name);
    return result;
}


const strvector &
mgSelection::keychoice (const unsigned int level)
{
    m_keychoice.clear ();
    if (level > keys.size ())
        return m_keychoice;
    map < string, keyfield * >::iterator it;
    map < string, keyfield * > possible_keys;
    for (it = all_keys.begin (); it != all_keys.end (); it++)
    {
	keyfield*f = (*it).second;
    	if (keycounts.find (f->choice ()) == keycounts.end ())
    	{
            keycounts[f->choice ()] = exec_count (f->KeyCountquery ());
    	}
    	unsigned int i = keycounts[f->choice ()];
        if ((&(*f) != &kcollection) && (&(*f) != &kcollectionitem) && (i < 2))
		;
	else
           possible_keys[string(tr(f->choice ().c_str()))] = &(*f);
    }

    for (it = possible_keys.begin (); it != possible_keys.end (); it++)
    {
        keyfield *k = (*it).second;
        if (level != 0 && k == &kcollection)
            continue;
        if (level != 1 && k == &kcollectionitem)
            continue;
        if (level == 1 && keys[0] != &kcollection && k == &kcollectionitem)
            continue;
        if (level == 1 && keys[0] == &kcollection && k != &kcollectionitem)
            continue;
        if (level > 1 && keys[0] == &kcollection)
            break;
        if (k == &kdecade && UsedBefore (&kyear, level))
            continue;
        if (!UsedBefore (k, level))
            m_keychoice.push_back (string(tr((*it).second->choice ().c_str())));
    }
    return m_keychoice;
}


void mgSelection::DumpState(mgValmap& nv)
{
	nv.put("Host",m_Host);
	nv.put("User",m_User);
	nv.put("Password",m_Password);
	nv.put("FallThrough",m_fall_through);
	nv.put("ShuffleMode",int(m_shuffle_mode));
	nv.put("LoopMode",int(m_loop_mode));
	nv.put("Directory",m_Directory);
	nv.put("ToplevelDir",m_ToplevelDir);
	nv.put("Level",int(m_level));
    	for (unsigned int i=0;i<keys.size();i++)
    	{
		char *n;
		asprintf(&n,"Keys.%d.Choice",i);
		nv.put(n,keys[i]->choice());
		asprintf(&n,"Keys.%d.Filter",i);
		nv.put(n,keys[i]->filter());
		if (i<m_level) {
			asprintf(&n,"Keys.%d.Position",i);
			nv.put(n,m_position[i]);
		}
	}
	nv.put("TrackId",m_trackid);
    	if (m_level == keys.size ())
		nv.put("Position",m_tracks_position);
	else
		nv.put("Position",m_position[m_level]);
	nv.put("TrackPosition",m_tracks_position);
}

map <string, string> *
mgSelection::UsedKeyValues()
{
	map <string, string> *result = new map<string, string>;
	for (unsigned int idx = 0 ; idx < level() ; idx++)
	{
		(*result)[keys[idx]->choice()] = keys[idx]->value();
	}
	if (level() < keys.size()-1)
	{
		string ch =  keys[level()]->choice();
		(*result)[ch] = getCurrentValue();
	}
	return result;
}
