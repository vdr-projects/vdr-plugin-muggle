/*!								-*- c++ -*-
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

#include "pcmplayer.h"

#include "vdr_sound.c"

mgPCMPlayer::mgPCMPlayer (mgSelection * plist)
: cPlayer(the_setup.ImgMode==imgLive? pmAudioOnly:pmAudioOnlyBlack ) {
	m_playlist = plist;

	m_active = true;
	m_started = false;

	m_ringbuffer = new cRingBufferFrame (MP3BUFSIZE);

	m_rframe = 0;
	m_pframe = 0;
	m_decoder = 0;

	m_playmode = pmStartup;
	m_state = msStop;

	m_index = 0;
	m_playing = false;
	m_current = 0;

}

mgPCMPlayer::~mgPCMPlayer () {
	Detach ();
	delete m_playlist;
	delete m_ringbuffer;

}

void
mgPCMPlayer::PlayTrack() {
	mgItemGd * newcurr = dynamic_cast<mgItemGd*>(m_playlist->getCurrentItem ());
	if (newcurr) {
		delete m_current;
		m_current = new mgItemGd(newcurr);
	}
	Play ();
}

void
mgPCMPlayer::Activate (bool on) {
	if (on) {
		if (m_playlist && !m_started) {
			m_playmode = pmStartup;
			Start ();

			m_started = true;
			m_current = 0;

			m_playmode_mutex.Lock ();
								 // wait for the decoder to become ready
			WaitPlayMode (pmStartup, true);
			m_playmode_mutex.Unlock ();

			Lock ();
			PlayTrack();
			Unlock ();
		}
	}
	else if (m_started && m_active) {
		Lock ();
		StopPlay ();
		Unlock ();

		m_active = false;
		SetPlayMode (pmStartup);

		Cancel (2);
	}
}

void
mgPCMPlayer::ReloadPlaylist() {
	Lock ();
	m_playlist->clearCache();
	if (!m_playing) {
		SkipFile();
		Play ();
	}
	Unlock ();
}

void
mgPCMPlayer::NewPlaylist (mgSelection * plist) {
	MGLOG ("mgPCMPlayer::NewPlaylist");
	plist->ShowState("mgPCMPlayer::NewPlaylist got:");

	Lock ();
	StopPlay ();

	delete m_current;
	m_current = 0;
	delete m_playlist;
	m_playlist = plist;
	PlayTrack();
	Unlock ();
}

void
mgPCMPlayer::SetPlayMode (emgPlayMode mode) {
	m_playmode_mutex.Lock ();
	if (mode != m_playmode) {
		m_playmode = mode;
		m_playmode_cond.Broadcast ();
	}
	m_playmode_mutex.Unlock ();
}

void
mgPCMPlayer::WaitPlayMode(emgPlayMode mode, bool inv) {
	// must be called with m_playmode_mutex LOCKED !!!

	while (m_active
	&& ((!inv && mode != m_playmode) || (inv && mode == m_playmode))) {
		m_playmode_cond.Wait (m_playmode_mutex);
	}
}

void
mgPCMPlayer::Action (void) {

	struct mgDecode *ds = 0;
	struct mad_pcm *pcm = 0;
	mgResample resample[2];
	unsigned int nsamples[2];
	const mad_fixed_t *data[2];
	mgScale scale;
	mgLevel level;
	mgNormalize norm;
	bool haslevel = false;
	struct LPCMFrame lpcmFrame;
	const unsigned char *p = 0;
	int pc = 0, only48khz = the_setup.Only48kHz;
	cPoller poll;
#ifdef DEBUG
	//     int beat = 0;
#endif

#ifdef DEBUGPES
	FILE *peslog = fopen( "pes.dump", "w" );
#endif

	dsyslog ("muggle: player thread started (pid=%d)", getpid ());

	memset (&lpcmFrame, 0, sizeof (lpcmFrame));
	lpcmFrame.PES[2] = 0x01;
	lpcmFrame.PES[3] = 0xbd;
	lpcmFrame.PES[6] = 0x87;
	lpcmFrame.LPCM[0] = 0xa0;	 // substream ID
	lpcmFrame.LPCM[1] = 0xff;
	lpcmFrame.LPCM[2] = 0;
	lpcmFrame.LPCM[3] = 4;
	lpcmFrame.LPCM[5] = 0x01;
	lpcmFrame.LPCM[6] = 0x80;

	dvbSampleRate = 48000;
	m_state = msStop;
	SetPlayMode (pmStopped);

#if VDRVERSNUM >= 10318
	cDevice::PrimaryDevice()->SetCurrentAudioTrack(ttAudio);
#endif
	while (m_active) {
#ifdef DEBUG
		/*
		  if (time (0) >= beat + 30)
		  {
			  std::
				  cout << "mgPCMPlayer::Action: heartbeat buffer=" << m_ringbuffer->
				  Available () << std::endl << std::flush;
			 scale.Stats ();
		 if (haslevel)
		   norm.Stats ();
		 beat = time (0);
		  }
		*/
