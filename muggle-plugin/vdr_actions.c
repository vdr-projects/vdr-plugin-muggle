/*!
 * \file   vdr_actions.c
 * \brief  Implements all actions for browsing media libraries within VDR
 *
 * \version $Revision: 1.27 $ * \date    $Date: 2004-12-25 16:52:35 +0100 (Sat, 25 Dec 2004) $
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

#include <menuitems.h>
#include <tools.h>
#include <plugin.h>

#include "vdr_setup.h"
#include "vdr_actions.h"
#include "vdr_menu.h"
#include "i18n.h"
#include <vdr/interface.h>

#define DEBUG
#include "mg_tools.h"
#include "mg_order.h"
#include "mg_sync.h"

static bool
IsEntry(mgActions i)
{
	return i == actEntry;
}

class mgOsdItem : public mgAction, public cOsdItem
{
	public:
		eOSState ProcessKey(eKeys key) { return mgAction::ProcessKey(key); }
};


void
mgAction::setHandle(unsigned int handle)
{
	m_handle = handle;
}

eOSState
mgAction::ProcessKey(eKeys key)
{
	if (key!=kNone)
		mgDebug(1,"mgAction::ProcessKey(%d)",key);
    eOSState result = Process(key);    
    if (result != osUnknown)
	    IgnoreNextEvent = true;
    return result;
}

class mgNone: public mgOsdItem
{	public:
		void Notify() {};
		bool Enabled(mgActions on) { return false; }
		eOSState Process(eKeys key) { return osUnknown; }
};

//! \brief used for normal data base items
class mgEntry : public mgOsdItem
{
	public:
		void Notify();
		bool Enabled(mgActions on) { return IsEntry(on);}
        	const char *MenuName (const unsigned int idx,const string value);
		eOSState Process(eKeys key);
		void Execute();
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
    string::size_type lparen = result.find("  [");
    result.erase(lparen,string::npos);
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
    int removed = osd()->moveselection->RemoveFromCollection (target);
    osd()->Message1 ("Removed %s entries",ltos(removed));
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
	m = 0;
	m_osd = 0;
	m_handle = 0;
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
};

class mgActOrder : public mgOsdItem
{
	public:
		const char* MenuName(const unsigned int idx,const string value);
		virtual eOSState Process(eKeys key);
		void Execute();
};

const char*
mgActOrder::MenuName(const unsigned int idx,const string value)
{
	return strdup(value.c_str());
}

eOSState
mgActOrder::Process(eKeys key)
{
    mgMenu *n = osd ()->newmenu;
    osd ()->newmenu = NULL;
    eOSState result = osContinue;
    switch (key)
    {
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
mgActOrder::Execute()
{
 	mgSelection *s = osd()->selection();
	mgOrder oldorder = s->getOrder();
	mgContentItem o;
	s->select();
	if (s->getNumTracks()==1)
		o = s->getTrack(0);
	osd()->UseNormalSelection();	// Default for all orders
	osd()->setOrder(s,osd()->Current());
	mgSelection *newsel = osd()->selection();
	newsel->selectfrom(oldorder,&o);
	osd()->newposition = newsel->getPosition();
	osd()->SaveState();
}

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
	selection()->setPosition(m_handle);
	selection()->gotoPosition();
	osd()->SaveState();
	mgAction::Notify();	// only after selection is updated
}


const char *
mgEntry::MenuName(const unsigned int idx,const string value)
{
	char *result;
	char ct[20];
	ct[0]=0;
	unsigned int selcount = selection()->valcount(value);
	if (selection()->level()<selection()->ordersize()-1 || selcount>1)
		sprintf(ct,"  [%u]",selcount);
	// when changing this, also change mgDoCollEntry::getTarget()
	if (selection()->isCollectionlist())
	{
		if (value == osd()->default_collection)
			asprintf(&result,"-> %s%s",value.c_str(),ct);
        	else
			asprintf(&result,"     %s%s",value.c_str(),ct);
	}
	else if (selection()->inCollection())
		asprintf(&result,"%4d %s%s",idx,value.c_str(),ct);
	else if (selection()->isLanguagelist())
		asprintf(&result,"%s%s",dgettext("iso_639",value.c_str()),ct);
	else
		asprintf(&result,"%s%s",value.c_str(),ct);
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


class mgExternal : public mgCommand
{
	public:
		const char *ButtonName();
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
		osd()->forcerefresh = true; // the ext cmd could change the database
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
	osd ()->newposition = osd()->getCurrentOrder();

}

class mgEditOrder : public mgCommand
{
    public:
	bool Enabled(mgActions on) { return true; }
	eOSState Process(eKeys key);
        void Execute () { osd ()->newmenu = new mgMenuOrder; }
        const char *ButtonName() { return tr("Edit"); }
};

eOSState
mgEditOrder::Process(eKeys key)
{
	if (key == kOk)
	{
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
        void Execute ();
        const char *ButtonName() { return tr("Create"); }
};

void 
mgCreateOrder::Execute()
{
	osd()->AddOrder();
	osd()->SaveState();
	osd()->forcerefresh = true;
}

class mgDeleteOrder : public mgCommand
{
    public:
	bool Enabled(mgActions on) { return true; }
        void Execute ();
        const char *ButtonName() { return tr("Delete"); }
};

void 
mgDeleteOrder::Execute()
{
	osd()->DeleteOrder();
	osd()->SaveState();
	osd()->forcerefresh = true;
	osd()->newposition = osd()->Current();
}

//! \brief show the normal selection list
class mgShowList: public mgOsdItem
{
    public:
	bool Enabled(mgActions) { return true; }
        const char *ButtonName () { return tr("List"); }
	void Execute() { osd()->newmenu=NULL; }
};


//! \brief show the command submenu
class mgShowCommands: public mgOsdItem
{
    public:
	bool Enabled(mgActions on) { return true; }
        const char *ButtonName () { return tr("Commands"); }
	void Execute() { osd()->newmenu = new mgSubmenu; }
};


//! \brief toggles between the normal and the collection selection
class mgToggleSelection:public mgCommand
{
    public:
	bool Enabled(mgActions on) { return true; }
        void Execute ();
        const char *ButtonName ();
};

const char *
mgToggleSelection::ButtonName ()
{
    if (osd ()->UsingCollection)
        return tr ("Browse");
    else
        return tr ("Collections");
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

class mgCmdSync : public mgOsdItem
{
	public:
		bool Enabled(mgActions on) { return true; }
		void Execute();
		eOSState ProcessKey(eKeys key);
		const char *ButtonName() { return tr("Synchronize database"); }
};


static char *sync_args[] =
{
	".",
	0
};

eOSState
mgCmdSync::ProcessKey(eKeys key)
{
	if (key==kOk)
		if (Interface->Confirm(tr("Synchronize database with track flles?")))
	{
		Execute();
		return osContinue;
	}
	return osUnknown;
}

void
mgCmdSync::Execute()
{
	mgSync s;
	s.Sync(sync_args);
}

//! \brief sets the default collection selection
class mgSetDefaultCollection:public mgCommand
{
    public:
	bool Enabled(mgActions on);
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
	const char *ButtonName() { return tr("Set"); }
};


//! \brief instant play
class mgInstantPlay : public mgCommand {
    public:
        void Execute ();
        const char *ButtonName () { return tr ("Instant play"); }
};

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
    osd ()->collselection()->leave_all();
    osd ()->newposition = osd()->collselection()->getPosition(); 
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
	bool Enabled(mgActions on);
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
	bool Enabled(mgActions on);
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
	bool Enabled(mgActions on);
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
	{
		string target = selection()->getCurrentValue();
		selection()->ClearCollection(target);
    		osd()->CollectionChanged(target);
	}
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

mgCreate::mgCreate(const char *mn) : mgEditAction(), cMenuEditStrItem(mn,m_value,30,tr(FileNameChars))
{
}

bool
mgCreate::Editing()
{
	return (strchr(cOsdItem::Text(),'[') && strchr(cOsdItem::Text(),']'));
}

void
mgCreate::Notify()
{
    if (!Editing())
	m->SetHelpKeys();
}

eOSState
mgCreate::Process(eKeys key)
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

class mgCreateCollection : public mgCreate
{
    public:
	mgCreateCollection();
	bool Enabled(mgActions on);
        void Execute ();
        const char *MenuName (const unsigned int idx=0,const string value="");
};

mgCreateCollection::mgCreateCollection() : mgCreate(MenuName())
{
}

const char*
mgCreateCollection::MenuName(const unsigned int idx,const string value)
{
            return strdup(tr ("Create collection"));
}

void
mgCreateCollection::Execute ()
{
        string name = trim(m_value); 
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

bool
mgCreateCollection::Enabled(mgActions on)
{
	return selection()->isCollectionlist();
}


//! \brief delete collection
class mgDeleteCollection:public mgCommand
{
    public:
        void Execute ();
	bool Enabled(mgActions on);
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

mgActions
mgAction::Type()
{
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
	if (t == typeid(mgExportTracklist)) return actExportTracklist;
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
	if (t == typeid(mgCreateOrder)) return actCreateOrder;
	if (t == typeid(mgDeleteOrder)) return actDeleteOrder;
	if (t == typeid(mgEditOrder)) return actEditOrder;
	if (t == typeid(mgCmdSync)) return actSync;
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
actGenerateKeyItem(const char *Name, int *Value, int NumStrings, const char * const * Strings)
{
	return new mgKeyItem(Name,Value,NumStrings,Strings);
}

mgAction*
actGenerateBoolItem(const char *Name, int *Value)
{
	return new mgBoolItem(Name,Value);
}

mgAction*
actGenerate(const mgActions action)
{
	mgAction * result = NULL;
	switch (action)
	{
		case actNone: 			result = new mgNone;break;
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
		case actShowList: 		result = new mgShowList;break;
		case actShowCommands: 		result = new mgShowCommands;break;
		case actSync: 			result = new mgCmdSync;break;
		case actSetDefaultCollection:	result = new mgSetDefaultCollection;break;
		case actOrder: result = new mgActOrder;break;
		case actUnused6: break;
		case actCreateOrder: result = new mgCreateOrder;break;
		case actDeleteOrder: result = new mgDeleteOrder;break;
		case actEditOrder: result = new mgEditOrder;break;
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

eOSState
mgKeyItem::Process(eKeys key)
{
	mgMenuOrder *menu = dynamic_cast<mgMenuOrder*>(m);
	if (key==kOk)
	{
		if (menu->ChangeOrder(key))
			return osContinue;
		else
		{
			menu->SaveOrder();
    			osd ()->newmenu = NULL;
    			return osContinue;
		}
	} else if (key==kBack)
	{
    		osd ()->newmenu = NULL;
    		return osContinue;
	}
	if (key==kUp || key==kDown)
		if (menu->ChangeOrder(key))
			return osContinue;
	return cMenuEditStraItem::ProcessKey(key);
}


eOSState
mgBoolItem::Process(eKeys key)
{
	mgMenuOrder *menu = dynamic_cast<mgMenuOrder*>(m);
	if (key==kOk)
	{
		if (menu->ChangeOrder(key))
			return osContinue;
		else
		{
			menu->SaveOrder();
    			osd ()->newmenu = NULL;
    			return osContinue;
		}
	} else if (key==kBack)
	{
    		osd ()->newmenu = NULL;
    		return osContinue;
	}
	if (key==kUp || key==kDown)
		if (menu->ChangeOrder(key))
			return osContinue;
	return cMenuEditBoolItem::ProcessKey(key);
}

