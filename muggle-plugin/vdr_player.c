/*!
 * \file vdr_player.c
 * \brief A generic PCM player for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.7 $
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

#include <mad.h>
#include <id3tag.h>

#include <player.h>
#include <device.h>
#include <thread.h>
#include <ringbuffer.h>
#include <tools.h>
#include <recording.h>
#include <status.h>

#include "vdr_player.h"
#include "vdr_decoder.h"
#include "vdr_config.h"
#include "vdr_setup.h"
#include "i18n.h"

#include "mg_tools.h"
#include "mg_playlist.h"
#include "mg_content_interface.h"


// ----------------------------------------------------------------

// TODO: check for use of constants
#define OUT_BITS 16                                          // output 16 bit samples to DVB driver
#define OUT_FACT (OUT_BITS/8*2)                              // output factor is 16 bit & 2 channels -> 4 bytes

// cResample
#define MAX_NSAMPLES (1152*7)                                // max. buffer for resampled frame

// cNormalize
#define MAX_GAIN   3.0                                       // max. allowed gain
#define LIM_ACC    12                                        // bit, accuracy for lookup table
#define F_LIM_MAX  (mad_fixed_t)((1<<(MAD_F_FRACBITS+2))-1)  // max. value covered by lookup table
#define LIM_SHIFT  (MAD_F_FRACBITS-LIM_ACC)                  // shift value for table lookup
#define F_LIM_JMP  (mad_fixed_t)(1<<LIM_SHIFT)               // lookup table jump between values

// cLevel
#define POW_WIN 100                                          // window width for smoothing power values
#define EPSILON 0.00000000001                                // anything less than EPSILON is considered zero

// cMP3Player
#define MAX_FRAMESIZE   2048                                 // max. frame size allowed for DVB driver
#define HDR_SIZE        9
#define LPCM_SIZE       7
#define LEN_CORR        3
#define SPEEDCHECKSTART ((MP3BUFSIZE*1000/32000/2/2)+1000)   // play time to fill buffers before speed check starts (ms)
#define SPEEDCHECKTIME  3000                                 // play time for speed check (ms)

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
  unsigned char Data[MAX_FRAMESIZE-HDR_SIZE-LPCM_SIZE];
};

#include "vdr_sound.c"


// --- mgPCMPlayer ----------------------------------------------------------

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

  //! \brief indicates, whether the player is currently active
  bool m_active;

  //! \brief indicates, whether the player has been started
  bool m_started;

  //! \brief a buffer for decoded sound
  cRingBufferFrame *m_ringbuffer;

  //! \brief a mutex for the playmode
  cMutex m_playmode_mutex;

  //! \brief a condition to signal playmode changes
  cCondVar m_playmode_cond;

  //! \brief the current playlist
  mgPlaylist *m_playlist;

  //! \brief the currently played or to be played item
  mgContentItem *m_current;

  //! \brief the currently playing item
  mgContentItem *m_playing;
  
  //! \brief the decoder responsible for the currently playing item
  mgDecoder *m_decoder;

  //! \brief the index where to start the playlist at
  int m_first;

  cFrame *m_rframe, *m_pframe;

  enum ePlayMode 
    { 
      pmPlay, 
      pmStopped, 
      pmPaused, 
      pmStartup 
    };
  ePlayMode m_playmode;

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

  void Empty();
  bool NextFile( );
  bool PrevFile();
  void StopPlay();

  void SetPlayMode(ePlayMode mode);
  void WaitPlayMode(ePlayMode mode, bool inv);

protected:
  virtual void Activate(bool On);
  virtual void Action(void);

public:
  mgPCMPlayer(mgPlaylist *plist, int first);
  virtual ~mgPCMPlayer();

  bool Active() { return m_active; }

  void Pause();
  void Play();
  void Forward();
  void Backward();

  void Goto(int Index, bool Still=false);
  void SkipSeconds(int secs);
  void ToggleShuffle(void);
  void ToggleLoop(void);

  virtual bool getIndex(int &Current, int &Total, bool SnapToIFrame=false);
  //  bool GetPlayInfo(cMP3PlayInfo *rm); // LVW

  void NewPlaylist( mgPlaylist *plist, int first );
  mgContentItem *getCurrent () { return m_current; }  
  mgPlaylist *getPlaylist () { return m_playlist; }  
};

mgPCMPlayer::mgPCMPlayer(mgPlaylist *plist, int first)
  : cPlayer( the_setup.BackgrMode? pmAudioOnly: pmAudioOnlyBlack ),
     m_first( first )
{
  m_playlist = plist;

  m_active   = true; 
  m_started  = false; 

  m_ringbuffer = new cRingBufferFrame( MP3BUFSIZE );

  m_rframe  = 0; 
  m_pframe  = 0; 
  m_decoder = 0;

  m_playmode = pmStartup; 
  m_state = msStop;

  m_index   = 0; 
  m_playing = 0; 
  m_current = 0; 
}

mgPCMPlayer::~mgPCMPlayer()
{
  Detach();

  delete m_ringbuffer;
}

void mgPCMPlayer::Activate(bool on)
{
  MGLOG( "mgPCMPlayer::Activate" );
  if( on ) 
    {
      if( m_playlist && !m_started )
	{
	  m_playmode = pmStartup; 
	  Start();

	  m_started = true;
	  m_current = 0;

	  m_playmode_mutex.Lock();
	  WaitPlayMode( pmStartup, true ); // wait for the decoder to become ready
	  m_playmode_mutex.Unlock();

	  Lock();

	  m_playlist->initialize( );
	  if( m_first > 0 )
	    {
	      m_playlist->gotoPosition( m_first );
	    }
	  m_current = m_playlist->getCurrent();
	  Play();

	  Unlock();
	}
    }
  else if( m_started && m_active ) 
    {
      Lock(); 
      StopPlay(); 
      Unlock();

      m_active = false;
      SetPlayMode( pmStartup );

      Cancel(2);
    }
}

void mgPCMPlayer::NewPlaylist( mgPlaylist *plist, int start )
{
  MGLOG( "mgPCMPlayer::NewPlaylist" );

  Lock(); 
  StopPlay(); 
  Unlock();

  // memory management of playlists should happen elsewhere (menu, content)
  m_playlist = plist;
  m_current = 0;

  if( start > 0 )
    {
      m_playlist->gotoPosition( (unsigned) start );
      m_current = m_playlist->getCurrent();
      Play();
    }  
  else if( NextFile() ) 
    {
      Play();
    }
}

void mgPCMPlayer::SetPlayMode(ePlayMode mode)
{
  m_playmode_mutex.Lock();
  if( mode != m_playmode ) 
    {
      m_playmode = mode;
      m_playmode_cond.Broadcast();
    }
  m_playmode_mutex.Unlock();
}

void mgPCMPlayer::WaitPlayMode(ePlayMode mode, bool inv)
{
  // must be called with m_playmode_mutex LOCKED !!!

  while( m_active && ( (!inv && mode != m_playmode) || (inv && mode == m_playmode) ) ) 
    {
      m_playmode_cond.Wait(m_playmode_mutex);
    }
}

void mgPCMPlayer::Action(void)
{
  MGLOG( "mgPCMPlayer::Action" );

  struct mgDecode *ds=0;
  struct mad_pcm *pcm=0;
  cResample resample[2];
  unsigned int nsamples[2];
  const mad_fixed_t *data[2];
  cScale scale;
  cLevel level;
  cNormalize norm;
  bool haslevel=false;
  struct LPCMFrame lpcmFrame;
  const unsigned char *p=0;
  int pc = 0, only48khz = the_setup.Only48kHz;
  cPoller poll;
#ifdef DEBUG
  int beat=0;
#endif

  dsyslog( "muggle: player thread started (pid=%d)", getpid() );

  memset( &lpcmFrame, 0, sizeof(lpcmFrame) );
  lpcmFrame.PES[2]=0x01;
  lpcmFrame.PES[3]=0xbd;
  lpcmFrame.PES[6]=0x87;
  lpcmFrame.LPCM[0]=0xa0; // substream ID
  lpcmFrame.LPCM[1]=0xff;
  lpcmFrame.LPCM[5]=0x01;
  lpcmFrame.LPCM[6]=0x80;

  dvbSampleRate = 48000;
  m_state = msStop;
  SetPlayMode( pmStopped );
 
  while( m_active ) 
    {
#ifdef DEBUG
      if(time(0)>=beat+30) 
	{
	  std::cout << "mgPCMPlayer::Action: heartbeat buffer=" << m_ringbuffer->Available() << std::endl << std::flush;
	  scale.Stats(); if(haslevel) norm.Stats();
	  beat=time(0);
	}
#endif

      Lock();
      
      if( !m_rframe && m_playmode == pmPlay ) 
	{
	  switch( m_state ) 
	    {
	    case msStart:
	      {
		m_index = 0; 
		m_playing = m_current;

		if( m_playing && m_playing != &(mgContentItem::UNDEFINED) )
		    {
		      std::string filename = m_playing->getSourceFile();
		      // mgDebug( 1, "mgPCMPlayer::Action: music file is %s", filename.c_str() );
        
		      if( ( m_decoder = mgDecoders::findDecoder( m_playing ) ) && m_decoder->start() )
		      {
			levelgood = true; 
			haslevel = false;
		    
			scale.Init();
			level.Init();
			
			m_state = msDecode;
			
			break;
		      }
		    }
		m_state = msEof;
	      } break;
	    case msDecode:
	      {
		ds = m_decoder->decode();
		switch( ds->status ) 
		  {
		  case dsPlay:
		    {
		      pcm   = ds->pcm;
		      m_index = ds->index/1000;
		      m_state = msNormalize;
		    } break;
		  case dsSkip:
		  case dsSoftError:
		    {
		      // skipping, state unchanged, next decode
		    } break;
		  case dsEof:
		    {
		      m_state = msEof;
		    } break;
		  case dsOK:
		  case dsError:
		    {
		      m_state = msError;
		    } break;
		  }
	      } break;
	    case msNormalize:
	      {
		if(!haslevel) 
		  { 
		    if( levelgood ) 
		      {
			level.GetPower( pcm ); 
		      }
		  }
		else 
		  {
		    norm.AddGain( pcm );
		  }
		m_state = msResample;
	      } break;
	    case msResample:
	      {
#ifdef DEBUG
		{
		  static unsigned int oldrate=0;
		  if(oldrate!=pcm->samplerate) 
		    {
		      std::cout << "mgPCMPlayer::Action: new input sample rate " << pcm->samplerate << std::endl << std::flush;
		      oldrate = pcm->samplerate;
		    }
		}
#endif
		nsamples[0] = nsamples[1] = pcm->length;
		data[0] = pcm->samples[0];
		data[1] = pcm->channels > 1 ? pcm->samples[1]: 0;
		
		lpcmFrame.LPCM[5]&=0xcf;
		dvbSampleRate=48000;
		if(!only48khz) 
		  {
		    switch(pcm->samplerate) 
		      {    // If one of the supported frequencies, do it without resampling.
		      case 96000:
			{ // Select a "even" upsampling frequency if possible, too.
			  lpcmFrame.LPCM[5] |= 1 << 4;
			  dvbSampleRate = 96000;
			} break;

			//case 48000: // this is already the default ...
			//  lpcmFrame.LPCM[5]|=0<<4;
			//  dvbSampleRate=48000;
			//  break;
		      case 11025:
		      case 22050:
		      case 44100:
			{
			  lpcmFrame.LPCM[5]|=2<<4;
			  dvbSampleRate = 44100;
			} break;
		      case 8000:
		      case 16000:
		      case 32000:
			{ 
			  lpcmFrame.LPCM[5]|=3<<4;
			  dvbSampleRate = 32000;
			} break;
		      }
		  }
		
		if( dvbSampleRate != pcm->samplerate ) 
		  {
		    if( resample[0].SetInputRate( pcm->samplerate, dvbSampleRate ) ) 
		      {
			nsamples[0] = resample[0].ResampleBlock( nsamples[0], data[0] );
			data[0]     = resample[0].Resampled();
		      }
		    if(data[1] && resample[1].SetInputRate( pcm->samplerate, dvbSampleRate ) ) 
		      {
			nsamples[1] = resample[1].ResampleBlock( nsamples[1], data[1] );
			data[1]     = resample[1].Resampled();
		      }
		  }
		m_state=msOutput;
	      } break;
	    case msOutput:
	      {
		if( nsamples[0] > 0 ) 
		  {
		    unsigned int outlen = scale.ScaleBlock( lpcmFrame.Data,
							    sizeof(lpcmFrame.Data),
							    nsamples[0], data[0],
							    data[1], 
							    the_setup.AudioMode? amDither: amRound );
		    if( outlen ) 
		      { 
			outlen += sizeof(lpcmFrame.LPCM)+LEN_CORR;
			lpcmFrame.PES[4] = outlen >> 8;
			lpcmFrame.PES[5] = outlen;
			m_rframe = new cFrame( (unsigned char *)&lpcmFrame,
					     outlen + sizeof( lpcmFrame.PES ) - LEN_CORR );
		      }
		  }
		else 
		  {
		    m_state=msDecode;
		  }
	      } break;
	    case msError:
	    case msEof:
	      {
		if( NextFile() ) 
		  {
		    m_state = msStart;
		  }
		else 
		  {
		    m_state = msWait;
		  }
	      } // fall through
	    case msStop:
	      {
		m_playing = 0;
		if( m_decoder ) 
		  { // who deletes decoder?
		    m_decoder->stop(); 
		    m_decoder = 0; 
		  }

		levelgood = false;

		scale.Stats(); 
		if( haslevel )
		  {
		    norm.Stats();
		  }
		if( m_state == msStop ) 
		  { // might be unequal in case of fall through from eof/error
		    SetPlayMode( pmStopped );
		  }
	      } break;
	    case msWait:
	      {
		if( m_ringbuffer->Available() == 0 ) 
		  {
		    m_active = false;
		    SetPlayMode(pmStopped);
		  }
	      } break;
	    }
	}
      
      if( m_rframe && m_ringbuffer->Put( m_rframe ) )
	{
	  m_rframe = 0;
	}
      
      if( !m_pframe && m_playmode == pmPlay ) 
	{
	  m_pframe = m_ringbuffer->Get();
	  if( m_pframe )
	    {
	      p = m_pframe->Data();
	      pc = m_pframe->Count();
	    }
	}
      
      if( m_pframe ) 
	{
	  int w = PlayVideo( p, pc );
	  if( w > 0 ) 
	    {
	      p += w; 
	      pc -= w;

	      if( pc <= 0 ) 
		{
		  m_ringbuffer->Drop(m_pframe);
		  m_pframe=0;
		}
	    }
	  else if( w < 0 && FATALERRNO ) 
	    {
	      LOG_ERROR;
	      break;
	    }
	}
      
      Unlock();
      
      if( (m_rframe || m_state == msWait) && m_pframe ) 
	{
	  // Wait for output to become ready
	  DevicePoll( poll, 500 );
	}
      else 
	{
	  if( m_playmode != pmPlay ) 
	    {
	      m_playmode_mutex.Lock();

	      if( m_playmode != pmPlay ) 
		{
		  WaitPlayMode( m_playmode, true );  // Wait on playMode change
		}
	      m_playmode_mutex.Unlock();
	    }
	}
    }
  
  Lock();

  if( m_rframe )
    {
      delete m_rframe; 
      m_rframe=0;
    }

  if( m_decoder ) 
    { // who deletes decoder?
      m_decoder->stop(); 
      m_decoder = 0; 
    }
  
  m_playing = 0;

  SetPlayMode(pmStopped);

  Unlock();

  m_active = false;
  
  dsyslog( "muggle: player thread ended (pid=%d)", getpid() );
}

void mgPCMPlayer::Empty(void)
{
  MGLOG( "mgPCMPlayer::Empty" );

  Lock();

  m_ringbuffer->Clear();
  DeviceClear();

  delete m_rframe; 
  m_rframe = 0; 
  m_pframe = 0;

  Unlock();
}

void mgPCMPlayer::StopPlay() 
{ // StopPlay() must be called in locked state!!!
  MGLOG( "mgPCMPlayer::StopPlay" );
  if( m_playmode != pmStopped ) 
    {
      Empty();
      m_state = msStop;
      SetPlayMode( pmPlay );
      Unlock();                 // let the decode thread process the stop signal

      m_playmode_mutex.Lock();
      WaitPlayMode( pmStopped, false );
      m_playmode_mutex.Unlock();

      Lock();
    }
}

bool mgPCMPlayer::NextFile( )
{
  mgContentItem *newcurr;

  bool res = false;
  bool m_partymode = false;
  bool m_shufflemode = false;

  if( m_partymode )
    {
      /*
      - Party mode (see iTunes)
        - initialization
          - find 15 titles according to the scheme below
        - playing 
          - before entering next title perform track selection
        - track selection
  	  - generate a random uid
	  - if file exists:
	    - determine maximum playcount of all tracks
	    - generate a random number n
	      - if n < playcount / max. playcount
	        - add the file to the end of the list
      */
    }
  else if( m_shufflemode )
    {
      /*
	- Handle shuffle mode in mgPlaylist
 	  - for next file:
	    - generate a random number 0..n-1
	    - move corresponding playlist item to front
	    - continue
      */
    }
  else
    {
      if( m_playlist->skipFwd() )
	{
	  newcurr = m_playlist->getCurrent();
	}
      else
	{
	  newcurr = &(mgContentItem::UNDEFINED);
	}
    }
  
  if( newcurr && newcurr != &(mgContentItem::UNDEFINED) ) 
    {
      m_current = newcurr; 
      res = true;
    }
  
  return res;
}

