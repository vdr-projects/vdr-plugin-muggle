/*!
 * \file vdr_player.h
 * \brief A player/control combination to let VDR play music
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * $Id$
 *
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 * 
 * and from the Music plugin (c) Morone
 */

#ifndef ___VDR_PLAYER_H
#define ___VDR_PLAYER_H

#include <vdr/player.h>
#include "mg_selection.h"
#include "mg_item_gd.h"
#include "mg_playcommands.h"

#if VDRVERSNUM >= 10307
class cOsd;
#endif

extern char coverpicture[256];
// -------------------------------------------------------------------

class mgPCMPlayer;
class mgImageProvider;

// -------------------------------------------------------------------

class mgPlayerOsdListItem;
/*!
 *  \brief exerts control over the player itself
 *
 *  This control is launched from the main menu and manages a link
 *  to the player. Key events are caught and signaled to the player.
 */
class mgPlayerControl:public cControl, cStatus
{
	private:

		//! \brief the reference to the player
		mgPCMPlayer * player;

		// copied from music plugin, mp3control.h:
		cOsd *osd;
		string imagefile;
		string _message;
		int fw, fh;
		//

		mgPlayOsd *cmdOsd;
		mgPlayerCommands *cmdMenu;

		bool shown, statusActive, refresh, flush, skiprew, skipfwd;
		time_t timeoutShow, greentime, oktime;
		time_t messagetime;
		int num, number;
		int lastkeytime, playstatus, timecount;
		bool selecting, selecthide, artistfirst;
		//
		mgItemGd * currItem;
		mgItemGd * prevItem;
		unsigned int currPos;
		unsigned int prevPos;
		bool orderchanged;
		time_t fliptime, listtime;
		time_t flushtime;
		int rows;
		int flip, flipint, osdwidth, osdheight, lh, showbuttons;
		int x0, x1, coverdepth, listdepth,CoverX, CoverWidth, TopHeight,BottomTop,PBTop,PBHeight,PBBottom;
		int InfoTop,InfoWidth,InfoBottom;
		int lastIndex, lastTotal, prevTop, prevScrollPosition;
		int framesPerSecond;

		int ScrollPosition;

		//
		bool jumpactive, jumphide, jumpsecs;
		int jumpmm, channelsSA, bandsSA, visualization;
		//
		static cBitmap bmShutdown , bmShuffle , bmLoop , bmLoopAll, bmStop , bmPlay , bmPause , bmRew , bmFwd , bmCopy;

		static cBitmap bmDelStar,bmRate00,bmRate05,bmRate10,bmRate15,bmRate20,bmRate25,bmRate30,bmRate35,bmRate40,bmRate45,bmRate50;

		// , bmRec;
		//
		int clrTopBG1;
		int clrTopTextFG1;
		int clrTopBG2;
		int clrTopTextFG2;
		int clrTopItemBG1;
		int clrTopItemInactiveFG;
		int clrTopItemActiveFG;
		int clrListBG1;
		int clrListBG2;
		int clrListTextFG;
		int clrListTextActiveFG;
		int clrListTextActiveBG;
		int clrInfoBG1;
		int clrInfoBG2;
		int clrInfoTextFG1;
		int clrInfoTitleFG1;
		int clrInfoTextFG2;
		int clrProgressBG1;
		int clrProgressBG2;
		int clrProgressbarFG;
		int clrProgressbarBG;
		int clrStatusBG;
		int clrStatusRed;
		int clrStatusGreen;
		int clrStatusYellow;
		int clrStatusBlue;
		int clrStatusTextFG;
#ifdef USE_BITMAP
		int imgalpha;
#endif
		int mpgdif;
		bool layout_initialized;
		//
		void InitLayout(void);
		void ShowList(void);
		void ShowTimed(int Seconds=0);
		void ShowStatus(bool force);
		void HideStatus(void);
		void HidePlayOsd(void);
		void ShowHelpButtons(int ShowButtons);
		void ShowProgress(bool open=false);
		void Display();
		void DisplayInfo(const char *s=0);
		void JumpDisplay(void);
		void JumpProcess(eKeys Key);
		void Jump(void);
		void LoadCover(void);
		void CopyTrack(void);
		inline void Flush(void);
		void ShutDown(void);
		int CurrentVolume(void);
		bool LoadImage(const char* fullname,bool coveronly);
		char Songname[256];
		bool CoverChanged();
		mgSelection *PlayList(void);
		string TrackInfo(const mgItemGd* item);
		const cFont * OsdFont(void);
		const cFont * BigFont(void);
		const cFont * SmallFont(void);
		void DumpAreas(const char *caller,const tArea *Areas, int NumAreas,eOsdError err);
		bool SetAreas(const char *caller,const tArea *Areas, int NumAreas);
		tArea * ProgressAreas(int& NumAreas);
		void Scroll(int by);
		void ShowCommandMenu();

		const char ShuffleChar();
		const char LoopChar();

		//! \brief the image provider
		mgImageProvider *m_img_provider;
		void CheckImage( );
		void TransferImageTFT( string cover );
		// background image handling stuff
		int m_lastshow;
		string m_current_image;
		string m_current_imagesource;
		time_t m_imageshowtime;
		string oneartist;

		vector<mgPlayerOsdListItem*> pl;

	public:

		mgItemGd * CurrentItem(void);

		/*! \brief construct a control with a playlist
		 *
		 *  \param plist - the playlist to be played
		 */
		mgPlayerControl (mgSelection * plist);

		/*! \brief destructor
		 */
		virtual ~ mgPlayerControl ();

		//! \brief indicate whether the corresponding player is active
		bool Active ();

		//! \brief indicate whether the corresponding player is playing
		bool Playing ();

		//! \brief stop the corresponding player
		void Stop ();

		//! \brief toggle the pause mode of the corresponding player
		void Pause ();

		//! \brief start playing
		void Play ();

		//! \brief skip to the next song
		void Forward ();

		//! \brief skip to the previous song
		void Backward ();

		/*! \brief skip a specified number of seconds
		 *
		 *  \param seconds - the number of seconds to skip
		 */
		void SkipSeconds (int seconds);

		/*! \brief goto a certain position in the playlist
		 *
		 *  \param index - the position in the playlist to skip to
		 */
		void Goto (int index);

		//! \brief toggle the shuffle mode of the corresponding player
		void ToggleShuffle ();

		//! \brief toggle the loop mode of the corresponding player
		void ToggleLoop ();

		/*! \brief tell the player to reload the play list.
		 * This is needed if we play a collection
		 * and the user changed the collection while playing it
		 */
		void ReloadPlaylist();

		/*! \brief signal a new playlist
		 *
		 *  The caller has to take care of deallocating the previous list
		 *
		 *  \param plist - the new playlist to be played
		 */
		void NewPlaylist (mgSelection * plist);

		/*! \brief signal a new image playlist
		 *
		 *  A directory is passed and all images (.jpg, .png) contained in this dir are to be displayed during playback.
		 *
		 *  \param directory - the directory containing images to be replayed
		 */
		void NewImagePlaylist (const char *directory);


		//! \brief a progress display
		void ShowContents ();

		//! \brief hide the osd, if present
		void Hide ();

		//! \brief process key events
		eOSState ProcessKey (eKeys key);

};
#endif							 //___VDR_PLAYER_H
