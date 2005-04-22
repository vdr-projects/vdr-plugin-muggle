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

#include <stdio.h>
#include <assert.h>

#include <typeinfo>
#include <string>
#include <vector>

#include <menuitems.h>
#include <tools.h>
#include <config.h>
#include <plugin.h>

#if VDRVERSNUM >= 10307
#include <interface.h>
#include <skins.h>
#endif

#include "vdr_setup.h"
#include "vdr_menu.h"
#include "vdr_player.h"
#include "mg_incremental_search.h"
#include "mg_thread_sync.h"
#include "i18n.h"

#define DEBUG
#include "mg_tools.h"

void
mgStatus::OsdCurrentItem(const char* Text)
{
	cOsdItem* i = main->Get(main->Current());
	if (!i) return;
	mgAction * a = dynamic_cast<mgAction *>(i);
	if (!a)
		mgError("mgStatus::OsdCurrentItem expected an mgAction*");
	if (a)
		a->TryNotify();
}

void Play(mgSelection *sel,const bool select) {
	mgSelection *s = new mgSelection(sel);
	if (select) s->select();
	s->skipItems(0);	// make sure we start with a valid item
	if (s->empty()) 	// no valid item exists
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
mgMainMenu::ChangeOrder()
{
	mgOrder* o = getOrder(Current());
	if (o->size()>0)
	{
		m_current_order = Current();
		UseNormalSelection();
		m_treesel->setOrder(o);
		newposition = m_treesel->getPosition();
		SaveState();
	}
	else
		mgWarning("mgMainMenu::ChangeOrder: orders[%u] is empty",Current());
}

mgOrder* mgMainMenu::getOrder(unsigned int idx)
{
	if (idx>=orders.size())
		mgError("mgMainMenu::getOrder(%u): orders.size() is %d",
				idx,orders.size());
	return orders[idx];
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
    if (CollectionEntered(name) || selection()->isCollectionlist())
       selection()->clearCache();
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
    return trim(selection ()->getKeyItem(0)->value()) == name;
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

eOSState
mgMenu::ExecuteAction(const mgActions action,mgActions on)
{
    mgAction *a = GenerateAction (action,on);
    if (a)
    {
    	a->Execute ();
    	delete a;
	return osContinue;
    }
    return osUnknown;
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
    m_parent_index=-1;
    TreeRedAction = actNone;
    TreeGreenAction = actNone;
    TreeYellowAction = actNone;
    TreeBlueAction = actNone;
    CollRedAction = actNone;
    CollGreenAction = actNone;
    CollYellowAction = actNone;
    CollBlueAction = actNone;
}


// ----------------------- mgMainMenu ----------------------


void
mgMainMenu::DumpOrders(mgValmap& nv)
{
	for (unsigned int idx=0;idx<orders.size();idx++)
	{
		mgOrder *o = orders[idx];
		if (!o) 
			mgError("DumpOrders:order[%u] is 0",idx);
		char prefix[20];
		sprintf(prefix,"order%u",idx);
		o->DumpState(nv,prefix);
	}
}

void
mgMainMenu::SaveState()
{
    char *oldfile;
    char *newfile;
    char *statefile;
    mgValmap nmain("MainMenu");
    mgValmap nsel("tree");
    mgValmap ncol("collection");
    asprintf(&oldfile,"%s/muggle.state.old",cPlugin::ConfigDirectory ("muggle"));
    asprintf(&newfile,"%s/muggle.state.new",cPlugin::ConfigDirectory ("muggle"));
    asprintf(&statefile,"%s/muggle.state",cPlugin::ConfigDirectory ("muggle"));
    FILE *f = fopen(newfile,"w");
    if (!f) 
    {
	    if (!m_save_warned)
		    mgWarning("Cannot write %s",newfile);
	    m_save_warned=true;
	    goto err_exit;
    }
    nmain.put("DefaultCollection",default_collection);
    nmain.put("UsingCollection",UsingCollection);
    nmain.put("TreeRedAction",int(Menus.front()->TreeRedAction));
    nmain.put("TreeGreenAction",int(Menus.front()->TreeGreenAction));
    nmain.put("TreeYellowAction",int(Menus.front()->TreeYellowAction));
    nmain.put("CollRedAction",int(Menus.front()->CollRedAction));
    nmain.put("CollGreenAction",int(Menus.front()->CollGreenAction));
    nmain.put("CollYellowAction",int(Menus.front()->CollYellowAction));
    nmain.put("CurrentOrder",m_current_order);
    DumpOrders(nmain);
    m_treesel->DumpState(nsel);
    m_collectionsel->DumpState(ncol);
    nmain.Write(f);
    nsel.Write(f);
    ncol.Write(f);
    fclose(f);
    rename(statefile,oldfile);
    rename(newfile,statefile);
err_exit:
    free(oldfile);
    free(newfile);
    free(statefile);
}

mgMainMenu::mgMainMenu ():cOsdMenu ("",25)
{
    m_Status = new mgStatus(this);
    m_message = 0;
    moveselection = 0;
    m_root = 0;
    external_commands = 0;
    queue_playing=false;
    instant_playing=false;
    m_save_warned=false;
    play_collection = tr("play");
    mgValmap nsel("tree");
    mgValmap ncol("collection");
    mgValmap nmain("MainMenu");

    // define defaults for values missing in state file:
    nsel.put("FallThrough",true);
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
    LoadOrders(nmain);
    default_collection = nmain.getstr("DefaultCollection");
    UsingCollection = nmain.getbool("UsingCollection");
    InitMapFromSetup(nsel);
    InitMapFromSetup(ncol);
    m_treesel = new mgSelection;
    m_treesel->setOrder(getOrder(m_current_order));
    m_treesel->InitFrom (nsel);
    m_treesel->CreateCollection(default_collection);
    if (default_collection!=play_collection)
	    m_treesel->CreateCollection(play_collection);
    vector<mgKeyTypes> kt;
    kt.push_back(keyCollection);
    kt.push_back(keyCollectionItem);
    mgOrder o;
    o.setKeys(kt);
    m_collectionsel = new mgSelection;
    m_collectionsel->setOrder(&o);
    m_collectionsel->InitFrom (ncol);
    m_playsel = new mgSelection;
    m_playsel->setOrder(&o);
    m_playsel->InitFrom(ncol);

    // initialize
    if (m_playsel->level()!=1)
    {
    	m_playsel->leave_all();
    	m_playsel->enter(play_collection);
    }
    UseNormalSelection ();
    unsigned int posi = selection()->gotoPosition();
    LoadExternalCommands();	// before AddMenu()
    m_root = new mgTree;
    m_root->TreeRedAction = mgActions(nmain.getuint("TreeRedAction"));
    m_root->TreeGreenAction = mgActions(nmain.getuint("TreeGreenAction"));
    m_root->TreeYellowAction = mgActions(nmain.getuint("TreeYellowAction"));
    m_root->CollRedAction = mgActions(nmain.getuint("CollRedAction"));
    m_root->CollGreenAction = mgActions(nmain.getuint("CollGreenAction"));
    m_root->CollYellowAction = mgActions(nmain.getuint("CollYellowAction"));
    AddMenu (m_root,posi);
    forcerefresh = false;
}

void
mgMainMenu::AddOrder()
{
	orders.push_back(new mgOrder);
}

void
mgMainMenu::DeleteOrder()
{
	mgOrder *o = orders[Current()];
	delete o;
	orders.erase(orders.begin()+Current());
}

void
mgMainMenu::LoadOrders(mgValmap& nv)
{
	for (unsigned int idx=0;idx<1000;idx++) 
	{
		char b[10];
		sprintf(b,"order%u",idx);
		mgOrder *o = new mgOrder(nv,b);
		if (o->size()==0)
		{
			delete o;
			break;
		}
		orders.push_back(o);
	}
    	m_current_order = nv.getuint("CurrentOrder");
	if (m_current_order >= orders.size())
		m_current_order=0;
	if (orders.size()>0) return;

    	nv.put("order0.Keys.0.Type",keyArtist);
    	nv.put("order0.Keys.1.Type",keyAlbum);
    	nv.put("order0.Keys.2.Type",keyTrack);

    	nv.put("order1.Keys.0.Type",keyAlbum);
    	nv.put("order1.Keys.1.Type",keyTrack);
	
    	nv.put("order2.Keys.0.Type",keyGenres);
    	nv.put("order2.Keys.1.Type",keyArtist);
    	nv.put("order2.Keys.2.Type",keyAlbum);
    	nv.put("order2.Keys.3.Type",keyTrack);

    	nv.put("order3.Keys.0.Type",keyArtist);
    	nv.put("order3.Keys.1.Type",keyTrack);
	
    	nv.put("CurrentOrder",0);
	LoadOrders(nv);
}

void
mgMainMenu::LoadExternalCommands()
{
// Read commands for collections in etc. /video/muggle/playlist_commands.conf
    external_commands = new cCommands ();

#if VDRVERSNUM >= 10318
    cString cmd_file = AddDirectory (cPlugin::ConfigDirectory ("muggle"),
        "playlist_commands.conf");
    mgDebug (1, "mgMuggle::Start: %d Looking for file %s",VDRVERSNUM, *cmd_file);
    bool have_cmd_file = external_commands->Load (*cmd_file);
#else
    const char *
        cmd_file = (const char *) AddDirectory (cPlugin::ConfigDirectory ("muggle"),
        "playlist_commands.conf");
    mgDebug (1, "mgMuggle::Start: %d Looking for file %s",VDRVERSNUM, cmd_file);
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
	delete m_treesel;
	delete m_collectionsel;
	delete m_playsel;
	delete m_Status;
	delete moveselection;
	delete m_root;
	delete external_commands;
	for (unsigned int i=0;i<orders.size();i++)
		delete orders[i];
}

void
mgMainMenu::InitMapFromSetup (mgValmap& nv)
{
    // values from setup override saved values
    nv["Directory"] = cPlugin::ConfigDirectory ("muggle");
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
    mgAction *a = GenerateAction(action,actNone);
    if (!a) return;
    a->SetText(osd()->hk(title));
    osd()->AddItem(a);
}

void
mgMainMenu::AddOrderActions(mgMenu* m)
{
    for (unsigned int idx=0;idx<orders.size();idx++)
    {
        mgOrder *o = orders[idx];
	if (!o) 
		mgError("AddOrderAction:orders[%u] is 0",idx);
    	mgAction *a = m->GenerateAction(actOrder,actNone);
    	assert(a);
    	a->SetText(hk(o->Name().c_str()));
    	AddItem(a);
    }
}

void
mgMenu::AddSelectionItems (mgSelection *sel,mgActions act)
{
    for (unsigned int i = 0; i < sel->listitems.size (); i++)
    {
    	mgAction *a = GenerateAction(act, actEntry);
	if (!a) continue;
	const char *name = a->MenuName(i+1,sel->listitems[i]);
	a->SetText(name,false);
	a->setHandle(i);
        osd()->AddItem(a);
    }
    if (osd()->ShowingCollections ())
    {
    	mgAction *a = GenerateAction(actCreateCollection,actNone);
    	if (a) 
	{
    		a->SetText(a->MenuName(),false);
    		osd()->AddItem(a);
	}
    }
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
    mgActions r,g,y,b;
    if (osd()->UsingCollection)
    {
	r = CollRedAction;
	g = CollGreenAction;
	y = CollYellowAction;
	b = CollBlueAction;
    }
    else
    {
	r = TreeRedAction;
	g = TreeGreenAction;
	y = TreeYellowAction;
	b = TreeBlueAction;
    }
    osd()->SetHelpKeys(HKey(r,on),
			HKey(g,on),
			HKey(y,on),
			HKey(b,on));
}


void
mgMainMenu::RefreshTitle()
{
    SetTitle(Menus.back()->Title().c_str());
    Display ();
}

void
mgMenu::InitOsd (const bool hashotkeys)
{
    osd ()->InitOsd (Title(),hashotkeys);
    SetHelpKeys();	// Default, will be overridden by the single items
}


void
mgMainMenu::InitOsd (string title,const bool hashotkeys)
{
    Clear ();
    SetTitle (title.c_str());
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

string
mgSubmenu::Title() const
{
    static char b[100];
    snprintf(b,99,tr("Commands:%s"),trim(osd()->selection()->getCurrentValue()).c_str());
    return b;
}

void
mgSubmenu::BuildOsd ()
{
    mgActions on = osd()->CurrentType();
    InitOsd ();
    if (!osd ()->Parent ())
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
    AddAction(actExportItemlist,on);
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
    mgActions result = actNone;
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
    mgActions action = actNone;
    if (osd()->UsingCollection)
    	switch (key)
    	{
        	case kRed: action = CollRedAction; break;
        	case kGreen: action = CollGreenAction; break;
        	case kYellow: action = CollYellowAction; break;
        	case kBlue: action = CollBlueAction; break;
        	default: break;
        }
    else
    	switch (key)
    	{
        	case kRed: action = TreeRedAction; break;
        	case kGreen: action = TreeGreenAction; break;
        	case kYellow: action = TreeYellowAction; break;
        	case kBlue: action = TreeBlueAction; break;
        	default: break;
        }
    return ExecuteAction(action,on);
}

mgTree::mgTree()
{
  TreeBlueAction = actShowCommands;
  CollBlueAction = actShowCommands;
  m_incsearch = NULL;
  m_start_position = 0;
}

eOSState
mgMenu::Process (eKeys key)
{
    return ExecuteButton(key);
}

void
mgTree::UpdateSearchPosition()
{
  int position = -1;
  if( !m_incsearch || m_filter.empty() )
	  position = m_start_position;
  else
  {
      // find the first item starting with m_filter
      mgListItems& listitems = osd()->selection()->listitems;
      for (unsigned int idx = 0 ; idx < listitems.size(); idx++)
	  if( strncasecmp( listitems[idx]->value().c_str(), m_filter.c_str(), m_filter.size() )>=0 )
	  {
	      position = idx;
	      break;
	  }
  }
  osd()->newposition = position;
}

bool
mgTree::UpdateIncrementalSearch( eKeys key )
{
  bool result; // false if no search active and keystroke was not used

  if( !m_incsearch )
    {
      switch( key )
	{
	case k0...k9:
	  { // create a new search object as this is the first keystroke
	    m_incsearch = new mgIncrementalSearch();
	    
	    // remember the position where we started to search
	    m_start_position = osd()->Current();

	    // interprete this keystroke
	    m_filter = m_incsearch->KeyStroke( key - k0 );
	    result = true;
	    UpdateSearchPosition();
	  } break;
	default:
	  {
	    result = false;
	  }
	}
    }
  else
    { // an incremental search is already active
      switch( key )
	{
	case kBack:
	  {
	    m_filter = m_incsearch->Backspace();

	    if( m_filter.empty() )
	      { // search should be terminated, returning to the previous item
		TerminateIncrementalSearch( false );
	      }
	    else
	      { // just find the first item for the current search string
		UpdateSearchPosition();
	      }
	    result = true;
	  } break;
	case k0...k9:
	  {
	    // evaluate the keystroke
	    m_filter = m_incsearch->KeyStroke( key - k0 );	    
	    result = true;
	    UpdateSearchPosition();
	  } break;
	default:
	  {
	    result = false;
	  }
	}  
    }
  return result;
}

void mgTree::TerminateIncrementalSearch( bool remain_on_current )
{
  if( m_incsearch )
    {
      m_filter = "";
      delete m_incsearch;
      m_incsearch = NULL;

      if( remain_on_current )
	{
	  m_start_position = osd()->Current();
	}

      UpdateSearchPosition();
    }
}

string
mgTree::Title () const
{
  string title = selection ()->getListname ();

  if( !m_filter.empty() )
    {
	    mgDebug(1,"m_filter.size():%d,string:%s",m_filter.size(),m_filter.c_str());
      title += " (" + m_filter + ")";
    }

  return title;
}

void
mgTree::BuildOsd ()
{
    InitOsd (false);
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
	      if (instant_playing && queue_playing) 
		{
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
      if (key==kPlay) 
	{ 
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
    	forcerefresh |= m_collectionsel->cacheIsEmpty();
    else
    	forcerefresh |= m_treesel->cacheIsEmpty();

    forcerefresh |= (newposition>=0);

    if (forcerefresh)
    {
    	forcerefresh = false;
	if (newposition<0) 
		newposition = selection()->gotoPosition();
        Menus.back ()->Display ();
    }
pr_exit:
    showMessage();
    return result;
}

void
mgMainMenu::CloseMenu()
{
    mgMenu* m = Menus.back();
    if (newposition==-1) newposition = m->getParentIndex();
    Menus.pop_back ();
    delete m;
}

void
mgMainMenu::showMessage()
{
    if (m_message)
    {
	showmessage(m_message);
	free(m_message);
	m_message = NULL;
    }
}

void
showmessage(const char * msg,int duration)
{
#if VDRVERSNUM >= 10307
    	Skins.Message (mtInfo, msg,duration);
    	Skins.Flush ();
#else
    	Interface->Status (msg);
    	Interface->Flush ();
#endif
}

void
showimportcount(unsigned int impcount,bool final=false)
{
	char b[100];
	if (final)
	{
		sprintf(b,tr("Import done:Imported %d items"),impcount);
		assert(strlen(b)<100);
		showmessage(b,1);
	}
	else
	{
		sprintf(b,tr("Imported %d items..."),impcount);
		assert(strlen(b)<100);
		showmessage(b);
	}
}

void
mgMainMenu::AddMenu (mgMenu * m,unsigned int position)
{
    Menus.push_back (m);
    m->setosd (this);
    m->setParentIndex(Current());
    if (Get(Current()))
    	m->setParentName(Get(Current())->Text());
    newposition = position;
    m->Display ();
}

void
mgMenu::setosd(mgMainMenu *osd)
{
    m_osd = osd;
    m_prevUsingCollection = osd->UsingCollection;
    m_prevpos=osd->selection()->getPosition();
}

mgSubmenu::mgSubmenu()
{
    TreeBlueAction = actShowList;
    CollBlueAction = actShowList;
}

string
mgMenuOrders::Title() const
{
	return tr("Select an order");
}

void
mgMenuOrders::BuildOsd ()
{
	TreeRedAction = actEditOrder;
	TreeGreenAction = actCreateOrder;
	TreeYellowAction = actDeleteOrder;
    	InitOsd ();
	osd()->AddOrderActions(this);
}

mgMenuOrder::mgMenuOrder()
{
    m_order=0;
}

mgMenuOrder::~mgMenuOrder()
{
    if (m_order)
	    delete m_order;
}

string
mgMenuOrder::Title() const
{
	return m_order->Name();
}

void
mgMenuOrder::BuildOsd ()
{
    if (!m_order)
    {
        m_order = new mgOrder;
	*m_order = *(osd()->getOrder(getParentIndex()));
    }
    InitOsd ();
    m_keytypes.clear();
    m_keytypes.reserve(mgKeyTypesNr+1);
    m_keynames.clear();
    m_keynames.reserve(50);
    m_orderbycount = m_order->getOrderByCount();
    for (unsigned int i=0;i<m_order->size();i++)
    {
	unsigned int kt;
	m_keynames.push_back(m_order->Choices(i,&kt));
	m_keytypes.push_back(kt);
	char buf[20];
	sprintf(buf,tr("Key %d"),i+1);
	mgAction *a = actGenerateKeyItem(buf,(int*)&m_keytypes[i],m_keynames[i].size(),&m_keynames[i][0]);
	a->SetMenu(this);
        osd()->AddItem(a);
    }
    mgAction *a = actGenerateBoolItem(tr("Sort by count"),&m_orderbycount);
    a->SetMenu(this);
    osd()->AddItem(a);
}

bool
mgMenuOrder::ChangeOrder(eKeys key)
{
    vector <mgKeyTypes> newtypes;
    newtypes.clear();
    for (unsigned int i=0; i<m_keytypes.size();i++)
    	newtypes.push_back(ktValue(m_keynames[i][m_keytypes[i]]));
    mgOrder n = mgOrder(newtypes);
    n.setOrderByCount(m_orderbycount);
    bool result = !(n == *m_order);
    *m_order = n;
    if (result)
    {
    	osd()->forcerefresh = true;
	int np = osd()->Current();
	if (key==kUp && np) np--;
	if (key==kDown) np++;
    	osd()->newposition = np;
    }
    return result;
}

void
mgMenuOrder::SaveOrder()
{
    *(osd()->getOrder(getParentIndex())) = *m_order;
    osd()->SaveState();
}


mgTreeCollSelector::mgTreeCollSelector()
{
    TreeBlueAction = actShowList;
    CollBlueAction = actShowList;
}

mgTreeCollSelector::~mgTreeCollSelector()
{
    osd()->UsingCollection = m_prevUsingCollection;
    osd()->newposition = m_prevpos;
}

string
mgTreeCollSelector::Title () const
{ 
    return m_title;
}

void
mgTreeCollSelector::BuildOsd ()
{
    osd()->UsingCollection = true;
    mgSelection *coll = osd()->collselection();
    InitOsd ();
    coll->leave_all();
    coll->setPosition(osd()->default_collection);
    AddSelectionItems (coll,coll_action());
    osd()->newposition = coll->gotoPosition();
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
mgMainMenu::DisplayGoto ()
{
    if (newposition >= 0)
    {
        if ((int)newposition>=Count())
	    newposition = Count() -1;
        SetCurrent (Get (newposition));
        RefreshCurrent ();
    }
    Display ();
}

void
mgMenu::Display ()
{
    BuildOsd ();
    osd ()->DisplayGoto ();
}

bool
create_question()
{
    char *b;
    asprintf(&b,tr("Create database %s?"),the_setup.DbName);
    bool result = Interface->Confirm(b);
    free(b);
    return result;
}

void
import()
{
    if (Interface->Confirm(tr("Import items?")))
    {
        mgThreadSync *s = mgThreadSync::get_instance();
        if (s)
       	{
		char *sync_args[] = { ".", 0 };
		s->Sync(sync_args,(bool)the_setup.DeleteStaleReferences);
	}
    }
}
