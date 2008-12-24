/*!
 * \file   vdr_actions.c
 * \brief  Implements all actions for browsing media libraries within VDR
 *
 * \version $Revision: 1.27 $
 * \date    $Date: 2004-12-25 16:52:35 +0100 (Sat, 25 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr61 $
 *
 * $Id: vdr_actions.c 276 2004-12-25 15:52:35Z wr61 $
 */

#include <stdio.h>
#include <libintl.h>

#include <typeinfo>
#include <string>
#include <vector>
#include <assert.h>

#include <vdr/menuitems.h>
#define __STL_CONFIG_H
#include <vdr/tools.h>
#include <vdr/plugin.h>

#include "mg_setup.h"
#include "vdr_actions.h"
#include "vdr_player.h"
#include "lyrics.h"
#include <vdr/interface.h>

#include "mg_tools.h"
#include "mg_thread_sync.h"

static bool
IsEntry(mgActions i) {
	return i == actEntry;
}

void
mgAction::setHandle(unsigned int handle) {
	m_handle = handle;
}

eOSState
mgAction::ProcessKey(eKeys key) {
	eOSState result = Process(key);
	if (result != osUnknown)
		IgnoreNextEvent = true;
	return result;
}

class mgNone: public mgOsdItem
{
	public:
		void Notify() {};
		bool Enabled(mgActions on) { return false; }
		eOSState Process(eKeys key) { return osUnknown; }
};

class mgBack : public mgAction
{
	const char* ButtonName() { return tr("Parent"); }
	bool Execute() { Back(); return true; }
};

class mgBack2 : public mgAction
{
	const char* ButtonName() { return tr("Parent"); }
	bool Execute() { osd()->CloseMenu();Back(); return true; }
};

//! \brief used for normal data base items
class mgEntry : public mgOsdItem
{
	public:
		void Notify();
		bool Enabled(mgActions on) { return IsEntry(on);}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
		eOSState Process(eKeys key);
		bool Execute();
		eOSState Back();
};

class mgKeyItem : public mgAction, public cMenuEditStraItem
{
	public:
		mgKeyItem(const char *Name, int *Value, int NumStrings, const char *const *Strings) : cMenuEditStraItem(Name, Value, NumStrings, Strings) {}
		eOSState ProcessKey(eKeys key) { return mgAction::ProcessKey(key); }
		eOSState Process(eKeys key);
};

class mgBoolItem: public mgAction, public cMenuEditBoolItem
{
	public:
		mgBoolItem(const char *Name,int *Value) : cMenuEditBoolItem(Name, Value) {}
		eOSState ProcessKey(eKeys key) { return mgAction::ProcessKey(key); }
		eOSState Process(eKeys key);
};

class mgDoCollEntry : public mgEntry
{
	public:
		virtual eOSState Process(eKeys key);
};

class mgAddCollEntry : public mgDoCollEntry
{
	public:
		bool Execute();
};

class mgRemoveCollEntry : public mgDoCollEntry
{
	public:
		bool Execute();
};

void
mgAction::TryNotify() {
	if (IgnoreNextEvent)
		IgnoreNextEvent = false;
	else
		Notify();
}

eOSState
mgDoCollEntry::Process(eKeys key) {
	mgMenu *n = Selosd ()->newmenu;
	Selosd ()->newmenu = NULL;
	eOSState result = osContinue;
	switch (key) {
		case kBack:
			break;
		case kOk:
			Execute ();
			break;
		default:
								 // wrong key: stay in submenu
			Selosd ()->newmenu = n;
			result = osUnknown;
			break;
	}
	return result;
}

bool
mgAddCollEntry::Execute() {
	string target = selection()->getCurrentValue();
	Selosd()->default_collection = target;
	if (target == Selosd()->play_collection)
		if (!PlayerControl())
			collselection()->ClearCollection(target);

	osd()->Message1 ("Added %s entries",itos (Selosd()->moveselection->AddToCollection (target)).c_str());
	Selosd()->CollectionChanged(target,true);
	return true;
}

bool
mgRemoveCollEntry::Execute() {
	string target = selection()->getCurrentValue();
	int removed = Selosd()->moveselection->RemoveFromCollection (target);
	osd()->Message1 ("Removed %s entries",ltos(removed).c_str());
	Selosd()->CollectionChanged(target,false);
	return true;
}

void
mgAction::Notify() {
	m->SetHelpKeys(Type());
}

void
mgAction::SetMenu(mgMenu *menu) {
	m = menu;
	m_osd = m->osd();
}

void
mgAction::SetText(const char *text,bool copy) {
	cOsdItem *c = dynamic_cast<cOsdItem*>(this);
	if (!c)
		mgError("mgAction::SetText() on wrong type");
	c->SetText(text,copy);
}

const char *
mgAction::Text() {
	cOsdItem *c = dynamic_cast<cOsdItem*>(this);
	if (!c)
		mgError("mgAction::Text() on wrong type");
	return c->Text();
}