bool mgPCMPlayer::PrevFile(void)
{
  bool res = false;

  if( m_playlist->skipBack() )
    {
      mgContentItem *newcurr = m_playlist->getCurrent();

      if( newcurr && newcurr != &(mgContentItem::UNDEFINED) ) 
	{
	  m_current = newcurr; 
	  res = true;
	}
    }

  return res;
}

void mgPCMPlayer::ToggleShuffle()
{
  m_playlist->toggleShuffleMode();
}

void mgPCMPlayer::ToggleLoop(void)
{
  m_playlist->toggleLoopMode();
}

void mgPCMPlayer::Pause(void)
{
  if( m_playmode == pmPaused ) 
    {
      Play();
    }
  else
    {
      if( m_playmode == pmPlay ) 
	{
	  //    DeviceFreeze();
	  SetPlayMode( pmPaused );
      }
    }
}

void mgPCMPlayer::Play(void)
{
  MGLOG( "mgPCMPlayer::Play" );

  Lock();

  if( m_playmode != pmPlay && m_current && m_current != &(mgContentItem::UNDEFINED) ) 
    {
      if( m_playmode == pmStopped ) 
	{
	  m_state = msStart;
	}
      //    DevicePlay(); // TODO? Commented out in original code, too
      SetPlayMode( pmPlay );
    }
  Unlock();
}

