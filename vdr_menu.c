/*!
 * \file   vdr_menu.c
 * \brief  Implements menu handling for browsing media libraries within VDR
 *
 * \version $Revision: 1.27 $ * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author$
 *
 * $Id$
 */

#include <typeinfo>
#include <string>
#include <vector>

#include <menuitems.h>
#include <tools.h>
#include <config.h>
#include <plugin.h>

#if VDRVERSNUM >= 10307
#include <vdr/interface.h>
#include <vdr/skins.h>
#endif

#include "vdr_setup.h"
#include "vdr_menu.h"
#include "vdr_player.h"
#include "i18n.h"

#define DEBUG
#include "mg_tools.h"

#include <config.h>
#if VDRVERSNUM >= 10307
#include <vdr/interface.h>
#include <vdr/skins.h>
#endif

void
mgStatus::OsdCurrentItem(const char* Text)
{
	cOsdItem* i = main->Get(main->Current());
	if (!i) return;
	mgAction * a = dynamic_cast<mgAction *>(i);
	if (!a)
		mgError("mgStatus::OsdCurrentItem expected an mgAction*");
	a->TryNotify();
}

void Play(mgSelection *sel,const bool select) {
	mgSelection *s = new mgSelection(sel);
	if (select) s->select();
	if (s->empty()) 
	{
		delete s;
		return;
	}
	mgPlayerControl *c = PlayerControl ();
	if (c)
		c->NewPlaylist (s);
	else
		cControl::Launch (new mgPlayerControl (s));
}


//! \brief queue the selection for playing, abort ongoing instant play
void
mgMainMenu::PlayQueue()
{
	queue_playing=true;
	instant_playing=false;
	Play(playselection());
}

//! \brief queue the selection for playing, abort ongoing queue playing
void
mgMainMenu::PlayInstant(const bool select)
{
	instant_playing=true;
	Play(selection(),select);
}

void
mgMainMenu::CollectionChanged(string name)
{
    delete moveselection;
    moveselection = NULL;
    forcerefresh = true; // TODO brauchen wir das?
    if (name == play_collection)
    {
	playselection()->clearCache();
	mgPlayerControl *c = PlayerControl();
	if (c)
	   c->ReloadPlaylist();
	else
	   PlayQueue();
    }
    if (CollectionEntered(name))
    {
       selection()->clearCache();
    }
}

bool
mgMainMenu::ShowingCollections()
{
    return (UsingCollection && selection ()->level () == 0);
}


bool
mgMainMenu::DefaultCollectionSelected()
{
    string this_sel = trim(selection ()->getCurrentValue());
    return (ShowingCollections () && this_sel == default_collection);
}

bool
mgMainMenu::CollectionEntered(string name)
{
    if (!UsingCollection) return false;
    if (selection()->level()==0) return false;
    string collection = trim(selection ()->getKeyValue(0));
    return (collection == name);
}


mgMenu *
mgMainMenu::Parent ()
{
    if (Menus.size () < 2)
        return NULL;
    return Menus[Menus.size () - 2];
}


mgAction* 
mgMenu::GenerateAction(const mgActions action,mgActions on)
{
	mgAction *result = actGenerate(action);
	if (result)
	{
		result->SetMenu(this);
		if (!result->Enabled(on))
		{
			delete result;
			result=NULL;
		}
	}
	return result;
}

void
mgMenu::ExecuteAction(const mgActions action,mgActions on)
{
    mgAction *a = GenerateAction (action,on);
    if (a)
    {
    	a->Execute ();
    	delete a;
    }
}


mgPlayerControl *
PlayerControl ()
{
    mgPlayerControl *result = NULL;
    cControl *control = cControl::Control ();
    if (control && typeid (*control) == typeid (mgPlayerControl))
// is there a running MP3 player?
        result = static_cast < mgPlayerControl * >(control);
    return result;
}


mgMenu::mgMenu ()
{
    m_osd = NULL;
    TreeRedAction = mgActions(0);
    TreeGreenAction = mgActions(0);
    TreeYellowAction = mgActions(0);
    CollRedAction = mgActions(0);
    CollGreenAction = mgActions(0);
    CollYellowAction = mgActions(0);
}


// ----------------------- mgMainMenu ----------------------


