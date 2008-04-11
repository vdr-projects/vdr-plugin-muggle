/*!
 * \file   vdr_menu.h
 * \brief  Implements menu handling for broswing media libraries within VDR
 *
 * \version $Revision: 1.13 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author$
 *
 *  $Id$
 */

#ifndef _VDR_MENU_H
#define _VDR_MENU_H

#include <string>
// #include <list>
#include <vector>

#include <vdr/osd.h>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include "vdr_actions.h"
#include "mg_menu.h"

//! \brief play a selection, aborting what is currently played
//! \param select if true, play only what the current position selects
void Play(mgSelection *sel,const bool select=false);

void showmessage(int duration, const char *msg, ...);
void showimportcount(unsigned int count);

class cCommands;

class mgSelection;
class mgSelMenu;
class mgIncrementalSearch;
class mgPlayerControl;

//! \brief if a player is running, return it
mgPlayerControl * PlayerControl ();

/*!
 * \brief the muggle main OSD
 */

class mgSelOsd : public mgOsd
{
	private:
		mgSelection *m_playsel;
		mgSelection *m_collectionsel;
		vector<mgSelection*> selections;
		unsigned int m_current_selection;
		void DumpSelections(mgValmap& nv);
		void LoadSelections(mgValmap& nv);
	public:
		void AddSelection();
		void DeleteSelection();
		void AddOrderActions(mgSelMenu *m);
		unsigned int getCurrentSelection() { return m_current_selection; }
		void setSelection(unsigned int idx,mgSelection *s);
		mgSelection* getSelection(unsigned int idx);
		bool SwitchSelection();

		mgSelection *moveselection;

		//! \brief play the play selection, abort ongoing instant play
		void PlayQueue();

		//! \brief instant play the selection, abort ongoing queue playing
		void PlayInstant(const bool select=false);

		//! \brief true if we are browsing m_collectionsel
		bool UsingCollection;

		//! \brief true if an item from the "playing" selection is being played
		bool queue_playing;

		//! \brief true if an item is being instant played
		bool instant_playing;

		//! \brief default constructor
		mgSelOsd ();

		//! \brief default destructor
		~mgSelOsd ();

		//! \brief save the entire muggle state
		void SaveState();

		//! \brief adds a new mgMenu to the stack
		virtual void AddMenu (mgMenu * m,int position=0);

		//! \brief initializes using values from nv
		void InitMapFromSetup (mgValmap& nv);

		//! \brief main entry point, called from vdr
		eOSState ProcessKey (eKeys Key);

		//! \brief from now on use the normal selection
		void UseNormalSelection () {
			UsingCollection= false;
		}

		//! \brief from now on use the collection selection
		void UseCollectionSelection () {
			UsingCollection= true;
		}

		//! \brief this is the collection things will be added to
		string default_collection;

		/*! \brief this is the "now playing" collection translated in
		 * the current language. When changing the OSD language, this
		 * collection will NOT be renamed in the data base, but a new
		 * empty collection will be started. The collection for the
		 * previous language will stay, so the user can copy from the
		 * old one to the new one.
		 */
		string play_collection;

		/*! \brief selects a certain line on the OSD and displays the OSD
		 */
		//! \brief external commands
		cCommands *external_commands;

		//! \brief clears the screen, sets a title and the hotkey flag
		void InitOsd (string title,const bool hashotkeys);

#if VDRVERSNUM >= 10307
		//! \brief expose the protected DisplayMenu() from cOsdMenu
		cSkinDisplayMenu *DisplayMenu(void) {
			return cOsdMenu::DisplayMenu();
		}
#endif

		//! \brief expose the protected cOsdMenu::hk()
		const char *hk (const char *s) {
			return cOsdMenu::hk (s);
		}

		//! \brief the current selection
		mgSelection* selection () const
		{
			if (UsingCollection)
				return m_collectionsel;
			else
				return selections[m_current_selection];
		}

		//! \brief the collection selection
		mgSelection* collselection() const
		{
			return m_collectionsel;
		}

		//! \brief the "now playing" selection
		mgSelection* playselection () const
		{
			return m_playsel;
		}

		//! \brief true if the cursor is placed in the collection list
		bool ShowingCollections();

		//! \brief true if the cursor is placed on the default collection
		bool DefaultCollectionSelected();

