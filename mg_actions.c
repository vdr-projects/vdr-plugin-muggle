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
#include "mg_order.h"

static bool
IsEntry(mgActions i)
{
	return i == actEntry;
}

class mgOsdItem : public mgAction, public cOsdItem
{
	public:
		eOSState ProcessKey(eKeys key);
	protected:
		virtual eOSState Process(eKeys key) { return osUnknown; }
};

eOSState
mgOsdItem::ProcessKey(eKeys key)
{
    eOSState result = Process(key);    
    if (result != osUnknown)
	    IgnoreNextEvent = true;
    return result;
}

//! \brief used for normal data base items
class mgEntry : public mgOsdItem
{
	public:
		void Notify();
		bool Enabled(mgActions on) { return IsEntry(on);}
		const char *ButtonName() { return ""; }
        	const char *MenuName (const unsigned int idx,const string value);
		virtual eOSState Process(eKeys key);
		void Execute();
		eOSState Back();
};

class mgDoCollEntry : public mgEntry
{
	public:
		virtual eOSState Process(eKeys key);
	protected:
		string getTarget();
};

class mgAddCollEntry : public mgDoCollEntry
{
	public:
		void Execute();
};

class mgRemoveCollEntry : public mgDoCollEntry
{
	public:
		void Execute();
};

void
mgAction::TryNotify()
{
    if (IgnoreNextEvent)
	IgnoreNextEvent = false;
    else
	Notify();
}

eOSState
mgDoCollEntry::Process(eKeys key)
{
    mgMenu *n = osd ()->newmenu;
    osd ()->newmenu = NULL;
    eOSState result = osContinue;
    switch (key)
    {
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

string
mgDoCollEntry::getTarget()
{
    string result = cOsdItem::Text();
    if (result[0]==' ')
	    result.erase(0,5);
    else
	    result.erase(0,3);
    return result;
}

void
mgAddCollEntry::Execute()
{
    string target = getTarget();
    osd()->default_collection = target;
    if (target == osd()->play_collection)
        if (!PlayerControl())
               collselection()->ClearCollection(target);

    osd()->Message1 ("Added %s entries",itos (osd()->moveselection->AddToCollection (target)));
    osd()->CollectionChanged(target);
}


void
mgRemoveCollEntry::Execute()
{
    string target = getTarget();
    osd()->Message1 ("Removed %s entries",itos (osd()->moveselection->RemoveFromCollection (target)));
    osd()->CollectionChanged(target);
}


void
mgAction::Notify()
{
	m->SetHelpKeys(Type());
}

void
mgAction::SetMenu(mgMenu *menu)
{
	m = menu;
	m_osd = m->osd();
}

void
mgAction::SetText(const char *text,bool copy)
{
	cOsdItem *c = dynamic_cast<cOsdItem*>(this);
	if (!c)
		mgError("mgAction::SetText() on wrong type");
	c->SetText(text,copy);
}

const char *
mgAction::Text()
{
	cOsdItem *c = dynamic_cast<cOsdItem*>(this);
	if (!c)
		mgError("mgAction::Text() on wrong type");
	return c->Text();
}


bool
mgAction::Enabled(mgActions on)
{
	return true;
}

mgAction::mgAction()
{
	m = NULL;
	m_osd = NULL;
	IgnoreNextEvent = false;
}

mgAction::~mgAction()
{
}

class mgCommand : public mgOsdItem
{
	public:
		bool Enabled(mgActions on);
		virtual eOSState Process(eKeys key);
		void Execute();
};

bool
mgCommand::Enabled(mgActions on)
{
	return IsEntry(on);
}

mgSelection*
mgAction::playselection ()
{
	return m->playselection ();
}
mgMainMenu*
mgAction::osd ()
{
	return m_osd;
}

eOSState
mgAction::Back()
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
	mgAction::Notify();	// only after selection is updated
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
	}
	else
	{
		m->ExecuteAction(actInstantPlay,Type());
	}
}