bool
mgAction::Enabled(mgActions on) {
	return true;
}

mgAction::mgAction() {
	m = 0;
	m_osd = 0;
	m_handle = 0;
	IgnoreNextEvent = false;
}

mgAction::~mgAction() {
}

class mgCommand : public mgOsdItem
{
	public:
		bool Enabled(mgActions on);
		virtual eOSState Process(eKeys key);
};

class mgActOrder : public mgOsdItem
{
	public:
		const char* MenuName(const unsigned int idx,const mgListItem* item);
		virtual eOSState Process(eKeys key);
		bool Execute();
};

const char*
mgActOrder::MenuName(const unsigned int idx,const mgListItem* item) {
	return strdup(item->value().c_str());
}

eOSState
mgActOrder::Process(eKeys key) {
	mgMenu *n = osd ()->newmenu;
	osd ()->newmenu = NULL;
	eOSState result = osContinue;
	switch (key) {
		case kBack:
			break;
		case kOk:
			if (Execute ())
				break;
		default:
			osd ()->newmenu = n; // wrong key: stay in submenu
			result = osUnknown;
			break;
	}
	return result;
}

bool
mgActOrder::Execute() {
	return Selosd()->SwitchSelection();
}

class mgPrevTrack : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char* ButtonName() { return tr("Track-"); }
		bool Execute();
};

bool
mgPrevTrack::Execute() {
	mgPlayerControl *c = PlayerControl();
	if (c)
		c->Backward();
	return c;
}

class mgNextTrack : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char* ButtonName() { return tr("Track+"); }
		bool Execute();
};

bool
mgNextTrack::Execute() {
	mgPlayerControl *c = PlayerControl();
	if (c)
		c->Forward();
	return c;
}

class mgToggleShuffle : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char* MenuName(const unsigned int idx,const mgListItem* item);
		bool Execute();
};

const char*
mgToggleShuffle::MenuName(const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Change shuffle mode"));
}

bool
mgToggleShuffle::Execute() {
	mgPlayerControl *c = PlayerControl();
	if (c)
		c->ToggleShuffle();
	return c;
}

class mgShowLyrics : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char* ButtonName() { return tr("Lyrics"); }
		bool Execute();
};

bool
mgShowLyrics::Execute() {
	osd()->newmenu = new mgLyrics;
	osd()->newposition = 0;
	return true;
}

class mgLoadExternalLyrics : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char* ButtonName() { return tr("Load Lyrics"); }
};

class mgSaveExternalLyrics : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char* ButtonName() { return tr("Save Lyrics"); }
};

class mgToggleLoop : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char* MenuName(const unsigned int idx,const mgListItem* item);
		bool Execute();
};

const char*
mgToggleLoop::MenuName(const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Change loop mode"));
}

bool
mgToggleLoop::Execute() {
	mgPlayerControl *c = PlayerControl();
	if (c)
		c->ToggleLoop();
	return c;
}

bool
mgCommand::Enabled(mgActions on) {
	return IsEntry(on);
}

mgSelection*
mgAction::playselection () {
	return Selosd()->playselection();
}

mgOsd*
mgAction::osd () {
	return m_osd;
}

mgSelOsd*
mgAction::Selosd () {
	return dynamic_cast<mgSelOsd *>(m_osd);
}

eOSState
mgAction::Back() {
	osd()->newmenu = NULL;
	return osContinue;
}

void
mgEntry::Notify() {
	selection()->setPosition(m_handle);
	selection()->gotoPosition();
	Selosd()->SaveState();
	mgAction::Notify();			 // only after selection is updated
}

const char *
mgEntry::MenuName(const unsigned int idx,const mgListItem* item) {
	char ct[20];
	unsigned int selcount = item->count();
	if (!selection()->inItems()) {
		char numct[20];
		sprintf(numct,"%u",selcount);
		memset(ct,' ',19);
		if (strlen(numct)<4)
			ct[6-strlen(numct)*2]=0;
		else
			ct[0]=0;
		strcat(ct,numct);
		strcat(ct," ");
		assert(strlen(ct)<20);
	}
	else
		ct[0]=0;
	char *result;
	if (selection()->isCollectionlist()) {
		if (item->value() == Selosd()->default_collection)
			msprintf(&result,"-> %s%s",ct,item->value().c_str());
		else
			msprintf(&result,"     %s%s",ct,item->value().c_str());
	}
	else if (selection()->inCollection())
		msprintf(&result,"%4d %s",idx,item->value().c_str());
	else if (selection()->isLanguagelist())
		msprintf(&result,"%s%s",ct,dgettext("iso_639",item->value().c_str()));
	else
		msprintf(&result,"%s%s",ct,item->value().c_str());
	return result;
}

