/*!
 * \file mg_db.h
 * \brief A database interface to the GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _DB_H
#define _DB_H
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <istream>
#include <sstream>
#include <ostream>
#include <i18n.h>
using namespace std;

#include "mg_tools.h"

typedef vector<string> strvector;

//! \brief a map for reading / writing configuration data. 
class mgValmap : public map<string,string> {
	private:
		const char *m_key;
	public:
		/*! \brief constructor
		 * \param key all names will be prefixed with key.
		 */
		mgValmap(const char *key);
		//! \brief read from file
		void Read(FILE *f);
		//! \brief write to file
		void Write(FILE *f);
		//! \brief enter a string value
		void put(const char*name, string value);
		//! \brief enter a C string value
		void put(const char*name, const char* value);
		//! \brief enter a long value
		void put(const char*name, long value);
		//! \brief enter a int value
		void put(const char*name, int value);
		//! \brief enter a unsigned int value
		void put(const char*name, unsigned int value);
		//! \brief enter a bool value
		void put(const char*name, bool value);
	 //! \brief return a string
	 string getstr(const char* name) {
		 return (*this)[name];
	 }
	 //! \brief return a C string
	 bool getbool(const char* name) {
		 return (getstr(name)=="true");
	 }
	 //! \brief return a long
	 long getlong(const char* name) {
		 return atol(getstr(name).c_str());
	 }
	 //! \brief return an unsigned int
	 unsigned int getuint(const char* name) {
		 return (unsigned long)getlong(name);
	 }
};

static const string EMPTY = "XNICHTGESETZTX";

class mgSelection;

//! \brief a generic keyfield
class keyfield
{
    public:
        //! \brief set the owning selection. 
        void setOwner(mgSelection *owner) { selection = owner; }

	//! \brief default constructor
        keyfield ()
        {
        };

	/*! \brief constructs a simple key field which only needs info from
 	 * the tracks table.
 	 * \param choice the internationalized name of this key field, e.g. "Jahr"
 	 */
        keyfield (const string choice);

	//! \brief default destructor
        virtual ~ keyfield ()
        {
        };

/*! \brief assigns a new id and value to the key field.
 * This also invalidates the cache of the owning mgSelection.
 * \param id used for lookups in the data base
 * \param value used for display
 */
        void set (const string id, const string value);

//! \brief helper function for streaming debug info
        void writeAt (ostream &) const;

//! \brief adds lookup data to a WHERE SQL statement
        string restrict (string & result) const;

//! \brief returns the internationalized name of this key field
        string choice () const
        {
            return m_choice;
        }

/*! \brief returns the id of this key field. This is the string used
 * for lookups in the data base, not for display
 */
        string id () const
        {
            return m_id;
        }

/*! \brief returns the filter for this key field.
 * \todo filters are not yet implemented but we already dump them in DumpState
 */
        string filter () const
        {
            return m_filter;
        }

/*! \brief returns the value of this key field. This is the string used
 * for the display.
 */
        string value () const
        {
            return m_value;
        }

	virtual string order() const
	{
	    return valuefield ();
	}

//! \brief returns the name of the corresponding field in the tracks table
        virtual string basefield () const { return ""; }

//! \brief returns the name of the field to be shown in the selection list
        virtual string valuefield () const
        {
            return basefield ();
        }

//! \brief returns the name of the identification field
        virtual string idfield () const
        {
            return basefield ();
        }

/*! \brief returns the name of the field needed to count how many
 * different values for this key field exist
 */
        virtual string countfield () const
        {
            return basefield ();
        }

/*! \brief returns a join clause needed for the composition of
 * a WHERE statement
 */
        virtual string join () const;

/*! \brief returns a join clause needed for the composition of
 * a WHERE statement especially for counting the number of items
 */
        virtual string countjoin () const
        {
            return join ();
        }

/*! \brief if true, the WHERE clause should also return values from
 * join tables
 */
        bool lookup;

//! \brief returns a SQL query command for counting different key
//values
        string KeyCountquery ();

