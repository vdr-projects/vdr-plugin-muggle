/*!
 * \file   mg_actions.c
 * \brief  Implements all actions for browsing media libraries within VDR
 *
 * \version $Revision: 1.27 $ * \date    $Date: 2004-12-25 16:52:35 +0100 (Sat, 25 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr61 $
 *
 * $Id: mg_actions.c 276 2004-12-25 15:52:35Z wr61 $
 */

#include <typeinfo>
#include <string>
#include <vector>

#include <menuitems.h>
#include <tools.h>
#include <plugin.h>

#include "vdr_setup.h"
#include "mg_actions.h"
#include "vdr_menu.h"
#include "i18n.h"
#include <vdr/interface.h>

#define DEBUG
#include "mg_tools.h"

void
mgAction::Notify()
{
	m->SetHelpKeys();
}

void
mgAction::SetMenu(mgMenu *menu)
{
	m = menu;
}

eOSState
mgAction::Back()
{
	return osUnknown;
}

bool
mgAction::Enabled()
{
	return true;
}

mgAction::mgAction()
{
	m = NULL;
}

mgAction::~mgAction()
{
}


//! \brief used for normal data base items
class mgEntry : public mgOsdItem
{
	public:
		void Notify();
		bool Enabled() { return true;}
		const char *ButtonName() { return ""; }
        	const char *MenuName (const unsigned int idx,const string value);
		eOSState ProcessKey(eKeys key);
		void Execute();
		eOSState Back();
};

class mgCommand : public mgOsdItem
{
	public:
		virtual eOSState ProcessKey(eKeys key);
		void Execute();
};


mgSelection*
mgAction::playselection ()
{
	return m->playselection ();
}
mgMainMenu*
mgAction::osd ()
{
	return m->osd ();
}

eOSState
mgOsdItem::Back()
{
	osd()->newmenu = NULL;
	return osContinue;
}


void
mgEntry::Notify()
{
	selection()->setPosition(osd()->Current());
	selection()->gotoPosition();
	osd()->SaveState();
	mgAction::Notify();
}

const char *
mgEntry::MenuName(const unsigned int idx,const string value)
{
	char *result;
	if (selection()->isCollectionlist())
	{
		if (value == osd()->default_collection)
			asprintf(&result,"-> %s",value.c_str());
        	else
			asprintf(&result,"     %s",value.c_str());
	}
	else if (selection()->inCollection())
		asprintf(&result,"%4d %s",idx,value.c_str());
	else
		result = strdup(value.c_str());
	return result;
}

void
mgEntry::Execute()
{
	if (selection ()->enter ())
	{
		osd()->forcerefresh = true;
		osd()->m_Status->IgnoreNextEventOn = this;
	}
	else
	{
		m->ExecuteAction(actInstantPlay);
	}
}

eOSState
mgEntry::ProcessKey(eKeys key)
{
	if (key!=kNone)
		mgDebug(3,"mgEntry(%s):ProcessKey(%d)",Text(),(int)key);

	switch (key) {
	case kOk:
		Execute();
		return osContinue;
	case kBack:
		osd()->m_Status->IgnoreNextEventOn = this;
		return Back();
        case kBlue:
		osd ()->newmenu = new mgSubmenu;
		return osContinue;
	default:
		return osUnknown;
	}
}


eOSState
mgEntry::Back()
{
	osd()->forcerefresh = true;
	if (!selection ()->leave ())
		osd()->newmenu = NULL;
	return osContinue;
}

eOSState
mgCommand::ProcessKey(eKeys key)
{
    if (key!=kNone)
    	mgDebug(3,"mgCommand::ProcessKey(%d)",(int)key);
    mgMenu *parent = osd ()->Parent ();
    mgMenu *n = osd ()->newmenu;
    osd ()->newmenu = NULL;
    eOSState result = osContinue;
    switch (key)
    {
        case kRed:
            if (osd()->UsingCollection)
		    parent->CollRedAction = Type();
	    else
		    parent->TreeRedAction = Type();
            break;
        case kGreen:
            if (osd()->UsingCollection)
		    parent->CollGreenAction = Type();
	    else
		    parent->TreeGreenAction = Type();
            break;
        case kYellow:
            if (osd()->UsingCollection)
		    parent->CollYellowAction = Type();
	    else
		    parent->TreeYellowAction = Type();
            break;
        case kBlue:
        case kBack:
            break;
        case kOk:
            Execute ();
            break;
        default:
            osd ()->newmenu = n;               // wrong key: stay in submenu
	    result = osUnknown;
            break;
    }
    return result;
}

