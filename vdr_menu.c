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

#include "mg_setup.h"
#include "vdr_menu.h"
#include "vdr_player.h"
#include "mg_incremental_search.h"
#include "mg_thread_sync.h"
#include "i18n.h"

#include "mg_tools.h"
#include "mg_sel_gd.h"

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
	mgSelection *s = GenerateSelection(sel);
	if (s->ordersize()==0)
		s->InitDefaultOrder(1);
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

mgSelection*
GenerateSelection(const mgSelection* s)
{
	return new mgSelectionGd(s);
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
mgMainMenu::SwitchSelection()
{
	UseNormalSelection();
	mgSelection* newsel = getSelection(Current());
	if (newsel->ordersize()>0)
	{
		newsel->CopyKeyValues(selection());
		m_current_selection = Current();
		newposition = selection()->getPosition();
		SaveState();
		return true;
	}
	else
	{
		Message1(tr("Order is undefined"),"");
		return false;
	}
}

void mgMainMenu::setSelection(unsigned int idx,mgSelection *s)
{
	if (idx>=selections.size())
		mgError("mgMainMenu::getSelection(%u): selections.size() is %d",
				idx,selections.size());
	delete selections[idx];
	selections[idx] = s;
}

mgSelection* mgMainMenu::getSelection(unsigned int idx)
{
	if (idx>=selections.size())
		mgError("mgMainMenu::getSelection(%u): selections.size() is %d",
				idx,selections.size());
	return selections[idx];
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
    return (UsingCollection && selection ()->orderlevel () == 0);
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
    if (selection()->orderlevel()==0) return false;
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
mgMainMenu::DumpSelections(mgValmap& nv)
{
	for (unsigned int idx=0;idx<selections.size();idx++)
	{
		mgSelection *s = selections[idx];
		if (!s) 
			mgError("DumpSelections:selection[%u] is 0",idx);
		char prefix[20];
		sprintf(prefix,"order%u",idx);
		s->DumpState(nv,prefix);
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
    nmain.put(default_collection,"DefaultCollection");
    nmain.put(UsingCollection,"UsingCollection");
    nmain.put(int(Menus.front()->TreeRedAction),"TreeRedAction");
    nmain.put(int(Menus.front()->TreeGreenAction),"TreeGreenAction");
    nmain.put(int(Menus.front()->TreeYellowAction),"TreeYellowAction");
    nmain.put(int(Menus.front()->CollRedAction),"CollRedAction");
    nmain.put(int(Menus.front()->CollGreenAction),"CollGreenAction");
    nmain.put(int(Menus.front()->CollYellowAction),"CollYellowAction");
    nsel.put(m_current_selection,"CurrentSelection");
    DumpSelections(nsel);
    m_collectionsel->DumpState(ncol,"collection");
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
    nsel.put(true,"FallThrough");
    nmain.put(play_collection,"DefaultCollection");
    nmain.put(false,"UsingCollection");
    nmain.put(int(actAddThisToCollection),"TreeRedAction");
    nmain.put(int(actInstantPlay),"TreeGreenAction");
    nmain.put(int(actToggleSelection),"TreeYellowAction");
    nmain.put(int(actAddThisToCollection),"CollRedAction");
    nmain.put(int(actInstantPlay),"CollGreenAction");
    nmain.put(int(actToggleSelection),"CollYellowAction");
    nmain.put(0,"CurrentOrder");


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
    InitMapFromSetup(nsel);
    InitMapFromSetup(ncol);
    LoadSelections(nsel);
    default_collection = nmain.getstr("DefaultCollection");
    UsingCollection = nmain.getbool("UsingCollection");
    selections[m_current_selection]->CreateCollection(default_collection);
    if (default_collection!=play_collection)
	    selections[m_current_selection]->CreateCollection(play_collection);
    m_collectionsel = GenerateSelection();
    m_collectionsel->InitFrom ("order0",ncol);
    m_collectionsel->MakeCollection();
    m_playsel = GenerateSelection();
    m_playsel->InitFrom("order0",ncol);
    m_playsel->MakeCollection();
    // initialize
    if (m_playsel->orderlevel()!=1)
    {
    	m_playsel->leave_all();
    	m_playsel->enter(play_collection);
    }
    mgSelection *s = selections[m_current_selection];
    s->CopyKeyValues(s);
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
mgMainMenu::AddSelection()
{
	selections.push_back(GenerateSelection());
}

void
mgMainMenu::DeleteSelection()
{
	mgSelection *o = selections[Current()];
	delete o;
	selections.erase(selections.begin()+Current());
}

void
mgMainMenu::LoadSelections(mgValmap& nv)
{
	for (unsigned int idx=0;idx<1000;idx++) 
	{
		char prefix[10];
		sprintf(prefix,"order%u",idx);
		mgSelection *s = GenerateSelection();
		s->InitFrom(prefix,nv);
		if (s->ordersize())
			selections.push_back(s);
		else
		{
			delete s;
			break;
		}
	}
	if (selections.size()==0)
	{
		for (unsigned int i=1; i<100;i++)
		{
			mgSelection* s=GenerateSelection();
			if (s->InitDefaultOrder(i))
				selections.push_back(s);
			else
			{
				delete s;
				break;
			}
		}
	}
    	m_current_selection = nv.getuint("CurrentSelection");
	if (m_current_selection >= selections.size())
		m_current_selection=0;
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
	delete m_collectionsel;
	delete m_playsel;
	delete m_Status;
	delete moveselection;
	delete m_root;
	delete external_commands;
	for (unsigned int i=0;i<selections.size();i++)
		delete selections[i];
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
    for (unsigned int idx=0;idx<selections.size();idx++)
    {
        mgSelection *o = selections[idx];
	if (!o) 
		mgError("AddOrderAction:selections[%u] is 0",idx);
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
  if( !m_incsearch || m_filter.empty() )
	osd()->newposition = m_start_position;
  else
	osd()->newposition = osd()->selection()->searchPosition(m_filter);
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

    forcerefresh |= selection()->cacheIsEmpty();

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
	showmessage(0,m_message);
	free(m_message);
	m_message = NULL;
    }
}

void
showmessage(int duration,const char * msg, ...)
{
	va_list ap;
	va_start(ap,msg);
	char buffer[200];
	vsnprintf(buffer,199,tr(msg),ap);
#if VDRVERSNUM >= 10307
	if (!duration) duration=2;
    	Skins.Message (mtInfo, buffer,duration);
    	Skins.Flush ();
#else
    	Interface->Status (buffer);
    	Interface->Flush ();
#endif
	va_end(ap);
}

void
showimportcount(unsigned int impcount,bool final=false)
{
	if (final)
		showmessage(1,"Import done:Imported %d items",impcount);
	else
		showmessage(2,"Imported %d items...",impcount);
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
    m_selection=0;
}

mgMenuOrder::~mgMenuOrder()
{
    if (m_selection)
	    delete m_selection;
}

string
mgMenuOrder::Title() const
{
	return m_selection->Name();
}

void
mgMenuOrder::BuildOsd ()
{
    if (!m_selection)
        m_selection = GenerateSelection(osd()->getSelection(getParentIndex()));
    InitOsd ();
    m_keytypes.clear();
    m_keytypes.reserve(mgKeyTypesNr+1);
    m_keynames.clear();
    m_keynames.reserve(50);
    m_orderbycount = m_selection->getOrderByCount();
    for (unsigned int i=0;i<m_selection->ordersize();i++)
    {
	unsigned int kt;
	m_keynames.push_back(m_selection->Choices(i,&kt));
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
mgMenuOrder::ChangeSelection(eKeys key)
{
    vector <const char*> newtypes;
    newtypes.clear();
    for (unsigned int i=0; i<m_keytypes.size();i++)
    	newtypes.push_back(m_keynames[i][m_keytypes[i]]);
    mgSelection *orgselection = m_selection;
    m_selection = GenerateSelection(orgselection);
    m_selection->setKeys(newtypes);
    m_selection->setOrderByCount(m_orderbycount);
    bool result = !m_selection->SameOrder(orgselection);
    if (orgselection)
    	delete orgselection;
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
mgMenuOrder::SaveSelection()
{
    osd()->setSelection(getParentIndex(),m_selection);
    m_selection = 0;
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

bool
import()
{
    if (!Interface->Confirm(tr("Import items?")))
	    return false;
    mgThreadSync *s = mgThreadSync::get_instance();
    if (!s)
	    return false;
    s->Sync();
    return true;
}
