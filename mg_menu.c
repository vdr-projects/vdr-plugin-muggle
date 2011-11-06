/*!								-*- c++ -*-
 * \file   vdr_menu.c
 * \brief  Implements menu handling for browsing media libraries within VDR
 *
 * \version $Revision: 1.27 $ * \date    $Date: 2008-03-14 21:12:59 +0100 (Fr, 14 Mär 2008) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author: woro $
 *
 * $Id: vdr_menu.c 1042 2008-03-14 20:12:59Z woro $
 */

#include <stdio.h>
#include <assert.h>
#include <fstream>

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

void
mgStatus::OsdCurrentItem(const char* Text) {
	cOsdItem* i = main->Get(main->Current());
	if (!i) return;
	mgAction * a = dynamic_cast<mgAction *>(i);
	if (a)
		a->TryNotify();
}

mgMenu *
mgOsd::Parent () {
	if (Menus.size () < 2)
		return NULL;
	return Menus[Menus.size () - 2];
}

mgAction*
mgMenu::GenerateAction(const mgActions action,mgActions on) {
	mgAction *result = actGenerate(action);
	if (result) {
		result->SetMenu(this);
		if (!result->Enabled(on)) {
			DELETENULL(result);
		}
	}
	return result;
}

eOSState
mgMenu::ExecuteAction(const mgActions action,mgActions on) {
	mgAction *a = GenerateAction (action,on);
	if (a) {
		a->Execute ();
		delete a;
		return osContinue;
	}
	return osUnknown;
}

mgMenu::mgMenu () {
	m_osd = NULL;
	m_parent_index=-1;
	mgDebug(1,"ich bin mgMenu::mgMenu %04X ,m_parent_index=%d",this,m_parent_index);
	RedAction = actNone;
	GreenAction = actNone;
	YellowAction = actNone;
	BlueAction = actNone;
}

void
mgOsd::SetHotkeyAction(eKeys key,mgActions action) {
	if (Parent())
		Parent()->SetHotkeyAction(key,action);
}

void
mgMenu::SetHotkeyAction(eKeys key,mgActions action) {
	switch (key) {
		case kRed:
			RedAction = action;
			break;
		case kGreen:
			GreenAction = action;
			break;
		case kYellow:
			YellowAction = action;
		default:
			break;
	}
}

// ----------------------- mgOsd ----------------------

mgOsd::mgOsd ():cOsdMenu ("",25) {
	mgDebug("mgOsd starts");
	m_Status = new mgStatus(this);
	m_message = 0;
	m_root = 0;
	external_commands = 0;
	m_save_warned=false;

	// get values from mgValmaps
	LoadExternalCommands();		 // before AddMenu()
	forcerefresh = false;
}

void
mgOsd::LoadExternalCommands() {
	// Read commands for collections in etc. /video/muggle/playlist_commands.conf
	external_commands = new cCommands ();

#if VDRVERSNUM >= 10318
	cString cmd_file = AddDirectory (the_setup.ConfigDirectory.c_str(),
		"playlist_commands.conf");
	mgDebug (1, "mgOsd::Start: %d Looking for file %s",VDRVERSNUM, *cmd_file);
	bool have_cmd_file = external_commands->Load (*cmd_file);
#else
	const char *
		cmd_file = (const char *) AddDirectory (the_setup.ConfigDirectory,
		"playlist_commands.conf");
	mgDebug (1, "mgOsd::Start: %d Looking for file %s",VDRVERSNUM, cmd_file);
	bool have_cmd_file = external_commands->Load ((const char *) cmd_file);
#endif

	if (!have_cmd_file) {
		DELETENULL(external_commands);
	}
}

mgOsd::~mgOsd() {
	mgDebug("mgOsd terminates");
	delete m_Status;
	delete m_root;
	delete external_commands;
}

void
mgOsd::InitMapFromSetup (mgValmap& nv) {
	// values from setup override saved values
	nv["Directory"] = the_setup.ConfigDirectory;
}