    protected:
//! \brief the owning selection. 
	mgSelection* selection;
	//! \brief the english name for this key field
        string m_choice;
	//! \brief used for lookup in the data base
        string m_id;
	//! \brief used for OSD display
        string m_value;
	//! \brief an SQL restriction like 'tracks.year=1982'
        string m_filter;
	/*! \brief should be defined as true if we need to join another
	 * table for getting user friendly values (like the name of a genre)
	 */
        virtual bool need_join () const;
	//! \brief escape the string as needed for calls to mysql
	string sql_string(const string s) const;
};


//! \brief orders by collection
class collectionkeyfield:public keyfield
{
    public:
        collectionkeyfield ():keyfield ("Collection")
        {
        }
        string basefield () const
        {
            return "playlist.id";
        }
        string valuefield () const
        {
            return "playlist.title";
        }
/* this join() would ensure that empty collections be suppressed. But we
 * want them all. so we don't need the join
 */
        string join () const
        {
            return "";
        }
};

//! \brief orders by position in collection
class collectionitemkeyfield:public keyfield
{
    public:
        collectionitemkeyfield ():keyfield ("Collection item")
        {
        }
        string basefield () const
        {
            return "playlistitem.tracknumber";
        }
        string valuefield () const
        {
            return "tracks.title";
        }
	string order () const
	{
	    return basefield ();
	}
        string join () const
        {
            return
                "tracks.id=playlistitem.trackid and playlist.id=playlistitem.playlist";
        }
};

//! \brief orders by album.title
class albumkeyfield:public keyfield
{
    public:
        albumkeyfield ():keyfield ("Album")
        {
        }
        string basefield () const
        {
            return "tracks.sourceid";
        }
        string valuefield () const
        {
            return "album.title";
        }
        string idfield () const
        {
            return "album.title";
        }
        string countfield () const
        {
            return "album.title";
        }
        string join () const
        {
            return "tracks.sourceid=album.cddbid";
        }
    protected:
        //!brief we always need to join table album
        bool need_join () const
        {
            return true;
        };
};

//! \brief orders by genre1
class genre1keyfield:public keyfield
{
    public:
        genre1keyfield ():keyfield ("Genre 1")
        {
        }
        string basefield () const
        {
            return "tracks.genre1";
        }
        string valuefield () const
        {
            return "genre.genre";
        }
        string idfield () const
        {
            return "genre.id";
        }
};

//! \brief orders by genre2
class genre2keyfield:public keyfield
{
    public:
        genre2keyfield ():keyfield ("Genre 2")
        {
        }
        string basefield () const
        {
            return "tracks.genre2";
        }
        string valuefield () const
        {
            return "genre.genre";
        }
        string idfield () const
        {
            return "genre.id";
        }
};

//! \brief orders by language
class langkeyfield:public keyfield
{
    public:
        langkeyfield ():keyfield ("Language")
        {
        }
        string basefield () const
        {
            return "tracks.lang";
        }
        string valuefield () const
        {
            return "language.language";
        }
        string idfield () const
        {
            return "language.id";
        }
};

//! \brief orders by tracks.artist
class artistkeyfield:public keyfield
{
    public:
        artistkeyfield ():keyfield ("Artist")
        {
        }
        string basefield () const
        {
            return "tracks.artist";
        }
};

//! \brief orders by tracks.rating
class ratingkeyfield:public keyfield
{
    public:
        ratingkeyfield ():keyfield ("Rating")
        {
        }
        string basefield () const
        {
            return "tracks.rating";
        }
};

//! \brief orders by tracks.year
class yearkeyfield:public keyfield
{
    public:
        yearkeyfield ():keyfield ("Year")
        {
        }
        string basefield () const
        {
            return "tracks.year";
        }
};

//! \brief orders by tracks.title
class titlekeyfield:public keyfield
{
    public:
        titlekeyfield ():keyfield ("Title")
        {
        }
        string basefield () const
        {
            return "tracks.title";
        }
};

//! \brief orders by tracks.tracknb and tracks.title
class trackkeyfield:public keyfield
{
    public:
        trackkeyfield ():keyfield ("Track")
        {
        }
        string basefield () const
        {
            return "tracks.tracknb";
        }
        string valuefield () const
        {
            return
                "concat("
		  "if(tracks.tracknb>0,"
		    "concat("
		      "if(tracks.tracknb<10,'  ',''),"
		      "tracks.tracknb,"
		      "' '"
		    "),''"
		  "),"
		"tracks.title)";
        }
};

