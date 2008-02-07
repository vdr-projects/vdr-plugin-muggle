/*!
 * \file  vdr_decoder_mp3.h
 * \brief An mp3 decoder for a VDR media plugin (muggle)
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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <cmath>
#include <iostream>

#include "vdr_config.h"
#include "vdr_decoder_mp3.h"
#include "vdr_stream.h"

#include "mg_tools.h"

#define d(x) x

// ----------------------------------------------------------------

int
mgMadStream (struct mad_stream *stream, mgStream * str)
{
    unsigned char *data;
    unsigned long len;
    if (str->stream (data, len, stream->next_frame))
    {
        if (len > 0)
        {
            mad_stream_buffer (stream, data, len);
        }
        return len;
    }
    return -1;
}


// --- mgMP3Decoder -------------------------------------------------------------

mgMP3Decoder::mgMP3Decoder (mgItemGd * item, bool preinit):mgDecoder
(item)
{
    m_stream = 0;
    m_isStream = false;

    m_filename = item->getSourceFile ();

    if (preinit)
    {
        m_stream = new mgStream (m_filename);
        printf ("m_stream for %s\n", m_filename.c_str ());

    }
    m_madframe = 0;
    m_madsynth = 0;

    memset (&m_madstream, 0, sizeof (m_madstream));

    init ();
}


mgMP3Decoder::~mgMP3Decoder ()
{
    clean ();
    delete m_stream;
}


void
mgMP3Decoder::init ()
{
    clean ();
    mad_stream_init (&m_madstream);

    m_madframe = new mad_frame;
    mad_frame_init (m_madframe);

    m_madsynth = new mad_synth;
    mad_synth_init (m_madsynth);

    mad_stream_options (&m_madstream, MAD_OPTION_IGNORECRC);

    m_playtime = mad_timer_zero;
    m_skiptime = mad_timer_zero;
    m_framenum = m_framemax = 0;
    m_frameinfo = 0;
    m_mute = m_errcount = 0;
}


void
mgMP3Decoder::clean ()
{
    m_playing = false;
    if (m_madsynth)
    {
        mad_synth_finish (m_madsynth);
        delete m_madsynth;
        m_madsynth = 0;
    }

    if (m_madframe)
    {
        mad_frame_finish (m_madframe);
        delete m_madframe;
        m_madframe = 0;
    }
    mad_stream_finish (&m_madstream);
}


bool mgMP3Decoder::valid (void)
{
    bool
        res = false;
    if (tryLock ())
    {
        if (start ())
        {
            struct mgDecode *
                dd;
            int
                count = 10;
            do
            {
                dd = decode ();
                if (dd->status == dsEof)
                {
                    count = 0;
                }
                if (dd->status != dsPlay)
                {
                    break;
                }
            }
            while (--count);
            if (!count)
            {
                res = true;
            }
            stop ();
        }
        unlock ();
    }
    return res;
}


mgPlayInfo *
mgMP3Decoder::playInfo ()
{
    if (m_playing)
    {
        m_playinfo.m_index = mad_timer_count (m_playtime, MAD_UNITS_SECONDS);

        return &m_playinfo;
    }
    return 0;
}


bool mgMP3Decoder::start ()
{
    lock (true);
    init ();
    m_playing = true;

    if (m_stream->open (true))
    {
        if (!m_isStream)
        {
            m_stream->seek ();

            printf
                ("mgMP3Decoder::start: m_framemax not determined, rewinding disabled!!!\n");

/* m_framemax = scan->Frames+20; // TODO

   m_frameinfo = new struct FrameInfo[m_framemax];
   if(!m_frameinfo)
   {
   printf( "mgMP3Decoder::start: no memory for frame index, rewinding disabled" );
   }
 */
        }
        unlock ();

        printf ("mgMP3Decoder::start: true\n");
        return true;
    }
    m_stream->close ();
    clean ();
    unlock ();
    printf ("mgMP3Decoder::start: false");
    return false;
}


bool mgMP3Decoder::stop (void)
{
    lock ();

    if (m_playing)
    {
        m_stream->close ();
        clean ();
    }
    unlock ();
    return true;
}


struct mgDecode *
mgMP3Decoder::done (eDecodeStatus status)
{
    m_ds.status = status;
    m_ds.index = mad_timer_count (m_playtime, MAD_UNITS_MILLISECONDS);
    m_ds.pcm = &m_madsynth->pcm;
    unlock ();                                    // release the lock from Decode()

    return &m_ds;
}


eDecodeStatus mgMP3Decoder::decodeError (bool hdr)
{
    if (m_madstream.error == MAD_ERROR_BUFLEN
        || m_madstream.error == MAD_ERROR_BUFPTR)
    {
        int
            s = mgMadStream (&m_madstream, m_stream);
        if (s < 0)
        {
            printf ("mgMP3Decoder::decodeError: dsError returned\n");
            return dsError;
        }
        if (s == 0)
        {
            printf ("mgMP3Decoder::decodeError: dsEof returned\n");
            return dsEof;
        }
    }
    else if (!MAD_RECOVERABLE (m_madstream.error))
    {
        printf
            ("mgMP3Decoder::decodeError: mad decode %sfailed, frame=%d: %s. Returning dsError\n",
            hdr ? "hdr " : "", m_framenum, mad_stream_errorstr (&m_madstream));
        return dsError;
    }
    else if (m_madstream.error!=MAD_ERROR_LOSTSYNC || m_framenum>0)
		// sync is always lost for frame 0, ignore this.
		// Do we use mad incorrectly?
    {
        m_errcount += hdr ? 1 : 100;
        printf
            ("mgMP3Decoder::decodeError: mad decode%s error, frame=%d count=%d: %s. Returning dsOK\n",
            hdr ? " hdr" : "", m_framenum, m_errcount,
            mad_stream_errorstr (&m_madstream));
    }
    return dsOK;
}