#endif

		Lock ();

		if (!m_rframe && m_playmode == pmPlay) {
			switch (m_state) {
				case msStart:
				{
					m_index = 0;

					m_playing = true;

					if ( m_current ) {
						string filename = m_current->getSourceFile ();
						if ((m_decoder = mgDecoders::findDecoder (m_current))
						&& m_decoder->start ()) {
							levelgood = true;
							haslevel = false;

							level.Init ();

							m_state = msDecode;
							break;
						}
						else {
							mgWarning("found no decoder for %s",filename.c_str());
								 // if loop mode is on and no decoder
							m_state=msStop;
							// for any track is found, we would
							// otherwise get into an endless loop
							// not stoppable with the remote.
							break;
						}
					}
					m_state = msEof;
				}
				break;
				case msDecode:
				{
					if (imagefile.size()) internShowMPGFile();
					ds = m_decoder->decode ();
					switch (ds->status) {
						case dsPlay:
						{
							pcm = ds->pcm;
							m_index = ds->index / 1000;
							m_state = msNormalize;
						}
						break;
						case dsSkip:
						case dsSoftError:
						{
							// skipping, state unchanged, next decode
						}
						break;
						case dsEof:
						{
							m_state = msEof;
						}
						break;
						case dsOK:
						case dsError:
						{
							m_state = msError;
						}
						break;
					}
				}
				break;
				case msNormalize:
				{
					if (!haslevel) {
						if (levelgood) {
							level.GetPower (pcm);
						}
					}
					else {
						norm.AddGain (pcm);
					}
					m_state = msResample;
				}
				break;
				case msResample:
				{
#ifdef DEBUG
					{
						static unsigned int oldrate = 0;
						if (oldrate != pcm->samplerate) {
							mgDebug ("mgPCMPlayer::Action: new input sample rate %d",pcm->samplerate);
							oldrate = pcm->samplerate;
						}
					}
#endif
					nsamples[0] = nsamples[1] = pcm->length;
					data[0] = pcm->samples[0];
					data[1] = pcm->channels > 1 ? pcm->samples[1] : 0;

					lpcmFrame.LPCM[5] &= 0xcf;
					dvbSampleRate = 48000;
					if (!only48khz) {
						switch (pcm->samplerate) {
								 // If one of the supported frequencies, do it without resampling.
							case 96000:
							{	 // Select a "even" upsampling frequency if possible, too.
								lpcmFrame.LPCM[5] |= 1 << 4;
								dvbSampleRate = 96000;
							}
							break;

							//case 48000: // this is already the default ...
							//  lpcmFrame.LPCM[5]|=0<<4;
							//  dvbSampleRate=48000;
							//  break;
							case 11025:
							case 22050:
							case 44100:
							{
								// lpcmFrame.LPCM[5] |= 2 << 4;
								lpcmFrame.LPCM[5] |= 0x20;
								dvbSampleRate = 44100;
							}
							break;
							case 8000:
							case 16000:
							case 32000:
							{
								// lpcmFrame.LPCM[5] |= 3 << 4;
								lpcmFrame.LPCM[5] |= 0x30;
								dvbSampleRate = 32000;
							}
							break;
						}
					}

					if (dvbSampleRate != pcm->samplerate) {
						if (resample[0].
						SetInputRate (pcm->samplerate, dvbSampleRate)) {
							nsamples[0] =
								resample[0].ResampleBlock (nsamples[0], data[0]);
							data[0] = resample[0].Resampled ();
						}
						if (data[1]
							&& resample[1].SetInputRate (pcm->samplerate,
						dvbSampleRate)) {
							nsamples[1] =
								resample[1].ResampleBlock (nsamples[1], data[1]);
							data[1] = resample[1].Resampled ();
						}
					}
					m_state = msOutput;
				}
				break;
				case msOutput:
				{
					if (nsamples[0] > 0) {
						unsigned int outlen =
							scale.ScaleBlock ( lpcmFrame.Data,
							sizeof (lpcmFrame.Data),
							nsamples[0],
							data[0], data[1],
							the_setup.AudioMode ?
							amDither : amRound );
						if (outlen) {
							outlen += sizeof (lpcmFrame.LPCM) + LEN_CORR;
							lpcmFrame.PES[4] = outlen >> 8;
							lpcmFrame.PES[5] = outlen;

							// lPCM has 600 fps which is 80 samples at 48kHz per channel
							// Frame size = (sample_rate * quantization * channels)/4800
							size_t frame_size = 2 * (dvbSampleRate*16)/4800;

							lpcmFrame.LPCM[1] = (outlen - LPCM_SIZE)/frame_size;
							m_rframe = new cFrame ((unsigned char *) &lpcmFrame,
								outlen +
								sizeof (lpcmFrame.PES) -
								LEN_CORR);
						}
					}
					else {
						m_state = msDecode;
					}
				}
				break;
				case msError:
				case msEof:
				{
					if (SkipFile ()) {
						m_state = msStart;
					}
					else {
						m_state = msWait;
					}
				}				 // fall through
				case msStop:
				{
					m_playing = false;

					if (m_decoder) {
								 // who deletes decoder?
						m_decoder->stop ();
						delete m_decoder;
						m_decoder = 0;
					}

					levelgood = false;

					scale.Stats ();
					if (haslevel) {
						norm.Stats ();
					}
					if (m_state == msStop) {
								 // might be unequal in case of fall through from eof/error
						SetPlayMode (pmStopped);
					}
				}
				break;
				case msWait:
				{
					if (m_ringbuffer->Available () == 0) {
						// m_active = false;
						SetPlayMode (pmStopped);
					}
				}
				break;
			}
		}

		if (m_rframe && m_ringbuffer->Put (m_rframe)) {
			m_rframe = 0;
		}

		if (!m_pframe && m_playmode == pmPlay) {
			m_pframe = m_ringbuffer->Get ();
			if (m_pframe) {
				p = m_pframe->Data ();
				pc = m_pframe->Count ();
			}
		}

		if (m_pframe) {
#ifdef DEBUGPES
			fwrite( (void *)p, pc, sizeof( char ), peslog );
#endif

#if VDRVERSNUM >= 10318
			int w = PlayPes (p, pc);
#else
			int w = PlayVideo (p, pc);
#endif
			if (w > 0) {
				p += w;
				pc -= w;

				if (pc <= 0) {
					m_ringbuffer->Drop (m_pframe);
					m_pframe = 0;
				}
			}
			else if (w < 0 && FATALERRNO) {
				LOG_ERROR;
				break;
			}
		}
								 // avoid helgrind warning
		eState curr_m_state = m_state;

		Unlock ();

		if ((m_rframe || curr_m_state == msWait) && m_pframe) {
			// Wait for output to become ready
			DevicePoll (poll, 500);
		}
		else {
			if (m_playmode != pmPlay) {
				m_playmode_mutex.Lock ();

				if (m_playmode != pmPlay) {
								 // Wait on playMode change
					WaitPlayMode (m_playmode, true);
				}
				m_playmode_mutex.Unlock ();
			}
		}
	}

	Lock ();

	if (m_rframe) {
		delete m_rframe;
		m_rframe = 0;
	}

	if (m_decoder) {
								 // who deletes decoder?
		m_decoder->stop ();
		delete m_decoder;
		m_decoder = 0;
	}

	m_playing = false;

	SetPlayMode (pmStopped);