bool
mgEntry::Execute() {
	if (selection()->inItems())
		m->ExecuteAction(actInstantPlay,Type());
	else {
		selection()->enter();
		osd()->forcerefresh = true;
	}
	return true;
}

eOSState
mgEntry::Process(eKeys key) {
	eOSState result = osUnknown;

								 // und 0 abfangen
	mgTree *menu = dynamic_cast<mgTree*>(m);
	switch (key) {
		case kOk:
		{
			if (menu)
				menu->TerminateIncrementalSearch( true );
			Execute();

			result = osContinue;
		} break;
		case k0...k9:
		{
			if (menu) {
				menu->UpdateIncrementalSearch( key );
				result = osContinue;
			}
		} break;
		case kBack:
		{
			if( menu && menu->UpdateIncrementalSearch( key ) ) {
								 // search is continued
				result = osContinue;
			}
			else {
								 // search is not active at all
				result = Back();
			}
		} break;
		default:
		{
			if( key != kNone ) {
				if (menu)
					menu->TerminateIncrementalSearch( true );
			}
			result = osUnknown;
		}
	}

	return result;
}

eOSState
mgEntry::Back() {
	osd()->forcerefresh = true;
	if (selection()) {
		if (!selection ()->leave ())
			osd()->newmenu = NULL;
	} else
	osd()->newmenu=NULL;
	return osContinue;
}

eOSState
mgCommand::Process(eKeys key) {
	mgMenu *n = osd ()->newmenu;
	osd ()->newmenu = NULL;
	eOSState result = osContinue;
	switch (key) {
		case kRed:
		case kGreen:
		case kYellow:
			if (strlen(ButtonName()))
				osd()->SetHotkeyAction(key,Type());
			else
				result=osUnknown;
			break;
		case kBack:
			break;
		case kOk:
			Execute ();
			break;
		default:
			osd ()->newmenu = n; // wrong key: stay in submenu
			result = osUnknown;
			break;
	}
	return result;
}

class mgExternal : public mgCommand
{
	public:
		const char *ButtonName();
		bool Execute();
		bool Enabled(mgActions on) { return true; }
	private:
		cCommand * Command();
};

class mgExternal0 : public mgExternal { };
class mgExternal1 : public mgExternal { };
class mgExternal2 : public mgExternal { };
class mgExternal3 : public mgExternal { };
class mgExternal4 : public mgExternal { };
class mgExternal5 : public mgExternal { };
class mgExternal6 : public mgExternal { };
class mgExternal7 : public mgExternal { };
class mgExternal8 : public mgExternal { };
class mgExternal9 : public mgExternal { };
class mgExternal10 : public mgExternal { };
class mgExternal11 : public mgExternal { };
class mgExternal12 : public mgExternal { };
class mgExternal13 : public mgExternal { };
class mgExternal14 : public mgExternal { };
class mgExternal15 : public mgExternal { };
class mgExternal16 : public mgExternal { };
class mgExternal17 : public mgExternal { };
class mgExternal18 : public mgExternal { };
class mgExternal19 : public mgExternal { };

const char*
mgExternal::ButtonName() {
	cCommand *command = Command();
	if (command) {
		return command->Title();
	}
	else
		return "";
}

cCommand *
mgExternal::Command() {
	cCommand *command = NULL;
	if (osd()->external_commands) {
		unsigned int idx = Type() - actExternal0;
		command = osd()->external_commands->Get (idx);
	}
	return command;
}

bool
mgExternal::Execute() {
	cCommand *command = Command();
	if (command) {
		bool confirmed = true;
		if (command->Confirm ()) {
			char *buffer;
			msprintf (&buffer, "%s?", command->Title ());
			confirmed = Interface->Confirm (buffer);
			free (buffer);
		}
		if (confirmed) {
			osd()->Message1 ("%s...", command->Title ());
			string m3u_file = selection ()->exportM3U ();
			if (!m3u_file.empty ()) {
				/*char *result = (char *)*/
				string quoted = "'" + m3u_file + "'";
				char prev[1000];
				if (!getcwd(prev,1000))
					mgError("current path too long");
				if (chdir(the_setup.ToplevelDir))
					mgError("cannnot change to directory %s",
						the_setup.ToplevelDir);
				command->Execute (quoted.c_str ());
				chdir(prev);
				selection()->clearCache();
								 // the ext cmd could change the database
				osd()->forcerefresh = true;
				/* What to do? Recode cMenuText (not much)?
								if (result)
								{
							free( result );
										return AddSubMenu( new cMenuText( command->Title(), result ) );
								}
				*/
			}
		}
	}
	return true;
}

//! \brief select search order
class mgChooseOrder : public mgCommand
{
	public:
		bool Enabled(mgActions on=mgActions(0));
		virtual eOSState Process(eKeys key);
		bool Execute ();
		const char *ButtonName() { return tr("Order"); }
		const char *MenuName(const unsigned int idx,const mgListItem* item)
			{ return strdup(tr("Select an order")); }
};