void mgPCMPlayer::Forward()
{
  MGLOG( "mgPCMPlayer::Forward" );

  Lock();
  if( NextFile() )
    { 
      StopPlay(); 
      Play(); 
    }
  Unlock();
}

void mgPCMPlayer::Backward(void)
{
  Lock();
  if( PrevFile() ) 
    { 
      StopPlay(); 
      Play(); 
    }
  Unlock();
}

void mgPCMPlayer::Goto( int index, bool still )
{
  m_playlist->gotoPosition( index-1 );
  mgContentItem *next = m_playlist->getCurrent();

  if( next && next != &(mgContentItem::UNDEFINED) ) //invalid
    {
      Lock();
      StopPlay();
      m_current = next;
      Play();
      Unlock();
    }
}

void mgPCMPlayer::SkipSeconds(int secs)
{
  if( m_playmode != pmStopped ) 
    {
      Lock();
      if( m_playmode == pmPaused ) 
	{
	  SetPlayMode( pmPlay );
	}
      if( m_decoder && m_decoder->skip( secs, m_ringbuffer->Available(), dvbSampleRate ) )
	{
	  levelgood=false;
	}
      Empty();
      Unlock();
    }
}

bool mgPCMPlayer::getIndex( int &current, int &total, bool snaptoiframe )
{
  if( m_current ) 
    {  
      current = SecondsToFrames( m_index ); 
      total = SecondsToFrames( m_current->getLength() );
      return true;
    }
  return false;
}