void
mgMenu::AddAction (const mgActions action, mgActions on,const bool hotkey) {
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
mgMenu::AddExternalAction(const mgActions action, const char *title) {
	mgAction *a = GenerateAction(action,actNone);
	if (!a) return;
	a->SetText(osd()->hk(title));
	osd()->AddItem(a);
}

const char*
mgMenu::HKey(const mgActions act,mgActions on) {
	const char* result = NULL;
	mgAction *a = GenerateAction(act,on);
	if (a) {
		result = a->ButtonName();
		delete a;
	}
	return result;
}

void
mgOsd::RefreshTitle() {
	SetTitle(Menus.back()->Title().c_str());
	Display ();
}

void
mgMenu::InitOsd (const bool hashotkeys) {
	osd ()->InitOsd (Title(),hashotkeys);
	SetHelpKeys();
}

void
mgMenu::SetHelpKeys(mgActions on) {
	osd()->SetHelpKeys(
		HKey(ButtonAction(kRed),on),
		HKey(ButtonAction(kGreen),on),
		HKey(ButtonAction(kYellow),on),
		HKey(ButtonAction(kBlue),on));
}

void
mgOsd::InitOsd (string title,const bool hashotkeys) {
	Clear ();
	SetTitle (title.c_str());
	if (hashotkeys) SetHasHotkeys ();
}

void
mgOsd::AddItem(mgAction *a) {
	cOsdItem *c = dynamic_cast<cOsdItem*>(a);
	if (!c)
		mgError("AddItem with non cOsdItem");
	Add(c);
}

//! \brief used for plain text items like in lyrics
class mgText : public mgOsdItem
{
	public:
		void Notify() {};
		bool Enabled(mgActions on) { return true;}
		const char *MenuName (const unsigned int idx,const mgListItem* item);
};

const char *
mgText::MenuName (const unsigned int idx,const mgListItem* item) {
	return strdup("mgtext::menuname");
}

void
mgOsd::AddText(const char* text,bool selectable) {
	mgText *item = new mgText;
	item->mgAction::SetText(text,true);
	AddItem(item);
	item->SetSelectable(selectable);
}

void
mgOsd::AddFile(const string filename) {
	FILE *fp = fopen(filename.c_str(),"r");
	if (fp) {
		char* line = NULL;
		size_t len = 0;
		while (getline(&line, &len , fp) != -1) {
			char *lf = strrchr(line,'\n');
			if (lf) *lf = 0;
			AddText(line,strlen(line));
		}
		if (line)
			free(line);
		fclose(fp);
	} else {
		char *s;
		msprintf(&s,tr("File %s not found"),filename.c_str());
		AddText(s);
		free(s);
	}
}

mgActions
mgOsd::CurrentType() {
	mgActions result = actNone;
	cOsdItem* c = Get(Current());
	if (c) {
		mgAction *a = dynamic_cast<mgAction*>(c);
		if (a)
			result = a->Type();
	}
	return result;
}

mgActions
mgMenu::ButtonAction(eKeys key) {
	mgActions result = actNone;
	switch (key) {
		case kRed: result = RedAction; break;
		case kGreen: result = GreenAction; break;
		case kYellow: result = YellowAction; break;
		case kBlue: result = BlueAction; break;
		default: break;
	}
	return result;
}

eOSState
mgMenu::ExecuteButton(eKeys key) {
	return ExecuteAction(ButtonAction(key),osd()->CurrentType());
}

eOSState
mgMenu::Process (eKeys key) {
	return ExecuteButton(key);
}

const char*
mgOsd::Message1(const char *msg, ...) {
	if (strlen(msg)==0) return 0;
	va_list ap;
	va_start(ap, msg);
	vmsprintf(&m_message, tr(msg), ap);
	va_end(ap);
	return m_message;
}

const char*
mgOsd::Message1(const char *msg, const string &arg) {
	if (strlen(msg)==0) return 0;
	msprintf(&m_message, tr(msg), arg.c_str());
	return m_message;
}

eOSState mgOsd::ProcessKey (eKeys key) {
	eOSState result = osContinue;
 	switch (key) {
 		case kFastRew:
		case kFastRew|k_Repeat:
		case kFastFwd:
		case kFastFwd|k_Repeat:
		case kPlay:
		case kPrev:
		case kPrev|k_Repeat:
		case kNext:
		case kNext|k_Repeat:
		case kChanUp:
		case kChanUp|k_Repeat:
		case kChanDn:
		case kChanDn|k_Repeat:
		// case kStop: does hide the player osd, but the player is still there in limbo
		case kPause: { 
			mgPlayerControl *c = PlayerControl();
			if (c) {
				result=c->ProcessPlayerKey(key);
				goto pr_exit;
			}
			break;
		}
		default:
			break;
	}
	if (Menus.size()<1)
		mgError("mgOsd::ProcessKey: Menus is empty");
	newmenu = Menus.back();		 // Default: Stay in current menu
	newposition = -1;

	{
		mgMenu * oldmenu = newmenu;

		// item specific key logic:
		result = cOsdMenu::ProcessKey (key);

		// mgMenu specific key logic:
		if (result == osUnknown)
			result = oldmenu->Process (key);
	}
	if (result == osBack) {
		mgError("vdr_menu found osBack: Should never happen");
		result=osUnknown;
	}
	// do nothing for unknown keys:
	if (result == osUnknown)
		goto pr_exit;

	// change OSD menu as requested:
	if (newmenu == NULL) {
		if (Menus.size () > 1) {
			CloseMenu();
			forcerefresh = true;
		}
		else {
			result = osBack;	 // game over
			goto pr_exit;
		}
	}
	else if (newmenu != Menus.back ())
		AddMenu (newmenu,newposition);

	forcerefresh |= (newposition>=0);

	if (forcerefresh) {
		forcerefresh = false;
		if (newposition<0) {
			mgSelMenu*sm = dynamic_cast<mgSelMenu*>(this);
			if (sm)
				newposition = sm->selection()->gotoPosition();
		}
		Menus.back ()->Display ();
	}
	pr_exit:
	showMessage();
	return result;
}

void
mgOsd::CloseMenu() {
	mgMenu* m = Menus.back();
	if (newposition==-1) newposition = m->getParentIndex();
	Menus.pop_back ();
	delete m;
}

void
mgOsd::showMessage() {
	if (m_message) {
		showmessage(0,m_message);
		free(m_message);
		m_message = NULL;
	}
}

void
showmessage(int duration,const char * msg, ...) {
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
mgOsd::AddMenu (mgMenu * m,int position) {
	Menus.push_back (m);
	m->setosd (this);
	m->setParentIndex(Current());
	if (Get(Current()))
		m->setParentName(Get(Current())->Text());
	if (position<0) position=0;
	newposition = position;
//	m->Display ();
}

void
mgMenu::setosd(mgOsd *osd) {
	m_osd = osd;
}

void
mgOsd::DisplayGoto () {
	if (newposition >= 0) {
		if ((int)newposition>=Count())
			newposition = Count() -1;
		SetCurrent (Get (newposition));
		RefreshCurrent ();
	}
	Display ();
}

void
mgMenu::Display () {
	BuildOsd ();
	osd ()->DisplayGoto ();
}

mgItemGd *
mgMenu::mutPlayingItem(void)
{
	if (!PlayerControl())
		return 0;
	else
		return PlayerControl()->CurrentItem();
}

const mgItemGd *
mgMenu::PlayingItem(void) const
{
	if (!PlayerControl())
		return 0;
	else
		return PlayerControl()->CurrentItem();
}

#if VDRVERSNUM >= 10712
// Borrowed from epgsearch
char *cCommand::result = NULL;

cCommand::cCommand(void)
{
	title = command = NULL;
	confirm = false;
}

cCommand::~cCommand()
{
	free(title);
	free(command);
}

bool cCommand::Parse(const char *s)
{
	const char *p = strchr(s, ':');
	if (p) {
		int l = p - s;
		if (l > 0) {
			title = MALLOC(char, l + 1);
			stripspace(strn0cpy(title, s, l + 1));
			if (!isempty(title)) {
				int l = strlen(title);
				if (l > 1 && title[l - 1] == '?') {
					confirm = true;
					title[l - 1] = 0;
				}
				command = stripspace(strdup(skipspace(p + 1)));
				return !isempty(command);
			}
		}
	}
	return false;
}

const char *cCommand::Execute(const char *Parameters)
{
	free(result);
	result = NULL;
	cString cmdbuf;
	if (Parameters)
		cmdbuf = cString::sprintf("%s %s", command, Parameters);
	const char *cmd = *cmdbuf ? *cmdbuf : command;
	dsyslog("executing command '%s'", cmd);
	cPipe p;
	if (p.Open(cmd, "r")) {
		int l = 0;
		int c;
		while ((c = fgetc(p)) != EOF) {
			if (l % 20 == 0)
				result = (char *)realloc(result, l + 21);
			result[l++] = char(c);
		}
		if (result)
			result[l] = 0;
		p.Close();
	}
	else
		esyslog("ERROR: can't open pipe for command '%s'", cmd);
	return result;
}
#endif