bool
mgChooseOrder::Enabled(mgActions on) {
	bool result = !Selosd()->UsingCollection;
	result &= IsEntry(on);
	return result;
}

eOSState
mgChooseOrder::Process(eKeys key) {
	if (key == kOk) {
		osd()->CloseMenu();
		Execute();
		return osContinue;
	}
	else
		return mgCommand::Process(key);
}

bool
mgChooseOrder::Execute() {
	Selosd ()->newmenu = new mgSelMenuOrders;
	osd ()->newposition = Selosd()->getCurrentSelection();
	return true;

}

class mgEditOrder : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		eOSState Process(eKeys key);
		bool Execute () { osd ()->newmenu = new mgSelMenuOrder; return true; }
		const char *ButtonName() { return tr("Button$Edit"); }
};

eOSState
mgEditOrder::Process(eKeys key) {
	if (key == kOk) {
		Execute();
		return osContinue;
	}
	else
		return mgCommand::Process(key);
}

class mgCreateOrder : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		bool Execute ();
		const char *ButtonName() { return tr("Create"); }
};

bool
mgCreateOrder::Execute() {
	Selosd()->AddSelection();
	Selosd()->SaveState();
	osd()->forcerefresh = true;
	return true;
}

class mgDeleteOrder : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		bool Execute ();
		const char *ButtonName() { return tr("Button$Delete"); }
};

bool
mgDeleteOrder::Execute() {
	Selosd()->DeleteSelection();
	Selosd()->SaveState();
	osd()->forcerefresh = true;
	osd()->newposition = osd()->Current();
	return true;
}

//! \brief show the normal selection list
class mgShowList: public mgOsdItem
{
	public:
		bool Enabled(mgActions) { return true; }
		const char *ButtonName () { return tr("List"); }
		bool Execute() { osd()->newmenu=NULL; return true;}
};

//! \brief show the command submenu
class mgShowCommands: public mgOsdItem
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char *ButtonName () { return tr("Commands"); }
		bool Execute() { osd()->newmenu = new mgSubmenu;return true; }
};

//! \brief toggles between the normal and the collection selection
class mgToggleSelection:public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		bool Execute ();
		const char *ButtonName ();
};

const char *
mgToggleSelection::ButtonName () {
	if (Selosd ()->UsingCollection)
		return tr ("Browse");
	else
		return tr ("Collections");
}

bool
mgToggleSelection::Execute () {
	if (Selosd ()->UsingCollection)
		Selosd ()->UseNormalSelection ();
	else {
		Selosd ()->UseCollectionSelection ();
		selection()->clearCache();
	}
	selection()->Activate();
	osd()->newposition = selection ()->gotoPosition ();
	return true;
}

class mgCmdSync : public mgOsdItem
{
	public:
		bool Enabled(mgActions on) { return true; }
		eOSState ProcessKey(eKeys key);
		const char *ButtonName() { return tr("Synchronize database"); }
};

eOSState
mgCmdSync::ProcessKey(eKeys key) {
	if (key==kOk) {
		extern bool import();
		import();
		return osContinue;
	}
	return osUnknown;
}

//! \brief sets the default collection selection
class mgSetDefaultCollection:public mgCommand
{
	public:
		bool Enabled(mgActions on);
		bool Execute ();
		const char *ButtonName () {
			return tr ("Default");
		}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
};

const char * mgSetDefaultCollection::MenuName(const unsigned int idx,const mgListItem* item) {
	char *b;
	msprintf (&b, tr("Set default to collection '%s'"),
		selection ()->getCurrentValue().c_str());
	return b;
}

bool
mgSetDefaultCollection::Enabled(mgActions on) {
	bool result = IsEntry(on);
	result &= (!Selosd()->DefaultCollectionSelected());
	result &= Selosd()->UsingCollection;
	result &= (selection ()->orderlevel () == 0);
	return result;
}

bool
mgSetDefaultCollection::Execute () {
	Selosd ()->default_collection = selection ()->getCurrentValue();
	osd()->Message1 (tr("Default collection now is '%s'"),
		Selosd ()->default_collection.c_str());
	return true;
}

class mgSetButton : public mgCommand
{
	public:
		bool Enabled(mgActions on) { return true; }
		const char *ButtonName() { return tr("Set"); }
};

//! \brief instant play
class mgInstantPlay : public mgCommand
{
	public:
		bool Execute ();
		const char *ButtonName () { return tr ("Instant play"); }
};

bool
mgInstantPlay::Execute() {
	Selosd()->PlayInstant(true);
	return true;
}

//! \brief add selected items to a collection
class mgAddAllToCollection:public mgCommand
{
	public:
		bool Enabled(mgActions on);
		bool Execute ();
		//! \brief adds the whole selection to a collection
		// \param collection the target collection. Default is the default collection
		const char *ButtonName () {
			return tr ("Add");
		}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
	protected:
		void ExecuteMove();
};