// --- mgPlayerControl -------------------------------------------------------

mgPlayerControl::mgPlayerControl( mgPlaylist *plist, int start )
  : cControl( m_player = new mgPCMPlayer( plist, start ) )
{
  MGLOG( "mgPlayerControl::mgPlayerControl" );

#if VDRVERSNUM >= 10307
  m_display = NULL;
  m_menu = NULL;
#endif
  m_visible = false;
  m_has_osd = false;
  m_track_view = true;
  m_progress_view = true;

  m_szLastShowStatusMsg = NULL;

  // Notify all cStatusMonitor
  StatusMsgReplaying();
}

mgPlayerControl::~mgPlayerControl()
{
  // Notify cleanup all cStatusMonitor
  cStatus::MsgReplaying(this, NULL);
  if( m_szLastShowStatusMsg )
    {  
      free(m_szLastShowStatusMsg);
      m_szLastShowStatusMsg = NULL;
    }

  InternalHide();
  Stop();
}

bool mgPlayerControl::Active(void)
{
  return m_player && m_player->Active();
}

void mgPlayerControl::Stop(void)
{
  if( m_player )
    {
      delete m_player; 
      m_player = 0;
    }
}

void mgPlayerControl::Pause(void)
{
  if( m_player )
    {
      m_player->Pause();
    }
}

