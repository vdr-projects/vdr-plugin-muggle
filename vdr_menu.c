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
	if (i == IgnoreNextEventOn)
	{
		IgnoreNextEventOn = NULL;
		return;
	}
	mgAction * a = dynamic_cast<mgAction *>(i);
	if (!a)
		mgError("mgStatus::OsdCurrentItem expected an mgAction*");
	a->Notify();
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
mgMainMenu::DefaultCollectionEntered()
{
    if (!UsingCollection) return false;
    if (selection()->level()==0) return false;
    string collection = trim(selection ()->getKeyValue(0));
    return (collection == default_collection);
}


mgMenu *
mgMainMenu::Parent ()
{
    if (Menus.size () < 2)
        return NULL;
    return Menus[Menus.size () - 2];
}


mgOsdItem* 
mgMenu::GenerateAction(const mgActions action)
{
	mgOsdItem *result = actGenerate(action);
	if (result)
	{
		result->SetMenu(this);
		if (!result->Enabled())
		{
			delete result;
			result=NULL;
		}
	}
	return result;
}

void
mgMenu::ExecuteAction(const mgActions action)
{
    mgAction *a = GenerateAction (action);
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
    queue_playing=false;
    instant_playing=false;
    play_collection = tr("play");
    mgValmap nsel("tree");
    mgValmap ncol("collection");
    mgValmap nmain("MainMenu");

    // define defaults for values missing in state file:
    ncol.put("Keys.0.Choice","Collection");
    ncol.put("Keys.1.Choice","Collection item");
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
    m_playsel.setKey(0,"Collection");
    m_playsel.setKey(1,"Collection item");
    m_playsel.enter(play_collection);
    UseNormalSelection ();
    unsigned int posi = selection()->gotoPosition();
    mgMenu *root = new mgTree;
    root->TreeRedAction = mgActions(nmain.getuint("TreeRedAction"));
    root->TreeGreenAction = mgActions(nmain.getuint("TreeGreenAction"));
    root->TreeYellowAction = mgActions(nmain.getuint("TreeYellowAction"));
    root->CollRedAction = mgActions(nmain.getuint("CollRedAction"));
    root->CollGreenAction = mgActions(nmain.getuint("CollGreenAction"));
    root->CollYellowAction = mgActions(nmain.getuint("CollYellowAction"));
    AddMenu (root);

    SetCurrent (Get (posi));

// Read commands for collections in etc. /video/muggle/playlist_commands.conf
    external_commands = new cCommands ();

    char *
        cmd_file = (char *) AddDirectory (cPlugin::ConfigDirectory ("muggle"),
        "playlist_commands.conf");
    mgDebug (1, "mgMuggle::Start: Looking for file %s", cmd_file);
    bool have_cmd_file = external_commands->Load ((const char *) cmd_file);

    if (!have_cmd_file)
    {
        delete external_commands;
        external_commands = NULL;
    }
    forcerefresh = false;
}