const char *
mgAddAllToCollection::MenuName (const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Add all to a collection"));
}

bool
mgAddAllToCollection::Execute() {
	// work on a copy, so we don't have to clear the cache of selection()
	// which would result in an osd()->forcerefresh which could scroll.
	Selosd()->moveselection = GenerateSelection(selection());
	ExecuteMove();
	return true;
}

void
mgAddAllToCollection::ExecuteMove() {
	if (osd() ->Menus.size()>1)
		osd ()->CloseMenu();	 // TODO Gebastel...
	char *b;
	msprintf(&b,tr("'%s' to collection"),selection()->getCurrentValue().c_str());
	osd ()->newmenu = new mgTreeAddToCollSelector(string(b));
	Selosd ()->collselection()->leave_all();
	osd ()->newposition = Selosd()->collselection()->getPosition();
	free(b);
}

//! \brief add selected items to default collection
class mgAddAllToDefaultCollection:public mgCommand
{
	public:
		bool Enabled(mgActions on);
		bool Execute ();
		const char *MenuName (const unsigned int idx,const mgListItem* item);
		//! \brief adds the whole selection to the default collection
		// \param collection the default collection.
		void ExecuteSelection (mgSelection *s);
};

const char *
mgAddAllToDefaultCollection::MenuName (const unsigned int idx,const mgListItem* item) {
	char *b;
	msprintf (&b, tr ("Add all to '%s'"),
		Selosd ()->default_collection.c_str ());
	return b;
}

bool
mgAddAllToDefaultCollection::Execute() {
	mgSelection *sel = GenerateSelection(selection());
	ExecuteSelection(sel);
	delete sel;
	return true;
}

void
mgAddAllToDefaultCollection::ExecuteSelection (mgSelection *s) {
	string target = Selosd()->default_collection;
	if (target == Selosd()->play_collection)
		if (!PlayerControl())
			collselection()->ClearCollection(target);

	osd()->Message1 ("Added %s entries",itos (s->AddToCollection (target)).c_str());

	if (target == Selosd()->play_collection) {
		playselection()->clearCache();
		mgPlayerControl *c = PlayerControl();
		if (c)
			c->ReloadPlaylist();
		else
			Selosd()->PlayQueue();
	}
}

//! \brief add selected items to a collection
class mgAddThisToCollection:public mgAddAllToCollection
{
	public:
		bool Execute ();
		const char *MenuName (const unsigned int idx,const mgListItem* item);
};

bool
mgAddThisToCollection::Execute () {
	// work on a copy, so we don't have to clear the cache of selection()
	// which would result in an osd()->forcerefresh which could scroll.
	Selosd()->moveselection = GenerateSelection(selection());
	Selosd()->moveselection->enter();
	ExecuteMove();
	return true;
}

bool
mgAddAllToCollection::Enabled(mgActions on) {
	return IsEntry(on);
}

const char *
mgAddThisToCollection::MenuName (const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Add to a collection"));
}

bool
mgAddAllToDefaultCollection::Enabled(mgActions on) {
	bool result = IsEntry(on);
	result &= (!Selosd()->DefaultCollectionSelected());
	return result;
}

//! \brief add selected items to default collection
class mgAddThisToDefaultCollection:public mgAddAllToDefaultCollection
{
	public:
		bool Execute ();
		const char *ButtonName () {
			return tr ("Add");
		}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
};

bool
mgAddThisToDefaultCollection::Execute () {
	// work on a copy, so we don't have to clear the cache of selection()
	// which would result in an osd()->forcerefresh which could scroll.
	mgSelection *sel = GenerateSelection(selection());
	sel->enter ();
	ExecuteSelection(sel);
	delete sel;
	return true;
}

const char *
mgAddThisToDefaultCollection::MenuName (const unsigned int idx,const mgListItem* item) {
	char *b;
	msprintf (&b, tr ("Add to '%s'"), Selosd ()->default_collection.c_str ());
	return b;
}

//! \brief remove selected items from default collection
class mgRemoveAllFromCollection:public mgCommand
{
	public:
		bool Enabled(mgActions on);
		bool Execute ();
		const char *MenuName (const unsigned int idx,const mgListItem* item);
	protected:
		void ExecuteRemove();
};

bool
mgRemoveAllFromCollection::Enabled(mgActions on) {
	return IsEntry(on);
}

bool
mgRemoveAllFromCollection::Execute () {
	Selosd()->moveselection = GenerateSelection(selection());
	ExecuteRemove();
	return true;
}

void
mgRemoveAllFromCollection::ExecuteRemove () {
	if (osd() ->Menus.size()>1)
		osd ()->CloseMenu();	 // TODO Gebastel...
	char *b;
	msprintf(&b,tr("Remove '%s' from collection"),Selosd()->moveselection->getListname().c_str());
	osd ()->newmenu = new mgTreeRemoveFromCollSelector(string(b));
	Selosd ()->collselection()->leave_all();
	osd ()->newposition = Selosd()->collselection()->getPosition();
	free(b);
}

