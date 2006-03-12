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

#include <osd.h>
#include <plugin.h>
#include <status.h>
#include "vdr_actions.h"

#include "vdr_player.h"

//! \brief play a selection, aborting what is currently played
//! \param select if true, play only what the current position selects
void Play(mgSelection *sel,const bool select=false);

void showmessage(int duration, const char *msg, ...);
void showimportcount(unsigned int count);

class cCommands;

class mgSelection;
class mgMenu;
class mgMainMenu;
class mgIncrementalSearch;

//! \brief if a player is running, return it
mgPlayerControl * PlayerControl ();

//! \brief callback class, monitors state changes in vdr
class mgStatus : public cStatus
{
	private:
		//! \brief the mgMainMenu that wants to be notified
		mgMainMenu *main;
	public:
		//! \brief default constructor
		mgStatus(mgMainMenu* m) { main = m;}
	protected:
		//! \brief the event we want to know about
		virtual void OsdCurrentItem(const char *Text);
};


/*!
 * \brief the muggle main OSD
 */

class mgMainMenu:public cOsdMenu
{
    private:
        mgSelection *m_playsel;
        mgSelection *m_collectionsel;
	char *m_message;
	void showMessage();
	void LoadExternalCommands();
	vector<mgSelection*> selections;
	unsigned int m_current_selection;
	void DumpSelections(mgValmap& nv);
	void LoadSelections(mgValmap& nv);
	mgMenu *m_root;
	bool m_save_warned;
    public:
	void AddSelection();
	void DeleteSelection();
	void AddOrderActions(mgMenu *m);
	unsigned int getCurrentSelection() { return m_current_selection; }
	void setSelection(unsigned int idx,mgSelection *s);
	mgSelection* getSelection(unsigned int idx);
	bool SwitchSelection();

	mgSelection *moveselection;
	mgActions CurrentType();

	//! \brief syntactic sugar: expose the protected cOsdMenu::SetHelp
	void SetHelpKeys(const char *Red,const char *Green, const char *Yellow, const char *Blue) { SetHelp(Red,Green,Yellow,Blue); }

	//! \brief callback object, lets vdr notify us about OSD changes
	mgStatus *m_Status;

	//! \brief play the play selection, abort ongoing instant play
	void PlayQueue();

	//! \brief instant play the selection, abort ongoing queue playing
	void PlayInstant(const bool select=false);

        //! \brief true if we are browsing m_collectionsel
        bool UsingCollection;

	//! \brief the different menus, the last one is active
        vector < mgMenu * >Menus;

	//! \brief true if an item from the "playing" selection is being played
	bool queue_playing;
	
	//! \brief true if an item is being instant played
	bool instant_playing;

	//! \brief parent menu if any
        mgMenu * Parent ();

        //! \brief default constructor
	mgMainMenu ();

        //! \brief default destructor
	~mgMainMenu ();

	//! \brief save the entire muggle state
        void SaveState();

	//! \brief adds a new mgMenu to the stack
        void AddMenu (mgMenu * m,unsigned int position=0);

	//! \brief initializes using values from nv
        void InitMapFromSetup (mgValmap& nv);

	//! \brief main entry point, called from vdr
        eOSState ProcessKey (eKeys Key);

	//! \brief from now on use the normal selection
        void UseNormalSelection ()
        {
            UsingCollection= false;
        }

	//! \brief from now on use the collection selection
        void UseCollectionSelection ()
        {
            UsingCollection= true;
        }

	//! \brief this is the collection things will be added to
        std::string default_collection;

/*! \brief this is the "now playing" collection translated in
 * the current language. When changing the OSD language, this
 * collection will NOT be renamed in the data base, but a new
 * empty collection will be started. The collection for the
 * previous language will stay, so the user can copy from the
 * old one to the new one.
 */
	std::string play_collection;

/*! \brief selects a certain line on the OSD and displays the OSD
 */
        void DisplayGoto ();

	//! \brief external commands
        cCommands *external_commands;

	//! \brief Actions can set newmenu which will then be displayed next
        mgMenu *newmenu;

	//! \brief Actions can set newstate which will then be returned to main vdr
        eOSState newstate;

	//! \brief Actions can set forcerefresh. This will force a redisplay of the OSD
        bool forcerefresh;

	//! \brief show a message. Can be called by actions. It will
	// only be shown at the end of the next mgMainMenu::ProcessKey
	// because that might do forcerefresh which overwrites the message
	void Message (const char *msg) { m_message = strdup(msg); }
        const char* Message1 (const char *msg, ...)
		__attribute__ ((format (printf, 2, 3)));
        const char* Message1 (const char *msg, std::string arg1)
 		{ return Message1(msg,arg1.c_str()); }

	//! \brief Actions can request a new position. -1 means none wanted
	int newposition;

	//! \brief clears the screen, sets a title and the hotkey flag
        void InitOsd (std::string title,const bool hashotkeys);

#if VDRVERSNUM >= 10307
	//! \brief expose the protected DisplayMenu() from cOsdMenu
	cSkinDisplayMenu *DisplayMenu(void)
	{
		return cOsdMenu::DisplayMenu();
	}
#endif

