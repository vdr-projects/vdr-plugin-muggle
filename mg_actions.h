/*!
 * \file   mg_actions.h
 * \brief  Implements all actions for broswing media libraries within VDR
 *
 * \version $Revision: 1.13 $
 * \date    $Date: 2004-12-25 16:52:35 +0100 (Sat, 25 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr61 $
 *
 *  $Id: mg_actions.h 276 2004-12-25 15:52:35Z wr61 $
 */

#ifndef _MG_ACTIONS_H
#define _MG_ACTIONS_H

#include <string>

#include <osd.h>
#include <plugin.h>
#include "i18n.h"

using namespace std;

class mgSelection;
class mgMenu;
class mgMainMenu;

/*! \brief defines all actions which can appear in command submenus.
 * Since these values are saved in muggle.state, new actions should
 * always be appended. The order does not matter. Value 0 means undefined.
 */
enum mgActions {
	actChooseOrder=1, 	//!< show a menu with all possible orders
	actToggleSelection,	//!< toggle between search and collection view
	actClearCollection,	//!< clear a collection,
	actCreateCollection,
	actInstantPlay,		//!< instant play
	actAddAllToCollection,	//!< add all items of OSD list to default collection
	actRemoveAllFromCollection,//!< remove from default collection
	actDeleteCollection,	//!< delete collection
	actExportTracklist,	//!< export track list into a *.m3u file
	actAddCollEntry,
	actRemoveCollEntry,
	actAddThisToCollection,	//!< add selected item to default collection
	actRemoveThisFromCollection,	//!< remove selected item from default collection
	actEntry, 		//!< used for normal data base items
	actSetButton,		//!< connect a button with an action
	ActOrderCollItem,	//!< order by collections
	ActOrderArtistAlbumTitle,	//!< order by Artist/Album/Title
	ActOrderArtistTitle,	//!< order by Artist/Title
	ActOrderAlbumTitle,	//!< order by Album/Title
	ActOrderGenreYearTitle,	//!< order by Genre1/Year/Title
	ActOrderGenreArtistAlbumTitle,	//!< order by Genre1/Artist/Album/Title
	actAddAllToDefaultCollection,
	actAddThisToDefaultCollection,
	actSetDefaultCollection,
	actExternal0 = 1000, 		//!< used for external commands, the number is the entry number in the .conf file starting with line 0
	actExternal1, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal2, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal3, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal4, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal5, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal6, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal7, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal8, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal9, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal10, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal11, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal12, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal13, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal14, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal15, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal16, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal17, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal18, 		//!< used for external commands, the number is the entry number in the .conf file
	actExternal19, 		//!< used for external commands, the number is the entry number in the .conf file
};

//! \brief the highest possible actExternal value
const mgActions actExternalHigh = actExternal19;

//! \brief a generic class for the definition of user actions
class mgAction 
{
    public:

	//! \brief if true, can be displayed
	virtual bool Enabled(mgActions on = mgActions(0));

	//! \brief the action to be executed
        virtual void Execute () = 0;

	//! \brief handles the kBack key
	virtual eOSState Back();

/*! \brief the name for a button.
 * The returned C string should be static, it will never be freed.
 */
        virtual const char *ButtonName ()
        {
            return NULL;
        }

/*! \brief the name for a menu entry. If empty, no button will be able
 * to execute this. The returned C string must be freeable at any time.
 * \param value a string that can be used for building the menu name.
 */
        virtual const char *MenuName (const unsigned int idx=0,const string value="") = 0;

	//! \brief default constructor
        mgAction ();

	//! \brief default destructor
        virtual ~ mgAction ();

	//! \brief assoiates the action with the menu which created it
        void SetMenu (mgMenu * const menu);

	//! \brief what to do when mgStatus::OsdCurrentItem is called
	//for this one
	mgActions Type();
	void SetText(const char *text,bool copy=true);
	const char *Text();
	
	void TryNotify();

		/*! \brief vdr calls OsdCurrentItem more often than we
		 * want. This tells mgStatus to ignore the next call 
		 * for a specific item.
		 * \todo is this behaviour intended or a bug in vdr
		 * or in muggle ?
		 */
	bool IgnoreNextEvent;
    protected:

	//! \brief returns the OSD owning the menu owning this item
        mgMainMenu* osd ();

        //! \brief the menu that created this object
	mgMenu * m;

	//! \brief returns the active selection
        mgSelection* selection ();

	//! \brief returns the collection selection
        mgSelection* collselection ();

	//! \brief returns the playselection
        mgSelection* playselection ();

	virtual void Notify();
    private:
	mgMainMenu *m_osd;
};


//! \brief generate an mgAction for action
mgAction* actGenerate(const mgActions action);

#endif