void mgMainMenu::SaveState()
{
    char *b;
    asprintf(&b,"%s/muggle.state",cPlugin::ConfigDirectory ("muggle"));
    FILE *f = fopen(b,"w");
    free(b);
    if (!f) return;
    mgValmap nmain("MainMenu");
    nmain.put("DefaultCollection",default_collection);
    nmain.put("UsingCollection",UsingCollection);
    nmain.put("TreeRedAction",int(Menus.front()->TreeRedAction));
    nmain.put("TreeGreenAction",int(Menus.front()->TreeGreenAction));
    nmain.put("TreeYellowAction",int(Menus.front()->TreeYellowAction));
    nmain.put("CollRedAction",int(Menus.front()->CollRedAction));
    nmain.put("CollGreenAction",int(Menus.front()->CollGreenAction));
    nmain.put("CollYellowAction",int(Menus.front()->CollYellowAction));
    mgValmap nsel("tree");
    m_treesel.DumpState(nsel);
    mgValmap ncol("collection");
    m_collectionsel.DumpState(ncol);
    nmain.Write(f);
    nsel.Write(f);
    ncol.Write(f);
    fclose(f);
}

mgMainMenu::mgMainMenu ():cOsdMenu ("")
{
    m_Status = new mgStatus(this);
    m_message = NULL;
    moveselection = NULL;
    external_commands = NULL;
    queue_playing=false;
    instant_playing=false;
    play_collection = tr("play");
    mgValmap nsel("tree");
    mgValmap ncol("collection");
    mgValmap nmain("MainMenu");

    // define defaults for values missing in state file:
    nsel.put("Keys.0.Choice",keyArtist);
    nsel.put("Keys.1.Choice",keyAlbum);
    nsel.put("Keys.2.Choice",keyTitle);
    ncol.put("Keys.0.Choice",keyCollection);
    ncol.put("Keys.1.Choice",keyCollectionItem);
    nmain.put("DefaultCollection",play_collection);
    nmain.put("UsingCollection","false");
    nmain.put("TreeRedAction",int(actAddThisToCollection));
    nmain.put("TreeGreenAction",int(actInstantPlay));
    nmain.put("TreeYellowAction",int(actToggleSelection));
    nmain.put("CollRedAction",int(actAddThisToCollection));
    nmain.put("CollGreenAction",int(actInstantPlay));
    nmain.put("CollYellowAction",int(actToggleSelection));

    // load values from state file
    char *b;
    asprintf(&b,"%s/muggle.state",cPlugin::ConfigDirectory ("muggle"));
    FILE *f = fopen(b,"r");
    free(b);
    if (f) {
	    nsel.Read(f);
	    ncol.Read(f);
	    nmain.Read(f);
	    fclose(f);
    }

    // get values from mgValmaps
    default_collection = nmain.getstr("DefaultCollection");
    UsingCollection = nmain.getbool("UsingCollection");
    InitMapFromSetup(nsel);
    m_treesel.InitFrom (nsel);
    InitMapFromSetup(ncol);
    m_collectionsel.InitFrom (ncol);
    m_playsel.InitFrom(ncol);

    // initialize
    m_collectionsel.CreateCollection(default_collection);
    m_collectionsel.CreateCollection(play_collection);
    m_playsel.setKey(0,keyCollection);
    m_playsel.setKey(1,keyCollectionItem);
    m_playsel.leave(0);
    m_playsel.enter(play_collection);
    UseNormalSelection ();
    unsigned int posi = selection()->gotoPosition();
    LoadExternalCommands();	// before AddMenu()
    mgMenu *root = new mgTree;
    root->TreeRedAction = mgActions(nmain.getuint("TreeRedAction"));
    root->TreeGreenAction = mgActions(nmain.getuint("TreeGreenAction"));
    root->TreeYellowAction = mgActions(nmain.getuint("TreeYellowAction"));
    root->CollRedAction = mgActions(nmain.getuint("CollRedAction"));
    root->CollGreenAction = mgActions(nmain.getuint("CollGreenAction"));
    root->CollYellowAction = mgActions(nmain.getuint("CollYellowAction"));
    AddMenu (root,posi);

    //SetCurrent (Get (posi));

    forcerefresh = false;
}