#ifdef DEBUGPES
	fclose( peslog );
#endif

	Unlock ();

	m_active = false;

	dsyslog ("muggle: player thread ended (pid=%d)", getpid ());
}

void
mgPCMPlayer::Empty (void) {

	Lock ();

	m_ringbuffer->Clear ();
	DeviceClear ();

	delete m_rframe;
	m_rframe = 0;
	m_pframe = 0;

	Unlock ();
}

void
mgPCMPlayer::StopPlay () {
							// StopPlay() must be called in locked state!!!
	if (m_playmode != pmStopped) {
		Empty ();
		m_state = msStop;
		SetPlayMode (pmPlay);
		Unlock ();				 // let the decode thread process the stop signal

		m_playmode_mutex.Lock ();
		WaitPlayMode (pmStopped, false);
		m_playmode_mutex.Unlock ();

		Lock ();
	}
}

bool mgPCMPlayer::SkipFile (bool skipforward) {
	mgItemGd * newcurr = NULL;
	int skip_direction=1;
	if (!skipforward)
		skip_direction=-1;
	if (m_playlist->skipItems (skip_direction)) {
		newcurr = dynamic_cast<mgItemGd*>(m_playlist->getCurrentItem ());
		if (newcurr) {
			delete m_current;
			m_current = new mgItemGd(newcurr);
		}
	}
	return (newcurr != NULL);
}

void
mgPCMPlayer::ToggleShuffle () {
	m_playlist->toggleShuffleMode ();
}

void
mgPCMPlayer::ToggleLoop (void) {
	m_playlist->toggleLoopMode ();
}

void
mgPCMPlayer::Pause (void) {
	if (m_playmode == pmPaused) {
		Play ();
	}
	else {
		if (m_playmode == pmPlay) {
			//    DeviceFreeze();
			SetPlayMode (pmPaused);
		}
	}
}