eOSState
mgEntry::Process(eKeys key)
{
	switch (key) {
	case kOk:
		Execute();
		return osContinue;
	case kBack:
		return Back();
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
mgCommand::Process(eKeys key)
{
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
class mgChooseOrder : public mgCommand
{
    public:
	bool Enabled(mgActions on=mgActions(0));
	virtual eOSState Process(eKeys key);
        void Execute ();
        const char *ButtonName() { return tr("Order"); }
        const char *MenuName(const unsigned int idx,const string value)
	{ return strdup(tr("Select an order")); }
};

bool
mgChooseOrder::Enabled(mgActions on)
{
	bool result = !osd()->UsingCollection;
	result &= IsEntry(on);
	return result;
}

eOSState
mgChooseOrder::Process(eKeys key)
{
	if (key == kOk)
	{
		osd()->CloseMenu();
		Execute();
		return osContinue;
	}
	else
		return mgCommand::Process(key);
}

void mgChooseOrder::Execute() 
{
        osd ()->newmenu = new mgMenuOrders;

}

//! \brief toggles between the normal and the collection selection
class mgToggleSelection:public mgCommand
{
    public:
	bool Enabled(mgActions on = mgActions(0)) { return true; }
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
class mgSetDefaultCollection:public mgCommand
{
    public:
	bool Enabled(mgActions on = mgActions(0));
        void Execute ();
        const char *ButtonName ()
        {
            return tr ("Default");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

const char * mgSetDefaultCollection::MenuName(const unsigned int idx,const string value)
{
    char *b;
    asprintf (&b, tr("Set default to collection '%s'"),
	    selection ()->getCurrentValue().c_str());
    return b;
}


bool
mgSetDefaultCollection::Enabled(mgActions on)
{
	bool result = IsEntry(on);
	result &= (!osd()->DefaultCollectionSelected());
	result &= osd()->UsingCollection;
        result &= (selection ()->level () == 0);
	return result;
}

void
mgSetDefaultCollection::Execute ()
{
    osd ()->default_collection = selection ()->getCurrentValue();
    osd()->Message1 ("Default collection now is '%s'",
            osd ()->default_collection);
}


class mgSetButton : public mgCommand
{
    public:
	bool Enabled(mgActions on) { return true; }
	const char *ButtonName()
	{
		return tr("Set");
	}
	const char *MenuName(const unsigned int idx,const string value) { return strdup(""); }
};


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

//! \brief add selected items to a collection
class mgAddAllToCollection:public mgCommand {
    public:
        void Execute ();
	//! \brief adds the whole selection to a collection
	// \param collection the target collection. Default is the default collection
        const char *ButtonName ()
        {
            return tr ("Add");
        }
        const char *MenuName (const unsigned int idx,const string value);
    protected:
	void ExecuteMove();
};

const char *
mgAddAllToCollection::MenuName (const unsigned int idx,const string value)
{
    return strdup(tr("Add all to a collection"));
}

void
mgAddAllToCollection::Execute()
{
// work on a copy, so we don't have to clear the cache of selection()
// which would result in an osd()->forcerefresh which could scroll.
    osd()->moveselection = new mgSelection(selection());
    ExecuteMove();
}

void
mgAddAllToCollection::ExecuteMove()
{
    if (osd() ->Menus.size()>1) 
	    osd ()->CloseMenu();	// TODO Gebastel...
    char *b;
    asprintf(&b,tr("'%s' to collection"),selection()->getCurrentValue().c_str());
    osd ()->newmenu = new mgTreeAddToCollSelector(string(b));
    osd ()->newposition = osd()->collselection()->getPosition(0); 
    free(b);
}


//! \brief add selected items to default collection
class mgAddAllToDefaultCollection:public mgCommand {
    public:
        void Execute ();
	//! \brief adds the whole selection to the default collection
	// \param collection the default collection.
        void ExecuteSelection (mgSelection *s);
        const char *ButtonName ()
        {
            return tr ("Add");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

const char *
mgAddAllToDefaultCollection::MenuName (const unsigned int idx,const string value)
{
    char *b;
    asprintf (&b, tr ("Add all to '%s'"),
        osd ()->default_collection.c_str ());
    return b;
}

void
mgAddAllToDefaultCollection::Execute()
{
    mgSelection *sel = new mgSelection(selection());
    sel->select ();
    ExecuteSelection(sel);
    delete sel;
}


void
mgAddAllToDefaultCollection::ExecuteSelection (mgSelection *s)
{
    string target = osd()->default_collection;
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

//! \brief add selected items to a collection
class mgAddThisToCollection:public mgAddAllToCollection
{
    public:
	bool Enabled(mgActions on = mgActions(0));
        void Execute ();
        const char *ButtonName ();
        const char *MenuName (const unsigned int idx,const string value);
};


void
mgAddThisToCollection::Execute ()
{
// work on a copy, so we don't have to clear the cache of selection()
// which would result in an osd()->forcerefresh which could scroll.
    osd()->moveselection = new mgSelection(selection());
    osd()->moveselection->select ();
    mgAddAllToCollection::ExecuteMove();
}

const char *
mgAddThisToCollection::ButtonName ()
{
    return tr("Add");
}

bool
mgAddThisToCollection::Enabled(mgActions on)
{
	return IsEntry(on);
}

const char *
mgAddThisToCollection::MenuName (const unsigned int idx,const string value)
{
    return strdup(tr("Add to a collection"));
}

//! \brief add selected items to default collection
class mgAddThisToDefaultCollection:public mgAddAllToDefaultCollection
{
    public:
	bool Enabled(mgActions on = mgActions(0));
        void Execute ();
        const char *ButtonName ();
        const char *MenuName (const unsigned int idx,const string value);
};


void
mgAddThisToDefaultCollection::Execute ()
{
// work on a copy, so we don't have to clear the cache of selection()
// which would result in an osd()->forcerefresh which could scroll.
    mgSelection *sel = new mgSelection(selection());
    sel->select ();
    mgAddAllToDefaultCollection::ExecuteSelection(sel);
    delete sel;
}

const char *
mgAddThisToDefaultCollection::ButtonName ()
{
    return tr("Add");
}

bool
mgAddThisToDefaultCollection::Enabled(mgActions on)
{
	bool result = IsEntry(on);
	result &= (!osd()->DefaultCollectionSelected());
	return result;
}

const char *
mgAddThisToDefaultCollection::MenuName (const unsigned int idx,const string value)
{
    char *b;
    asprintf (&b, tr ("Add to '%s'"), osd ()->default_collection.c_str ());
    return b;
}

class mgOrderNew : public mgCommand
{
    protected:
	virtual void NewOrder() = 0;
    public:
	bool Enabled(mgActions on = mgActions(0)) { return true; }
	void Execute();
};

void
mgOrderNew::Execute()
{
 	mgSelection *s = osd()->selection();
	map<mgKeyTypes,string>* oldkeys = s->UsedKeyValues();
	mgContentItem o;
	s->select();
	if (s->getNumTracks()==1)
	{
		o = s->getTrack(0);
	}
	osd()->UseNormalSelection();	// Default for all orders
	NewOrder();
	mgSelection *newsel = osd()->selection();
	newsel->leave(0);
	for (unsigned int idx = 0; idx < newsel->ordersize(); idx++)
	{
		string selval = "";
		mgKeyTypes new_kt = newsel->getKeyType(idx);
		if (oldkeys->count(new_kt)>0) 
			selval=(*oldkeys)[new_kt];
		else if (o.getId()>=0)
			selval=o.getKeyValue(new_kt);
		if (!selval.empty())
		{
			if (idx<newsel->ordersize()-1)
				newsel->select(selval);
			else
				newsel->setPosition(selval);
			continue;
		}
		break;
	}
	delete oldkeys;
	osd()->newposition = newsel->getPosition(newsel->level());
	newsel->clearCache();
}

class mgOrderCollItem : public mgOrderNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Collection -> Item")); }
	void NewOrder() { osd ()->UseCollectionSelection (); }
};


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
    if (osd() ->Menus.size()>1) 
	    osd ()->CloseMenu();	// TODO Gebastel...
    char *b;
    asprintf(&b,tr("Remove '%s' from collection"),osd()->moveselection->getListname().c_str());
    osd ()->newmenu = new mgTreeRemoveFromCollSelector(string(b));
    free(b);
}

const char *
mgRemoveAllFromCollection::MenuName (const unsigned int idx,const string value)
{
    return strdup(tr("Remove all from a collection"));
}

class mgClearCollection : public mgCommand
{
    public:
	bool Enabled(mgActions on = mgActions(0));
        void Execute ();
        const char *ButtonName ()
        {
            return tr ("Clear");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

const char *
mgClearCollection::MenuName (const unsigned int idx,const string value)
{
	return strdup(tr("Clear the collection"));
}

bool
mgClearCollection::Enabled(mgActions on)
{
	return selection()->isCollectionlist();
}

void
mgClearCollection::Execute()
{
	if (Interface->Confirm(tr("Clear the collection?")))
			selection()->ClearCollection(selection()->getCurrentValue());
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
// work on a copy, so we don't have to clear the cache of selection()
// which would result in an osd()->forcerefresh which could scroll.
    osd()->moveselection = new mgSelection(selection());
    osd()->moveselection->select ();
    mgRemoveAllFromCollection::Execute();
}


const char *
mgRemoveThisFromCollection::MenuName (const unsigned int idx,const string value)
{
    return strdup(tr("Remove from a collection"));
}

/*! \brief create collection directly in the collection list
 */
class mgCreateCollection: public mgAction, public cMenuEditStrItem
{
    public:
	mgCreateCollection();
	void Notify();
	bool Enabled(mgActions on = mgActions(0));
	eOSState ProcessKey(eKeys key);
        void Execute ();
        const char *MenuName (const unsigned int idx=0,const string value="");
     private:
	bool Editing();
	char value[30];
};


bool
mgCreateCollection::Editing()
{
	return (strchr(cOsdItem::Text(),'[') && strchr(cOsdItem::Text(),']'));
}


void
mgCreateCollection::Notify()
{
    if (!Editing())
	m->SetHelpKeys();
}

const char*
mgCreateCollection::MenuName(const unsigned int idx,const string value)
{
            return strdup(tr ("Create collection"));
}

mgCreateCollection::mgCreateCollection() : mgAction(), cMenuEditStrItem(MenuName(),value,30,tr(FileNameChars))
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
mgCreateCollection::Enabled(mgActions on)
{
	return selection()->isCollectionlist();
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
	    if (selection()->isCollectionlist())
	    {
//	    	selection ()->setPosition(selection()->id(keyCollection,name));
	    	selection ()->setPosition(name);
	    }
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
	bool Enabled(mgActions on = mgActions(0));
        const char *ButtonName ()
        {
            return tr ("Delete");
        }
        const char *MenuName (const unsigned int idx,const string value);
};

bool
mgDeleteCollection::Enabled(mgActions on)
{
	bool result = IsEntry(on);
	result &= selection()->isCollectionlist();
	if (result)
	{
		string name = selection ()->getCurrentValue();
		result &= (name != osd()->play_collection);
	}
	return result;
}

const char* mgDeleteCollection::MenuName(const unsigned int idx,const string value)
{
    return strdup(tr("Delete the collection"));
}


void
mgDeleteCollection::Execute ()
{
	if (!Interface->Confirm(tr("Delete the collection?"))) return;
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

class mgOrderArtistAlbumTitle : public mgOrderNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Artist -> Album -> Track")); }
	void NewOrder()
	{ 
            selection ()->setKey (0, keyArtist);
            selection ()->setKey (1, keyAlbum);
            selection ()->setKey (2, keyTrack);
	}
};


class mgOrderAlbumTitle : public mgOrderNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Album -> Track")); }
	void NewOrder()
	{ 
            selection ()->setKey (0, keyAlbum);
            selection ()->setKey (1, keyTrack);
	}
};


class mgOrderGenreYearTitle : public mgOrderNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Genre 1 -> Year -> Track")); }
	void NewOrder()
	{ 
            selection ()->setKey (0, keyGenre1);
            selection ()->setKey (1, keyYear);
            selection ()->setKey (2, keyTrack);
	}
};


class mgOrderGenreArtistAlbumTitle : public mgOrderNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Genre 1 -> Artist ->Album -> Track")); }
	void NewOrder()
	{ 
            selection ()->setKey (0, keyGenre1);
            selection ()->setKey (1, keyArtist);
            selection ()->setKey (2, keyAlbum);
            selection ()->setKey (3, keyTrack);
	}
};


class mgOrderArtistTitle : public mgOrderNew
{
        const char *MenuName(const unsigned int idx,const string value) { return strdup(tr("Artist -> Track")); }
	void NewOrder()
	{ 
            selection ()->setKey (0, keyArtist);
            selection ()->setKey (1, keyTrack);
	}
};


mgActions
mgAction::Type()
{
	const type_info& t = typeid(*this);
	if (t == typeid(mgChooseOrder)) return actChooseOrder;
	if (t == typeid(mgToggleSelection)) return actToggleSelection;
	if (t == typeid(mgClearCollection)) return actClearCollection;
	if (t == typeid(mgCreateCollection)) return actCreateCollection;
	if (t == typeid(mgInstantPlay)) return actInstantPlay;
	if (t == typeid(mgAddAllToCollection)) return actAddAllToCollection;
	if (t == typeid(mgAddAllToDefaultCollection)) return actAddAllToDefaultCollection;
	if (t == typeid(mgRemoveAllFromCollection)) return actRemoveAllFromCollection;
	if (t == typeid(mgDeleteCollection)) return actDeleteCollection;
	if (t == typeid(mgExportTracklist)) return actExportTracklist;
	if (t == typeid(mgAddCollEntry)) return actAddCollEntry;
	if (t == typeid(mgRemoveCollEntry)) return actRemoveCollEntry;
	if (t == typeid(mgAddThisToCollection)) return actAddThisToCollection;
	if (t == typeid(mgAddThisToDefaultCollection)) return actAddThisToDefaultCollection;
	if (t == typeid(mgRemoveThisFromCollection)) return actRemoveThisFromCollection;
	if (t == typeid(mgEntry)) return actEntry;
	if (t == typeid(mgSetButton)) return actSetButton;
	if (t == typeid(mgOrderCollItem)) return ActOrderCollItem;
	if (t == typeid(mgOrderArtistAlbumTitle)) return ActOrderArtistAlbumTitle;
	if (t == typeid(mgOrderArtistTitle)) return ActOrderArtistTitle;
	if (t == typeid(mgOrderAlbumTitle)) return ActOrderAlbumTitle;
	if (t == typeid(mgOrderGenreYearTitle)) return ActOrderGenreYearTitle;
	if (t == typeid(mgOrderGenreArtistAlbumTitle)) return ActOrderArtistAlbumTitle;
	if (t == typeid(mgSetDefaultCollection)) return actSetDefaultCollection;
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
	mgError("Unknown mgAction %s",t.name());
	return mgActions(0);
}

mgAction*
actGenerate(const mgActions action)
{
	mgAction * result = NULL;
	switch (action)
	{
		case actChooseOrder: 		result = new mgChooseOrder;break;
		case actToggleSelection: 	result = new mgToggleSelection;break;
		case actClearCollection:	result = new mgClearCollection;break;
		case actCreateCollection:	result = new mgCreateCollection;break;
		case actInstantPlay:		result = new mgInstantPlay;break;
		case actAddAllToCollection:	result = new mgAddAllToCollection;break;
		case actAddAllToDefaultCollection:	result = new mgAddAllToDefaultCollection;break;
		case actRemoveAllFromCollection:result = new mgRemoveAllFromCollection;break;
		case actDeleteCollection:	result = new mgDeleteCollection;break;
		case actExportTracklist:	result = new mgExportTracklist;break;
		case actAddCollEntry:		result = new mgAddCollEntry;break;
		case actRemoveCollEntry:		result = new mgRemoveCollEntry;break;
		case actAddThisToCollection:	result = new mgAddThisToCollection;break;
		case actAddThisToDefaultCollection:	result = new mgAddThisToDefaultCollection;break;
		case actRemoveThisFromCollection: result = new mgRemoveThisFromCollection;break;
		case actEntry: result = new mgEntry;break;
		case actSetButton: result = new mgSetButton;break;
		case ActOrderCollItem: result = new mgOrderCollItem;break;
		case ActOrderArtistAlbumTitle: result = new mgOrderArtistAlbumTitle;break;
		case ActOrderArtistTitle: result = new mgOrderArtistTitle;break;
		case ActOrderAlbumTitle: result = new mgOrderAlbumTitle;break;
		case ActOrderGenreYearTitle: result = new mgOrderGenreYearTitle;break;
		case ActOrderGenreArtistAlbumTitle: result = new mgOrderGenreArtistAlbumTitle;break;
		case actSetDefaultCollection:	result = new mgSetDefaultCollection;break;
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

