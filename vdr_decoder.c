/*!
 * \file vdr_decoder.h
 * \brief A generic decoder for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2004/05/28 15:29:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: lvw $
 *
 * $Id: vdr_decoder.c,v 1.2 2004/05/28 15:29:18 lvw Exp $
 *
 * Adapted from:
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001-2003 Stefan Huelswitt <huels@iname.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <videodir.h>
#include <interface.h>

// #include "common.h"
// #include "data-mp3.h"
// #include "decoder-core.h"
// #include "decoder-mp3-stream.h"
// #include "decoder-snd.h"
// #include "decoder-ogg.h"

#include "vdr_decoder.h"
#include "vdr_decoder_mp3.h"

using namespace std;

// --- mgDecoders ---------------------------------------------------------------

mgMediaType mgDecoders::getMediaType( string s )
{
  // TODO: currently handles only mp3. LVW
  return MT_MP3;
}

mgDecoder *mgDecoders::findDecoder( string filename )
{
  mgDecoder *decoder = 0;

  switch( getMediaType( filename ) ) 
    {
    case MT_MP3:  
      {
	// prepend filename with path to music library?? TODO
	printf( "mp3 file detected, launching mgMP3Decoder\n" );
	decoder = new mgMP3Decoder(filename); 
      } break;
      
      /*
	case MT_MP3_STREAM: decoder = new mgMP3StreamDecoder(full); break;
	#ifdef HAVE_SNDFILE
	case MT_SND:  decoder = new cSndDecoder(full); break;
	#endif
	#ifdef HAVE_VORBISFILE
	case MT_OGG:  decoder = new mgOggDecoder(full); break;
	#endif
      */
    default:       
      { 
	esyslog("ERROR: unknown media type" );
      } break;
    }

  if( decoder && !decoder->valid() ) 
    {
      // no decoder found or decoder doesn't match
      
      delete decoder; // might be carried out on NULL pointer!
      decoder = 0;
      
      esyslog("ERROR: no valid decoder found for %s", filename.c_str() );
    }
  return decoder;
}

// --- mgDecoder ----------------------------------------------------------------

mgDecoder::mgDecoder(string filename)
{
  m_filename = filename;
  m_locked = 0; 
  m_urgentLock = false;
  m_playing = false;
}

mgDecoder::~mgDecoder()
{
}

void mgDecoder::lock(bool urgent)
{
  m_locklock.Lock();

  if( urgent && m_locked ) 
    {
      m_urgentLock = true; // signal other locks to release quickly
    }
  m_locked ++;

  m_locklock.Unlock(); // don't hold the "locklock" when locking "lock", may cause a deadlock
  m_lock.Lock();
  m_urgentLock = false;
}

void mgDecoder::unlock(void)
{
  m_locklock.Lock();

  m_locked--;

  m_lock.Unlock();
  m_locklock.Unlock();
}

bool mgDecoder::tryLock(void)
{
  bool res = false;
  m_locklock.Lock();

  if( !m_locked && !m_playing ) 
    {
      lock();
      res = true;
    }
  m_locklock.Unlock();
  return res;
}
