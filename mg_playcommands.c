//								-*- c++ -*-

#include <string>
#include <fstream>

#include <vdr/interface.h>
#include <vdr/menu.h>
#include <vdr/plugin.h>
#include <vdr/status.h>

#include "mg_playcommands.h"
#include "mg_skin.h"
#include "mg_setup.h"

#define X_DISPLAY_MISSING
#define MAXLENGTH 256
#define THEMESEXT "*.theme"
#define VISEXT "*.visual"
#define FINDCMD "cd '%s' && find '%s' -iname '%s' -printf '%%p\n' | sort -f"

char urlname[256];

extern char coverpicture[256];


// ------------ mgPlayerCommands --------------------------------------------------------------------------------------------------- //

mgPlayerCommands::mgPlayerCommands(void) {
}

mgPlayerCommands::~mgPlayerCommands() {
}

void
mgPlayerCommands::BuildOsd() {
	RedAction=actBack;
	InitOsd(true);
	AddAction(actToggleShuffle);
	AddAction(actToggleLoop);
	AddAction(actShowLyrics);
}

string
mgPlayerCommands::Title() const
{
	return tr("Commands");
}

void mgPlayerCommands::LoadCommands() {
	commands.Load(AddDirectory(the_setup.ConfigDirectory.c_str(), "data/musiccmds.dat"), true);
}

eOSState mgPlayerCommands::Execute(void) {
	return osContinue;
}