void mgPlayerControl::Play(void)
{
  if( m_player ) 
    {
      m_player->Play();
    }
}

void mgPlayerControl::Forward(void)
{
  if( m_player ) 
    {
      m_player->Forward();
    }
}

void mgPlayerControl::Backward(void)
{
  if( m_player ) 
    {
      m_player->Backward();
    }
}

void mgPlayerControl::SkipSeconds(int Seconds)
{
  if( m_player ) 
    {
      m_player->SkipSeconds(Seconds);
    }
}

void mgPlayerControl::Goto(int Position, bool Still)
{
  if( m_player )
    {
      m_player->Goto(Position, Still);
    }
}

void mgPlayerControl::ToggleShuffle(void)
{
  if( m_player )
    {
      m_player->ToggleShuffle();
    }
}

void mgPlayerControl::ToggleLoop(void)
{
  if( m_player ) 
    {
      m_player->ToggleLoop();
    }
}

void mgPlayerControl::NewPlaylist(mgPlaylist *plist, int start)
{
  if( m_player ) 
    {
      m_player->NewPlaylist(plist, start);
    }
}

void mgPlayerControl::ShowContents()
{
#if VDRVERSNUM >= 10307
  if( !m_menu )
    {
      m_menu = Skins.Current()->DisplayMenu();
    }

  if( m_player && m_menu ) 
    {
      int num_items = m_menu->MaxItems();

      if( m_track_view )
	{
	  m_menu->Clear();
	  m_menu->SetTitle( "Track info view" );

	  m_menu->SetTabs( 15 );
	  
	  char *buf;
	  if( num_items > 0 )
	    {
	      asprintf( &buf, "Title:\t%s", m_player->getCurrent()->getLabel(0).c_str() );
	      m_menu->SetItem( buf, 0, false, false );
	      free( buf );
	    }
	  if( num_items > 1 )
	    {
	      asprintf( &buf, "Artist:\t%s", m_player->getCurrent()->getLabel(1).c_str() );
	      m_menu->SetItem( buf, 1, false, false );
	      free( buf );
	    }
	  if( num_items > 2 )
	    {
	      asprintf( &buf, "Album:\t%s", m_player->getCurrent()->getLabel(2).c_str() );
	      m_menu->SetItem( buf, 2, false, false );
	      free( buf );
	    }
	  if( num_items > 3 )
	    {
	      asprintf( &buf, "Genre:\t%s", m_player->getCurrent()->getLabel(3).c_str() );
	      m_menu->SetItem( buf, 3, false, false );
	      free( buf );
	    }
	  if( num_items > 4 )
	    {
	      int len = m_player->getCurrent()->getLength();
	      asprintf( &buf, "Length:\t%s", IndexToHMSF( SecondsToFrames( len ) ) );
	      m_menu->SetItem( buf, 4, false, false );
	      free( buf );
	    }
	  if( num_items > 5 )
	    {
	      asprintf( &buf, "Bit rate:\t%s", m_player->getCurrent()->getBitrate().c_str() );
	      m_menu->SetItem( buf, 5, false, false );
	      free( buf );
	    }
	  if( num_items > 6 )
	    {
	      int sr = m_player->getCurrent()->getSampleRate();
	      
	      asprintf( &buf, "Sampling rate:\t%d", sr );
	      m_menu->SetItem( buf, 6, false, false );
	      free( buf );
	    }
	}
      else
	{
	  mgPlaylist *list = m_player->getPlaylist();
	  if( list )
	    {
	      // use items for playlist tag display
	      m_menu->Clear();
	      m_menu->SetTitle( "Playlist info view" );

	      int cur = list->getIndex();
	      
	      char *buf;
	      for( int i=0; i < num_items; i ++ )
		{
		  mgContentItem *item = list->getItem( cur-3+i );
		  if( item->isValid() )
		    {
		      asprintf( &buf, "%s\t%s", item->getLabel(0).c_str(), item->getLabel(1).c_str() );
		      if( i < 3 )
			{ // already played
			  m_menu->SetItem( buf, i, false, false );
			}
		      if( i > 3 )
			{ // to be played
			  m_menu->SetItem( buf, i, false, true );
			}
		      if( i == 3 )
			{
			  m_menu->SetItem( buf, i, true, true );
			}
		      free( buf );
		    }
		}
	    }
	}
    }
#endif
}

