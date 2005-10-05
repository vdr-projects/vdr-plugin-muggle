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
 */

#ifndef ___VDR_PLAYER_H
#define ___VDR_PLAYER_H

#include <player.h>
#include "mg_selection.h"
#if VDRVERSNUM >= 10307
class cOsd;
#endif

// -------------------------------------------------------------------

class mgPCMPlayer;

// -------------------------------------------------------------------

/*!
 *  \brief exerts control over the player itself
 *
 *  This control is launched from the main menu and manages a link
 *  to the player. Key events are caught and signaled to the player.
 */
class mgPlayerControl:public cControl
{
    private:

//! \brief the reference to the player
        mgPCMPlayer * player;

//! \brief indicates, whether the osd should be visible
        bool m_visible;

//! \brief indicates, whether an osd is currently displayed
        bool m_has_osd;

        bool m_track_view;
        bool m_progress_view;

#if VDRVERSNUM >= 10307
//! \brief a replay display to show the progress during playback
        cSkinDisplayReplay *m_display;
        cSkinDisplayMenu *m_menu;

        cOsd *osd;
        const cFont *font;
#endif

//! \brief Last Message for Statusmonitor
        char *m_szLastShowStatusMsg;

    public:

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
 *  \param still - currently unused
 *  \todo Goto is currently unused and has an obvious memory leak
 */
        void Goto (int index, bool still = false);

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
        void ShowProgress ();

        void Display ();

        void ShowContents ();

//! \brief hide the osd, if present
        void Hide ();

//! \brief hide the osd, if present
        void InternalHide ();

//! \brief process key events
        eOSState ProcessKey (eKeys key);

    protected:
//! \brief signal a played file to any cStatusMonitor inside vdr
        void StatusMsgReplaying ();
};
#endif                                            //___VDR_PLAYER_H