void
mgMainMenu::LoadExternalCommands()
{
// Read commands for collections in etc. /video/muggle/playlist_commands.conf
    external_commands = new cCommands ();

#if VDRVERSNUM >= 10318
    cString cmd_file = AddDirectory (cPlugin::ConfigDirectory ("muggle"),
        "playlist_commands.conf");
    mgDebug (1, "mgMuggle::Start: 10318 Looking for file %s", *cmd_file);
    bool have_cmd_file = external_commands->Load (*cmd_file);
#else
    const char *
        cmd_file = (const char *) AddDirectory (cPlugin::ConfigDirectory ("muggle"),
        "playlist_commands.conf");
    mgDebug (1, "mgMuggle::Start: 10317 Looking for file %s", cmd_file);
    bool have_cmd_file = external_commands->Load ((const char *) cmd_file);
#endif

    if (!have_cmd_file)
    {
        delete external_commands;
        external_commands = NULL;
    }
}

mgMainMenu::~mgMainMenu()
{
	delete m_Status;
	if (moveselection)
		delete moveselection;
}

void
mgMainMenu::InitMapFromSetup (mgValmap& nv)
{
    // values from setup override saved values
    nv["Host"] = the_setup.DbHost;
    nv["User"] = the_setup.DbUser;
    nv["Password"] = the_setup.DbPass;
    nv["Directory"] = cPlugin::ConfigDirectory ("muggle");
    nv["ToplevelDir"] = the_setup.ToplevelDir;
}

void
mgMenu::AddAction (const mgActions action, mgActions on,const bool hotkey)
{
    mgAction *a = GenerateAction(action,on);
    if (!a) return;
    const char *mn = a->MenuName();
    if (strlen(mn)==0)
	    mgError("AddAction(%d):MenuName is empty",int(action));
    if (hotkey)
    	a->SetText(osd()->hk(mn));
    else
    	a->SetText(mn);
    free(const_cast<char*>(mn));
    osd()->AddItem(a);
}


void
mgMenu::AddExternalAction(const mgActions action, const char *title)
{
    mgAction *a = GenerateAction(action,mgActions(0));
    if (!a) return;
    a->SetText(osd()->hk(title));
    osd()->AddItem(a);
}

void
mgMenu::AddSelectionItems (mgSelection *sel,mgActions act)
{
    for (unsigned int i = 0; i < sel->values.size (); i++)
    {
    	mgAction *a = GenerateAction(act, actEntry);
	if (!a) continue;
        a->SetText(a->MenuName(i+1,sel->values[i]),false);
        osd()->AddItem(a);
    }
    if (osd()->ShowingCollections ())
    {
    	mgAction *a = GenerateAction(actCreateCollection,mgActions(0));
    	if (a) 
	{
    		a->SetText(a->MenuName(),false);
    		osd()->AddItem(a);
	}
    }
}


const char *
mgMenu::BlueName (mgActions on)
{
    // do not use typeid because we want to catch descendants too
    if (dynamic_cast<mgTreeCollSelector*>(this))
        return tr ("List");
    else if (osd()->Current()<0)
	return tr("Commands");
    else if (dynamic_cast<mgTree*>(this))
	return tr("Commands");
    else
        return tr ("List");
}

const char*
mgMenu::HKey(const mgActions act,mgActions on)
{
    const char* result = NULL;
    mgAction *a = GenerateAction(act,on);
    if (a)
    {
        result = a->ButtonName();	
	delete a;
    }
    return result;
}

void
mgMenu::SetHelpKeys(mgActions on)
{
    mgActions r,g,y;
    if (osd()->UsingCollection)
    {
	r = CollRedAction;
	g = CollGreenAction;
	y = CollYellowAction;
    }
    else
    {
	r = TreeRedAction;
	g = TreeGreenAction;
	y = TreeYellowAction;
    }
    osd()->SetHelpKeys(HKey(r,on),
			HKey(g,on),
			HKey(y,on),
                        BlueName(on));
}


void
mgMenu::InitOsd (const char *title,const bool hashotkeys)
{
    osd ()->InitOsd (title,hashotkeys);
    SetHelpKeys();	// Default, will be overridden by the single items
}


void
mgMainMenu::InitOsd (const char *title,const bool hashotkeys)
{
    Clear ();
    SetTitle (title);
    if (hashotkeys) SetHasHotkeys ();
}