		//! \brief true if the cursor is placed in the default collection
		bool CollectionEntered(string name);

		void CollectionChanged(string name,bool added);
};

//! \brief a generic muggle menu
class mgSelMenu : public mgMenu
{
	protected:
		bool m_prevUsingCollection;

		//! \brief adds entries for all selected data base items to the OSD menu.
		// If this is the list of collections, appends a command for collection
		// creation.
		void AddSelectionItems (mgSelection *sel,mgActions act = actEntry);
		//! \brief the name of the blue button depends of where we are
		mgActions ButtonAction(eKeys key);
	public:
		mgSelMenu ();

		void SetHotkeyAction(eKeys key,mgActions action);

		mgSelOsd * Selosd() const { return dynamic_cast<mgSelOsd *>(m_osd); }

		void setosd(mgOsd *osd);

		//! \brief the currently active selection of the owning mgSelOsd
		mgSelection* selection ()  const
		{
			return Selosd ()->selection ();
		}
		//! \brief the playselection of the owning mgSelOsd
		mgSelection* playselection () {
			return Selosd ()->playselection ();
		}

		/*! \brief Process() should be abstract but then we cannot compile.
		 * \return Process may decide that we want another screen to be displayed.
		 * If the mgSelMenu* returned is not "this", the caller will use the return
		 * value for a new display. If NULL is returned, the caller will display
		 * the previous menu.
		 */
		virtual eOSState Process (eKeys Key);

		//! \brief the ID of the action defined by the red button.
		mgActions CollRedAction;

		//! \brief the ID of the action defined by the green button.
		mgActions CollGreenAction;

		//! \brief the action defined by the yellow button.
		mgActions CollYellowAction;

		//! \brief the action defined by the blue button.
		mgActions CollBlueAction;
};

//! \brief an mgSelMenu class for navigating through the data base
class mgTree:public mgSelMenu
{
	public:

		mgTree();

		//! \brief computes the title
		string Title() const;

		bool UpdateIncrementalSearch( eKeys key );

		void TerminateIncrementalSearch( bool remain_on_current );

	protected:
		void BuildOsd ();

	private:

		void UpdateSearchPosition();

		mgIncrementalSearch *m_incsearch;

		string m_filter;

		int m_start_position;
};

//! \brief an mgSelMenu class for submenus
class mgSubmenu:public mgSelMenu
{
	public:
		mgSubmenu();
		//! \brief computes the title
		string Title() const;
	protected:
		void BuildOsd ();
};

//! \brief an mgSelMenu class for selecting a selection
class mgSelMenuOrders:public mgSelMenu
{
	public:
		//! \brief computes the title
		string Title() const;
	protected:
		void BuildOsd ();
};

class mgSelMenuOrder : public mgSelMenu
{
	public:
		mgSelMenuOrder();
		~mgSelMenuOrder();
		//! \brief computes the title
		string Title() const;
		bool ChangeSelection(eKeys key);
		void SaveSelection();
	protected:
		void BuildOsd ();
	private:
		void AddKeyActions(mgSelMenu *m,mgSelection *o);
		mgSelection * m_orgselection;
		mgSelection * m_selection;
		int m_orderbycount;
		vector<int> m_keytypes;
		vector < vector <const char*> > m_keynames;
};

//! \brief an mgSelMenu class for selecting a collection
class mgTreeCollSelector:public mgSelMenu
{
	public:
		mgTreeCollSelector();
		~mgTreeCollSelector();
		//! \brief computes the title
		string Title() const;
	protected:
		void BuildOsd ();
		virtual mgActions coll_action() = 0;
		string m_title;
};

class mgTreeAddToCollSelector:public mgTreeCollSelector
{
	public:
		//! \brief computes the title
		mgTreeAddToCollSelector(string title);
	protected:
		virtual mgActions coll_action() { return actAddCollEntry; }
};

//! \brief an mgSelMenu class for selecting a collection
class mgTreeRemoveFromCollSelector:public mgTreeCollSelector
{
	public:
		//! \brief computes the title
		mgTreeRemoveFromCollSelector(string title);
	protected:
		virtual mgActions coll_action() { return actRemoveCollEntry; }
};

mgSelection* GenerateSelection(const mgSelection *s=0);
#endif