void
mgPCMPlayer::Play (void) {

	if (m_playmode != pmPlay && m_current) {
		Lock ();

		if (m_playmode == pmStopped) {
			m_state = msStart;
		}
		//    DevicePlay(); // TODO? Commented out in original code, too
		SetPlayMode (pmPlay);
		Unlock ();
	}
}

void
mgPCMPlayer::Forward () {
	Lock ();
	if (SkipFile ()) {
		StopPlay ();
		Play ();
	}
	Unlock ();
}

void
mgPCMPlayer::Backward (void) {
	Lock ();
	if (SkipFile (false)) {
		StopPlay ();
		Play ();
	}
	Unlock ();
}

void
mgPCMPlayer::Goto (int index) {
	m_playlist->GotoItemPosition (index );
	mgItemGd *next = dynamic_cast<mgItemGd*>(m_playlist->getCurrentItem ());

	if (next) {
		Lock ();
		StopPlay ();
		delete m_current;
		m_current = new mgItemGd(next);
		Play ();
		Unlock ();
	}
}

void
mgPCMPlayer::SkipSeconds (int secs) {
	if (m_playmode != pmStopped) {
		Lock ();

		if (m_playmode == pmPaused) {
			SetPlayMode (pmPlay);
		}
		if ( m_decoder
			&& m_decoder->skip (secs, m_ringbuffer->Available(),
		dvbSampleRate) ) {
			levelgood = false;
		}

		Empty ();
		Unlock ();
	}
}

bool mgPCMPlayer::GetIndex (int &current, int &total, bool snaptoiframe) {
	if (m_current) {
		current = SecondsToFrames (m_index);
		total = SecondsToFrames (m_current->getDuration ());
		return total>=0;
	}
	return false;
}

bool mgPCMPlayer::GetReplayMode(bool &Play, bool &Forward, int &Speed) {
  Speed = -1;
  Forward = true;
  switch(PlayMode()) {
    case pmPlay:  Play = true; break;
    default:  
                  Play = false; break;
  }
  return true;
}

void mgPCMPlayer::internShowMPGFile() {
	if (!imagefile.size())
		return;
	uchar *buffer;
	int fd;
	struct stat st;
	struct video_still_picture sp;

	if ( (fd = open( imagefile.c_str(), O_RDONLY ) ) >= 0 ) {
		mgDebug(9,"internShowMPGFile:%s",imagefile.c_str());
		fstat (fd, &st);
		sp.iFrame = (char *) malloc (st.st_size);
		if ( sp.iFrame ) {
			sp.size = st.st_size;
			if ( read (fd, sp.iFrame, sp.size) > 0 ) {
				buffer = (uchar *) sp.iFrame;

				if ( the_setup.UseDeviceStillPicture ) {
					cCondWait::SleepMs(80);
					DeviceStillPicture( buffer, sp.size );
				}
				else {
					for (int i = 1; i <= 25; i++) {
						send_pes_packet (buffer, sp.size, i);
					}
				}
			}
			free (sp.iFrame);
		}
		else {
			esyslog ("muggle[%d]: cannot allocate memory (%d bytes) for still image",
				getpid(), (int) st.st_size);
		}
		close (fd);
	}
	else {
		esyslog ("muggle[%d]: cannot open image file '%s'",
			getpid(), imagefile.c_str() );
	}
	imagefile.clear();
}

void mgPCMPlayer::send_pes_packet(unsigned char *data, int len, int timestamp) {
#define PES_MAX_SIZE 2048
	int ptslen = timestamp ? 5 : 1;
	static unsigned char pes_header[PES_MAX_SIZE];

	pes_header[0] = pes_header[1] = 0;
	pes_header[2] = 1;
	pes_header[3] = 0xe0;

	while(len > 0) {
		int payload_size = len;
		if (6 + ptslen + payload_size > PES_MAX_SIZE)
			payload_size = PES_MAX_SIZE - (6 + ptslen);

		pes_header[4] = (ptslen + payload_size) >> 8;
		pes_header[5] = (ptslen + payload_size) & 255;

		if (ptslen == 5) {
			int x;
			x = (0x02 << 4) | (((timestamp >> 30) & 0x07) << 1) | 1;
			pes_header[8] = x;
			x = ((((timestamp >> 15) & 0x7fff) << 1) | 1);
			pes_header[7] = x >> 8;
			pes_header[8] = x & 255;
			x = ((((timestamp) & 0x7fff) < 1) | 1);
			pes_header[9] = x >> 8;
			pes_header[10] = x & 255;
		}
		else {
			pes_header[6] = 0x0f;
		}

		memcpy(&pes_header[6 + ptslen], data, payload_size);
#if VDRVERSNUM >= 10318
		PlayPes(pes_header, 6 + ptslen + payload_size);
#else
		PlayVideo(pes_header, 6 + ptslen + payload_size);
#endif

		len -= payload_size;
		data += payload_size;
		ptslen = 1;
	}
}
