#include <string.h>
#include <fstream>

#include <vdr/interface.h>
#include <vdr/menu.h>
#include <vdr/plugin.h>

#include "lyrics.h"
#include "mg_item_gd.h"
#include "vdr_actions.h"
#include "mg_setup.h"
#include "vdr_player.h"

int mgLyrics::RunCommand(const string cmd) {
	int res=-1;
	if (access(cmd.c_str(),R_OK|X_OK)) return res;
	char *tmp;
#if VDRVERSNUM < 10504
	const char *backgr="&";
#else
	const char *backgr="";
#endif	
	msprintf(&tmp,"%s \"%s\" \"%s\" \"%s\" %s",cmd.c_str(),
		playItem->getArtist().c_str(),
		playItem->getTitle().c_str(),
		playItem->getCachedFilename("lyrics.tmp").c_str(),
		backgr);
	mgDebug(1,"muggle[%d]: lyrics: executing '%s'\n",getpid (), tmp);
#if VDRVERSNUM >= 10504
	res=SystemExec(tmp,true); // run detached
#else
	res=system(tmp); // SystemExec cannot yet run detached 	
#endif
	free(tmp);
	return res;
}

bool
mgLyrics::HasMoreVersions() {
	struct stat stbuf;
	if (!displayItem)
		return 0;
	return (!stat(displayItem->getCachedFilename("lyrics.tmp.new").c_str(),&stbuf));
}

void
mgLyrics::LoadExternal() {
mgLog("LoadExternal");
	if (!HasMoreVersions())
		osd()->Message1(tr("Loading lyrics from internet..."));
	string script=the_setup.ConfigDirectory + "/scripts/muggle_getlyrics";
	if (RunCommand(script)==0) {
		state=lyricsLoading;
		playItem->setCheckedForTmpLyrics(time(0));
	}
}

void
mgLyrics::SaveExternal() {
	const mgItemGd *item = PlayingItem();
	if (!item) return;
	string local=item->getCachedFilename("lyrics");
	string tmp=item->getCachedFilename("lyrics.tmp");
	PlayerControl()->CurrentItem()->resetHasLyricsFile();
	char *cmd;
	msprintf(&cmd, "mv -f \"%s\" \"%s\" >/dev/null 2>&1 & ", tmp.c_str(), local.c_str());
	mgDebug(1,"muggle[%d]: lyrics: Executing %s\n",getpid(), cmd);
	if (!SystemExec(cmd)) {
		BlueAction=actLoadExternalLyrics;
		SetHelpKeys();
	}
	free(cmd);
}

void
mgLyrics::ThrowTmpAway(const mgItemGd& item) {
	char *cmd;
	msprintf(&cmd,"rm -f \"%s\"",item.getCachedFilename("lyrics.tmp").c_str());
	SystemExec(cmd);
	free(cmd);
	state=lyricsSaved;
}

eOSState
mgLyrics::Process(eKeys key) {
	playItem=mutPlayingItem();
	LyricsState prevstate=state;
	if (displayItem!=playItem && prevstate==lyricsAsking) 
		ThrowTmpAway(displayItem);
	long cl=playItem->getCheckedForTmpLyrics();
	if (displayItem!=playItem || (cl>0 && cl<time(0))) {
		if (!access(playItem->getCachedFilename("lyrics.tmp.loading").c_str(),R_OK)) {
			state=lyricsLoading;
			playItem->setCheckedForTmpLyrics(time(0));
		} else {
			bool normfound=!access(playItem->getCachedFilename("lyrics").c_str(),R_OK);
			if (!access(playItem->getCachedFilename("lyrics.tmp").c_str(),R_OK)) {
				playItem->setCheckedForTmpLyrics(0);
				if (!normfound) {
					SaveExternal();
					state=lyricsSaved;
				} else {
					state=lyricsAsking;
					BuildOsd();
					osd()->Display();
					if (Interface->Confirm(tr("Save this version?"))) {
						SaveExternal();
						state=lyricsSaved;
					}  else {
						if (HasMoreVersions())
							LoadExternal();
						else {
							ThrowTmpAway(playItem);
							state=lyricsSaved;
						}
					}
				}
			} else if (displayItem!=playItem) {
				if (normfound) {
					state=lyricsSaved;
					playItem->setCheckedForTmpLyrics(0);
				} else {
					LoadExternal();
				}
			} else {
				state=lyricsSaved;
				playItem->setCheckedForTmpLyrics(0);
			}
		}
	} 
	if (displayItem!=playItem || state!=prevstate) {
		BuildOsd();
		osd()->Display();
	}
	eOSState result = osContinue;
	switch (key) {
		case kBlue:
			if (BlueAction==actSaveExternalLyrics)
				SaveExternal();
			else if (BlueAction==actLoadExternalLyrics) {
				LoadExternal();
				BuildOsd();
				osd()->Display();
			}
			break;
		default:
			result = mgMenu::Process(key);
			break;
	}
	displayItem=playItem;
	return result;
}

mgLyrics::mgLyrics() {
	playItem=0;
	displayItem=0;
	state=lyricsSaved;
}

string
mgLyrics::Title() const
{
	const mgItemGd *item = PlayingItem();
	if (!item) return "";
	return item->getArtist() + ":" + item->getTitle();
}

void
mgLyrics::BuildOsd () {
	playItem = mutPlayingItem();
	if (!playItem) return;
	RedAction=actBack2;
	GreenAction=actPrevTrack;
	YellowAction=actNextTrack;
	string loadfile="";
	switch (state) {
		case lyricsLoading:
			BlueAction=actNone;
			break;
		case lyricsAsking:
			BlueAction=actLoadExternalLyrics;
			loadfile=playItem->getCachedFilename("lyrics.tmp");
			break;
		default:
			BlueAction=actLoadExternalLyrics;
			loadfile=playItem->getCachedFilename("lyrics");
			break;
	}
	InitOsd();
	if (!access(loadfile.c_str(),R_OK))
		osd()->AddFile(loadfile);
}