mgMainMenu::~mgMainMenu()
{
	delete m_Status;
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
mgMenu::AddAction (const mgActions action, const bool hotkey)
{
    mgOsdItem *a = GenerateAction(action);
    if (!a) return;
    const char *mn = a->MenuName();
    if (strlen(mn)==0)
	    mgError("AddAction(%d):MenuName is empty",int(action));
    if (hotkey)
    	a->SetText(osd()->hk(mn));
    else
    	a->SetText(mn);
    free(const_cast<char*>(mn));
    osd()->Add(a);
}


void
mgMenu::AddExternalAction(const mgActions action, const char *title)
{
    mgOsdItem *a = GenerateAction(action);
    if (!a) return;
    a->SetText(osd()->hk(title));
    osd()->Add(a);
}

void
mgMenu::AddSelectionItems ()
{
    for (unsigned int i = 0; i < selection()->values.size (); i++)
    {
    	mgOsdItem *a = GenerateAction(actEntry);
	if (!a) continue;
        a->SetText(a->MenuName(i+1,selection()->values[i]),false);
        osd()->Add(a);
    }
    if (osd()->ShowingCollections ())
    {
    mgCreateCollection *a = new mgCreateCollection;
    if (!a) return;
    a->SetMenu(this);
    if (!a->Enabled())
	{
		delete a;
		a=NULL;
	}
    if (!a) return;
    a->SetText(a->MenuName(),false);
    osd()->Add(a);
    }
}


const char *
mgMenu::BlueName ()
{
    if (typeid (*this) == typeid (mgTree))
	    return tr("Commands");
    else
        return tr ("List");
}

void
mgMenu::SetHelpKeys()
{
    const char *Red = NULL;
    const char *Green = NULL;
    const char *Yellow = NULL;
    mgOsdItem *a;
    if (osd()->UsingCollection)
    {
    	if ((a = GenerateAction (CollRedAction)))
        	Red = a->ButtonName ();
    	if ((a = GenerateAction (CollGreenAction)))
        	Green = a->ButtonName ();
    	if ((a = GenerateAction (CollYellowAction)))
        	Yellow = a->ButtonName ();
    }
    else 
    {
    	if ((a = GenerateAction (TreeRedAction)))
        	Red = a->ButtonName ();
    	if ((a = GenerateAction (TreeGreenAction)))
        	Green = a->ButtonName ();
    	if ((a = GenerateAction (TreeYellowAction)))
        	Yellow = a->ButtonName ();
    }
    osd()->SetHelpKeys(Red,Green,Yellow,BlueName());
}


void
mgMenu::InitOsd (const char *title,const bool hashotkeys)
{
    osd ()->InitOsd (title,hashotkeys);
}


void
mgMainMenu::InitOsd (const char *title,const bool hashotkeys)
{
    Clear ();
    SetTitle (title);
    if (hashotkeys) SetHasHotkeys ();
}


void
mgSubmenu::BuildOsd ()
{
    static char b[100];
    snprintf(b,99,tr("Commands:%s"),trim(osd()->selection()->getCurrentValue()).c_str());
    InitOsd (b);
    mgMenu *p = osd ()->Parent ();
    if (!p)
	    return;
    AddAction(actInstantPlay);
    AddAction(actAddThisToCollection);
    AddAction(actRemoveThisFromCollection);
    AddAction(actToggleSelection);
    AddAction(actSetDefault);
    AddAction(actDeleteCollection);
    AddAction(actChooseSearch);
    AddAction(actExportTracklist);
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


eOSState
mgTree::Process (eKeys key)
{
    eOSState result = osUnknown;
    switch (key)
    {
        case kRed:
            	if (osd()->UsingCollection)
			ExecuteAction (CollRedAction);
		else
			ExecuteAction (TreeRedAction);
		return osContinue;
        case kGreen:
            	if (osd()->UsingCollection)
			ExecuteAction (CollGreenAction);
		else
			ExecuteAction (TreeGreenAction);
		return osContinue;
        case kYellow:
            	if (osd()->UsingCollection)
			ExecuteAction (CollYellowAction);
		else
			ExecuteAction (TreeYellowAction);
		return osContinue;
        default:
	    result = osUnknown;
            break;
    }
    return result;
}

void
mgTree::BuildOsd ()
{
    InitOsd (selection ()->getListname ().c_str (), false);
    AddSelectionItems ();
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
    if (key!=kNone)
    	mgDebug (3, "MainMenu::ProcessKey(%d)", (int) key);

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
// catch osBack for empty OSD lists 
// (because if the list was empty, no mgOsdItem::ProcessKey was ever called)
    if (result == osBack)
    {
	    // do as if there was an entry
	    mgOsdItem *a = Menus.back()->GenerateAction(actEntry);
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
            Menus.pop_back ();
            forcerefresh = true;
        }
        else
	{
            result = osBack;	// game over
	    goto pr_exit;
	}
    }
    else if (newmenu != Menus.back ())
    {
        AddMenu (newmenu);
    }

    if (UsingCollection)
    	forcerefresh |= m_collectionsel.cacheIsEmpty();
    else
    	forcerefresh |= m_treesel.cacheIsEmpty();

    forcerefresh |= (newposition>=0);

    if (forcerefresh)
    {
	mgDebug(2,"forced refresh");
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
mgMainMenu::showMessage()
{
    if (m_message)
    {
#if VDRVERSNUM >= 10307
    	Skins.Message (mtInfo, m_message);
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
mgMainMenu::AddMenu (mgMenu * m)
{
    Menus.push_back (m);
    m->setosd (this);
    m->Display (0);
}


eOSState
mgSubmenu::Process (eKeys key)
{
    return osUnknown;
}


void
mgTreeViewSelector::BuildOsd ()
{
    InitOsd (tr ("Tree View Selection"));
    AddAction(actSearchCollItem);
    AddAction(actSearchArtistAlbumTitle);
    AddAction(actSearchArtistTitle);
    AddAction(actSearchAlbumTitle);
    AddAction(actSearchGenreYearTitle);
    AddAction(actSearchGenreArtistAlbumTitle);
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
    DisplayMenu()->SetTabs(25);
}


void
mgMenu::Display (const unsigned int position)
{
    BuildOsd ();
    osd ()->DisplayGoto (position);
}

