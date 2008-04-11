/*!
 * \file vdr_player.c
 * \brief A generic PCM player for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.7 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author$
 *
 * $Id$
 *
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 * and from the Music plugin (c) Morone
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include <string>
#include <iostream>
#include <fstream>

#include <mad.h>

#include <linux/types.h>		 // this should really be included by linux/dvb/video.h
#include <linux/dvb/video.h>
#include <vdr/player.h>
#include <vdr/device.h>
#include <vdr/remote.h>
#include <vdr/thread.h>
#include <vdr/ringbuffer.h>
#define __STL_CONFIG_H
#include <vdr/tools.h>
#include <vdr/recording.h>
#include <vdr/status.h>
#include <vdr/plugin.h>

#include "vdr_player.h"
#include "vdr_config.h"
#include "mg_setup.h"
#include "mg_skin.h"
#include "common.h"
#include "bitmap.h"
#include "pcmplayer.h"
#include "mg_image_provider.h"

#include "mg_tools.h"

char coverpicture[256];
#define SPAN_PROVIDER_CHECK_ID  "Span-ProviderCheck-v1.0"
#define SPAN_CLIENT_CHECK_ID    "Span-ClientCheck-v1.0"
#define SPAN_SET_PCM_DATA_ID    "Span-SetPcmData-v1.1"
#define SPAN_SET_PLAYINDEX_ID   "Span-SetPlayindex-v1.0"
#define SPAN_GET_BAR_HEIGHTS_ID "Span-GetBarHeights-v1.0"

#include "symbols/shuffle.xpm"
#include "symbols/loop.xpm"
#include "symbols/loopall.xpm"
#include "symbols/stop.xpm"
#include "symbols/play.xpm"
#include "symbols/pause.xpm"
#include "symbols/rew.xpm"
#include "symbols/fwd.xpm"
#include "symbols/copy.xpm"

#if 0
#include "symbols/delstar.xpm"
#include "symbols/rate00.xpm"
#include "symbols/rate05.xpm"
#include "symbols/rate10.xpm"
#include "symbols/rate15.xpm"
#include "symbols/rate20.xpm"
#include "symbols/rate25.xpm"
#include "symbols/rate30.xpm"
#include "symbols/rate35.xpm"
#include "symbols/rate40.xpm"
#include "symbols/rate45.xpm"
#include "symbols/rate50.xpm"
#endif

cBitmap mgPlayerControl::bmShuffle(shuffle_xpm);
cBitmap mgPlayerControl::bmLoop(loop_xpm);
cBitmap mgPlayerControl::bmLoopAll(loopall_xpm);
cBitmap mgPlayerControl::bmStop(stop_xpm);
cBitmap mgPlayerControl::bmPlay(play_xpm);
cBitmap mgPlayerControl::bmPause(pause_xpm);
cBitmap mgPlayerControl::bmRew(rew_xpm);
cBitmap mgPlayerControl::bmFwd(fwd_xpm);
cBitmap mgPlayerControl::bmCopy(copy_xpm);

// ----------------------------------------------------------------

// #define DEBUGPES

class cProgressBar : public cBitmap
{
	public:
		cProgressBar(int Width, int Height, int Current, int Total, int FgColor, int BgColor );
};

// --- mgPlayerControl -------------------------------------------------------

const char
mgPlayerControl::ShuffleChar() {
	char result='.';
	switch (PlayList()->getShuffleMode ()) {
		default:
			break;
		case mgSelection::SM_NORMAL:
			result = 'S';		 // Shuffle mode normal
			break;
		case mgSelection::SM_PARTY:
			result = 'P';		 // Shuffle mode party
			break;
	}
	return result;
}

const char
mgPlayerControl::LoopChar() {
	char result='.';
	switch (PlayList()->getLoopMode ()) {
		default:
			break;
		case mgSelection::LM_SINGLE:
			result = 'S';		 // Loop mode single
			break;
		case mgSelection::LM_FULL:
			result = 'F';		 // Loop mode fuel
			break;
	}
	return result;
}

mgPlayerControl::mgPlayerControl (mgSelection * plist):
cControl (player = new mgPCMPlayer (plist)),
cmdOsd(NULL) {
	osd = 0 ;
	cmdOsd = 0;
	cmdMenu = 0;
	m_img_provider = 0;

	// Notify all cStatusMonitor
	prevItem = 0;
	prevPos=0;
	orderchanged=false;
	layout_initialized=false;
	
	ScrollPosition=-1;
	oneartist=PlayList()->OneArtist();
	jumpactive=jumphide=false;
	skipfwd=skiprew=shown=selecting=selecthide=statusActive=refresh=false;
	showbuttons=0;
	timeoutShow=0;
	messagetime=0;
	lastkeytime=number=timecount=0;
	x1=depth=0;
	framesPerSecond=SecondsToFrames(1);

#if APIVERSNUM >= 10338
	cStatus::MsgReplaying(this,"muggle",0,true);
#else
	cStatus::MsgReplaying(this,"muggle");
#endif
}

mgPlayerControl::~mgPlayerControl () {
	Stop();
	// Notify cleanup all cStatusMonitor
#if VDRVERSNUM >= 10338
	cStatus::MsgReplaying (this, 0, 0, false);
#else
	cStatus::MsgReplaying (this, 0);
#endif

	delete m_img_provider;
	m_img_provider = NULL;
	
	Hide ();
	Stop ();
}

bool mgPlayerControl::Active (void) {
	return player && player->Active ();
}

bool mgPlayerControl::Playing (void) {
	return player && player->Playing ();
}

void
mgPlayerControl::Stop (void) {
#if VDRVERSNUM >= 10338
	cStatus::MsgReplaying( this, 0, 0, false );
#else
	cStatus::MsgReplaying( this, 0);
#endif
	if (player) {
		delete player;
		player = 0;
	}
}

void
mgPlayerControl::Pause (void) {
	if (player) {
		player->Pause ();
	}
}

void
mgPlayerControl::Play (void) {
	if (player) {
		player->Play ();
	}
	ScrollPosition=-1;
}

void
mgPlayerControl::Forward (void) {
	if (player) {
		player->Forward ();
	}
	ScrollPosition=-1;
}

void
mgPlayerControl::Backward (void) {
	if (player) {
		player->Backward ();
	}
	ScrollPosition=-1;
}

void
mgPlayerControl::SkipSeconds(int Seconds) {
	if (player) {
		player->SkipSeconds(Seconds);
	}
}

void
mgPlayerControl::Goto (int Position) {
	if (player) {
		player->Goto (Position);
	}
	ScrollPosition=-1;
}

void
mgPlayerControl::ToggleShuffle (void) {
	if (player) {
		player->ToggleShuffle ();
		orderchanged=true;
	}
}

void
mgPlayerControl::ToggleLoop (void) {
	if (player) {
		player->ToggleLoop ();
		orderchanged=true;
	}
}

void
mgPlayerControl::ReloadPlaylist () {
	if (player) {
		player->ReloadPlaylist ();
		orderchanged=true;
	}
	oneartist=PlayList()->OneArtist();
}

void
mgPlayerControl::NewPlaylist (mgSelection * plist) {
	if (player) {
		player->NewPlaylist (plist);
	}
	ScrollPosition=-1;
	oneartist=PlayList()->OneArtist();
}

void
mgPlayerControl::NewImagePlaylist( const char *directory ) {
	MGLOG ("mgPlayerControl::NewImagePlaylist");

	delete m_img_provider;
	m_img_provider = new mgImageProvider( directory ); // TODO
}



void
mgPlayerControl::InitLayout(void) {
	if (layout_initialized) return;
	layout_initialized=true;
	fw=6;
	fh=27;

	artistfirst = the_setup.ArtistFirst;
	depth                 = 4;
	clrTopBG1             = mgSkin.clrTopBG1;
	clrTopTextFG1         = mgSkin.clrTopTextFG1;

	clrTopBG2             = mgSkin.clrTopBG2;
	clrTopTextFG2         = mgSkin.clrTopTextFG2;
	clrTopItemBG1         = mgSkin.clrTopItemBG1;
	clrTopItemInactiveFG  = mgSkin.clrTopItemInactiveFG;
	clrTopItemActiveFG    = mgSkin.clrTopItemActiveFG;

	clrListBG1            = mgSkin.clrListBG1;
	clrListBG2            = mgSkin.clrListBG2;
	clrListTextFG         = mgSkin.clrListTextFG;
	clrListTextActiveFG   = mgSkin.clrListTextActiveFG;
	clrListTextActiveBG   = mgSkin.clrListTextActiveBG;

	clrInfoBG1            = mgSkin.clrInfoBG1;
	clrInfoBG2            = mgSkin.clrInfoBG2;
	clrInfoTextFG1        = mgSkin.clrInfoTextFG1;
	clrInfoTitleFG1       = mgSkin.clrInfoTitleFG1;
	clrInfoTextFG2        = mgSkin.clrInfoTextFG2;

	clrProgressBG1        = mgSkin.clrProgressBG1;
	clrProgressBG2        = mgSkin.clrProgressBG2;

	clrStatusBG           = mgSkin.clrStatusBG;
	clrStatusRed          = mgSkin.clrStatusRed;
	clrStatusGreen        = mgSkin.clrStatusGreen;
	clrStatusYellow       = mgSkin.clrStatusYellow;
	clrStatusBlue         = mgSkin.clrStatusBlue;
	clrStatusTextFG       = mgSkin.clrStatusTextFG;

	clrProgressbarFG      = clrStatusBG;
	clrProgressbarBG      = clrStatusTextFG;

#ifdef USE_BITMAP
	imgalpha = the_setup.ImgAlpha;
#endif

	rows        = 7;
	osdheight   = Setup.OSDHeight;
	osdwidth    = Setup.OSDWidth;
	mpgdif      = 0;

	lh   = 3*fh +(rows * fh) + fh/2;
	x1 = osdwidth;
	TopHeight = fh - 3; 
	BottomTop=osdheight-TopHeight+1;
	PBHeight=2*fw+10;
	PBTop=BottomTop-PBHeight-10;
	PBBottom=PBTop+PBHeight-1;
	InfoTop=lh;
	InfoBottom = PBTop - 1;
	int imagex1,imagey1,imagex2,imagey2;
	if (the_setup.BackgrMode==1) {
		CoverWidth = PBBottom-lh;
		imagex1=Setup.OSDLeft+CoverX-10;
		imagey1=Setup.OSDTop+InfoTop;
		imagex2=Setup.OSDLeft+CoverX+CoverWidth+3;
		imagey2=BottomTop+Setup.OSDTop-1	;
		CoverX = osdwidth - CoverWidth -3*fw -2;
		CoverX /=4;
		CoverX *=4;
		InfoWidth = CoverX -27 -3*fw;
	} else if (the_setup.BackgrMode==2) {
		imagex1=0;
		imagey1=0;
		imagex2=703; // fix PAL
		imagey2=575;
		CoverWidth=0;
		CoverX = osdwidth;
		CoverX /=4;
		CoverX *=4;
		InfoWidth = CoverX -27- 3*fw;
	}
	if (!m_img_provider) {
		tArea coverarea = { imagex1, imagey1, imagex2, imagey2};
#if USE_BITMAP
		m_img_provider = new mgImageProvider(coverarea);
#else
		m_img_provider = new mgMpgImageProvider(coverarea);
#endif		
	}
}

string
mgPlayerControl::TrackInfo(const mgItemGd* trai) {
	string result,p2;
	if (!trai) return "";
	if (oneartist.size()>0) {
		result=trai->getTitle();
		p2="";
	}
	else if (artistfirst) {
		result =trai->getArtist();
		p2 =trai->getTitle();
	}
	else {
		result =trai->getTitle();
		p2 =trai->getArtist();
	}
	if (result.size()>0 && p2.size()>0) result += " - ";
	result += p2;
	if (result.size()==0)
		result=trai->getSourceFile(false);
	return result;
}

class mgPlayerOsdListItem {
	private:
		const cFont *font;
		int fg;
		int bg;
		string number;
		string text;
		bool changed;
	public:
		mgPlayerOsdListItem() { changed=false;font=0;fg=-1;bg=-1;number="XX";text="fdsfdsafdsaf";}
		const cFont * Font() { return font; }
		int Fg() { return fg; }
		int Bg() { return bg; }
		string Number() { return number; }
		string Text() { return text; }
		void setFont(const cFont *f) { changed|=font!=f;font=f;}
		void setFg(int f) { changed|=fg!=f;fg=f;}
		void setBg(int b) { changed|=bg!=b;bg=b;}
		void setNumber(string n) { changed|=number!=n;number=n; }
		void setText(string t) { changed|=text!=t;text=t; }
		bool Changed() { bool result=changed;changed=false;return result; }
};

void
mgPlayerControl::ShowList(void) {
	int listsize=PlayList()->items().size();
	int middle=ScrollPosition;
	if (middle<0) middle=currPos;
	int top=middle - rows/2;
	if (top+rows>listsize) top=listsize-rows;
	if (top<0) top=0;
	if (!prevItem) {
		for (unsigned int idx=0;idx<pl.size();idx++)
			delete pl[idx];
		pl.clear();
		for (int idx=0;idx<rows;idx++)
			pl.push_back(new mgPlayerOsdListItem);
	}
	if (top!=prevTop || ScrollPosition!=prevScrollPosition||currItem!=prevItem) {
		num=top;
		char *buf;
		msprintf(&buf,tr("%d tracks : %s"),PlayList()->items().size(),oneartist.c_str());
		osd->DrawText( 8*fw, 0 , buf, clrStatusTextFG, clrStatusBG, OsdFont(), 60*fw, fh-2, taLeft);
		free(buf);
		for(int i=0 ; i<rows; i++,num++) {
			mgItemGd *pi=0;
			if (num<listsize)
				pi = dynamic_cast<mgItemGd*>(PlayList()->getItem (num));
			int fg,bg;
			const cFont *font;
			if (ScrollPosition==num) {
				font=BigFont();
				bg=clrListTextActiveFG;
				fg=clrListTextActiveBG;
			}
			else if (pi==currItem) {
				font=BigFont();
				bg=clrListTextActiveBG;
				fg=clrListTextActiveFG;
			}
			else {
				font=cFont::GetFont(fontSml);
				fg=clrListTextFG;
				bg=clrListBG2;
			}
			char number[15];
			snprintf(number,sizeof(number),"%d    ",num+1);
			string info=TrackInfo(pi);
			if (info.size()==0) number[0]=0;
			pl[i]->setFont(font);
			pl[i]->setFg(fg);
			pl[i]->setBg(bg);
			pl[i]->setNumber(number);
			pl[i]->setText(info);
			if (pl[i]->Changed()) {
				osd->DrawEllipse( 5*fw                    , 2*fh + (fh/2) + (i*fh)   , 	(5*fw)+(fh/2) , 2*fh + (fh/2)+ (i*fh)+fh , bg , 7);
				osd->DrawText( (5*fw) +(fh/2) +1          , (2*fh) + (fh/2) + (i*fh) , number , fg , bg , font , 10*fw          , fh , taRight);
				osd->DrawText( (5*fw) +(fh/2) +1 +(10*fw), 2*fh + (fh/2) + (i*fh)   , info.c_str(), fg , bg , font , x1 - (29*fw) -fh -2, fh , taLeft);
				osd->DrawRectangle( 5*fw +fh/2+10*fw+(x1-29*fw)-fh -1, 2*fh+(fh/2)+(i*fh), x1 -(5*fw) -fh/2 +1, 2*fh + (fh/2)+ (i*fh) +fh -1, bg);
				osd->DrawEllipse( x1 -(5*fw) -fh/2        , 2*fh + (fh/2) + (i*fh)   , x1 - (5*fw), 2*fh + (fh/2) +(i*fh) + fh, bg , 5);
				flush=true;
			}
		}
		listtime=time(0);
		prevTop=top;
		prevScrollPosition=ScrollPosition;
	}
}

bool
mgPlayerControl::SetAreas(const char *caller,const tArea *Areas, int NumAreas) {
	eOsdError result = osd->CanHandleAreas(Areas, NumAreas);
	if (result == oeOk) 
		osd->SetAreas(Areas, NumAreas);
	else {
		for (int i=0;i<NumAreas;i++)
				mgDebug(1,"Area %d: x1=%d y1=%d x2=%d y2=%d width=%d height=%d",
					i,Areas[i].x1,Areas[i].y1,Areas[i].x2,Areas[i].y2,Areas[i].Width(),Areas[i].Height());
		const char *errormsg = NULL;
		switch (result) {
			case oeTooManyAreas:
				errormsg = "music: Too many OSD areas"; break;
			case oeTooManyColors:
				errormsg = "music: Too many colors"; break;
			case oeBppNotSupported:
				errormsg = "music: Depth not supported"; break;
			case oeAreasOverlap:
				errormsg = "music: Areas are overlapped"; break;
			case oeWrongAlignment:
				errormsg = "music: Areas not correctly aligned"; break;
			case oeOutOfMemory:
				errormsg = "music: OSD memory overflow"; break;
			case oeUnknown:
				errormsg = "music: Unknown OSD error"; break;
			default:
				break;
		}
		esyslog("muggle: %s: ERROR! osd open failed! can't handle areas (%d)-%s\n", caller, result, errormsg);
		if (osd){ delete osd; osd=0;}
	}
	return result==oeOk;
}

void
mgPlayerControl::Display() {
	ShowProgress();
}

void
mgPlayerControl::ShowProgress (bool open) {
	CheckImage();
	InitLayout();
	int current_frame, total_frames;
	player->GetIndex (current_frame, total_frames);
	if (!osd) {
		osd=cOsdProvider::NewOsd(Setup.OSDLeft, Setup.OSDTop,50);
		if (!osd) return;
		tArea Area[] = {
			{ 0,  0, x1 -1,  TopHeight, 2 },			// border top
			{ 0, fh -2, x1 -1,  2*fh -1, 2 },			// between top and tracklist
#ifdef HAVE_OSDMOD
			{ 0,  2*fh, x1 -1, lh -1, 4 },				// tracklist
			{ 0, lh, CoverX-1, PBTop-1 , 4 },			// Info
#else
			{ 0,  2*fh, x1 -1, lh -1, 2 },				// tracklist
			{ 0, lh, CoverX-1, PBTop-1 , 2 },			// Info
#endif
#ifdef USE_BITMAP
			{ CoverX, lh, x1 - 1, BottomTop-1, depth },		// Cover
#endif
			{ 0, PBTop , CoverX-1, BottomTop-1 , 2 },		// Progressbar
			{ 0, BottomTop , x1 -1, osdheight-1 , 4 },		// Bottom
		};

		if (!SetAreas("ShowProgress",Area,sizeof(Area) / sizeof(tArea)))
			return;

		//top
		
		osd->DrawRectangle(0, 0, x1 - 1, fh -3, clrTopBG1);
		osd->DrawEllipse(0, 0, fh/2 , fh/2,  clrTransparent, -2);
		osd->DrawEllipse(x1 -1 - fh/2, 0, x1 -1 , fh/2, clrTransparent, -1);

		osd->DrawRectangle(0, fh -2,    x1 -1 , 2*fh -1  ,clrTopBG2);

		//tracklist
		osd->DrawRectangle(0, 2*fh  ,  x1 -1 , lh -1 ,clrListBG1);
		osd->DrawRectangle(3*fw, 2*fh ,  x1 - 3*fw , lh - fh/2 -1 ,clrListBG2);
		osd->DrawEllipse(3*fw  , 2*fh , 3*fw + fh/2 , 2*fh  + fh/2, clrListBG1, -2);
		osd->DrawEllipse(x1 - 3*fw - fh/2, 2*fh, x1 - 3*fw , 2*fh + fh/2, clrListBG1, -1);
		osd->DrawEllipse(3*fw             , lh -fh -1, 3*fw + fh/2 , lh - fh/2 -1, clrListBG1, -3);
		osd->DrawEllipse(x1 - 3*fw - fh/2 , lh -fh -1, x1 - 3*fw   , lh - fh/2 -1, clrListBG1, -4);

		//info
		osd->DrawRectangle(0                    , InfoTop               , CoverX-1    , PBBottom ,clrInfoBG1);
								 
		// Cover
#ifdef USE_BITMAP
		osd->DrawRectangle(CoverX     , InfoTop               , x1 -1               , PBBottom ,clrInfoBG1);
#endif
				
		// Info
		osd->DrawRectangle(3*fw, lh + 27 + fh/2   , CoverX-2*fw+1, PBBottom , clrInfoBG2);
		osd->DrawEllipse(3*fw  , lh + 27 + fh/2   , 3*fw + fh/2        , lh + 54, clrInfoBG1 , -2);
		osd->DrawEllipse(CoverX-fw-fh/2+1 , lh + fh + fh/2   , CoverX-2*fw+1    , lh + 2*fh, clrInfoBG1, -1);

		//progressbar

		osd->DrawRectangle(0, PBTop, CoverX - 1 , BottomTop-1 ,clrInfoBG1);
		
		//bottom
								 // border top
		osd->DrawRectangle(0, BottomTop , x1 - 1 , osdheight-1 , clrStatusBG);
		osd->DrawEllipse(0, BottomTop , fh/2 , osdheight -1,  clrTransparent, -3);
		osd->DrawEllipse(x1 -1 - fh/2, BottomTop, x1 -1 , osdheight -1, clrTransparent, -4);

		ShowHelpButtons(showbuttons);
		LoadCover();

		osd->Flush();

		ShowStatus(true);

#if VDRVERSNUM >= 10500
		SetNeedsFastResponse(osd!=0);
#else
		needsFastResponse=true;
#endif
		fliptime=listtime=0; flipint=0; flip=-1; prevTop=-1; lastIndex=lastTotal=-1;
		prevScrollPosition=-1;
		prevItem=0;

	}

	currItem=CurrentItem();
	currPos=player->Position();
	bool changed=(prevItem != currItem);
	char buf[256];

	if (currItem) {				 // send progress to status monitor
		if (changed||orderchanged) {
			snprintf(buf,sizeof(buf),currItem->getArtist().size()?"[%c%c] (%d/%d) %s - %s":"[%c%c] (%d/%d) %s",
				LoopChar(),ShuffleChar(),currPos,PlayList()->items().size(),currItem->getTitle().c_str(),currItem->getArtist().c_str());
#if APIVERSNUM >= 10338
			cStatus::MsgReplaying(this,buf,currItem->getSourceFile().size()?currItem->getSourceFile().c_str():0,true);
#else
			cStatus::MsgReplaying(this,buf);
#endif

		}
	}

		flush=false;

		if (CoverChanged()) {
#ifdef USE_BITMAP
			osd->DrawRectangle(CoverX, lh, x1 -1, PBBottom, clrInfoBG1);
#endif
			LoadCover();
			flush = true;
		}

		const char *lyrics= (CurrentItem() && CurrentItem()->HasLyrics()) ? tr("Lyrics: Yes") : tr("Lyrics: No");
		osd->DrawText( 30, lh + 3*fh + fh/2, lyrics, clrInfoTextFG2, clrInfoBG2, cFont::GetFont(fontSml), 44 * fw, fh, taLeft);

		if (!selecting && changed && !statusActive || refresh) {
			if (ScrollPosition==-1) {
				snprintf(buf,sizeof(buf),"%.1f kHz, %s kbps", currItem->getSampleRate()/1000.0,currItem->getBitrate().c_str());
				osd->DrawText( 30, lh + 2*fh + fh/2 +4, buf, clrInfoTextFG2, clrInfoBG2, SmallFont(), InfoWidth, fh, taLeft);
				flush=true;
			}
			else {
				snprintf(buf,sizeof(buf),"%.1f kHz, %s kbps", currItem->getSampleRate()/1000.0,currItem->getBitrate().c_str());
				osd->DrawText( 30, lh + 2*fh + fh/2 +4, buf, clrInfoTextFG2, clrInfoBG2, cFont::GetFont(fontSml), InfoWidth, fh, taLeft);
				snprintf(buf,sizeof(buf),currItem->getArtist().size()?"%s - %s":"%s%s",currItem->getTitle().c_str(),currItem->getArtist().c_str());
				if (buf[0]) {
					if (!statusActive) {
						DisplayInfo(buf);
						flush=true;
					}
				}
			}
		}

		if (!prevItem || orderchanged) {
			mgSelection::LoopMode lm=PlayList()->getLoopMode();
			if (lm==mgSelection::LM_SINGLE)
				osd->DrawBitmap(x1 - 3*fw - 246, fh , bmLoop, clrTopItemActiveFG, clrTopItemBG1 );
			else if (lm==mgSelection::LM_FULL)
				osd->DrawBitmap(x1 - 3*fw - 246, fh , bmLoopAll, clrTopItemActiveFG, clrTopItemBG1 );
			else
				osd->DrawBitmap(x1 - 3*fw - 246, fh , bmLoop, clrTopBG2, clrTopBG2);
			flush=true;
		}

		if (!prevItem || orderchanged) {
			if (PlayList()->getShuffleMode()!=mgSelection::SM_NONE)
				osd->DrawBitmap(x1 - 3*fw - 216, fh , bmShuffle, clrTopItemActiveFG, clrTopItemBG1);
			else
				osd->DrawBitmap(x1 - 3*fw - 216, fh , bmShuffle, clrTopBG2, clrTopBG2);
			flush=true;
		}

		switch(player->PlayMode()) {
			case pmStopped:
				osd->DrawBitmap(osdwidth - 3*fw - 160, fh , bmStop, clrTopItemActiveFG, clrTopItemBG1);
				osd->DrawBitmap(osdwidth - 3*fw - 130, fh , bmPlay, clrTopItemInactiveFG, clrTopItemBG1);
				osd->DrawBitmap(osdwidth - 3*fw - 100, fh , bmPause, clrTopItemInactiveFG, clrTopItemBG1);
				break;
			case pmPlay:
				osd->DrawBitmap(osdwidth - 3*fw - 160, fh , bmStop, clrTopItemInactiveFG, clrTopItemBG1);
				osd->DrawBitmap(osdwidth - 3*fw - 130, fh , bmPlay, clrTopItemActiveFG, clrTopItemBG1);
				osd->DrawBitmap(osdwidth - 3*fw - 100, fh , bmPause, clrTopItemInactiveFG, clrTopItemBG1);
				break;
			case pmPaused:
				osd->DrawBitmap(osdwidth - 3*fw - 160, fh , bmStop, clrTopItemInactiveFG, clrTopItemBG1);
				osd->DrawBitmap(osdwidth - 3*fw - 130, fh , bmPlay, clrTopItemInactiveFG, clrTopItemBG1);
				osd->DrawBitmap(osdwidth - 3*fw - 100, fh , bmPause, clrTopItemActiveFG, clrTopItemBG1);
			case pmStartup:
				break;
		}

		if (skiprew)
			osd->DrawBitmap(osdwidth - 3*fw - 70, fh , bmRew, clrTopItemActiveFG, clrTopItemBG1);
		else
			osd->DrawBitmap(osdwidth - 3*fw - 70, fh , bmRew, clrTopItemInactiveFG, clrTopItemBG1);

		if (skipfwd)  osd->DrawBitmap(osdwidth - 3*fw - 40, fh , bmFwd, clrTopItemActiveFG, clrTopItemBG1);
		else
			osd->DrawBitmap(osdwidth - 3*fw - 40, fh , bmFwd, clrTopItemInactiveFG, clrTopItemBG1);

		osd->DrawText( 8*fw, lh + fh/2 -fw, tr("Now playing :"), clrInfoTextFG1, clrInfoBG1, cFont::GetFont(fontOsd), (46*fw), fh, taLeft);
		flush=true;

		//Volume
		snprintf(buf,sizeof(buf),"%s: %d", tr("Volume"), CurrentVolume());
		osd->DrawText( 30, lh + 4*fh + fh/2 -4, buf , clrInfoTextFG2, clrInfoBG2, cFont::GetFont(fontSml), 0, fh, taLeft);

		current_frame/=SecondsToFrames(1); total_frames/=SecondsToFrames(1);
		if (current_frame!=lastIndex || total_frames!=lastTotal) {
			if (total_frames>0) {
				int my_left=3*fw;
				int my_right=CoverX - 11; //2*fw+1;
				int my_top=InfoBottom+1;
				int pb_left=my_left+fh/2;
				int pb_bottom=PBTop+2*fw+9;
				int pb_right=my_right-fh/2;
				int pb_width=pb_right-pb_left+1;
				int pb_height=pb_bottom-PBTop+1;
			//	osd->DrawRectangle( 7*fw, my_top, my_right - 7*fw, my_top + fh, clrInfoBG2);
				osd->DrawEllipse(my_left, my_top, pb_left, pb_bottom, clrProgressbarFG, 3);
				cProgressBar ProgressBar(pb_width, pb_height, current_frame, total_frames,
					clrProgressbarFG , clrProgressbarBG);
				osd->DrawBitmap(pb_left, my_top, ProgressBar);
				osd->DrawEllipse(pb_right+1, my_top, my_right, pb_bottom,  clrProgressbarFG,4);	
			}
			flush=true;
		}

		if (!jumpactive) {
			//TODO        if (!scrollmode) {
			if (true) {
				bool doflip=false;
				if (!prevItem || orderchanged)
					doflip=true;
				if (!currItem || changed) {
					fliptime=time(0); flip=0;
					doflip=true;
				}
				else if (time(0)>fliptime+flipint) {
					fliptime=time(0);
					flip++; if (flip>=the_setup.DisplayMode) flip=0;
					doflip=true;
				}
				if (doflip) {
					buf[0]=0;

					switch(flip) {
						default:
							flip=0;
						case 0:
							snprintf(buf,sizeof(buf),"%s",currItem->getTitle().c_str());
							flipint=6;
							break;
						case 1:
							if (currItem->getArtist().size()) {
								snprintf(buf,sizeof(buf),"%s: %s",tr("Artist"),currItem->getArtist().c_str());
								flipint=4;
							}
							else fliptime = 0;
							break;
						case 2:
						{
							const char *a=currItem->getAlbum().c_str();
							if (a[0] && strcmp(a,"Unassigned")) {
								snprintf(buf,sizeof(buf),"Album: %s",a);
								flipint=4;
							}
							else fliptime=0;
						}
						break;
						case 3:
							if (currItem->getGenre().size()) {
								snprintf(buf,sizeof(buf),"%s: %s",tr("Genre"), currItem->getGenre().c_str());
								flipint=3;
							}
							else fliptime=0;
							break;
						case 4:
							if (currItem->getYear() > 2) {
								snprintf(buf,sizeof(buf),"%s: %d",tr("Year"), currItem->getYear());
								flipint=3;
							}
							else fliptime=0;
							break;
					}
					if (buf[0]) {
						if (!statusActive) {
							DisplayInfo(buf);
							flush=true;
						}
						else { mgDebug(5,"music: ctrl: displayinfo skipped due to status active!\n"); }
					}
				}
			}
		}

		ShowList();
		if (flush) Flush();
	skiprew=0;
	skipfwd=0;
	lastIndex=current_frame; lastTotal=total_frames;
	prevPos=currPos;
	prevItem=currItem;
	refresh=false;
	orderchanged=false;
}

void
mgPlayerControl::Hide () {
	HideStatus();
#if 0
	if (cmdMenu) {
		delete cmdMenu;
		cmdMenu=0;
	}
#endif
	if (cmdOsd) {
		delete cmdOsd;
		cmdOsd=0;
	}

#if 0
	if (rateMenu) {
		delete rateMenu;
		rateMenu=0;
	}
#endif
	HidePlayOsd();
}

void
mgPlayerControl::HidePlayOsd() {
	delete osd; osd=0;
}

void
mgPlayerControl::Scroll(int by) {
	int listsize=PlayList()->items().size();

	if (ScrollPosition<0)
		ScrollPosition=player->Position();
	ScrollPosition += by;
	if (ScrollPosition<0) ScrollPosition=0;
	if (ScrollPosition>listsize-1)
		ScrollPosition=listsize-1;
}

void
mgPlayerControl::ShowCommandMenu() {
	Hide();
	cmdOsd = new mgPlayOsd;
	cmdMenu = new mgPlayerCommands;
	cmdOsd->AddMenu(cmdMenu);
	cmdOsd->Display();
}

eOSState mgPlayerControl::ProcessKey(eKeys Key) {
	if (m_img_provider) {
		if (m_img_provider->updateItem(CurrentItem())) {
			// if the images are cached, we want them ASAP
			m_imageshowtime = 0;
		}
		CheckImage();
	}
	switch(Key) {
		case kFastRew:
		case kFastRew|k_Repeat:
			player->SkipSeconds(-the_setup.Jumptime);
			skiprew=1;
			return osContinue;
		case kFastFwd:
		case kFastFwd|k_Repeat:
			skipfwd=1;
			player->SkipSeconds(the_setup.Jumptime);
			return osContinue;
		case kPlay:
			player->Play();
			return osContinue;
		case kPrev:
		case kPrev|k_Repeat:
			Backward();
			return osContinue;
		case kNext:
		case kNext|k_Repeat:
			Forward();
			return osContinue;
		case kPause:
			Pause();
			return osContinue;
		case kStop:
			Hide();
			Stop();
			return osEnd;
		default:
			break;
	}
	
	if (cmdOsd) {
		eOSState st=cmdOsd->ProcessKey(Key);
		if (st==osBack) {
			delete cmdOsd; cmdOsd=0;
			return osContinue;
		} else if (st==osContinue)
		return osContinue;
	}

	if (!player->Active()) return osEnd;

	if (timecount>0)  timecount--;

	if (messagetime) {
		if (time(0)-messagetime<Setup.OSDMessageTime && Key==kNone) {
			return osContinue;
		}
		else {
			HidePlayOsd();
			messagetime=0;
		}
	}
	else if (timecount == 0) {
		Display();
	}

	ShowStatus(Key==kNone);

	if (jumpactive && Key!=kNone) { JumpProcess(Key); return osContinue; }

	if (cmdOsd) {

		eOSState eOSRet = cmdOsd->ProcessKey(Key);
		switch(eOSRet) {
			case kRed:
			case osBack:
				delete cmdOsd;
				cmdOsd = NULL;
				Display();

				return osContinue;
			default: return eOSRet;
		}
	}
#if 0
	else if (false) {			 // war rateMenu
		eOSState eOSRet = rateMenu->ProcessKey(Key);
		switch(eOSRet) {
			case kRed:
			case osBack:
				delete rateMenu;
				rateMenu = NULL;
				Display();

				return osContinue;
			default: return eOSRet;
		}
	}
	else
#endif
	{

		switch(Key) {

			case kMenu:
			case kUp:
			case kUp|k_Repeat:
				Scroll(-1);
				break;
			case kDown:
			case kDown|k_Repeat:
				Scroll(1);
				break;

			case kLeft:
			case kLeft|k_Repeat:
				Scroll(-rows);
				break;

			case kRight:
			case kRight|k_Repeat:
				Scroll(rows);
				break;

			case kRed:

				// MENUE  OK
				if (showbuttons==0) {
					ShowCommandMenu();
				}
				else
					// BACK  OK
				if (showbuttons==1) {
					ShowHelpButtons(0);
					ShowCommandMenu();
					ShowHelpButtons(0);
				}
				break;

			case kGreen:
			case kGreen|k_Repeat:
				// TRACK-  OK
				if (showbuttons==0) {
					Backward();
				}
				break;

			case kYellow:
			case kYellow|k_Repeat:
				if (showbuttons==0) {
					Forward();
				}
				else
					// JUMP
				if (showbuttons==1) {
					ShowHelpButtons(3);
					Jump();
				}
				else
					// COPY  OK
				if (showbuttons==2) {
					ShowHelpButtons(0);
				}
				break;

			case kBlue:			 //OK
				if (showbuttons==0) {
					ShowHelpButtons(1);
				}
				else
				if (showbuttons==1) {
					ShowHelpButtons(2);
				}
				else
				if (showbuttons==2) {
					ShowHelpButtons(0);
				}
				break;

			case kOk:
				refresh = true;
				if (ScrollPosition>=0) Goto(ScrollPosition);
				Display();
				break;

			case k0 ... k9:
			{
				int listsize=PlayList()->items().size();
				number=number*10+Key-k0;
				if (prevItem && number>0 && number<listsize) {
					if (!osd) { ShowTimed(); selecthide=true; }
					selecting=true; lastkeytime=time_ms();
					char buf[32];
					snprintf(buf,sizeof(buf),"%s %d- %s %d","No.",number,tr("of"),listsize);
					osd->DrawText( 5*fw, lh + 3*fh + fh/2, buf, clrInfoTextFG2, clrInfoBG2, SmallFont(), 40 * fw , fh, taLeft);
					Flush();
					break;
				}
				number=0; lastkeytime=0;
			}
			case kNone:

				if (selecting && time_ms()-lastkeytime>SELECT_TIMEOUT) {
					if (number>0) Goto(number);
					if (selecthide) timeoutShow=time(0)+SELECTHIDE_TIMEOUT;
					number=0; selecting=selecthide=false;
				}

				break;

			case kBack:
				if (ScrollPosition>=0) {
					ScrollPosition=player->Position();
					Display();

				}
				else {
					Hide();
#if APIVERSNUM >= 10332
					cRemote::CallPlugin("muggle");
					return osBack;
#else
					return osEnd;
#endif
				}
			default:
				return osUnknown;
		}

		return osContinue;
	}
}

void mgPlayerControl::Flush(void) {
	if (osd) osd->Flush();
}

int mgPlayerControl::CurrentVolume(void) {
	return cDevice::PrimaryDevice()->CurrentVolume();
}

const cFont *
mgPlayerControl::OsdFont(void) {
	return cFont::GetFont(fontOsd);
}

const cFont *
mgPlayerControl::BigFont(void) {
#ifdef IM_A_MORON
	return cFont::GetFont(fontBig);
#else
	return cFont::GetFont(fontOsd);
#endif
}

const cFont *
mgPlayerControl::SmallFont(void) {
	return cFont::GetFont(fontSml);
}

void mgPlayerControl::DisplayInfo(const char *s) {
	if (osd)
		osd->DrawText( 30, lh + fh +fh/2 +fw, s, s?clrInfoTitleFG1:clrInfoBG2, clrInfoBG2, BigFont(), InfoWidth, fh, taLeft);
}

// --- cAsyncStatus ------------------------------------------------------------
#include "menu-async.h"
cAsyncStatus asyncStatus;

cAsyncStatus::cAsyncStatus(void) {
	text=0;
	changed=false;
}

cAsyncStatus::~cAsyncStatus() {
	free((void *)text);
}

void cAsyncStatus::Set(const char *Text) {
	Lock();
	free((void *)text);
	text=Text ? strdup(Text) : 0;
	changed=true;
	Unlock();
}

const char *cAsyncStatus::Begin(void) {
	Lock();
	return text;
}

void cAsyncStatus::Finish(void) {
	changed=false;
	Unlock();
}

void mgPlayerControl::ShowStatus(bool force) {

	//TODO if (cmdOsd) return;
	InitLayout();
	if ((asyncStatus.Changed() || (force && !statusActive))) {
		const char *text=asyncStatus.Begin();
		if (text) {
			if (!statusActive) { osd->SaveRegion( 30, lh +fh +fh/2 +fw, InfoWidth, lh + 3*fh +fw); }
			osd->DrawText(30, lh + fh +fh/2 +fw, text, clrInfoTitleFG1, clrInfoBG2, OsdFont(), InfoWidth, fh, taCenter);
			osd->Flush();
			statusActive=true;
		}
		else
			HideStatus();
		asyncStatus.Finish();
	}

}

void mgPlayerControl::HideStatus(void) {
	if (!osd)
		return;
	if (statusActive) {
		osd->RestoreRegion();
		osd->Flush();
	}
	statusActive=false;
}

bool mgPlayerControl::CoverChanged(void) {
	return strcmp(coverpicture,m_current_imagesource.c_str());
}

void mgPlayerControl::LoadCover(void) {
	if (!player) return;
	if (!m_current_imagesource.size()) return;
	strcpy(coverpicture,m_current_imagesource.c_str());

	fw=6;
	fh=27;

#if USE_BITMAP
	int bmpcolors = 15; // TODO xine can handle 256
	int w1 = CoverWidth;
	int h1 = CoverWidth;
	cMP3Bitmap *bmp;
	if ((bmp = cMP3Bitmap::Load(coverpicture, imgalpha, h1, w1, bmpcolors)) !=NULL) {
		osd->DrawRectangle(CoverX, lh, osdwidth -1, PBBottom, clrInfoBG1);
		osd->DrawBitmap(CoverX , PBBottom -CoverWidth, bmp->Get(), clrTransparent, clrTransparent, true);
	}
#endif
	
#ifdef HAVE_TUNED_GTFT
	cPlugin *graphtft=cPluginManager::GetPlugin("graphtft");
	if (graphtft) cStatus::MsgImageFile(coverpicture);
#else
	cPlugin *graphtft=cPluginManager::GetPlugin("graphtft");
	if (graphtft) graphtft->SetupParse("CoverImage", coverpicture);
#endif

}

void mgPlayerControl::ShowHelpButtons(int ShowButtons) {
	if (!osd) return;
	int tab;
	InitLayout();
	tab = Setup.OSDWidth/4;
	showbuttons = ShowButtons;

	osd->DrawEllipse(      14, BottomTop + 6,         28 , BottomTop +20, clrStatusRed, 0);
	osd->DrawEllipse(  tab+14, BottomTop + 6,   tab + 28 , BottomTop +20, clrStatusGreen, 0);
	osd->DrawEllipse(2*tab+14, BottomTop + 6, 2*tab + 28 , BottomTop +20, clrStatusYellow, 0);
	osd->DrawEllipse(3*tab+14, BottomTop + 6, 3*tab + 28 , BottomTop +20, clrStatusBlue, 0);
	switch(showbuttons) {
		case 0:
			osd->DrawText(        30, BottomTop, tr("Commands"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText(  tab + 30, BottomTop, tr("Track-"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 2*tab +30, BottomTop, tr("Track+"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 3*tab +30, BottomTop, tr("More.."), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			break;
		case 1:
// red was rating, green was cover:
			osd->DrawText(        30, BottomTop, tr("Commands"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText(  tab + 30, BottomTop, "", clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 2*tab +30, BottomTop, tr("Jump"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 3*tab +30, BottomTop, tr("Parent"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			break;
		case 2:
			osd->DrawText(        30, BottomTop, tr("Delete"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText(  tab + 30, BottomTop, tr("Clear"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 2*tab +30, BottomTop, tr("Copy"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 3*tab +30, BottomTop, tr("Parent"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			break;
		case 3:
			osd->DrawText(        30, BottomTop, tr("Parent"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText(  tab + 30, BottomTop, "<<", clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 2*tab +30, BottomTop, ">>", clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			osd->DrawText( 3*tab +30, BottomTop, tr("Min/Sec"), clrStatusTextFG, clrStatusBG, cFont::GetFont(fontSml), 14*fw, fh, taLeft);
			break;
	}
}

// --- cProgressBar ------------------------------------------------------------

cProgressBar::cProgressBar(int Width, int Height, int Current, int Total, int fgColor, int BgColor)
:cBitmap(Width, Height, 2) {
	char buf[256];
	cBitmap txt(Width,Height,2);
	if (Total > 0) {
		int p = Current * Width / Total;;
		snprintf(buf,sizeof(buf),Total?"%s: %02d:%02d %s %02d:%02d":"%s: %02d:%02d ",
			tr("Time"),Current/60,Current%60, "-",Total/60,Total%60);
		txt.DrawText(0,0,buf,fgColor,BgColor,cFont::GetFont(fontOsd),Width,Height,taCenter);
		DrawRectangle(0, 0, p, Height, fgColor);
		DrawRectangle(p + 1, 0, Width, Height, BgColor);
		for (int x=0;x<Width;x++) {
			for (int y=0;y<Height;y++) {
				tIndex newcolor= *(txt.Data(x,y)) ^ *Data(x,y);
				SetIndex(x,y,newcolor);
			}
		}
	}
}

void mgPlayerControl::JumpDisplay(void) {
	char buf[64];
	const char *j=tr("Jump: "), u=jumpsecs?'s':'m';
	if(!jumpmm)
		sprintf(buf, "%s- %c", j,u);
	else
		sprintf(buf,"%s%d- %c",j,jumpmm,u);

	DisplayInfo(buf);
}

void mgPlayerControl::JumpProcess(eKeys Key) {
	int n=Key-k0, d=jumpsecs?1:60;
	switch(Key) {
		case k0 ... k9:
			if(jumpmm*10+n <= lastTotal/d) jumpmm=jumpmm*10+n;
			JumpDisplay();
			break;
		case kBlue:
			jumpsecs=!jumpsecs;
			JumpDisplay();
			break;
		case kPlay:
		case kUp:
			jumpmm-=lastIndex/d;
			// fall through
		case kFastRew:
		case kFastFwd:
		case kGreen:
		case kYellow:
			player->SkipSeconds(jumpmm*d * ((Key==kGreen) ? -1:1));
			// fall through
		default:
			jumpactive=false;
			break;
	}

	if(!jumpactive) {
		if(jumphide) Hide();
		ShowHelpButtons(0);
	}
}

void mgPlayerControl::Jump(void) {
	jumpmm=1; jumphide=jumpsecs=false;
	if(!osd) {
		ShowTimed(); if(!osd) return;
		jumphide=true;
	}
	JumpDisplay();
	jumpactive=true; fliptime=0; flip=-1;
}

void mgPlayerControl::ShowTimed(int Seconds) {
	if(!osd) {
		ShowProgress(true);
		if(Seconds>0) timeoutShow=time(0)+Seconds;
	}
}

mgSelection *
mgPlayerControl::PlayList() {
	if (!player) return 0;
	return player->getPlaylist();
}

mgItemGd *
mgPlayerControl::CurrentItem(void) {
	if (!player) return 0;
	return dynamic_cast<mgItemGd*>(player->getPlaylist()->getCurrentItem ());
}

void
mgPlayerControl::CheckImage() {
	if (!m_img_provider)
		return;
#ifdef USE_BITMAP
	if (cmdOsd) return;
#endif
	if (m_img_provider) {
		if (time(0)-m_imageshowtime >= the_setup.ImageShowDuration) {
								 // all n decoding steps
			m_current_image = m_img_provider->getImagePath(m_current_imagesource);

			// check for TFT display of image
			if ( !m_current_imagesource.empty() ) {
					TransferImageTFT( m_current_imagesource );
			}

			if ( !m_current_image.empty() )
				player->ShowMPGFile(m_current_image);
			m_imageshowtime=time(0);
		}
	}
}

void
mgPlayerControl::TransferImageTFT( string cover ) {
	cPlugin * graphtft = cPluginManager::GetPlugin("graphtft");
	if ( graphtft ) {
		graphtft->SetupParse( "CoverImage", cover.c_str() );
	}
}