//! \brief orders by decade (deduced from tracks.year)
class decadekeyfield:public keyfield
{
    public:
        decadekeyfield ():keyfield ("Decade")
        {
        }
        string basefield () const
        {
            return "substring(convert(10 * floor(tracks.year/10), char),3)";
        }
};

//! \brief represents a content item like an mp3 file
class mgContentItem
{
    public:
        mgContentItem ()
        {
        }
	//! \brief copy constructor
        mgContentItem(const mgContentItem* c);

	//! \brief construct an item from an SQL row
        mgContentItem (const MYSQL_ROW row, const string ToplevelDir);
//! \brief returns track id
        long getId () const
        {
            return m_id;
        }

//! \brief returns title
        string getTitle () const
        {
            return m_title;
        }

//! \brief returns filename
        string getSourceFile () const
        {
            return m_mp3file;
        }

//! \brief returns artist
        string getArtist () const
        {
            return m_artist;
        }

//! \brief returns the name of the album
        string getAlbum ();

//! \brief returns the name of genre 1
        string getGenre1 ();

//! \brief returns the name of genre 2
        string getGenre2 ();

//! \brief returns the name of genre 1
        string getGenre ()
        {
            return getGenre1 ();
        }

//! \brief returns the bitrate
        string getBitrate () const
        {
            return m_bitrate;
        }

//! \brief returns the file name of the album image
        string getImageFile ();

//! \brief returns year
        int getYear () const
        {
            return m_year;
        }

//! \brief returns rating
        int getRating () const
        {
            return m_rating;
        }

//! \brief returns duration
        int getDuration () const
        {
            return m_duration;
        }

//! \brief returns samplerate
        int getSampleRate () const
        {
            return m_samplerate;
        }

//! \brief returns # of channels
        int getChannels () const
        {
            return m_channels;
        }
    private:
        long m_id;
        string m_title;
        string m_mp3file;
        string m_artist;
        string m_albumtitle;
        string m_genre1;
        string m_genre2;
        string m_bitrate;
        int m_year;
        int m_rating;
        int m_duration;
        int m_samplerate;
        int m_channels;
};

/*!
 * \brief the only interface to the database.
 */
class mgSelection
{
    private:
	class mgSelStrings 
	{
		friend class mgSelection;
		private:
			strvector strings;
			mgSelection* m_sel;
			void setOwner(mgSelection* sel);
		public:
			string& operator[](unsigned int idx);
			size_t size();
	};

    public:
/*! \brief define various ways to play music in random order
 * \todo Party mode is not implemented, does same as SM_NORMAL
 */
        enum ShuffleMode
        {
            SM_NONE,                              //!< \brief play normal sequence
            SM_NORMAL,                            //!< \brief a shuffle with a fair distribution
            SM_PARTY                              //!< \brief select the next few songs randomly, continue forever
        };

//! \brief define various ways to play music in a neverending loop
        enum LoopMode
        {
            LM_NONE,                              //!< \brief do not loop
            LM_SINGLE,                            //!< \brief loop a single track
            LM_FULL                               //!< \brief loop the whole track list
        };

//! \brief escapes special characters
	string sql_string(const string s) const;

//! \brief the default constructor. Does not start a DB connection.
        mgSelection();

/*! \brief the main constructor
 * \param Host where the data base lives. If not localhost, TCP/IP is used.
 * \param User if empty, the current user is used.
 * \param Password no comment
 * \param fall_through if TRUE: If enter() returns a choice
 * containing only one item, that item is automatically entered.
 * The analog happens with leave()
 */
        mgSelection (const string Host, const string User =
            "", const string Password = "", const bool fall_through =
            false);

/*! \brief a copy constructor. Does a deep copy.
 * Some of the data base content will only be retrieved by the
 * new mgSelection as needed, so some data base
 * overhead is involved
 */
        mgSelection (const mgSelection& s);
/*! \brief a copy constructor. Does a deep copy.
 * Some of the data base content will only be retrieved by the
 * new mgSelection as needed, so some data base
 * overhead is involved
 */
        mgSelection (const mgSelection* s);

/*! \brief the assignment operator. Does a deep copy.
 * Some of the data base content will only be retrieved by the
 * new mgSelection as needed, so some data base
 * overhead is involved
 */
	const mgSelection& operator=(const mgSelection& s);

//! \brief initializes from a map.
	void InitFrom(mgValmap& nv);

//! \brief the normal destructor
        ~mgSelection ();

