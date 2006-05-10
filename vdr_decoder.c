/*!
 * \file vdr_decoder.h
 * \brief A generic decoder for a VDR media plugin (muggle)
 * \ingroup vdr
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * $Id$
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

#include "mg_selection.h"

#include <vdr/videodir.h>
#include <vdr/interface.h>


#include "vdr_decoder.h"
#include "vdr_decoder_mp3.h"
#include "vdr_decoder_ogg.h"
#include "vdr_decoder_flac.h"
#include "vdr_decoder_sndfile.h"

extern void showmessage(int duration,const char *,...);

#ifdef HAVE_VORBISFILE
#include "vdr_decoder_ogg.h"
#endif

#ifdef HAVE_FLAC
#include "vdr_decoder_flac.h"
#endif


// --- mgDecoders ---------------------------------------------------------------

mgMediaType mgDecoders::getMediaType (std::string s)
{
  mgMediaType mt = MT_UNKNOWN;

  char *
    f = (char *) s.c_str ();
  char *
    p = f + strlen (f) - 1;                   // point to the end

  while (p >= f && *p != '.')
    --p;

  if (!strcasecmp (p, ".mp3"))
    {
      mt = MT_MP3;
    }
  else
    {
      if (!strcasecmp (p, ".ogg"))
        {
#ifdef HAVE_VORBISFILE
	  mt = MT_OGG;
#else
	      	mgWarning("Support for vorbis not compiled in, define HAVE_VORBISFILE in Makefile");
#endif
        }
      else
	{
	  if (!strcasecmp (p, ".flac"))
	    {
#ifdef HAVE_FLAC
	      mt = MT_FLAC;
#else
	      	mgWarning("Support for flac not compiled in, define HAVE_FLAC in Makefile");
#endif
	    }
	  else
	    {
	      if( !strcasecmp( p, ".wav" ) )
		{
#ifdef HAVE_SNDFILE
		  mt = MT_SND;
#else
		  mgWarning("Support for wav files not compiled in, define HAVE_SNDFILE in Makefile" );
#endif
		}
	    }
	}
    }
  return mt;
}


mgDecoder *
mgDecoders::findDecoder (mgItemGd * item)
{
    mgDecoder *decoder = 0;

    std::string filename = item->getSourceFile ();

    switch (getMediaType (filename))
      {
      case MT_MP3:
	{
	  decoder = new mgMP3Decoder (item);
	} break;
#ifdef HAVE_VORBISFILE
      case MT_OGG:
	{
	  decoder = new mgOggDecoder (item);
	} break;
#endif
#ifdef HAVE_FLAC
      case MT_FLAC:
	{
	  decoder = new mgFlacDecoder( item );
	} break;
#endif
#ifdef HAVE_SNDFILE
      case MT_SND:  
	{
	  decoder = new mgSndfileDecoder( item ); 
	} break;
#endif
      default:
	{
	  esyslog ("ERROR: unknown media type ");
	}
	break;
      }
    
    if (decoder && !decoder->valid ())
      {
	// no decoder found or decoder doesn't match
	
        delete decoder;                           // might be carried out on NULL pointer
        decoder = 0;
	
        esyslog ("ERROR: no valid decoder found for %s", filename.c_str ());
      }
    return decoder;
}


// --- mgDecoder ----------------------------------------------------------------

mgDecoder::mgDecoder (mgItemGd * item)
{
    m_item = item;
    m_locked = 0;
    m_urgentLock = false;
    m_playing = false;
}

mgDecoder::~mgDecoder ()
{
}

void
mgDecoder::lock (bool urgent)
{
    m_locklock.Lock ();

    if (urgent && m_locked)
      {
        m_urgentLock = true;                      // signal other locks to release quickly
      }
    m_locked++;
    
    m_locklock.Unlock ();                         // don't hold the "locklock" when locking "lock", may cause a deadlock
    m_lock.Lock ();
    m_urgentLock = false;
}


void
mgDecoder::unlock (void)
{
    m_locklock.Lock ();

    m_locked--;

    m_lock.Unlock ();
    m_locklock.Unlock ();
}


bool mgDecoder::tryLock (void)
{
    bool
        res = false;
    m_locklock.Lock ();

    if (!m_locked && !m_playing)
    {
    	m_locked++;
    
    	m_locklock.Unlock ();                  	// don't hold the "locklock" when locking
						// "lock", may cause a deadlock
    	m_lock.Lock ();
    	m_urgentLock = false;
        res = true;
    }
    else
    	m_locklock.Unlock ();
    return res;
}