	//! \brief expose the protected cOsdMenu::hk()
        const char *hk (const char *s)
        {
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
	bool CollectionEntered(std::string name);

	void AddItem(mgAction *a);

	void CollectionChanged(std::string name,bool added);

	void CloseMenu();

	void RefreshTitle();
};

//! \brief a generic muggle menu
class mgMenu
{
    private:
        mgMainMenu*  m_osd;
	const char *HKey(const mgActions act,mgActions on);
    protected:
	unsigned int m_prevpos;
	bool m_prevUsingCollection;
	eOSState ExecuteButton(eKeys key);
//! \brief adds the wanted action to the OSD menu
// \param hotkey if true, add this as a hotkey
	void AddAction(const mgActions action, mgActions on = mgActions(0),const bool hotkey=true);

	//! \brief add an external action, always with hotkey
	void AddExternalAction(const mgActions action, const char *title);

//! \brief adds entries for all selected data base items to the OSD menu.
// If this is the list of collections, appends a command for collection 
// creation.
        void AddSelectionItems (mgSelection *sel,mgActions act = actEntry);
	//! \brief the name of the blue button depends of where we are
	int m_parent_index;
	std::string m_parent_name;
    public:
	//! sets the correct help keys.
	void SetHelpKeys(mgActions on = mgActions(0));
//! \brief generates an object for the wanted action
	mgAction* GenerateAction(const mgActions action,mgActions on);

//! \brief executes the wanted action
        eOSState ExecuteAction (const mgActions action, const mgActions on);

//! \brief sets the pointer to the owning mgMainMenu
        void setosd (mgMainMenu* osd);

	void setParentIndex(int idx) { m_parent_index = idx; }
	int getParentIndex() { return m_parent_index; }
	void setParentName(std::string name) { m_parent_name = name; }
	std::string getParentName() { return m_parent_name; }

//! \brief the pointer to the owning mgMainMenu
        mgMainMenu* osd () const
        {
            return m_osd;
        }

//! \brief the currently active selection of the owning mgMainMenu
        mgSelection* selection () const
        {
            return osd ()->selection ();
        }
//! \brief the playselection of the owning mgMainMenu
        mgSelection* playselection () const
        {
            return osd ()->playselection ();
        }

        mgMenu ();

        virtual ~ mgMenu ()
        {
        }

//! \brief computes the title
	 virtual std::string Title() const = 0;

//! \brief clears the screen, sets a title and the hotkey flag
        void InitOsd (const bool hashotkeys=true);

//! \brief display OSD and go to osd()->newposition
        void Display ();

//! \brief BuildOsd() should be abstract but then we cannot compile
        virtual void BuildOsd ()
        {
        }

/*! \brief Process() should be abstract but then we cannot compile.
 * \return Process may decide that we want another screen to be displayed.
 * If the mgMenu* returned is not "this", the caller will use the return
 * value for a new display. If NULL is returned, the caller will display
 * the previous menu.
 */
        virtual eOSState Process (eKeys Key);

//! \brief the ID of the action defined by the red button.
        mgActions TreeRedAction;
        mgActions CollRedAction;

//! \brief the ID of the action defined by the green button.
        mgActions TreeGreenAction;
        mgActions CollGreenAction;

//! \brief the action defined by the yellow button.
        mgActions TreeYellowAction;
        mgActions CollYellowAction;

//! \brief the action defined by the blue button.
        mgActions TreeBlueAction;
        mgActions CollBlueAction;
};

//! \brief an mgMenu class for navigating through the data base
class mgTree:public mgMenu
{
    public:

	mgTree();

	//! \brief computes the title
	std::string Title() const;

	bool UpdateIncrementalSearch( eKeys key );

	void TerminateIncrementalSearch( bool remain_on_current );

    protected:
        void BuildOsd ();

 private:
	
	void UpdateSearchPosition();

	mgIncrementalSearch *m_incsearch;

	std::string m_filter;

	int m_start_position;
};

//! \brief an mgMenu class for submenus
class mgSubmenu:public mgMenu
{
    public:
	mgSubmenu();
//! \brief computes the title
	std::string Title() const;
    protected:
        void BuildOsd ();
};

//! \brief an mgMenu class for selecting a selection
class mgMenuOrders:public mgMenu
{
    public:
//! \brief computes the title
	std::string Title() const;
    protected:
        void BuildOsd ();
};

class mgMenuOrder : public mgMenu
{
    public:
        mgMenuOrder();
        ~mgMenuOrder();
//! \brief computes the title
	std::string Title() const;
	bool ChangeSelection(eKeys key);
	void SaveSelection();
    protected:
        void BuildOsd ();
    private:
	void AddKeyActions(mgMenu *m,mgSelection *o);
	mgSelection * m_orgselection;
	mgSelection * m_selection;
	int m_orderbycount;
	vector<int> m_keytypes;
	vector < vector <const char*> > m_keynames;
};

//! \brief an mgMenu class for selecting a collection
class mgTreeCollSelector:public mgMenu
{
    public:
        mgTreeCollSelector();
        ~mgTreeCollSelector();
//! \brief computes the title
	std::string Title() const;
    protected:
        void BuildOsd ();
	virtual mgActions coll_action() = 0;
	std::string m_title;
};

class mgTreeAddToCollSelector:public mgTreeCollSelector
{
    public:
//! \brief computes the title
	mgTreeAddToCollSelector(std::string title);
    protected:
	virtual mgActions coll_action() { return actAddCollEntry; }
};

//! \brief an mgMenu class for selecting a collection
class mgTreeRemoveFromCollSelector:public mgTreeCollSelector
{
    public:
//! \brief computes the title
	mgTreeRemoveFromCollSelector(std::string title);
    protected:
	virtual mgActions coll_action() { return actRemoveCollEntry; }
};

mgSelection* GenerateSelection(const mgSelection *s=0);

#endif
