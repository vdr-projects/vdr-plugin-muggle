/*!
 * \file mg_selection.h
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_SELECTION_H
#define _MG_SELECTION_H
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <i18n.h>
using namespace std;

#include "mg_tools.h"
#include "mg_valmap.h"
#include "mg_order.h"
#include "mg_content.h"

typedef vector<string> strvector;


/*!
 * \brief the only interface to the database.
 * Some member functions are declared const although they can modify the inner state of mgSelection.
 * But they only modify variables used for caching. With const, we want to express
 * the logical constness. E.g. the selected tracks can change without breaking constness:
 * The selection never defines concrete tracks but only how to choose them. 
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
			bool operator==(const mgSelStrings&x) const;
			size_t size() const;
	};
    public:
//! \brief defines an order to be used 
        void setOrder(mgOrder *o);

	mgOrder& getOrder() { return order; }

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

/*! \brief the main constructor
 * \param fall_through if TRUE: If enter() returns a choice
 * containing only one item, that item is automatically entered.
 * The analog happens with leave()
 */
        mgSelection (  const bool fall_through = false);

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


//! \brief initializes from a map.
	void InitFrom(mgValmap& nv);

//! \brief the normal destructor
        ~mgSelection ();

/*! \brief represents all values for the current level. The result
 * is cached in values, subsequent accesses to values only incur a
 * small overhead for building the SQL WHERE command. The values will
 * be reloaded when the SQL command changes
 */
        mutable mgSelStrings values;

/*! \brief returns the name of a key
 */
        mgKeyTypes getKeyType (const unsigned int level) const;

//! \brief return the current value of this key
        string getKeyValue (const unsigned int level) const;
	unsigned int getKeyIndex(const unsigned int level) const;
	
/*! \brief returns the current item from the value() list
 */
	string getCurrentValue();

//! \brief returns a map (new allocated) for all used key fields and their values
	map<mgKeyTypes,string> UsedKeyValues();

//! \brief the number of key fields used for the query
        unsigned int ordersize ();

//! \brief the number of music items currently selected
        unsigned int count () const;

//! \brief the current position
        unsigned int getPosition ()const;

	//! \brief go to the current position. If it does not exist,
	// go to the nearest.
        unsigned int gotoPosition ();


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

	bool selectid (const string id)
	{
	    return select(idindex(id));
	}

	void selectfrom(mgOrder& oldorder,mgContentItem* o);

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
        void leave_all ();

//! \brief the current level in the tree
        unsigned int level () const
        {
            return m_level;
        }

	//! \brief true if the selection holds no items
	bool empty();

/*! \brief returns detailed info about all selected tracks.
 * The ordering is done only by the keyfield of the current level.
 * This might have to be changed - suborder by keyfields of detail
 * levels. This list is cached so several consequent calls mean no
 * loss of performance. See value(), the same warning applies.
 * \todo call this more seldom. See getNumTracks()
 */
        const vector < mgContentItem > &tracks () const;

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
        void setShuffleMode (const ShuffleMode shuffle_mode);

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

	/*! import/export tags like
	 * \par path can be a file or a directory. If directory, 
	 * sync all files within but by default non recursive
	 * \par recursive recurse into all directories beneath path
	 * \par assorted see mugglei -h
	 * \par delete_missing if the file does not exist, delete the
	 * data base entry. If the file is unreadable, do not delete.
	 */
	void Sync(string path, bool recursive=false,bool assorted=false,bool delete_missing=false);

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
        void setTrackPosition (unsigned int position) const;

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
        unsigned long getCompletedLength () const;

/*! returns the number of tracks in the track list
 *  \todo should not call tracks () which loads all track info.
 *  instead, only count the tracks. If the size differs from
 *  m_tracks.size(), invalidate m_tracks
 */
        unsigned int getNumTracks () const
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
        string getListname () const;

/*! \brief true if this selection currently selects a list of collections
 */
        bool isCollectionlist () const;

/*! \brief true if this selection currently selects a list of languages
 */
        bool isLanguagelist () const;

	//! \brief true if we have entered a collection
	bool inCollection(const string Name="") const;

	/*! \brief dumps the entire state of this selection into a map,
	 * \param nv the values will be entered into this map
	 */
        void DumpState(mgValmap& nv) const;

        /*! \brief creates a new selection using saved definitions
	 * \param nv this map contains the saved definitions
	 */
	mgSelection(mgValmap&  nv);

	//! \brief clear the cache, next access will reload from data base
        void clearCache() const;

        void refreshValues() const;

	//! \brief true if values and tracks need to be reloaded
	bool cacheIsEmpty() const
	{
		return (m_current_values=="" && m_current_tracks=="");
	}
	string value(mgKeyTypes kt, string id) const;
	string value(mgKey* k, string id) const;
	string value(mgKey* k) const;
	string id(mgKeyTypes kt, string val) const;
	string id(mgKey* k, string val) const;
	string id(mgKey* k) const;
	unsigned int keycount(mgKeyTypes kt);
	vector <const char *> choices(mgOrder *o,unsigned int level, unsigned int *current);
	unsigned int valcount (string val);

    private:
	mutable map <mgKeyTypes, map<string,string> > map_values;
	mutable map <mgKeyTypes, map<string,string> > map_ids;
        mutable string m_current_values;
        mutable string m_current_tracks;
//! \brief be careful when accessing this, see mgSelection::tracks()
        mutable vector < mgContentItem > m_tracks;
        mutable strvector m_ids;
	mutable vector < unsigned int > m_counts;
	//! \brief initializes maps for id/value mapping in both direction
	bool loadvalues (mgKeyTypes kt) const;
        bool m_fall_through;
        unsigned int m_position;
        mutable unsigned int m_tracks_position;
        ShuffleMode m_shuffle_mode;
        void Shuffle() const;
        LoopMode m_loop_mode;
        MYSQL *m_db;
	void setDB(MYSQL *db);
        unsigned int m_level;
        long m_trackid;

        mgOrder order;
	bool UsedBefore (mgOrder *o,const mgKeyTypes kt, unsigned int level) const;
        void InitSelection ();
        void Connect ();
	/*! \brief returns the SQL command for getting all values. 
	 * For the leaf level, all values are returned. For upper
	 * levels, every distinct value is returned only once.
	 * This must be so for the leaf level because otherwise
	 * the value() entries do not correspond to the track()
	 * entries and the wrong tracks might be played.
	 */
        string sql_values ();
        unsigned int valindex (const string val,const bool second_try=false) const;
        unsigned int idindex (const string val,const bool second_try=false) const;
        string ListFilename ();
        string m_Directory;
        void loadgenres ();
	MYSQL_RES * exec_sql(string query) const;
        string get_col0 (string query) const;

	void InitFrom(const mgSelection* s);

/*! \brief executes a query and returns the integer value from
 * the first column in the first row. The query shold be a COUNT query
 * returning only one row.
 * \param query the SQL query to be executed
 */
        unsigned long exec_count (string query) const;

	
};


unsigned int randrange (const unsigned int high);


#endif                                            // _DB_H
