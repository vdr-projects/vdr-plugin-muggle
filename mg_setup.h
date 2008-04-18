/*!
 * \file vdr_setup.h
 * \brief A setup class for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-01-24 15:45:30 +0100 (Mon, 24 Jan 2005) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: wr61 $
 *
 * $Id: vdr_setup.h 399 2005-01-24 14:45:30Z wr61 $
 *
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___SETUP_MG_H
#define ___SETUP_MG_H

#include <string>

using namespace std;

#define MAX_STRING_LEN 128

#define DEFAULT_TARGET_LEVEL  25
#define MAX_TARGET_LEVEL      50
#define DEFAULT_LIMITER_LEVEL 70
#define MIN_LIMITER_LEVEL     25

#define backgrBlack 0
#define backgrCoverSmall 1
#define backgrCoverBig 2
#define backgrLive 3

/*!
 * \brief storage for setup data
 */
class mgSetup
{
	public:
		mgSetup ();
		~mgSetup (void);
		string ConfigDirectory;
		const char *HelpText();
		bool ProcessArguments(int argc, char *argv[]);
		bool NoHost() const;
		int InitLoopMode;
		int InitShuffleMode;
		int AudioMode;
		int DisplayMode;
		int BackgrMode;
		int TargetLevel;
		int LimiterLevel;
		int Only48kHz;

		char *DbHost;
		char *DbSocket;
		char *DbName;
		char *DbUser;
		char *DbPass;
		char *DbDatadir;

		int DbPort;
		char *ToplevelDir;

		int  ImageShowDuration;
		char *CacheDir;
		int UseDeviceStillPicture;

		// Player:
		int ArtistFirst;
#ifdef USE_BITMAP
		int ImgAlpha;
#endif
		int Jumptime;

		int DeleteStaleReferences;
		bool CreateMode;
		bool IsMugglei() const;
		void SetMugglei();
		bool utf8;
	private:
		bool m_mugglei;
};

extern mgSetup the_setup;
#endif