	//! \brief sets the top level directory where content is stored
	void setToplevelDir(string ToplevelDir) { m_ToplevelDir = ToplevelDir; }

/*! \brief represents all values for the current level. The result
 * is cached in values, subsequent accesses to values only incur a
 * small overhead for building the SQL WHERE command. The values will
 * be reloaded when the SQL command changes
 * \todo we should do more caching. The last 5 result sets should be cached.
 */
        mgSelStrings values;

/*! \brief defines a field to be used as key for selection
 *
 * \param level 0 is the top level
 * \param name of the key field, internationalized. Possible values
 * are defined by keychoice()
 */
        void setKey (const unsigned int level, const string name);

/*! \brief returns the name of a key
 */
        string getKeyChoice (const unsigned int level)
        {
            return keys[level]->choice ();
        }
	//! \brief return the current value of this key
        string getKeyValue (const unsigned int level)
        {
            return keys[level]->value ();
        }

//! \brief returns a map (new allocated) for all used key fields and their values
	map<string,string> * UsedKeyValues();

//! \brief helper function for << operator (dumps debug info)
        void writeAt (ostream &);

/*! \brief returns FROM and WHERE clauses for the current state
 * of the selection.
 * \param want_trackinfo work in progress, should disappear I hope
 */
        string where (bool want_trackinfo = false);

//! \brief the number of music items currently selected
        unsigned int count ();

//! \brief the number of key fields used for the query
        unsigned int size ();

//! \brief the current position in the current level
        unsigned int gotoPosition () 
        {
            return gotoPosition (m_level);
        }

//! \brief the current position
        unsigned int getPosition (unsigned int level) const;

	//! \brief go to the current position. If it does not exist,
	// go to the nearest.
        unsigned int gotoPosition (unsigned int level);

//! \brief the current position in the tracks list
        unsigned int getTrackPosition () const;

	//! \brief go to the current track position. If it does not exist,
	// go to the nearest.
        unsigned int gotoTrackPosition ();

/*! \brief enter the the next higher level, go one up in the tree.
 * If fall_through (see constructor) is set to true, and the
 * level entered by enter() contains only one item, automatically
 * goes down further until a level with more than one item is reached.
 * \param position is the position in the current level that is to be expanded
 * \return returns false if there is no further level
 */
        bool enter (unsigned int position);

	/*! \brief like enter but if we are at the leaf level simply select
	 * the entry at position
	 */
        bool select (unsigned int position);

/*! \brief enter the next higher level, expanding the current position.
 * See also enter(unsigned int position)
 */
        bool enter ()
        {
            return enter (gotoPosition ());
        }

	/*! \brief like enter but if we are at the leaf level simply select
	 * the current entry
	 */
        bool select ()
        {
            return select (gotoPosition ());
        }

/*! \brief enter the next higher level, expanding the position holding a certain value
 * \param value the position holding value will be expanded.
 */
        bool enter (const string value)
        {
            return enter (valindex (value));
        }

	/*! \brief like enter but if we are at the leaf level simply select
	 * the current entry
	 */
        bool select (const string value)
        {
            return select (valindex(value));
        }

/*! \brief leave the current level, go one up in the tree.
 * If fall_through (see constructor) is set to true, and the
 * level entered by leave() contains only one item, automatically
 * goes up further until a level with more than one item is reached.
 * \return returns false if there is no further upper level
 */
        bool leave ();

/*! \brief leave the current level, go up in the tree until
 * target level is reached.
 * If fall_through (see constructor) is set to true, and the
 * level entered by leave() contains only one item, automatically
 * goes up further until a level with more than one item is reached.
 * \return returns false if there is no further upper level
 */
        bool leave (const unsigned int target_level)
	{
		while (m_level>target_level)
			if (!leave()) return false;
		return true;
	}

//! \brief the current level in the tree
        unsigned int level () const
        {
            return m_level;
        }

/*! \brief the possible choices for a keyfield in this level.
 * keyfields already used in upper levels are no possible
 * choices, neither are most keyfields if their usage would
 * allow less than 2 choices.
 */
        const strvector &keychoice (const unsigned int level);

/*! \brief returns the current item from the value() list
 */
	string getCurrentValue();