void mgPlayerControl::ShowProgress()
{
  if( m_player )
    {
      char *buf;
      bool play = true, forward = true;
      int speed = -1;

      int current_frame, total_frames;
      m_player->getIndex( current_frame, total_frames ); 
      
      if( !m_track_view )
	{ // playlist stuff
	  mgPlaylist *list = m_player->getPlaylist();
	  if( list )
	    {
	      total_frames = SecondsToFrames( list->getLength() );
	      current_frame += SecondsToFrames( list->getCompletedLength() );
	      asprintf( &buf, "Playlist %s (%d/%d)", list->getListname().c_str(), list->getIndex()+1, list->getNumItems() );
	    }
	}
      else
	{ // track view
	  asprintf( &buf, "%s: %s", m_player->getCurrent()->getLabel(1).c_str(), m_player->getCurrent()->getTitle().c_str() );
	}

#if VDRVERSNUM >= 10307
      if( !m_display )
	{
	  m_display = Skins.Current()->DisplayReplay(false);
	}
      if( m_display ) 
	{
	  m_display->SetProgress( current_frame, total_frames );
	  m_display->SetCurrent( IndexToHMSF( current_frame ) );
	  m_display->SetTotal( IndexToHMSF( total_frames ) );	  
	  m_display->SetTitle( buf );
	  m_display->SetMode( play, forward, speed );
	  m_display->Flush();
	}
#else
      int w = Interface->Width();
      int h = Interface->Height();
      
      Interface->WriteText( w/2, h/2, "Muggle is active!" );
      Interface->Flush();
#endif
      free( buf );
    }
}