void
mgMainMenu::AddItem(mgAction *a)
{
    cOsdItem *c = dynamic_cast<cOsdItem*>(a);
    if (!c)
	    mgError("AddItem with non cOsdItem");
    Add(c);
}

void
mgSubmenu::BuildOsd ()
{
    static char b[100];
    snprintf(b,99,tr("Commands:%s"),trim(osd()->selection()->getCurrentValue()).c_str());
    mgActions on = osd()->CurrentType();
    InitOsd (b);
    mgMenu *p = osd ()->Parent ();
    if (!p)
	    return;
    AddAction(actInstantPlay,on);
    AddAction(actAddThisToCollection,on);
    AddAction(actAddThisToDefaultCollection,on);
    AddAction(actSetDefaultCollection,on);
    AddAction(actRemoveThisFromCollection,on);
    AddAction(actToggleSelection,on);
    AddAction(actDeleteCollection,on);
    AddAction(actClearCollection,on);
    AddAction(actChooseOrder,on);
    AddAction(actExportTracklist,on);
    cCommand *command;
    if (osd()->external_commands)
    {
        int idx=0;
    	while ((command = osd ()->external_commands->Get (idx)) != NULL)
        {
		if (idx>actExternalHigh-actExternal0)
		{
			mgWarning("Too many external commands");
			break;
		}
        	AddExternalAction (mgActions(idx+int(actExternal0)),command->Title());
		idx++;
	}
    }
    TreeRedAction = actSetButton;
    TreeGreenAction = actSetButton;
    TreeYellowAction = actSetButton;
    CollRedAction = actSetButton;
    CollGreenAction = actSetButton;
    CollYellowAction = actSetButton;
}

mgActions
mgMainMenu::CurrentType()
{
    mgActions result = mgActions(0);
    cOsdItem* c = Get(Current());
    if (c)
    {
	mgAction *a = dynamic_cast<mgAction*>(c);
	if (!a)
		mgError("Found an OSD item which is not mgAction:%s",c->Text());
	result = a->Type();
    }
    return result;
}

eOSState
mgMenu::ExecuteButton(eKeys key)
{
    mgActions on = osd()->CurrentType();
    switch (key)
    {
        case kRed:
            	if (osd()->UsingCollection)
			ExecuteAction (CollRedAction,on);
		else
			ExecuteAction (TreeRedAction,on);
		return osContinue;
        case kGreen:
            	if (osd()->UsingCollection)
			ExecuteAction (CollGreenAction,on);
		else
			ExecuteAction (TreeGreenAction,on);
		return osContinue;
        case kYellow:
            	if (osd()->UsingCollection)
			ExecuteAction (CollYellowAction,on);
		else
			ExecuteAction (TreeYellowAction,on);
		return osContinue;
        case kBlue:
		osd()->newmenu = new mgSubmenu;
		return osContinue;
        default:
            break;
    }
    return osUnknown;
}

eOSState
mgTree::Process (eKeys key)
{
    return ExecuteButton(key);
}

void
mgTree::BuildOsd ()
{
    InitOsd (selection ()->getListname ().c_str (), false);
    AddSelectionItems (selection());
}

void
mgMainMenu::Message1(const char *msg, const char *arg1)
{
    if (strlen(msg)==0) return;
    asprintf (&m_message, tr (msg), arg1);
}


