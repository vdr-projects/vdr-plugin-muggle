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
class mgOsdItem;

/*! \brief defines all actions which can appear in command submenus.
 * Since these values are saved in muggle.state, new actions should
 * always be appended. The order does not matter. Value 0 means undefined.
 */
enum mgActions {
	actChooseSearch=1, 	//!< show a menu with all possible search schemas
	actToggleSelection,	//!< toggle between search and collection view
	actSetDefault,	   	//!< set default collection
	actUnused1,
	actInstantPlay,		//!< instant play
	actAddAllToCollection,	//!< add all items of OSD list to default collection
	actRemoveAllFromCollection,//!< remove from default collection
	actDeleteCollection,	//!< delete collection
	actExportTracklist,	//!< export track list into a *.m3u file
	actUnused2,
	actUnused3,
	actAddThisToCollection,	//!< add selected item to default collection
	actRemoveThisFromCollection,	//!< remove selected item from default collection
	actEntry, 		//!< used for normal data base items
	actSetButton,		//!< connect a button with an action
	actSearchCollItem,	//!< search in the collections
	actSearchArtistAlbumTitle,	//!< search by Artist/Album/Title
	actSearchArtistTitle,	//!< search by Artist/Title
	actSearchAlbumTitle,	//!< search by Album/Title
	actSearchGenreYearTitle,	//!< search by Genre1/Year/Title
	actSearchGenreArtistAlbumTitle,	//!< search by Genre1/Artist/Album/Title
	actExternal0, 		//!< used for external commands, the number is the entry number in the .conf file starting with line 0
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
	virtual bool Enabled();

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
 * \todo use virtual Set() instead? Compare definition of mgMenu::AddAction
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
	virtual void Notify();
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
};

//! \brief a generic class for all actions that can be used in the
// command submenu
class mgOsdItem : public mgAction, public cOsdItem
{
    public:
	//! \brief the enum mgActions value of this class
	virtual mgActions Type();

	//! \brief to be executed for kBack. We might want to call this
	// directly, so expose it.
	virtual eOSState Back();
};

//! \brief generate a mgOsdItem for action
mgOsdItem* actGenerate(const mgActions action);


/*! \brief create collection directly in the collection list
 */
class mgCreateCollection:public cMenuEditStrItem, public mgAction
{
    public:
	mgCreateCollection();
	void Notify();
	bool Enabled();
	eOSState ProcessKey(eKeys key);
        void Execute ();
        const char *MenuName (const unsigned int idx=0,const string value="");
     private:
	bool Editing();
	char value[30];
};
#endif