	//! \brief true if the selection holds no items
	bool empty();

/*! \brief returns detailled info about all selected tracks.
 * The ordering is done only by the keyfield of the current level.
 * This might have to be changed - suborder by keyfields of detail
 * levels. This list is cached so several consequent calls mean no
 * loss of performance. See value(), the same warning applies.
 * \todo call this more seldom. See getNumTracks()
 */
        const vector < mgContentItem > &tracks ();

/*! \brief returns an item from the tracks() list
 * \param position the position in the tracks() list
 * \return returns NULL if position is out of range
 */
        mgContentItem* getTrack (unsigned int position);

/*! \brief returns the current item from the tracks() list
 */
        mgContentItem* getCurrentTrack ()
        {
            return getTrack (gotoTrackPosition());
        }

/*! \brief toggles the shuffle mode thru all possible values.
 * When a shuffle modus SM_NORMAL or SM_PARTY is selected, the
 * order of the tracks in the track list will be randomly changed.
 */
        ShuffleMode toggleShuffleMode ();

//! \brief toggles the loop mode thru all possible values
        LoopMode toggleLoopMode ();

//! \brief returns the current shuffle mode
        ShuffleMode getShuffleMode () const
        {
            return m_shuffle_mode;
        }

//! \brief sets the current shuffle mode
        void setShuffleMode (const ShuffleMode shuffle_mode)
        {
            m_shuffle_mode = shuffle_mode;
        }

//! \brief returns the current loop mode
        LoopMode getLoopMode () const
        {
            return m_loop_mode;
        }

//! \brief sets the current loop mode
        void setLoopMode (const LoopMode loop_mode)
        {
            m_loop_mode = loop_mode;
        }

/*! \brief adds the whole current track list to a collection
 * \param Name the name of the collection. If it does not yet exist,
 * it will be created.
 */
        unsigned int AddToCollection (const string Name);

/*! \brief removes the whole current track from a the collection
 * Remember - this selection can be configured to hold exactly
 * one list, so this command can be used to clear a selected list.
 * \param Name the name of the collection
 */
        unsigned int RemoveFromCollection (const string Name);
//! \brief delete a collection
        bool DeleteCollection (const string Name);
/*! \brief create a collection only if it does not yet exist.
 * \return true only if it has been created. false if it already existed.
 */
        bool CreateCollection(const string Name);

//! \brief remove all items from the collection
        void ClearCollection (const string Name);

/*! generates an m3u file containing all tracks. The directory
 * can be indicated by SetDirectory().
 * The file name will be built from the list name, slashes
 * and spaces converted
 */
        string exportM3U ();

/*! \brief go to a position in the current level. If we are at the
 * most detailled level this also sets the track position since
 * they are identical.
 * \param position the wanted position. If it is too big, go to the 
 * last existing position
 * \return only if no position exists, false will be returned
 */
        void setPosition (unsigned int position);

/*! \brief go to the position with value in the current level
 * \param value the value of the wanted position
 */
        void setPosition (const string value)
        {
            setPosition (valindex (value));
        }

/*! \brief go to a position in the track list
 * \param position the wanted position. If it is too big, go to the 
 * last existing position
 * \return only if no position exists, false will be returned
 */
        void setTrack (unsigned int position);

/*! \brief skip some tracks in the track list
 * \return false if new position does not exist
 */
        bool skipTracks (int step=1);

/*! \brief skip forward by 1 in the track list
 * \return false if new position does not exist
 */
        bool skipFwd ()
        {
            return skipTracks (+1);
        }

/*! \brief skip back by 1 in the track list
 * \return false if new position does not exist
 */
        bool skipBack ()
        {
            return skipTracks (-1);
        }

//! \brief returns the sum of the durations of all tracks
        unsigned long getLength ();

/*! \brief returns the sum of the durations of completed tracks
 * those are tracks before the current track position
 */
        unsigned long getCompletedLength ();

/*! returns the number of tracks in the track list
 *  \todo should not call tracks () which loads all track info.
 *  instead, only count the tracks. If the size differs from
 *  m_tracks.size(), invalidate m_tracks
 */
        unsigned int getNumTracks ()
        {
            return tracks ().size ();
        }

//! sets the directory for the storage of m3u file
        void SetDirectory (const string directory)
        {
            m_Directory = directory;
        }

/*! returns the name of the current play list. If no play list is active,
 * the name is built from the name of the key fields.
 */
        string getListname ();

/*! \brief true if this selection currently selects a list of collections
 */
        bool isCollectionlist ();