eOSState mgMainMenu::ProcessKey (eKeys key)
{
    eOSState result = osContinue;

    if (Menus.size()<1)
	mgError("mgMainMenu::ProcessKey: Menus is empty");
    
    mgPlayerControl * c = PlayerControl ();
    if (c)
    {
        if (!c->Active ())
	{
            	c->Shutdown ();
		if (instant_playing && queue_playing) {
			PlayQueue();
		} 
		else
		{
			instant_playing = false;
			queue_playing = false;
		}
	}
	else
        {
            switch (key)
            {
                case kPause:
                    c->Pause ();
                    break;
                case kStop:
		    if (instant_playing && queue_playing) {
			    PlayQueue();
		    }
		    else
		    {
			    queue_playing = false;
                    	c->Stop ();
		    }
                    break;
                case kChanUp:
                    c->Forward ();
                    break;
                case kChanDn:
                    c->Backward ();
                    break;
                default:
                    goto otherkeys;
            }
            goto pr_exit;
        }
    }
    else
	    if (key==kPlay) { 
		    PlayQueue();
		    goto pr_exit;
	    }
otherkeys:
    newmenu = Menus.back();           // Default: Stay in current menu
    newposition = -1;

    {
	 mgMenu * oldmenu = newmenu;

// item specific key logic:
   	 result = cOsdMenu::ProcessKey (key);

// mgMenu specific key logic:
    	if (result == osUnknown)
        	result = oldmenu->Process (key);
    }
// catch osBack for empty OSD lists . This should only happen for playlistitems
// (because if the list was empty, no mgActions::ProcessKey was ever called)
    if (result == osBack)
    {
	    // do as if there was an entry
	    mgAction *a = Menus.back()->GenerateAction(actEntry,actEntry);
	    if (a) 
	    {
		result = a->Back();
		delete a;
	    }
    }

// do nothing for unknown keys:
    if (result == osUnknown) 
	    goto pr_exit;

// change OSD menu as requested:
    if (newmenu == NULL)
    {
        if (Menus.size () > 1)
        {
   	    CloseMenu();
            forcerefresh = true;
        }
        else
	{
            result = osBack;	// game over
	    goto pr_exit;
	}
    }
    else if (newmenu != Menus.back ())
        AddMenu (newmenu,newposition);

    if (UsingCollection)
    	forcerefresh |= m_collectionsel.cacheIsEmpty();
    else
    	forcerefresh |= m_treesel.cacheIsEmpty();

    forcerefresh |= (newposition>=0);

    if (forcerefresh)
    {
    	forcerefresh = false;
	if (newposition<0) 
		newposition = selection()->gotoPosition();
        Menus.back ()->Display (newposition);
    }
pr_exit:
    showMessage();
    return result;
}

void
mgMainMenu::CloseMenu()
{
    mgMenu* m = Menus.back();
    Menus.pop_back ();
    delete m;
}

void
mgMainMenu::showMessage()
{
    if (m_message)
    {
#if VDRVERSNUM >= 10307
    	Skins.Message (mtInfo, m_message,2);
    	Skins.Flush ();
#else
    	Interface->Status (m_message);
    	Interface->Flush ();
#endif
	free(m_message);
	m_message = NULL;
    }
}


void
mgMainMenu::AddMenu (mgMenu * m,unsigned int position)
{
    Menus.push_back (m);
    m->setosd (this);
    m->Display (position);
}

void
mgMenu::setosd(mgMainMenu *osd)
{
    m_osd = osd;
    m_prevUsingCollection = osd->UsingCollection;
}

eOSState
mgSubmenu::Process (eKeys key)
{
    return osUnknown;
}


void
mgMenuOrders::BuildOsd ()
{
    InitOsd (tr ("Select an order"));
    AddAction(ActOrderCollItem);
    AddAction(ActOrderArtistAlbumTitle);
    AddAction(ActOrderArtistTitle);
    AddAction(ActOrderAlbumTitle);
    AddAction(ActOrderGenreYearTitle);
    AddAction(ActOrderGenreArtistAlbumTitle);
}


mgTreeCollSelector::~mgTreeCollSelector()
{
    osd()->UsingCollection = m_prevUsingCollection;
}

void
mgTreeCollSelector::BuildOsd ()
{
    osd()->UsingCollection = true;
    mgSelection *coll = osd()->collselection();
    InitOsd (m_title.c_str());
    coll->leave(0);
    coll->setPosition(osd()->default_collection);
    AddSelectionItems (coll,coll_action());
    osd()->newposition = coll->gotoPosition(0);
    cOsdItem *c = osd()->Get(osd()->newposition);
    mgAction *a = dynamic_cast<mgAction *>(c);
    a->IgnoreNextEvent = true;
}

mgTreeAddToCollSelector::mgTreeAddToCollSelector(string title)
{
    m_title = title;
}

mgTreeRemoveFromCollSelector::mgTreeRemoveFromCollSelector(string title)
{
    m_title = title;
}

void
mgMainMenu::DisplayGoto (unsigned int select)
{
    if (select >= 0)
    {
        SetCurrent (Get (select));
        RefreshCurrent ();
    }
    Display ();
#if VDRVERSNUM >= 10307
    DisplayMenu()->SetTabs(25);
#endif
}


void
mgMenu::Display (const unsigned int position)
{
    BuildOsd ();
    osd ()->DisplayGoto (position);
}

