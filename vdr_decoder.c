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

#include <videodir.h>
#include <interface.h>


#include "mg_setup.h"
#include "vdr_decoder.h"
#include "vdr_decoder_mp3.h"

extern void showmessage(const char *,int duration=0);

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
	}
    }
  return mt;
}


mgDecoder *
mgDecoders::findDecoder (mgContentItem * item)
{
    mgDecoder *decoder = 0;

    std::string filename = item->getSourceFile ();

    struct stat st;
    if (stat (filename.c_str (), &st))
    {
	char *b=0;
	int nsize = filename.size();
	if (nsize<30)
		asprintf(&b,tr("%s not readable"),filename.c_str());
	else
		asprintf(&b,tr("%s..%s not readable"),filename.substr(0,20).c_str(),filename.substr(nsize-20).c_str());;
	showmessage(b);
	free(b);
        esyslog ("ERROR: cannot stat %s. Meaning not found, not a valid file, or no access rights", filename.c_str ());
        return 0;
    }

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
	/*
	  case MT_MP3_STREAM: decoder = new mgMP3StreamDecoder(full); break;
	  #ifdef HAVE_SNDFILE
	  case MT_SND:  decoder = new cSndDecoder(full); break;
	  #endif
	*/
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

mgDecoder::mgDecoder (mgContentItem * item)
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