	//! \brief true if we have entered a collection
	bool inCollection(const string Name="");

	/*! \brief dumps the entire state of this selection into a map,
	 * \param nv the values will be entered into this map
	 */
        void DumpState(mgValmap& nv);

        /*! \brief creates a new selection using saved definitions
	 * \param nv this map contains the saved definitions
	 */
	mgSelection(mgValmap&  nv);

	//! \brief clear the cache, next access will reload from data base
        void clearCache();

	//! \todo soll sql_values() nur noch bei Bedarf bauen, also muessen
	// alle Aenderungen, die Einfluss darauf haben, clearCache machen
        void refreshValues();

	//! \brief true if values and tracks need to be reloaded
	bool cacheIsEmpty()
	{
		return (m_current_values=="" && m_current_tracks=="");
	}
    private:
	void AddOrder(const string sql,list<string>& orderlist, const string item);
    	list < string > m_fromtables; //!< \brief part result from previous where()
    	string m_from;	//!< \brief part result from previous where()
	string m_where;	//!< \brief part result from previous where()
        bool m_fall_through;
        vector < unsigned int >m_position;
        unsigned int m_tracks_position;
        ShuffleMode m_shuffle_mode;
        LoopMode m_loop_mode;
        MYSQL *m_db;
        string m_Host;
        string m_User;
        string m_Password;
	string m_ToplevelDir;
        unsigned int m_level;
        long m_trackid;
        string m_current_values;
        string m_current_tracks;

//! \brief be careful when accessing this, see mgSelection::tracks()
        vector < mgContentItem > m_tracks;
        strvector m_ids;
        strvector m_keychoice;
        artistkeyfield kartist;
        ratingkeyfield krating;
        yearkeyfield kyear;
        decadekeyfield kdecade;
        albumkeyfield kalbum;
        collectionkeyfield kcollection;
        collectionitemkeyfield kcollectionitem;
        genre1keyfield kgenre1;
        genre2keyfield kgenre2;
        langkeyfield klanguage;
        titlekeyfield ktitle;
        trackkeyfield ktrack;
        map < string, keyfield * >all_keys;
        map < string, keyfield * >trall_keys;
        vector < keyfield * >keys;
        bool UsedBefore (keyfield const *k, unsigned int level);
        void InitSelection ();
        void InitDatabase ();
        void initkey (keyfield & f);
	/*! \brief returns the SQL command for getting all values. 
	 * For the leaf level, all values are returned. For upper
	 * levels, every distinct value is returned only once.
	 * This must be so for the leaf level because otherwise
	 * the value() entries do not correspond to the track()
	 * entries and the wrong tracks might be played.
	 */
        string sql_values ();
	//! \todo das nach mgSelStrings verlagern
        unsigned int valindex (const string val,const bool second_try=false);
        string ListFilename ();
        string m_Directory;
        void loadgenres ();
        MYSQL_RES *exec_sql (string query);
        string get_col0 (string query);

	void InitFrom(const mgSelection* s);

/*! \brief executes a query and returns the integer value from
 * the first column in the first row. The query shold be a COUNT query
 * returning only one row.
 * \param query the SQL query to be executed
 */
        unsigned long mgSelection::exec_count (string query);

        keyfield* findKey (const string name);
	map < string, unsigned int > keycounts;
};

//! \brief streams debug info about a selection
ostream & operator<< (ostream &, mgSelection & s);

//! \brief convert the shuffle mode into a string
// \return strings "SM_NONE" etc.
string toString (mgSelection::ShuffleMode);

//! \brief same as toString but returns a C string
const char *toCString (mgSelection::ShuffleMode);

//string toString(long int l);

string itos (int i);
unsigned int randrange (const unsigned int high);


#endif                                            // _DB_H