void mgPlayerControl::Display()
{
  StatusMsgReplaying();

  if( m_visible )
    {
      if( !m_has_osd )
	{
	  // open the osd if its not already there...
#if VDRVERSNUM >= 10307
#else
	  Interface->Open();
#endif
	  m_has_osd = true;
	}
      
      // now an osd is open, go on
      if( m_progress_view )
	{
#if VDRVERSNUM >= 10307
	  if( m_menu )
	    {
	      delete m_menu;
	      m_menu = NULL;
	    }
#endif
	  ShowProgress();
	}
      else
	{
#if VDRVERSNUM >= 10307
	  if( m_display )
	    {
	      delete m_display;
	      m_display = NULL;
	    }
#endif
	  ShowContents();
	}
    }
  else
    {
      InternalHide();
    }
}

void mgPlayerControl::Hide()
{
  m_visible = false;
  
  InternalHide();
}


void mgPlayerControl::InternalHide()
{
  if( m_has_osd )
    {
#if VDRVERSNUM >= 10307
      if( m_display )
	{
	  delete m_display;
	  m_display = NULL;
	}
      if( m_menu )
	{
	  delete m_menu;
	  m_menu = NULL;
	}
#else
      Interface->Close();
#endif
      m_has_osd = false;
    }
}

eOSState mgPlayerControl::ProcessKey(eKeys key)
{
  if( !Active() )
    {
      return osEnd;
    }

  Display();

  eOSState state = cControl::ProcessKey(key);

  if( state == osUnknown )
    {
      switch( key )
	{
	case kUp:
	  {
	    Forward();
	  } break;
	case kDown:
	  {
	    Backward();
	  } break;
	case kRed:
	  {
	    if( !m_visible && m_player )
	      {
		mgPlaylist *pl = m_player->getPlaylist();

		std::string s;
		switch( pl->toggleLoopMode() )
		  {
		  case mgPlaylist::LM_NONE:
		    {
		      s = tr( "Loop mode off" );
		    } break;
		  case mgPlaylist::LM_SINGLE:
		    {
		      s = tr( "Loop mode single" );
		    } break;
		  case mgPlaylist::LM_FULL:
		    {
		      s = tr( "Loop mode full" );
		    } break;
		  default:
		    {
		      s = tr( "Unknown loop mode" );
		    }
#if VDRVERSNUM >= 10307
			Skins.Message(mtInfo, s.c_str() );
			Skins.Flush();
#else
			Interface->Status( s.c_str() );
			Interface->Flush();
#endif
		  }
	      }
	    else
	      {
		// toggle progress display between simple and detail
		m_progress_view = !m_progress_view;
		Display();
	      }
	  } break;
	case kGreen:
	  {
	    if( !m_visible && m_player )
	      {
		mgPlaylist *pl = m_player->getPlaylist();

		std::string s;
		switch( pl->toggleShuffleMode() )
		  {
		  case mgPlaylist::SM_NONE:
		    {
		      s = tr( "Shuffle mode off" );
		    } break;
		  case mgPlaylist::SM_NORMAL:
		    {
		      s = tr( "Shuffle mode normal" );
		    } break;
		  case mgPlaylist::SM_PARTY:
		    {
		      s = tr( "Shuffle mode party" );
		    } break;
		  default:
		    {
		      s = tr( "Unknown shuffle mode" );
		    }
#if VDRVERSNUM >= 10307
			Skins.Message(mtInfo, s.c_str() );
			Skins.Flush();
#else
			Interface->Status( s.c_str() );
			Interface->Flush();
#endif
		  }
	      }
	    else
	      {
		// toggle progress display between playlist and track
		m_track_view = !m_track_view;
		Display();
	      }
	  } break;
	case kPause:
	case kYellow:
	  {
	    Pause();
	  } break;
	case kStop:
	case kBlue:
	  {
	    InternalHide();
	    Stop();
	    
	    return osEnd;
	  } break;
	case kOk:
	  {
	    m_visible = !m_visible;
	    Display();

	    return osContinue;
	  } break;
	case kBack:
	  {
	    InternalHide();
	    Stop();

	    return osEnd;
	  } break;
	default:
	  {
	    return osUnknown;
	  }
	}
    }
  return osContinue;
}

