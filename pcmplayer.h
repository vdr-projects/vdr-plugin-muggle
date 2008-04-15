/*!
 * \file pcmplayer.c
 * \brief A generic PCM player for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.7 $
 * \date    $Date: 2008-03-17 02:44:14 +0100 (Mo, 17 Mär 2008) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author: woro $
 *
 * $Id: vdr_player.c 1046 2008-03-17 01:44:14Z woro $
 *
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
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

#include "vdr_decoder.h"
#include "vdr_config.h"
#include "mg_setup.h"
#include "mg_image_provider.h"
#include "bitmap.h"

#include "mg_tools.h"

enum emgPlayMode
{
	pmPlay,
	pmStopped,
	pmPaused,
	pmStartup
};

// ----------------------------------------------------------------

// #define DEBUGPES

// mgResample
#define MAX_NSAMPLES (1152*7)	 // max. buffer for resampled frame

// mgNormalize
#define MAX_GAIN   3.0			 // max. allowed gain
#define LIM_ACC    12			 // bit, accuracy for lookup table

								 // max. value covered by lookup table
#define F_LIM_MAX  (mad_fixed_t)((1<<(MAD_F_FRACBITS+2))-1)
								 // shift value for table lookup
#define LIM_SHIFT  (MAD_F_FRACBITS-LIM_ACC)
								 // lookup table jump between values
#define F_LIM_JMP  (mad_fixed_t)(1<<LIM_SHIFT)

// mgLevel
#define POW_WIN 100				 // window width for smoothing power values
#define EPSILON 0.00000000001	 // anything less than EPSILON is considered zero

// cMP3Player
#define MAX_FRAMESIZE   2048	 // max. frame size allowed for DVB driver
#define HDR_SIZE        9
#define LPCM_SIZE       7
#define LEN_CORR        3
								 // play time to fill buffers before speed check starts (ms)
#define SPEEDCHECKSTART ((MP3BUFSIZE*1000/32000/2/2)+1000)
#define SPEEDCHECKTIME  3000	 // play time for speed check (ms)

/*
struct LPCMHeader { int id:8;              // id
					int frame_count:8;     // number of frames
					int access_ptr:16;     // first acces unit pointer, i.e. start of audio frame
					bool emphasis:1;       // audio emphasis on-off
					bool mute:1;           // audio mute on-off
					bool reserved:1;       // reserved
					int frame_number:5;    // audio frame number
					int quant_wlen:2;      // quantization word length
					int sample_freq:2;     // audio sampling frequency (48khz=0, 96khz=1, 44,1khz=2, 32khz=3)
					bool reserved2:1;      // reserved
int chan_count:3;      // number of audio channels - 1 (e.g. stereo = 1)
int dyn_range_ctrl:8;  // dynamic range control (0x80 if off)
};
*/

struct LPCMFrame
{
	unsigned char PES[HDR_SIZE];
	unsigned char LPCM[LPCM_SIZE];
	unsigned char Data[MAX_FRAMESIZE - HDR_SIZE - LPCM_SIZE];
};

/*!
 * \brief a generic PCM player class
 *
 * this class implements a state machine that obtains decoded data from a generic data
 * and moves it to the DVB device. It inherits from cPlayer in order to be hooked into
 * VDR as a player and inherits from cThread in order to implement a separate thread
 * for the decoding process.
 */
class mgPCMPlayer : public cPlayer, cThread
{
	private:

		//! \brief indicates whether the player is currently active
		bool m_active;

		//! \brief indicates whether the player has been started
		bool m_started;

		//! \brief indicates whether the player is currently playing
		bool m_playing;

		//! \brief a buffer for decoded sound
		cRingBufferFrame *m_ringbuffer;

		//! \brief a mutex for the playmode
		cMutex m_playmode_mutex;

		//! \brief a condition to signal playmode changes
		cCondVar m_playmode_cond;

		//! \brief the current playlist
		mgSelection *m_playlist;

		//! \brief the currently played or to be played item
		mgItemGd *m_current;

		//! \brief the decoder responsible for the currently playing item
		mgDecoder *m_decoder;

		cFrame *m_rframe, *m_pframe;

		emgPlayMode m_playmode;

		enum eState
		{
			msStart, msStop,
			msDecode, msNormalize,
			msResample, msOutput,
			msError, msEof, msWait
		};
		eState m_state;

		bool levelgood;
		unsigned int dvbSampleRate;

		//
		int m_index;

		string imagefile;

		void Empty ();
		bool SkipFile (bool skipforward=true);
		void PlayTrack();
		void StopPlay ();

		void SetPlayMode (emgPlayMode mode);
		void WaitPlayMode (emgPlayMode mode, bool inv);
		void internShowMPGFile();
		void send_pes_packet(unsigned char *data, int len, int timestamp);

	protected:
		virtual void Activate (bool On);
		virtual void Action (void);

	public:
		mgPCMPlayer (mgSelection * plist);
		virtual ~ mgPCMPlayer ();

		bool Active () {
			return m_active;
		}

		bool Playing () {
			return m_playing || m_active;
		}

		emgPlayMode PlayMode () {
			return m_playmode;
		}

		unsigned int Position() {
			return m_playlist->getItemPosition();
		}

		void Pause ();
		void Play ();
		void Forward ();
		void Backward ();

		void Goto (int Index);
		void SkipSeconds (int secs);
		void ToggleShuffle (void);
		void ToggleLoop (void);
		void ShowMPGFile(const string f ) { imagefile=f; }

		virtual bool GetIndex (int &Current, int &Total, bool SnapToIFrame = false);
		//  bool GetPlayInfo(cMP3PlayInfo *rm); // LVW

		void ReloadPlaylist();
		void NewPlaylist (mgSelection * plist);

		mgItemGd *getCurrent () {
			return m_current;
		}

		mgSelection *getPlaylist () {
			return m_playlist;
		}

};
