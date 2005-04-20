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
#include <string>
#include <list>
#include <vector>
#include <map>
using namespace std;

#include "mg_tools.h"
#include "mg_valmap.h"
#include "mg_order.h"
#include "mg_content.h"
#include "mg_db.h"

typedef vector<string> strvector;

class mgListItems 
{
	public:
		mgListItems() { m_sel=0; }
		void setOwner(mgSelection* sel);
		mgListItem& operator[](unsigned int idx);
		string& id(unsigned int);
		unsigned int count(unsigned int);
		bool operator==(const mgListItems&x) const;
		size_t size() const;
       		unsigned int valindex (const string v) const;
       		unsigned int idindex (const string i) const;
		void clear();
		void push_back(mgListItem& item) { m_items.push_back(item); }
		vector<mgListItem>& items() { return m_items; }	//! \brief use only for loading!
	private:
       		unsigned int index (const string s,bool val,bool second_try=false) const;
		vector<mgListItem> m_items;
		mgSelection* m_sel;
};

/*!
 * \brief the only interface to the database.
 * Some member functions are declared const although they can modify the inner state of mgSelection.
 * But they only modify variables used for caching. With const, we want to express
 * the logical constness. E.g. the selected items can change without breaking constness:
 * The selection never defines concrete items but only how to choose them. 
 */
class mgSelection
{
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
            LM_SINGLE,                            //!< \brief loop a single item
            LM_FULL                               //!< \brief loop the whole item list
        };

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

/*! \brief represents all items for the current level. The result
 * is cached, subsequent accesses to values only incur a
 * small overhead for building the SQL WHERE command. The items will
 * be reloaded when the SQL command changes
 */
        mutable mgListItems listitems;

/*! \brief returns the name of a key
 */
        mgKeyTypes getKeyType (const unsigned int level) const;

//! \brief return the current value of this key
        mgListItem& getKeyItem (const unsigned int level) const;
	
/*! \brief returns the current item from the value() list
 */
	string getCurrentValue();

//! \brief returns a map (new allocated) for all used key fields and their values
	map<mgKeyTypes,string> UsedKeyValues();

//! \brief the current position
        unsigned int getPosition ()const;

	//! \brief go to the current position. If it does not exist,
	// go to the nearest.
        unsigned int gotoPosition ();


//! \brief the current position in the item list
        unsigned int getItemPosition () const;

	//! \brief go to the current item position. If it does not exist,
	// go to the nearest.
        unsigned int gotoItemPosition ();

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
            return enter (listitems.valindex (value));
        }

	/*! \brief like enter but if we are at the leaf level simply select
	 * the current entry
	 */
        bool select (const string value)
        {
            return select (listitems.valindex(value));
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

//! \brief the current level in the tree. This is at most order.size().
        unsigned int level () const
        {
            return m_level;
        }

	//! \brief true if the selection holds no items
	bool empty();

/*! \brief returns detailed info about all selected items.
 * The ordering is done only by the keyfield of the current level.
 * This might have to be changed - suborder by keyfields of detail
 * levels. This list is cached so several consequent calls mean no
 * loss of performance. See value(), the same warning applies.
 * \todo call this more seldom. See getNumItems()
 */
        const vector < mgContentItem > &items () const;

/*! \brief returns an item from the items() list
 * \param position the position in the items() list
 * \return returns NULL if position is out of range
 */
        mgContentItem* getItem (unsigned int position);

/*! \brief returns the current item from the items() list
 */
        mgContentItem* getCurrentItem ()
        {
            return getItem (gotoItemPosition());
        }

/*! \brief toggles the shuffle mode thru all possible values.
 * When a shuffle modus SM_NORMAL or SM_PARTY is selected, the
 * order of the items in the item list will be randomly changed.
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

/*! \brief adds the whole current item list to a collection
 * \param Name the name of the collection. If it does not yet exist,
 * it will be created.
 */
        unsigned int AddToCollection (const string Name);

/*! \brief removes the whole current item from a the collection
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

/*! generates an m3u file containing all items. The directory
 * can be indicated by SetDirectory().
 * The file name will be built from the list name, slashes
 * and spaces converted
 */
        string exportM3U ();


/*! \brief go to a position in the current level. If we are at the
 * most detailled level this also sets the item position since
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
            setPosition (listitems.valindex (value));
        }

/*! \brief go to a position in the item list
 * \param position the wanted position. If it is too big, go to the 
 * last existing position. If the position is not valid, find the
 * next valid one.
 * \return only if no position exists, false will be returned
 */
        void GotoItemPosition (unsigned int position) const;

/*! \brief skip some items in the item list
 * \return false if new position does not exist
 */
        bool skipItems (int step=1) const;

//! \brief returns the sum of the durations of all items
        unsigned long getLength ();

/*! \brief returns the sum of the durations of completed items
 * those are items before the current item position
 */
        unsigned long getCompletedLength () const;

/*! returns the number of items in the item list
 *  \todo should not call items () which loads all item info.
 *  instead, only count the items. If the size differs from
 *  m_items.size(), invalidate m_items
 */
        unsigned int getNumItems () const
        {
            return items ().size ();
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

	//! \brief true if values and items need to be reloaded
	bool cacheIsEmpty() const
	{
		return (m_current_values=="" && m_current_tracks=="");
	}
	string value(mgKeyTypes kt, string idstr) const;
	string value(mgKey* k, string idstr) const;
	string value(mgKey* k) const;
	string id(mgKeyTypes kt, string val) const;
	string id(mgKey* k, string val) const;
	string id(mgKey* k) const;
	bool Connected() { return m_db->Connected(); }

    private:
        mutable string m_current_values;
        mutable string m_current_tracks;
//! \brief be careful when accessing this, see mgSelection::items()
        mutable vector < mgContentItem > m_items;
        bool m_fall_through;
        unsigned int m_position;
        mutable unsigned int m_items_position;
        ShuffleMode m_shuffle_mode;
        void Shuffle() const;
        LoopMode m_loop_mode;
        mutable mgDb* m_db;
        unsigned int m_level;

        mgOrder order;
        void InitSelection ();
        string ListFilename ();

	void InitFrom(const mgSelection* s);

};


unsigned int randrange (const unsigned int high);


#endif                                            // _DB_H
