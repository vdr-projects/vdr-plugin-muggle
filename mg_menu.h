/*!								-*- c++ -*-
 * \file   vdr_menu.h
 * \brief  Implements menu handling for broswing media libraries within VDR
 *
 * \version $Revision: 1.13 $
 * \date    $Date: 2008-03-17 14:58:54 +0100 (Mo, 17 Mär 2008) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author: woro $
 *
 *  $Id: vdr_menu.h 1051 2008-03-17 13:58:54Z woro $
 */

#ifndef _MG_MENU_H
#define _MG_MENU_H

#include <string>
// #include <list>
#include <vector>

#include <vdr/osd.h>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include "vdr_actions.h"

void showmessage(int duration, const char *msg, ...);

class cCommands;

class mgSelection;
class mgMenu;
class mgOsd;
class mgIncrementalSearch;
class mgPlayerControl;
class mgItemGd;

//! \brief callback class, monitors state changes in vdr
class mgStatus : public cStatus
{
	private:
		//! \brief the mgOsd that wants to be notified
		mgOsd *main;
	public:
		//! \brief default constructor
		mgStatus(mgOsd* m) { main = m;}
	protected:
		//! \brief the event we want to know about
		virtual void OsdCurrentItem(const char *Text);
};

/*!
 * \brief the muggle main OSD
 */

class mgOsd : public cOsdMenu
{
	protected:
		bool m_save_warned;
		mgMenu *m_root;
		char *m_message;
		void LoadExternalCommands();
		void showMessage();
	public:

		mgActions CurrentType();

		virtual void SetHelpKeys(const char *Red,const char *Green, const char *Yellow, const char *Blue) { SetHelp(Red,Green,Yellow,Blue); }

		//! \brief callback object, lets vdr notify us about OSD changes
		mgStatus *m_Status;

		//! \brief the different menus, the last one is active
		vector < mgMenu * >Menus;

		//! \brief parent menu if any
		mgMenu * Parent ();

		//! \brief default constructor
		mgOsd ();

		//! \brief default destructor
		virtual ~mgOsd ();

		//! \brief save the entire muggle state
		virtual void SaveState() =0 ;

		//! \brief adds a new mgMenu to the stack
		virtual void AddMenu (mgMenu * m,int position=0);

		//! \brief

		//! \brief initializes using values from nv
		void InitMapFromSetup (mgValmap& nv);

		//! \brief main entry point, called from vdr
		virtual eOSState ProcessKey (eKeys Key);

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
		// only be shown at the end of the next mgOsd::ProcessKey
		// because that might do forcerefresh which overwrites the message
		void Message (const char *msg) { m_message = strdup(msg); }
		const char* Message1 (const char *msg, ...)
			__attribute__ ((format (printf, 2, 3)));

		const char* Message1 (const char *msg, const string &arg);

		//! \brief Actions can request a new position. -1 means none wanted
		int newposition;

		//! \brief clears the screen, sets a title and the hotkey flag
		void InitOsd (std::string title,const bool hashotkeys);

#if VDRVERSNUM >= 10307
		//! \brief expose the protected DisplayMenu() from cOsdMenu
		cSkinDisplayMenu *DisplayMenu(void) { return cOsdMenu::DisplayMenu(); }
#endif

		//! \brief expose the protected cOsdMenu::hk()
		const char *hk (const char *s) { return cOsdMenu::hk (s); }

		void AddItem(mgAction *a);

		void AddText(const char* text,bool selectable=true);

		void AddLongText(const char* text);

		void AddFile(const string filename);

		void CloseMenu();

		void RefreshTitle();

		virtual void SetHotkeyAction(eKeys key,mgActions action);
};

//! \brief a generic muggle menu
class mgMenu
{
	private:
		const char *HKey(const mgActions act,mgActions on=mgActions(0));
	protected:
		mgOsd*  m_osd;
		unsigned int m_prevpos;
		virtual mgActions ButtonAction(eKeys key);

		eOSState ExecuteButton(eKeys key);
		//! \brief adds the wanted action to the OSD menu
		// \param hotkey if true, add this as a hotkey
		void AddAction(const mgActions action, mgActions on = mgActions(0),const bool hotkey=true);

		//! \brief add an external action, always with hotkey
		void AddExternalAction(const mgActions action, const char *title);

		//! \brief the name of the blue button depends of where we are
		int m_parent_index;
		string m_parent_name;
	public:
		//! sets the correct help keys.
		void SetHelpKeys(mgActions on = mgActions(0));
		//! \brief generates an object for the wanted action
		mgAction* GenerateAction(const mgActions action,mgActions on);

		//! \brief executes the wanted action
		eOSState ExecuteAction (const mgActions action, const mgActions on);

		//! \brief sets the pointer to the owning mgOsd
		virtual void setosd (mgOsd* osd);

		void setParentIndex(int idx) { m_parent_index = idx; }
		int getParentIndex() { return m_parent_index; }
		void setParentName(std::string name) { m_parent_name = name; }
		string getParentName() { return m_parent_name; }

		//! \brief the pointer to the owning mgOsd
		mgOsd* osd () const { return m_osd; }

		mgMenu ();

		virtual ~ mgMenu () { }

		//! \brief computes the title
		virtual string Title() const = 0;

		//! \brief clears the screen, sets a title and the hotkey flag
		virtual void InitOsd (const bool hashotkeys=true);

		//! \brief display OSD and go to osd()->newposition
		void Display ();

		//! \brief BuildOsd() should be abstract but then we cannot compile
		virtual void BuildOsd () {
		}

		/*! \brief Process() should be abstract but then we cannot compile.
		 * \return Process may decide that we want another screen to be displayed.
		 * If the mgMenu* returned is not "this", the caller will use the return
		 * value for a new display. If NULL is returned, the caller will display
		 * the previous menu.
		 */
		virtual eOSState Process (eKeys Key);

		//! \brief the ID of the action defined by the red button.
		mgActions RedAction;

		//! \brief the ID of the action defined by the green button.
		mgActions GreenAction;

		//! \brief the action defined by the yellow button.
		mgActions YellowAction;

		//! \brief the action defined by the blue button.
		mgActions BlueAction;

		virtual void SetHotkeyAction(eKeys key,mgActions action);

		const mgItemGd * PlayingItem(void ) const;
		mgItemGd * mutPlayingItem(void );
};
#endif