struct mgDecode *
mgMP3Decoder::decode ()
{
    lock ();                                      // this is released in Done()
    eDecodeStatus r;

    while (m_playing)
    {
        if (m_errcount >= MAX_FRAME_ERR * 100)
        {
            printf ("mgMP3Decoder::decode: excessive decoding errors,"
                " aborting file %s\n", m_filename.c_str ());
            return done (dsError);
        }

        if (mad_header_decode (&m_madframe->header, &m_madstream) == -1)
        {
            if ((r = decodeError (true)))
            {
                return done (r);
            }
        }
        else
        {
            if (!m_isStream)
            {
#ifdef DEBUG2
                if (m_framenum >= m_framemax)
                {
                    printf ("mgMP3Decoder::start: framenum >= framemax!!!!\n");
                }
#endif
                if (m_frameinfo && m_framenum < m_framemax)
                {
                    m_frameinfo[m_framenum].Pos =
                        m_stream->bufferPos () +
                        (m_madstream.this_frame - m_madstream.buffer);
                    m_frameinfo[m_framenum].Time = m_playtime;
                }
            }

            mad_timer_add (&m_playtime, m_madframe->header.duration);
            m_framenum++;

            if (mad_timer_compare (m_playtime, m_skiptime) >= 0)
            {
                m_skiptime = mad_timer_zero;
            }
            else
            {
                return done (dsSkip);             // skipping, decode next header
            }

            if (mad_frame_decode (m_madframe, &m_madstream) == -1)
            {
                if ((r = decodeError (false)))
                {
                    return done (r);
                }
            }
            else
            {
                m_errcount = 0;

// TODO: // m_scan->InfoHook( &frame->header );

                mad_synth_frame (m_madsynth, m_madframe);

                if (m_mute)
                {
                    m_mute--;
                    return done (dsSkip);
                }
                return done (dsPlay);
            }
        }
    }
    return done (dsError);
}


void
mgMP3Decoder::makeSkipTime (mad_timer_t * skiptime,
mad_timer_t playtime,
int secs, int avail, int dvbrate)
{
    mad_timer_t time;

    *skiptime = playtime;

    mad_timer_set (&time, abs (secs), 0, 0);

    if (secs < 0)
    {
        mad_timer_negate (&time);
    }

    mad_timer_add (skiptime, time);

                                                  // Byte/s = samplerate * 16 bit * 2 chan
    float bufsecs = (float) avail / (float) (dvbrate * (16 / 8 * 2));

    printf ("mgMP3Decoder::makeSkipTime: skip: avail=%d bufsecs=%f\n", avail,
        bufsecs);

    int full = (int) bufsecs;
    bufsecs -= (float) full;

    mad_timer_set (&time, full, (int) (bufsecs * 1000.0), 1000);

    mad_timer_negate (&time);

    mad_timer_add (skiptime, time);

    printf
        ("mgMP3Decoder::makeSkipTime: skip: playtime=%ld secs=%d full=%d bufsecs=%f skiptime=%ld\n",
        mad_timer_count (playtime, MAD_UNITS_MILLISECONDS), secs, full, bufsecs,
        mad_timer_count (*skiptime, MAD_UNITS_MILLISECONDS));
}


bool mgMP3Decoder::skip(int seconds, int avail, int rate)
{
    lock ();

    bool
        res = false;
    if (m_playing && !m_isStream)
    {
        if (!mad_timer_compare (m_skiptime, mad_timer_zero))
        {                                         // allow only one skip at any time
            mad_timer_t
                time;
            makeSkipTime (&time, m_playtime, seconds, avail, rate);

            if (mad_timer_compare (m_playtime, time) <= 0)
            {                                     // forward skip
#ifdef DEBUG
                int
                    i = mad_timer_count (time, MAD_UNITS_SECONDS);
                printf ("mgMP3Decoder::skip: forward skipping to %02d:%02d\n",
                    i / 60, i % 60);
#endif
                m_skiptime = time;
                m_mute = 1;
                res = true;
            }
            else
            {                                     // backward skip
                if (m_frameinfo)
                {
#ifdef DEBUG
                    int
                        i = mad_timer_count (time, MAD_UNITS_SECONDS);
                    printf ("mgMP3Decoder::skip: rewinding to %02d:%02d\n",
                        i / 60, i % 60);
#endif
                    while (m_framenum
                        && mad_timer_compare (time,
                        m_frameinfo[--m_framenum].
                        Time) < 0);
                    m_mute = 2;
                    if (m_framenum >= 2)
                    {
                        m_framenum -= 2;
                    }
                    m_playtime = m_frameinfo[m_framenum].Time;
                    m_stream->seek (m_frameinfo[m_framenum].Pos);
                                                  // reset stream buffer
                    mad_stream_finish (&m_madstream);
                    mad_stream_init (&m_madstream);
#ifdef DEBUG
                    i = mad_timer_count (m_playtime, MAD_UNITS_MILLISECONDS);
                    printf
                        ("mgMP3Decoder::skip: new playtime=%d framenum=%d filepos=%lld\n",
                        i, m_framenum, m_frameinfo[m_framenum].Pos);
#endif
                    res = true;
                }
            }
        }
    }
    unlock ();
    return res;
}