const char *
mgRemoveAllFromCollection::MenuName (const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Remove all from a collection"));
}

class mgClearCollection : public mgCommand
{
	public:
		bool Enabled(mgActions on);
		bool Execute ();
		const char *ButtonName () {
			return tr ("Clear");
		}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
};

const char *
mgClearCollection::MenuName (const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Clear the collection"));
}

bool
mgClearCollection::Enabled(mgActions on) {
	return selection()->isCollectionlist();
}

bool
mgClearCollection::Execute() {
	if (Interface->Confirm(tr("Clear the collection?"))) {
		string target = selection()->getCurrentValue();
		selection()->ClearCollection(target);
		Selosd()->CollectionChanged(target,false);
	}
	return true;
}

//! \brief remove selected items from default collection
class mgRemoveThisFromCollection:public mgRemoveAllFromCollection
{
	public:
		bool Execute ();
		const char *ButtonName () {
			return tr ("Remove");
		}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
};

bool
mgRemoveThisFromCollection::Execute () {
	// work on a copy, so we don't have to clear the cache of selection()
	// which would result in an osd()->forcerefresh which could scroll.
	Selosd()->moveselection = GenerateSelection(selection());
	Selosd()->moveselection->enter();
	ExecuteRemove();
	return true;
}

const char *
mgRemoveThisFromCollection::MenuName (const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Remove from a collection"));
}

class mgEditAction : public mgAction
{
	public:
		mgEditAction() : mgAction() { memset(m_value,0,30); }
	protected:
		char m_value[30];
};

class mgCreate : public mgEditAction, public cMenuEditStrItem
{
	public:
		mgCreate(const char* mn);
		virtual bool Enabled(mgActions on)=0;
		void Notify();
		eOSState ProcessKey(eKeys key) { return mgAction::ProcessKey(key); }
		eOSState Process(eKeys key);
	protected:
		bool Editing();
};

mgCreate::mgCreate(const char *mn) : mgEditAction(), cMenuEditStrItem(mn,m_value,30,tr(FileNameChars)) {
}

bool
mgCreate::Editing() {
	return (strchr(cOsdItem::Text(),'[') && strchr(cOsdItem::Text(),']'));
}

void
mgCreate::Notify() {
	if (!Editing())
		m->SetHelpKeys();
}

eOSState
mgCreate::Process(eKeys key) {
	if (key == kOk) {
		if (Editing())
			Execute();
		else
			return cMenuEditStrItem::ProcessKey(kRight);
	}
	if (key != kYellow || Editing())
		return cMenuEditStrItem::ProcessKey(key);
	else
		return osUnknown;
}

class mgCreateCollection : public mgCreate
{
	public:
		mgCreateCollection();
		bool Enabled(mgActions on);
		bool Execute ();
		const char *MenuName (const unsigned int idx=0,const mgListItem* item=0);
};

mgCreateCollection::mgCreateCollection() : mgCreate(MenuName()) {
}

const char*
mgCreateCollection::MenuName(const unsigned int idx,const mgListItem* item) {
	return strdup(tr ("Create collection"));
}

bool
mgCreateCollection::Execute () {
	string name = trim(m_value);
	if (name.empty()) return false;
	bool created = selection ()->CreateCollection (name);
	if (created) {
		mgDebug(1,"created collection %s",name.c_str());
		Selosd()->default_collection = name;
		selection ()->clearCache();
		if (selection()->isCollectionlist()) {
			selection ()->setPosition(name);
		}
		osd()->forcerefresh = true;
		return true;
	}
	else {
		osd()->Message1 ("Collection '%s' NOT created", name.c_str());
		return false;
	}
}

bool
mgCreateCollection::Enabled(mgActions on) {
	return selection()->isCollectionlist();
}

//! \brief delete collection
class mgDeleteCollection:public mgCommand
{
	public:
		bool Execute ();
		bool Enabled(mgActions on);
		const char *ButtonName () {
			return tr ("Button$Delete");
		}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
};

bool
mgDeleteCollection::Enabled(mgActions on) {
	bool result = IsEntry(on);
	result &= selection()->isCollectionlist();
	if (result) {
		string name = selection ()->getCurrentValue();
		result &= (name != Selosd()->play_collection);
	}
	return result;
}

const char* mgDeleteCollection::MenuName(const unsigned int idx,const mgListItem* item) {
	return strdup(tr("Delete the collection"));
}

bool
mgDeleteCollection::Execute () {
	if (!Interface->Confirm(tr("Delete the collection?"))) return false;
	string name = selection ()->getCurrentValue();
	if (selection ()->DeleteCollection (name)) {
		osd()->Message1 ("Collection '%s' deleted", name.c_str());
		mgDebug(1,"Deleted collection %s",name.c_str());
		selection ()->clearCache();
		osd()->forcerefresh = true;
		return true;
	}
	else {
		osd()->Message1 ("Collection '%s' NOT deleted", name.c_str());
		return false;
	}
}

