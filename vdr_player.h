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


#ifndef ___DVB_MP3_H
#define ___DVB_MP3_H

#include <player.h>
#if VDRVERSNUM >= 10307
class cOsd;
#endif

// -------------------------------------------------------------------

class mgPCMPlayer;
class mgPlaylist;

// -------------------------------------------------------------------

/*! 
 *  \brief exerts control over the player itself
 *
 *  This control is launched from the main menu and manages a link
 *  to the player. Key events are caught and signaled to the player.
 */
class mgPlayerControl : public cControl
{
private:

  //! \brief the reference to the player
  mgPCMPlayer *m_player;

  //! \brief indicates, whether the osd should be visible
  bool m_visible;

  //! \brief indicates, whether an osd is currently displayed
  bool m_has_osd;
#if VDRVERSNUM >= 10307
  cOsd *osd;
  const cFont *font;
#endif

public:

  /*! \brief construct a control with a playlist
   *
   *  \param plist - the playlist to be played
   */
  mgPlayerControl(mgPlaylist *plist);

  /*! \brief destructor
   */
  virtual ~mgPlayerControl();

  //! \brief indicate, whether the corresponding player is active
  bool Active();

  //! \brief stop the corresponding player
  void Stop();

  //! \brief toggle the pause mode of the corresponding player
  void Pause();

  //! \brief start playing
  void Play();

  //! \brief skip to the next song
  void Forward();

  //! \brief skip to the previous song
  void Backward();

  /*! \brief skip a specified number of seconds
   *
   *  \param seconds - the number of seconds to skip
   */
  void SkipSeconds(int seconds);

  /*! \brief goto a certain position in the playlist
   *
   *  \param index - the position in the playlist to skip to
   *  \param still - currently unused
   */
  void Goto(int index, bool still = false);

  //! \brief toggle the shuffle mode of the corresponding player
  void ToggleShuffle();

  //! \brief toggle the loop mode of the corresponding player
  void ToggleLoop();

  /*! \brief signal a new playlist
   *
   *  The caller has to take care of deallocating the previous list
   *
   *  \param plist - the new playlist to be played
   */
  void NewPlaylist( mgPlaylist *plist );

  //! \brief a progress display
  void ShowProgress();

  //! \brief hide the osd, if present
  void Hide();

  //! \brief process key events
  eOSState ProcessKey(eKeys key);
};

#endif //___DVB_MP3_H

/************************************************************
 *
 * $Log: vdr_player.h,v $
 * Revision 1.2  2004/05/28 15:29:19  lvw
 * Merged player branch back on HEAD branch.
 *
 * Revision 1.1.2.9  2004/05/25 06:48:24  lvw
 * Documentation and code polishing.
 *
 * Revision 1.1.2.8  2004/05/25 00:10:45  lvw
 * Code cleanup and added use of real database source files
 *
 * Revision 1.1.2.7  2004/05/11 06:35:16  lvw
 * Added debugging while hunting stop bug.
 *
 * Revision 1.1.2.6  2004/05/07 06:46:41  lvw
 * Removed a bug in playlist deallocation. Added infrastructure to display information while playing.
 *
 *
 ***********************************************************/