void mgPlayerControl::StatusMsgReplaying()
{
  char *szBuf=NULL;
  if(m_player 
      && m_player->getCurrent() 
      && m_player->getCurrent()->isValid() 
      && m_player->getCurrent()->getContentType() == mgContentItem::GD_AUDIO
      && m_player->getPlaylist()) 
    {
      /* 
	 if(m_player->getCurrent()->getAlbum().length() > 0)
	 {  
	 asprintf(&szBuf,"[%c%c] (%d/%d) %s - %s",
	 m_player->getPlaylist()->isLoop()?'L':'.',
	 m_player->getPlaylist()->isShuffle()'S':'.',
	 m_player->getPlaylist()->getIndex() + 1,m_player->getPlaylist()->getCount(),
	 m_player->getCurrent()->getTitle().c_str(),
	 m_player->getCurrent()->getAlbum().c_str());
	 }
	 else */
      {  
	asprintf(&szBuf,"[%c%c] (%d/%d) %s",
		 /* TODO m_player->GetPlaylist()->isLoop()?'L':*/'.',
		 /* TODO m_player->GetPlaylist()->isShuffle()'S':*/'.',
		 m_player->getPlaylist()->getIndex() + 1,m_player->getPlaylist()->getNumItems(),
		 m_player->getCurrent()->getTitle().c_str());
      }
    }
  else
    {
      asprintf(&szBuf,"[muggle]");
    }
  
  if(szBuf 
     && ( m_szLastShowStatusMsg == NULL 
	  || 0 != strcmp(szBuf,m_szLastShowStatusMsg) ) )  
    {  
      cStatus::MsgReplaying(this,szBuf);

      if(m_szLastShowStatusMsg)
	{
	  free(m_szLastShowStatusMsg);
	}
      m_szLastShowStatusMsg = szBuf;
    }
  else
    {
      free(szBuf);
    }
}