//! \brief export item list for all selected items
class mgExportItemlist:public mgCommand
{
	public:
		bool Execute ();
		const char *ButtonName () {
			return tr ("Export");
		}
		const char *MenuName (const unsigned int idx,const mgListItem* item) {
			return strdup(tr ("Export track list"));
		}
};

bool
mgExportItemlist::Execute () {
	string m3u_file = selection ()->exportM3U ();
	osd()->Message1 ("written to %s", m3u_file.c_str());
	return true;
}

mgActions
mgAction::Type() {
	const type_info& t = typeid(*this);
	if (t == typeid(mgNone)) return actNone;
	if (t == typeid(mgChooseOrder)) return actChooseOrder;
	if (t == typeid(mgToggleSelection)) return actToggleSelection;
	if (t == typeid(mgClearCollection)) return actClearCollection;
	if (t == typeid(mgCreateCollection)) return actCreateCollection;
	if (t == typeid(mgInstantPlay)) return actInstantPlay;
	if (t == typeid(mgAddAllToCollection)) return actAddAllToCollection;
	if (t == typeid(mgAddAllToDefaultCollection)) return actAddAllToDefaultCollection;
	if (t == typeid(mgRemoveAllFromCollection)) return actRemoveAllFromCollection;
	if (t == typeid(mgDeleteCollection)) return actDeleteCollection;
	if (t == typeid(mgExportItemlist)) return actExportItemlist;
	if (t == typeid(mgAddCollEntry)) return actAddCollEntry;
	if (t == typeid(mgRemoveCollEntry)) return actRemoveCollEntry;
	if (t == typeid(mgAddThisToCollection)) return actAddThisToCollection;
	if (t == typeid(mgAddThisToDefaultCollection)) return actAddThisToDefaultCollection;
	if (t == typeid(mgRemoveThisFromCollection)) return actRemoveThisFromCollection;
	if (t == typeid(mgEntry)) return actEntry;
	if (t == typeid(mgSetButton)) return actSetButton;
	if (t == typeid(mgShowList)) return actShowList;
	if (t == typeid(mgShowCommands)) return actShowCommands;
	if (t == typeid(mgSetDefaultCollection)) return actSetDefaultCollection;
	if (t == typeid(mgActOrder)) return actOrder;
	if (t == typeid(mgToggleShuffle)) return actToggleShuffle;
	if (t == typeid(mgToggleLoop)) return actToggleLoop;
	if (t == typeid(mgCreateOrder)) return actCreateOrder;
	if (t == typeid(mgDeleteOrder)) return actDeleteOrder;
	if (t == typeid(mgEditOrder)) return actEditOrder;
	if (t == typeid(mgCmdSync)) return actSync;
	if (t == typeid(mgShowLyrics)) return actShowLyrics;
	if (t == typeid(mgPrevTrack)) return actPrevTrack;
	if (t == typeid(mgNextTrack)) return actNextTrack;
	if (t == typeid(mgExternal0)) return actExternal0;
	if (t == typeid(mgExternal1)) return actExternal1;
	if (t == typeid(mgExternal2)) return actExternal2;
	if (t == typeid(mgExternal3)) return actExternal3;
	if (t == typeid(mgExternal4)) return actExternal4;
	if (t == typeid(mgExternal5)) return actExternal5;
	if (t == typeid(mgExternal6)) return actExternal6;
	if (t == typeid(mgExternal7)) return actExternal7;
	if (t == typeid(mgExternal8)) return actExternal8;
	if (t == typeid(mgExternal9)) return actExternal9;
	if (t == typeid(mgExternal10)) return actExternal10;
	if (t == typeid(mgExternal11)) return actExternal11;
	if (t == typeid(mgExternal12)) return actExternal12;
	if (t == typeid(mgExternal13)) return actExternal13;
	if (t == typeid(mgExternal14)) return actExternal14;
	if (t == typeid(mgExternal15)) return actExternal15;
	if (t == typeid(mgExternal16)) return actExternal16;
	if (t == typeid(mgExternal17)) return actExternal17;
	if (t == typeid(mgExternal18)) return actExternal18;
	if (t == typeid(mgExternal19)) return actExternal19;
	return mgActions(0);
}

mgAction*
actGenerateKeyItem(const char *Name, int *Value, int NumStrings, const char * const * Strings) {
	return new mgKeyItem(Name,Value,NumStrings,Strings);
}

mgAction*
actGenerateBoolItem(const char *Name, int *Value) {
	return new mgBoolItem(Name,Value);
}