void
mgCommand::Execute()
{
}

class mgExternal : public mgCommand
{
	public:
		const char *ButtonName();
        	const char *MenuName (const unsigned int idx,const string value);
		void Execute();
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
mgExternal::ButtonName()
{
    cCommand *command = Command();
    if (command)
    {
	    return command->Title();
    }
    else
	    return "";
}

const char*
mgExternal::MenuName(const unsigned int idx,const string value)
{
	return strdup(ButtonName());
}

cCommand *
mgExternal::Command()
{
    cCommand *command = NULL;
    if (osd()->external_commands)
    {
        unsigned int idx = Type() - actExternal0;
	command = osd()->external_commands->Get (idx);
    }
    return command;
}

void
mgExternal::Execute()
{
    cCommand *command = Command();
    if (command)
    {
	    mgDebug(1,"external command:%s",command->Title());
        bool confirmed = true;
        if (command->Confirm ())
        {
            char *buffer;
            asprintf (&buffer, "%s?", command->Title ());
            confirmed = Interface->Confirm (buffer);
            free (buffer);
        }
        if (confirmed)
        {
            osd()->Message1 ("%s...", command->Title ());
            selection ()->select ();
            string m3u_file = selection ()->exportM3U ();
            selection ()->leave ();
            if (!m3u_file.empty ())
            {
                /*char *result = (char *)*/
	        command->Execute (m3u_file.c_str ());
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
}

//! \brief select search order
class mgChooseSearch : public mgCommand
{
    public:
	bool Enabled();
	eOSState ProcessKey(eKeys key);
        void Execute ();
        const char *ButtonName() { return tr("Search"); }
        const char *MenuName(const unsigned int idx,const string value)
	{ return strdup(tr("Select search order")); }
};

bool
mgChooseSearch::Enabled()
{
	bool result = mgOsdItem::Enabled();
	result &= (!selection()->isCollectionlist());
	return result;
}

eOSState
mgChooseSearch::ProcessKey(eKeys key)
{
	if (key!=kNone)
    mgDebug(3,"mgChooseSearch::ProcessKey(%d)",int(key));
	if (key == kOk)
	{
		osd()->Menus.pop_back();
		Execute();
		return osContinue;
	}
	else
		return mgCommand::ProcessKey(key);
}

void mgChooseSearch::Execute() 
{
        osd ()->newmenu = new mgTreeViewSelector;

}

//! \brief toggles between the normal and the collection selection
class mgToggleSelection:public mgCommand
{
    public:
        void Execute ();
        const char *ButtonName ();
        const char *MenuName (const unsigned int idx,const string value);
};

const char *
mgToggleSelection::ButtonName ()
{
    if (osd ()->UsingCollection)
        return tr ("Search");
    else
        return tr ("Collections");
}


const char *
mgToggleSelection::MenuName (const unsigned int idx,const string value)
{
    if (osd ()->UsingCollection)
        return strdup(tr ("Search"));
    else
        return strdup(tr ("Collections"));
}


void
mgToggleSelection::Execute ()
{
    if (osd ()->UsingCollection)
        osd ()->UseNormalSelection ();
    else
    {
        osd ()->UseCollectionSelection ();
	selection()->clearCache();
    }
    osd()->newposition = selection ()->gotoPosition ();
}


//! \brief sets the default collection selection
class mgSetDefault:public mgCommand
{
    public:
	bool Enabled();
        void Execute ();
        const char *ButtonName ()
        {
            return tr ("Default");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

const char * mgSetDefault::MenuName(const unsigned int idx,const string value)
{
    char *b;
    asprintf (&b, tr("Set default to collection '%s'"),
	    selection ()->getCurrentValue().c_str());
    return b;
}

class mgSetButton : public mgCommand
{
	const char *ButtonName()
	{
		return tr("Set");
	}
	const char *MenuName(const unsigned int idx,const string value) { return strdup(""); }
};


bool
mgSetDefault::Enabled()
{
	bool result = mgOsdItem::Enabled();
	result &= (!osd()->DefaultCollectionSelected());
	result &= osd()->UsingCollection;
        result &= (selection ()->level () == 0);
	return result;
}

void
mgSetDefault::Execute ()
{
    if (!Enabled())
	    mgError("mgSetDefault not enabled");
    osd ()->default_collection = selection ()->getCurrentValue();
    osd()->Message1 ("Default collection now is '%s'",
            osd ()->default_collection);
}


//! \brief instant play
class mgInstantPlay : public mgCommand {
    public:
        void Execute ();
        const char *ButtonName ()
        {
            return tr ("Instant play");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

const char *
mgInstantPlay::MenuName (const unsigned int idx,const string value)
{
    return strdup(tr("Instant play"));
}

void
mgInstantPlay::Execute()
{
    osd()->PlayInstant(true);
}

//! \brief add selected items to default collection
class mgAddAllToCollection:public mgCommand {
    public:
        void Execute ();
	//! \brief adds the whole selection to a collection
	// \param collection the target collection. Default is the default collection
        void ExecuteSelection (mgSelection *s,const string collection="");
        const char *ButtonName ()
        {
            return tr ("Add");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

const char *
mgAddAllToCollection::MenuName (const unsigned int idx,const string value)
{
    char *b;
    asprintf (&b, tr ("Add all to '%s'"),
        osd ()->default_collection.c_str ());
    return b;
}

void
mgAddAllToCollection::Execute()
{
	ExecuteSelection(selection());
}

void
mgAddAllToCollection::ExecuteSelection (mgSelection *s, const string collection)
{
    string target = collection;
    if (target.empty()) 
	target = osd()->default_collection;
    if (target == osd()->play_collection)
        if (!PlayerControl())
	   collselection()->ClearCollection(target);

    osd()->Message1 ("Added %s entries",itos (s->AddToCollection (target)));

    if (target == osd()->play_collection)
    {
	playselection()->clearCache();
        mgPlayerControl *c = PlayerControl();
        if (c)
	    c->ReloadPlaylist();
	else
	    osd()->PlayQueue();
    }
}

//! \brief add selected items to default collection
class mgAddThisToCollection:public mgAddAllToCollection
{
    public:
	bool Enabled();
        void Execute ();
        const char *ButtonName ();
        const char *MenuName (const unsigned int idx,const string value);
};


void
mgAddThisToCollection::Execute ()
{
// work on a copy, so we don't have to clear the cache of selection()
// which would result in an osd()->forcerefresh which could scroll.
    mgSelection *s = new mgSelection(selection());
    s->select ();
    mgAddAllToCollection::ExecuteSelection(s);
    delete s;
}

class mgSearchNew : public mgCommand
{
	virtual void NewSearch() = 0;
	void Execute();
};

void
mgSearchNew::Execute()
{
	mgSelection *oldsel = osd()->selection();
 	map<string,string>* oldkeys = oldsel->UsedKeyValues();
	osd()->UseNormalSelection();	// Default for all searches
	NewSearch();
	mgSelection *newsel = osd()->selection();
	for (unsigned int idx = 0; idx < newsel->size(); idx++)
	{
		string new_keyname = newsel->getKeyChoice(idx);
		if (oldkeys->count(new_keyname)==0) break;
		newsel->select((*oldkeys)[new_keyname]);
	}
	newsel->leave();
	delete oldkeys;
}

class mgSearchCollItem : public mgSearchNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Collection -> Item")); }
	void NewSearch() { osd ()->UseCollectionSelection (); }
};


const char *
mgAddThisToCollection::ButtonName ()
{
    return tr("Add");
}

bool
mgAddThisToCollection::Enabled()
{
	bool result = mgOsdItem::Enabled();
	result &= (!osd()->DefaultCollectionSelected());
	return result;
}

const char *
mgAddThisToCollection::MenuName (const unsigned int idx,const string value)
{
    char *b;
    asprintf (&b, tr ("Add to '%s'"),
       	osd ()->default_collection.c_str ());
    return b;
}

//! \brief remove selected items from default collection
class mgRemoveAllFromCollection:public mgCommand
{
    public:
        void Execute ();
        const char *ButtonName ()
        {
            return tr ("Remove");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

void
mgRemoveAllFromCollection::Execute ()
{
    osd()->Message1 ("Removed %s entries",
        itos (collselection ()->RemoveFromCollection (
			osd ()->default_collection.c_str ())));
    if (osd()->default_collection == osd()->play_collection)
    {
        mgPlayerControl *c = PlayerControl();
        if (c)
	    c->ReloadPlaylist();
    }
}


const char *
mgRemoveAllFromCollection::MenuName (const unsigned int idx,const string value)
{
    char *b;
    asprintf (&b, tr ("Remove all from '%s'"),
        osd ()->default_collection.c_str ());
    return b;
}

//! \brief remove selected items from default collection
class mgRemoveThisFromCollection:public mgRemoveAllFromCollection
{
    public:
        void Execute ();
        const char *ButtonName ()
        {
            return tr ("Remove");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

void
mgRemoveThisFromCollection::Execute ()
{
    if (osd()->DefaultCollectionSelected()) 
    {
	selection()->ClearCollection(selection()->getCurrentValue().c_str());
    	osd()->Message ("Removed all entries");
    }
    else
    {
        selection ()->select ();
    	osd()->Message1 ("Removed %s entries",
        	itos (selection ()->RemoveFromCollection (osd ()->default_collection.c_str ())));
    	selection ()->leave ();
    	if (osd()->DefaultCollectionEntered())
    	{
	    selection()->clearCache();
	    osd()->forcerefresh = true;
    	}
    }
}


const char *
mgRemoveThisFromCollection::MenuName (const unsigned int idx,const string value)
{
    char *b;
    string this_sel = selection()->getCurrentValue();
    if (osd()->DefaultCollectionSelected()) 
	asprintf(&b,tr("Remove all entries from '%s'"),this_sel.c_str());
    else
        asprintf (&b, tr ("Remove from '%s'"),
           trim(osd ()->default_collection).c_str ());
    return b;
}

bool
mgCreateCollection::Editing()
{
	return (strchr(cOsdItem::Text(),'[') && strchr(cOsdItem::Text(),']'));
}


void
mgCreateCollection::Notify()
{
	if (!Editing())
		osd()->SetHelpKeys(NULL,NULL,NULL,NULL);
}

const char*
mgCreateCollection::MenuName(const unsigned int idx,const string value)
{
            return strdup(tr ("Create collection"));
}

mgCreateCollection::mgCreateCollection() : cMenuEditStrItem(MenuName(),value,30,tr(FileNameChars)), mgAction()
{
	value[0]=0;
}

eOSState
mgCreateCollection::ProcessKey(eKeys key)
{
	if (key == kOk)
		if (Editing())
			Execute();
		else
			return cMenuEditStrItem::ProcessKey(kRight);
	if (key != kYellow || Editing()) 
		return cMenuEditStrItem::ProcessKey(key);
	else
		return osUnknown;
}

bool
mgCreateCollection::Enabled()
{
	bool result = mgAction::Enabled();
	result &= selection()->isCollectionlist();
	return result;
}

void
mgCreateCollection::Execute ()
{
        string name = trim(value); 
	if (name.empty()) return;
        bool created = selection ()->CreateCollection (name);
        if (created)
	{
  	    mgDebug(1,"created collection %s",name.c_str());
	    osd()->default_collection = name;
	    selection ()->clearCache();
	    osd()->forcerefresh = true;
	}
	else
            osd()->Message1 ("Collection '%s' NOT created", name);
}

//! \brief delete collection
class mgDeleteCollection:public mgCommand
{
    public:
        void Execute ();
	bool Enabled();
        const char *ButtonName ()
        {
            return tr ("Delete");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

bool
mgDeleteCollection::Enabled()
{
	bool result = mgOsdItem::Enabled();
	result &= selection()->isCollectionlist();
	result &= (!osd()->DefaultCollectionSelected());
	if (result)
	{
		string name = selection ()->getCurrentValue();
		result &= (name != osd()->play_collection);
	}
	return result;
}

const char* mgDeleteCollection::MenuName(const unsigned int idx,const string value)
{
    return strdup(tr("Delete collection"));
}


void
mgDeleteCollection::Execute ()
{
	string name = selection ()->getCurrentValue();
        if (selection ()->DeleteCollection (name))
	{
            osd()->Message1 ("Collection '%s' deleted", name);
  	    mgDebug(1,"Deleted collection %s",name.c_str());
	    selection ()->clearCache();
	    osd()->forcerefresh = true;
	}
	else
            osd()->Message1 ("Collection '%s' NOT deleted", name);
}


//! \brief export track list for all selected items
class mgExportTracklist:public mgCommand
{
    public:
        void Execute ();
        const char *ButtonName ()
        {
            return tr ("Export");
        }
        const char *MenuName (const unsigned int idx,const string value)
        {
            return strdup(tr ("Export track list"));
        }
};

void
mgExportTracklist::Execute ()
{
    selection ()->select ();
    string m3u_file = selection ()->exportM3U ();
    selection ()->leave ();
    osd()->Message1 ("written to %s", m3u_file);
}

class mgSearchArtistAlbumTitle : public mgSearchNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Artist -> Album -> Title")); }
	void NewSearch()
	{ 
            selection ()->setKey (0, tr ("Artist"));
            selection ()->setKey (1, tr ("Album"));
            selection ()->setKey (2, tr ("Title"));
	}
};


class mgSearchAlbumTitle : public mgSearchNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Album -> Title")); }
	void NewSearch()
	{ 
            selection ()->setKey (0, tr ("Album"));
            selection ()->setKey (1, tr ("Title"));
	}
};


class mgSearchGenreYearTitle : public mgSearchNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Genre 1 -> Year -> Title")); }
	void NewSearch()
	{ 
            selection ()->setKey (0, tr ("Genre 1"));
            selection ()->setKey (1, tr ("Year"));
            selection ()->setKey (2, tr ("Title"));
	}
};


class mgSearchGenreArtistAlbumTitle : public mgSearchNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Genre 1 -> Artist ->Album -> Title")); }
	void NewSearch()
	{ 
            selection ()->setKey (0, tr ("Genre 1"));
            selection ()->setKey (1, tr ("Artist"));
            selection ()->setKey (2, tr ("Album"));
            selection ()->setKey (3, tr ("Title"));
	}
};


class mgSearchArtistTitle : public mgSearchNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Artist -> Title")); }
	void NewSearch()
	{ 
            selection ()->setKey (0, tr ("Artist"));
            selection ()->setKey (1, tr ("Title"));
	}
};


mgActions
mgOsdItem::Type()
{
	const type_info& t = typeid(*this);
	if (t == typeid(mgChooseSearch)) return actChooseSearch;
	if (t == typeid(mgToggleSelection)) return actToggleSelection;
	if (t == typeid(mgSetDefault)) return actSetDefault;
	if (t == typeid(mgInstantPlay)) return actInstantPlay;
	if (t == typeid(mgAddAllToCollection)) return actAddAllToCollection;
	if (t == typeid(mgRemoveAllFromCollection)) return actRemoveAllFromCollection;
	if (t == typeid(mgDeleteCollection)) return actDeleteCollection;
	if (t == typeid(mgExportTracklist)) return actExportTracklist;
	if (t == typeid(mgAddThisToCollection)) return actAddThisToCollection;
	if (t == typeid(mgRemoveThisFromCollection)) return actRemoveThisFromCollection;
	if (t == typeid(mgEntry)) return actEntry;
	if (t == typeid(mgSetButton)) return actSetButton;
	if (t == typeid(mgSearchCollItem)) return actSearchCollItem;
	if (t == typeid(mgSearchArtistAlbumTitle)) return actSearchArtistAlbumTitle;
	if (t == typeid(mgSearchArtistTitle)) return actSearchArtistTitle;
	if (t == typeid(mgSearchAlbumTitle)) return actSearchAlbumTitle;
	if (t == typeid(mgSearchGenreYearTitle)) return actSearchGenreYearTitle;
	if (t == typeid(mgSearchGenreArtistAlbumTitle)) return actSearchArtistAlbumTitle;
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
	mgError("Unknown mgOsdItem %s",t.name());
	return mgActions(0);
}

mgOsdItem*
actGenerate(const mgActions action)
{
	mgOsdItem * result = NULL;
	switch (action)
	{
		case actChooseSearch: 		result = new mgChooseSearch;break;
		case actToggleSelection: 	result = new mgToggleSelection;break;
		case actSetDefault:		result = new mgSetDefault;break;
		case actUnused1:		break;
		case actInstantPlay:		result = new mgInstantPlay;break;
		case actAddAllToCollection:	result = new mgAddAllToCollection;break;
		case actRemoveAllFromCollection:result = new mgRemoveAllFromCollection;break;
		case actDeleteCollection:	result = new mgDeleteCollection;break;
		case actExportTracklist:	result = new mgExportTracklist;break;
		case actUnused2:		break;
		case actUnused3:		break;
		case actAddThisToCollection:	result = new mgAddThisToCollection;break;
		case actRemoveThisFromCollection: result = new mgRemoveThisFromCollection;break;
		case actEntry: result = new mgEntry;break;
		case actSetButton: result = new mgSetButton;break;
		case actSearchCollItem: result = new mgSearchCollItem;break;
		case actSearchArtistAlbumTitle: result = new mgSearchArtistAlbumTitle;break;
		case actSearchArtistTitle: result = new mgSearchArtistTitle;break;
		case actSearchAlbumTitle: result = new mgSearchAlbumTitle;break;
		case actSearchGenreYearTitle: result = new mgSearchGenreYearTitle;break;
		case actSearchGenreArtistAlbumTitle: result = new mgSearchGenreArtistAlbumTitle;break;
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
mgAction::selection()
{
	return osd()->selection();
}


mgSelection *
mgAction::collselection()
{
	return osd()->collselection();
}

