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

#include <vdr/menuitems.h>
#define __STL_CONFIG_H
#include <vdr/tools.h>
#include <vdr/config.h>
#include <vdr/plugin.h>
#include <vdr/i18n.h>

#if VDRVERSNUM >= 10307
#include <vdr/interface.h>
#include <vdr/skins.h>
#endif

#include "mg_setup.h"
#include "vdr_menu.h"
#include "vdr_player.h"
#include "mg_incremental_search.h"
#include "mg_thread_sync.h"

#include "mg_tools.h"
#include "mg_sel_gd.h"

void Play(mgSelection *sel, bool enter) {
	char *b;
	msprintf(&b,"Play, enter=%d",enter);
	sel->ShowState(b);
	free(b);
	mgSelection *s = GenerateSelection(sel);
	if (s->ordersize()==0)
		s->InitDefaultOrder(1);
	if (enter)
		s->enter();
	if (!s->skipItems(0)) {		 // no valid item exists
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
GenerateSelection(const mgSelection* s) {
	return new mgSelectionGd(s);
}

//! \brief queue the selection for playing, abort ongoing instant play
void
mgSelOsd::PlayQueue() {
	queue_playing=true;
	instant_playing=false;
	Play(playselection());
}

//! \brief queue the selection for playing, abort ongoing queue playing
void
mgSelOsd::PlayInstant(bool enter) {
	instant_playing=true;
	Play(selection(),enter);
}

bool
mgSelOsd::SwitchSelection() {
	UseNormalSelection();
	mgSelection* newsel = getSelection(Current());
	if (newsel->ordersize()>0) {
		newsel->CopyKeyValues(selection());
		newsel->Activate();
		m_current_selection = Current();
		newposition = selection()->getPosition();
		SaveState();
		return true;
	}
	else {
		Message1(tr("Order is undefined"));
		return false;
	}
}

void mgSelOsd::setSelection(unsigned int idx,mgSelection *s) {
	if (idx>=selections.size())
		mgError("mgSelOsd::getSelection(%u): selections.size() is %d",
			idx,selections.size());
	delete selections[idx];
	selections[idx] = s;
}

mgSelection* mgSelOsd::getSelection(unsigned int idx) {
	if (idx>=selections.size())
		mgError("mgSelOsd::getSelection(%u): selections.size() is %d",
			idx,selections.size());
	return selections[idx];
}

void
mgSelOsd::CollectionChanged(string name,bool added) {
	delete moveselection;
	moveselection = NULL;
	forcerefresh = true;		 // TODO brauchen wir das?
	if (name == play_collection) {
		playselection()->clearCache();
		mgPlayerControl *c = PlayerControl();
		if (c)
			c->ReloadPlaylist();
		else if (added)
			PlayQueue();
	}
	if (CollectionEntered(name) || selection()->isCollectionlist())
		selection()->clearCache();
}

bool
mgSelOsd::ShowingCollections() {
	return (UsingCollection && selection ()->orderlevel () == 0);
}

bool
mgSelOsd::DefaultCollectionSelected() {
	string this_sel = trim(selection ()->getCurrentValue());
	return (ShowingCollections () && this_sel == default_collection);
}

bool
mgSelOsd::CollectionEntered(string name) {
	if (!UsingCollection) return false;
	if (selection()->orderlevel()==0) return false;
	return trim(selection ()->getKeyItem(0)->value()) == name;
}

mgSelMenu::mgSelMenu () {
	CollRedAction = actNone;
	CollGreenAction = actNone;
	CollYellowAction = actNone;
	CollBlueAction = actNone;
}

void
mgSelMenu::SetHotkeyAction(eKeys key,mgActions action) {
	switch (key) {
		case kRed:
			if (Selosd()->UsingCollection)
				CollRedAction = action;
			else
				RedAction = action;
			break;
		case kGreen:
			if (Selosd()->UsingCollection)
				CollGreenAction = action;
			else
				GreenAction = action;
			break;
		case kYellow:
			if (Selosd()->UsingCollection)
				CollYellowAction = action;
			else
				YellowAction = action;
		default:
			break;
	}
}

mgPlayerControl *
PlayerControl () {
	mgPlayerControl *result = NULL;
	cControl *control = cControl::Control ();
	if (control && typeid (*control) == typeid (mgPlayerControl))
		// is there a running MP3 player?
		result = static_cast < mgPlayerControl * >(control);
	return result;
}

// ----------------------- mgSelOsd ----------------------

void
mgSelOsd::DumpSelections(mgValmap& nv) {
	for (unsigned int idx=0;idx<selections.size();idx++) {
		mgSelection *s = selections[idx];
		if (!s)
			mgError("DumpSelections:selection[%u] is 0",idx);
		char prefix[20];
		sprintf(prefix,"order%u",idx);
		s->DumpState(nv,prefix);
	}
}

void
mgSelOsd::SaveState() {
	char *oldfile;
	char *newfile;
	char *statefile;
	mgSelMenu *mfront = dynamic_cast<mgSelMenu *>(Menus.front());
	mgValmap nmain("MainMenu");
	mgValmap nsel("tree");
	mgValmap ncol("collection");
	msprintf(&oldfile,"%s/muggle.state.old",the_setup.ConfigDirectory.c_str());
	msprintf(&newfile,"%s/muggle.state.new",the_setup.ConfigDirectory.c_str());
	msprintf(&statefile,"%s/muggle.state",the_setup.ConfigDirectory.c_str());
	FILE *f = fopen(newfile,"w");
	if (!f) {
		if (!m_save_warned)
			mgWarning("Cannot write %s",newfile);
		m_save_warned=true;
		goto err_exit;
	}
	nmain.put(default_collection,"DefaultCollection");
	nmain.put(UsingCollection,"UsingCollection");
	nmain.put(int(mfront->RedAction),"RedAction");
	nmain.put(int(mfront->GreenAction),"GreenAction");
	nmain.put(int(mfront->YellowAction),"YellowAction");
	nmain.put(int(mfront->CollRedAction),"CollRedAction");
	nmain.put(int(mfront->CollGreenAction),"CollGreenAction");
	nmain.put(int(mfront->CollYellowAction),"CollYellowAction");
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

mgSelOsd::mgSelOsd () {
	mgDebug("mgSelOsd starts");
	moveselection = 0;
	queue_playing=false;
	instant_playing=false;
	play_collection = tr("play");
	mgValmap nsel("tree");
	mgValmap ncol("collection");
	mgValmap nmain("MainMenu");

	// define defaults for values missing in state file:
	nsel.put(true,"FallThrough");
	nmain.put(play_collection,"DefaultCollection");
	nmain.put(false,"UsingCollection");
	nmain.put(int(actAddThisToCollection),"RedAction");
	nmain.put(int(actInstantPlay),"GreenAction");
	nmain.put(int(actToggleSelection),"YellowAction");
	nmain.put(int(actAddThisToCollection),"CollRedAction");
	nmain.put(int(actInstantPlay),"CollGreenAction");
	nmain.put(int(actToggleSelection),"CollYellowAction");
	nmain.put(0,"CurrentOrder");

	// load values from state file
	char *b;
	msprintf(&b,"%s/muggle.state",the_setup.ConfigDirectory.c_str());
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
	if (m_playsel->orderlevel()!=1) {
		m_playsel->leave_all();
		m_playsel->enter(play_collection);
	}
	mgSelection *s = selections[m_current_selection];
	s->Activate();
	unsigned int posi = selection()->gotoPosition();
	LoadExternalCommands();		 // before AddMenu()
	mgSelMenu* m_selroot = new mgTree;
	m_root = m_selroot;
	m_root->RedAction = mgActions(nmain.getuint("RedAction"));
	m_root->GreenAction = mgActions(nmain.getuint("GreenAction"));
	m_root->YellowAction = mgActions(nmain.getuint("YellowAction"));
	m_selroot->CollRedAction = mgActions(nmain.getuint("CollRedAction"));
	m_selroot->CollGreenAction = mgActions(nmain.getuint("CollGreenAction"));
	m_selroot->CollYellowAction = mgActions(nmain.getuint("CollYellowAction"));
	AddMenu (m_root,posi);
	m_root->Display();
	forcerefresh = false;
}

void
mgSelOsd::AddSelection() {
	selections.push_back(GenerateSelection());
	newposition = selections.size()-1;
}

void
mgSelOsd::DeleteSelection() {
	mgSelection *o = selections[Current()];
	delete o;
	selections.erase(selections.begin()+Current());
}

void
mgSelOsd::LoadSelections(mgValmap& nv) {
	for (unsigned int idx=0;idx<1000;idx++) {
		char prefix[10];
		sprintf(prefix,"order%u",idx);
		mgSelection *s = GenerateSelection();
		s->InitFrom(prefix,nv);
		if (s->ordersize())
			selections.push_back(s);
		else {
			delete s;
			break;
		}
	}
	if (selections.size()==0) {
		for (unsigned int i=1; i<100;i++) {
			mgSelection* s=GenerateSelection();
			if (s->InitDefaultOrder(i))
				selections.push_back(s);
			else {
				delete s;
				break;
			}
		}
	}
	m_current_selection = nv.getuint("CurrentSelection");
	if (m_current_selection >= selections.size())
		m_current_selection=0;
}

mgSelOsd::~mgSelOsd() {
	mgDebug("mgSelOsd terminates");
	delete m_collectionsel;
	delete m_playsel;
	delete moveselection;
	for (unsigned int i=0;i<selections.size();i++)
		delete selections[i];
}

void
mgSelOsd::InitMapFromSetup (mgValmap& nv) {
	// values from setup override saved values
	nv["Directory"] = the_setup.ConfigDirectory;
}

void
mgSelOsd::AddOrderActions(mgSelMenu* m) {
	for (unsigned int idx=0;idx<selections.size();idx++) {
		mgSelection *o = selections[idx];
		if (!o)
			mgError("AddOrderAction:selections[%u] is 0",idx);
		mgAction *a = m->GenerateAction(actOrder,actNone);
		assert(a);
		string name = o->Name(); // do not combine these 2 lines!
		const char *oname = name.c_str();
		if (strlen(oname)==0)
			oname = tr("Order is undefined");
		a->SetText(hk(oname));
		AddItem(a);
	}
}

void
mgSelMenu::AddSelectionItems (mgSelection *sel,mgActions act) {
	sel->Activate();
	sel->listitems.refresh();
	for (unsigned int i = 0; i < sel->listitems.size (); i++) {
		mgAction *a = GenerateAction(act, actEntry);
		if (!a) continue;
		const char *name = a->MenuName(i+1,sel->listitems[i]);
		a->SetText(name,false);
		a->setHandle(i);
		osd()->AddItem(a);
	}
	if (Selosd()->ShowingCollections ()) {
		mgAction *a = GenerateAction(actCreateCollection,actNone);
		if (a) {
			a->SetText(a->MenuName(),false);
			osd()->AddItem(a);
		}
	}
}

string
mgSubmenu::Title() const
{
	static char b[100];
	snprintf(b,99,tr("Commands:%s"),trim(Selosd()->selection()->getCurrentValue()).c_str());
	return b;
}

void
mgSubmenu::BuildOsd () {
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
	if (osd()->external_commands) {
		int idx=0;
		while ((command = osd ()->external_commands->Get (idx)) != NULL) {
			if (idx>actExternalHigh-actExternal0) {
				mgWarning("Too many external commands");
				break;
			}
			AddExternalAction (mgActions(idx+int(actExternal0)),command->Title());
			idx++;
		}
	}
	RedAction = actSetButton;
	GreenAction = actSetButton;
	YellowAction = actSetButton;
	CollRedAction = actSetButton;
	CollGreenAction = actSetButton;
	CollYellowAction = actSetButton;
}

mgActions
mgSelMenu::ButtonAction(eKeys key) {
	mgActions result = actNone;
	if (Selosd()->UsingCollection)
	switch (key) {
		case kRed: result = CollRedAction; break;
		case kGreen: result = CollGreenAction; break;
		case kYellow: result = CollYellowAction; break;
		case kBlue: result = CollBlueAction; break;
		default: break;
	}
	else
	switch (key) {
		case kRed: result = RedAction; break;
		case kGreen: result = GreenAction; break;
		case kYellow: result = YellowAction; break;
		case kBlue: result = BlueAction; break;
		default: break;
	}
	return result;
}

mgTree::mgTree() {
	BlueAction = actShowCommands;
	CollBlueAction = actShowCommands;
	m_incsearch = NULL;
	m_start_position = 0;
}

eOSState
mgSelMenu::Process (eKeys key) {
	return ExecuteButton(key);
}

void
mgTree::UpdateSearchPosition() {
	if( !m_incsearch || m_filter.empty() )
		osd()->newposition = m_start_position;
	else
		osd()->newposition = Selosd()->selection()->searchPosition(m_filter);
}

bool
mgTree::UpdateIncrementalSearch( eKeys key ) {
	bool result;				 // false if no search active and keystroke was not used

	if( !m_incsearch ) {
		switch( key ) {
			case k0...k9:
			{					 // create a new search object as this is the first keystroke
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
	else {
								 // an incremental search is already active
		switch( key ) {
			case kBack:
			{
				m_filter = m_incsearch->Backspace();

				if( m_filter.empty() ) {
								 // search should be terminated, returning to the previous item
					TerminateIncrementalSearch( false );
				}
				else {
								 // just find the first item for the current search string
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

void mgTree::TerminateIncrementalSearch( bool remain_on_current ) {
	if( m_incsearch ) {
		m_filter = "";
		delete m_incsearch;
		m_incsearch = NULL;

		if( remain_on_current ) {
			m_start_position = osd()->Current();
		}

		UpdateSearchPosition();
	}
}

string
mgTree::Title () const
{
	string title = selection ()->getListname ();

	if( !m_filter.empty() ) {
		title += " (" + m_filter + ")";
	}

	return title;
}

void
mgTree::BuildOsd () {
	InitOsd (false);
	AddSelectionItems (selection());
	if (m_osd->Count()==0)
		AddAction(actSync);
}

eOSState mgSelOsd::ProcessKey (eKeys key) {
	eOSState result = osContinue;
	mgPlayerControl * c = PlayerControl ();
	if (c) {
		if (!c->Playing ()) {
			c->Shutdown();
			if (instant_playing && queue_playing) {
				PlayQueue();
			}
			else {
				instant_playing = false;
				queue_playing = false;
			}
		}
		else {
			switch (key) {
				case kStop:
					if (instant_playing && queue_playing) {
						PlayQueue();
					}
					else {
						queue_playing = false;
						c->Stop ();
					}
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
	forcerefresh |= selection()->cacheIsEmpty();
	return mgOsd::ProcessKey(key);
	pr_exit:
	showMessage();
	return result;
}

void
showimportcount(unsigned int impcount,bool final=false) {
#if 0
	// we should not write to the OSD since this is not the
	// foreground thread. We could go thru port 2001.
	if (final)
		showmessage(1,"Import done:Imported %d items",impcount);
	else
		showmessage(2,"Imported %d items...",impcount);
#endif
}

void
mgSelOsd::AddMenu (mgMenu * m,int position) {
	selection()->Activate();
	mgOsd::AddMenu(m,position);
}

void
mgSelMenu::setosd(mgOsd *osd) {
	m_osd = osd;
								 // TODO sollte direkt parent fragen
	m_prevUsingCollection = Selosd()->UsingCollection;
								 // TODO sollte direkt parent fragen
	m_prevpos=Selosd()->selection()->getPosition();
	// TODO dann muss dieses setosd auch nicht mehr virtual sein.
}

mgSubmenu::mgSubmenu() {
	BlueAction = actShowList;
	CollBlueAction = actShowList;
}

string
mgSelMenuOrders::Title() const
{
	return tr("Select an order");
}

void
mgSelMenuOrders::BuildOsd () {
	RedAction = actEditOrder;
	GreenAction = actCreateOrder;
	YellowAction = actDeleteOrder;
	InitOsd ();
	Selosd()->AddOrderActions(this);
}

mgSelMenuOrder::mgSelMenuOrder() {
	m_selection=0;
	m_orgselection = 0;
}

mgSelMenuOrder::~mgSelMenuOrder() {
	if (m_selection)
		delete m_selection;
}

string
mgSelMenuOrder::Title() const
{
	return m_selection->Name();
}

void
mgSelMenuOrder::BuildOsd () {
	if (!m_orgselection)
		m_orgselection = Selosd()->getSelection(getParentIndex());;
	if (!m_selection)
		m_selection = GenerateSelection(m_orgselection);
	if (m_selection->ordersize()==0)
		m_selection->InitDefaultOrder(1);
	InitOsd ();
	m_keytypes.clear();
	m_keynames.clear();
	m_orderbycount = m_selection->getOrderByCount();
	for (unsigned int i=0;i<m_selection->ordersize();i++) {
		if (m_selection->getKeyType(i)==keyGdUnique)
			break;
		unsigned int kt;
		m_keynames.push_back(m_selection->Choices(i,&kt));
		m_keytypes.push_back(kt);
	}
	for (unsigned int i=0;i<m_selection->ordersize();i++) {
		if (m_selection->getKeyType(i)==keyGdUnique)
			break;
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
mgSelMenuOrder::ChangeSelection(eKeys key) {
	vector <const char*> newtypes;
	newtypes.clear();
	for (unsigned int i=0; i<m_keytypes.size();i++)
		newtypes.push_back(m_keynames[i][m_keytypes[i]]);
	mgSelection *newsel = GenerateSelection(m_orgselection);
	newsel->setKeys(newtypes);
	newsel->setOrderByCount(m_orderbycount);
	bool changed = !newsel->SameOrder(m_selection);
	if (changed) {
		delete m_selection;
		m_selection = newsel;
		osd()->forcerefresh = true;
		int np = osd()->Current();
		if (key==kUp && np) np--;
		if (key==kDown) np++;
		osd()->newposition = np;
	}
	else
		delete newsel;
	return changed;
}

void
mgSelMenuOrder::SaveSelection() {
	m_selection->CopyKeyValues(Selosd()->selection());
	m_selection->Activate();
	Selosd()->setSelection(getParentIndex(),m_selection);
	m_selection = 0;
	m_orgselection = 0;
	Selosd()->SaveState();
}

mgTreeCollSelector::mgTreeCollSelector() {
	BlueAction = actShowList;
	CollBlueAction = actShowList;
}

mgTreeCollSelector::~mgTreeCollSelector() {
	Selosd()->UsingCollection = m_prevUsingCollection;
	osd()->newposition = m_prevpos;
}

string
mgTreeCollSelector::Title () const
{
	return m_title;
}

void
mgTreeCollSelector::BuildOsd () {
	Selosd()->UsingCollection = true;
	mgSelection *coll = Selosd()->collselection();
	InitOsd ();
	coll->leave_all();
	coll->setPosition(Selosd()->default_collection);
	AddSelectionItems (coll,coll_action());
	osd()->newposition = coll->gotoPosition();
	cOsdItem *c = osd()->Get(osd()->newposition);
	mgAction *a = dynamic_cast<mgAction *>(c);
	a->IgnoreNextEvent = true;
}

mgTreeAddToCollSelector::mgTreeAddToCollSelector(string title) {
	m_title = title;
}

mgTreeRemoveFromCollSelector::mgTreeRemoveFromCollSelector(string title) {
	m_title = title;
}

bool
create_question() {
	char *b;
	msprintf(&b,tr("Create database %s?"),the_setup.DbName);
	bool result = Interface->Confirm(b);
	free(b);
	return result;
}

bool
import() {
	if (!Interface->Confirm(tr("Import items?")))
		return false;
	mgDb *db = GenerateDB(false);// make sure in main thread that DB exists
	bool db_exists=db->Connect();
	delete db;
	if (!db_exists)
		return false;
	mgThreadSync *s = mgThreadSync::get_instance();
	if (!s)
		return false;
	static const char *tld_arg[] = { ".", 0};
	int res = chdir(the_setup.ToplevelDir);
	if (res) {
		showmessage(2,tr("Cannot access directory %s:%d"),
			the_setup.ToplevelDir,errno);
		return false;
	}
	s->Sync(tld_arg);
	return true;
}