mgAction*
actGenerate(const mgActions action) {
	mgAction * result = NULL;
	switch (action) {
		case actNone:           result = new mgNone;break;
		case actChooseOrder:        result = new mgChooseOrder;break;
		case actToggleSelection:    result = new mgToggleSelection;break;
		case actClearCollection:    result = new mgClearCollection;break;
		case actCreateCollection:   result = new mgCreateCollection;break;
		case actInstantPlay:        result = new mgInstantPlay;break;
		case actAddAllToCollection: result = new mgAddAllToCollection;break;
		case actAddAllToDefaultCollection:  result = new mgAddAllToDefaultCollection;break;
		case actRemoveAllFromCollection:result = new mgRemoveAllFromCollection;break;
		case actDeleteCollection:   result = new mgDeleteCollection;break;
		case actExportItemlist: result = new mgExportItemlist;break;
		case actAddCollEntry:       result = new mgAddCollEntry;break;
		case actRemoveCollEntry:        result = new mgRemoveCollEntry;break;
		case actAddThisToCollection:    result = new mgAddThisToCollection;break;
		case actAddThisToDefaultCollection: result = new mgAddThisToDefaultCollection;break;
		case actRemoveThisFromCollection: result = new mgRemoveThisFromCollection;break;
		case actEntry: result = new mgEntry;break;
		case actSetButton: result = new mgSetButton;break;
		case actShowList:       result = new mgShowList;break;
		case actShowCommands:       result = new mgShowCommands;break;
		case actSync:           result = new mgCmdSync;break;
		case actSetDefaultCollection:   result = new mgSetDefaultCollection;break;
		case actOrder: result = new mgActOrder;break;
		case actToggleShuffle: result = new mgToggleShuffle;break;
		case actToggleLoop: result = new mgToggleLoop;break;
		case actUnused6: break;
		case actCreateOrder: result = new mgCreateOrder;break;
		case actDeleteOrder: result = new mgDeleteOrder;break;
		case actEditOrder: result = new mgEditOrder;break;
		case actPrevTrack: result = new mgPrevTrack;break;
		case actNextTrack: result = new mgNextTrack;break;
		case actShowLyrics: result = new mgShowLyrics;break;
		case actLoadExternalLyrics: result = new mgLoadExternalLyrics;break;
		case actSaveExternalLyrics: result = new mgSaveExternalLyrics;break;
		case actBack: result = new mgBack;break;
		case actBack2: result = new mgBack2;break;
		case actExternal0: result = new mgExternal0;break;
		case actExternal1: result = new mgExternal1;break;
		case actExternal2: result = new mgExternal2;break;
		case actExternal3: result = new mgExternal3;break;
		case actExternal4: result = new mgExternal4;break;
		case actExternal5: result = new mgExternal5;break;
		case actExternal6: result = new mgExternal6;break;
		case actExternal7: result = new mgExternal7;break;
		case actExternal8: result = new mgExternal8;break;
		case actExternal9: result = new mgExternal9;break;
		case actExternal10: result = new mgExternal10;break;
		case actExternal11: result = new mgExternal11;break;
		case actExternal12: result = new mgExternal12;break;
		case actExternal13: result = new mgExternal13;break;
		case actExternal14: result = new mgExternal14;break;
		case actExternal15: result = new mgExternal15;break;
		case actExternal16: result = new mgExternal16;break;
		case actExternal17: result = new mgExternal17;break;
		case actExternal18: result = new mgExternal18;break;
		case actExternal19: result = new mgExternal19;break;
	}
	return result;
}

mgSelection *
mgAction::selection() {
	if (Selosd())
		return Selosd()->selection();
	else
		return 0;
}

mgSelection *
mgAction::collselection() {
	return Selosd()->collselection();
}

eOSState
mgKeyItem::Process(eKeys key) {
	mgSelMenuOrder *menu = dynamic_cast<mgSelMenuOrder*>(m);
	if (key==kOk) {
		if (menu->ChangeSelection(key))
			return osContinue;
		else {
			menu->SaveSelection();
			osd ()->newmenu = NULL;
			return osContinue;
		}
	} else if (key==kBack)
	{
		osd ()->newmenu = NULL;
		return osContinue;
	}
	if (key==kUp || key==kDown)
		if (menu->ChangeSelection(key))
			return osContinue;
	return cMenuEditStraItem::ProcessKey(key);
}

eOSState
mgBoolItem::Process(eKeys key) {
	mgSelMenuOrder *menu = dynamic_cast<mgSelMenuOrder*>(m);
	if (key==kOk) {
		if (menu->ChangeSelection(key))
			return osContinue;
		else {
			menu->SaveSelection();
			osd ()->newmenu = NULL;
			return osContinue;
		}
	} else if (key==kBack)
	{
		osd ()->newmenu = NULL;
		return osContinue;
	}
	if (key==kUp || key==kDown)
		if (menu->ChangeSelection(key))
			return osContinue;
	return cMenuEditBoolItem::ProcessKey(key);
}
