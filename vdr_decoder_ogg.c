/*! \file vdr_decoder_ogg.c
 *  \ingroup vdr
 *
 *  The file implements a decoder which is used by the player to decode ogg vorbis audio files.
 *
 *  Adapted from
 *  MP3/MPlayer plugin to VDR (C++)
 *  (C) 2001-2003 Stefan Huelswitt <huels@iname.com>
 */

#ifdef HAVE_VORBISFILE

#include "vdr_decoder_ogg.h"

#include <mad.h>
#include <vorbis/vorbisfile.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "vdr_setup.h"

// --- mgOggFile ----------------------------------------------------------------

class mgOggFile                                   // : public mgFileInfo
{
    private:

        bool m_opened, m_canSeek;

        OggVorbis_File vf;

        void error (const char *action, const int err);

        std::string m_filename;

    public:

        mgOggFile (std::string filename);
        ~mgOggFile ();

        bool open (bool log = true);

        void close (void);

        long long seek (long long posMs = 0, bool relativ = false);

        int stream (short *buffer, int samples);

        bool canSeek ()
        {
            return m_canSeek;
        }

        long long indexMs (void);
};

mgOggFile::mgOggFile (std::string filename):
m_filename (filename)
{
    m_canSeek = false;
    m_opened = false;
}


mgOggFile::~mgOggFile ()
{
    close ();
}


bool mgOggFile::open (bool log)
{
    if (m_opened)
    {
        if (m_canSeek)
        {
            return (seek () >= 0);
        }
        return true;
    }

    FILE *
        f = fopen (m_filename.c_str (), "r");
    if (f)
    {
        int
            r = ov_open (f, &vf, 0, 0);
        if (!r)
        {
            m_canSeek = (ov_seekable (&vf) != 0);
            m_opened = true;
        }
        else
        {
            fclose (f);
            if (log)
            {
                error ("open", r);
            }
        }
    }
    else
    {
        if (log)
        {
//      esyslog("ERROR: failed to open file %s: %s", m_filename.c_str(), strerror(errno) );
        }
    }
    return m_opened;
}


void
mgOggFile::close ()
{
    if (m_opened)
    {
        ov_clear (&vf);
        m_opened = false;
    }
}


void
mgOggFile::error (const char *action, const int err)
{
    char *errstr;
    switch (err)
    {
        case OV_FALSE:
            errstr = "false/no data available";
            break;
        case OV_EOF:
            errstr = "EOF";
            break;
        case OV_HOLE:
            errstr = "missing or corrupted data";
            break;
        case OV_EREAD:
            errstr = "read error";
            break;
        case OV_EFAULT:
            errstr = "internal error";
            break;
        case OV_EIMPL:
            errstr = "unimplemented feature";
            break;
        case OV_EINVAL:
            errstr = "invalid argument";
            break;
        case OV_ENOTVORBIS:
            errstr = "no Ogg Vorbis stream";
            break;
        case OV_EBADHEADER:
            errstr = "corrupted Ogg Vorbis stream";
            break;
        case OV_EVERSION:
            errstr = "unsupported bitstream version";
            break;
        case OV_ENOTAUDIO:
            errstr = "ENOTAUDIO";
            break;
        case OV_EBADPACKET:
            errstr = "EBADPACKET";
            break;
        case OV_EBADLINK:
            errstr = "corrupted link";
            break;
        case OV_ENOSEEK:
            errstr = "stream not seekable";
            break;
        default:
            errstr = "unspecified error";
            break;
    }
//  esyslog( "ERROR: vorbisfile %s failed on %s: %s", action, m_filename.c_str(), errstr );
}


long long
mgOggFile::indexMs (void)
{
    double p = ov_time_tell (&vf);
    if (p < 0.0)
    {
        p = 0.0;
    }

    return (long long) (p * 1000.0);
}


long long
mgOggFile::seek (long long posMs, bool relativ)
{
    if (relativ)
    {
        posMs += indexMs ();
    }

    int r = ov_time_seek (&vf, (double) posMs / 1000.0);

    if (r)
    {
        error ("seek", r);
        return -1;
    }

    posMs = indexMs ();
    return posMs;
}


int
mgOggFile::stream (short *buffer, int samples)
{
    int n;
    do
    {
        int stream;
        n = ov_read (&vf, (char *) buffer, samples * 2, 0, 2, 1, &stream);
    }
    while (n == OV_HOLE);

    if (n < 0)
    {
        error ("read", n);
    }

    return (n / 2);
}


// --- mgOggDecoder -------------------------------------------------------------

mgOggDecoder::mgOggDecoder (mgItemGd * item):mgDecoder (item)
{
  m_filename = item->getSourceFile ();
  m_file = new mgOggFile (m_filename);
  m_pcm = 0;
  init ();
}


mgOggDecoder::~mgOggDecoder ()
{
    clean ();
    delete m_file;
}


bool mgOggDecoder::valid ()
{
    bool
        res = false;
    if (tryLock ())
    {
        if (m_file->open (false))
        {
            res = true;
        }
        unlock ();
    }
    return res;
}


mgPlayInfo *
mgOggDecoder::playInfo (void)
{
    if (m_playing)
    {
//    m_playinfo.m_index = index/1000;
//    m_playinfo.m_total = info.Total;

        return &m_playinfo;
    }

    return 0;
}


void
mgOggDecoder::init ()
{
    clean ();
    m_pcm = new struct mad_pcm;
    m_index = 0;
}


bool mgOggDecoder::clean ()
{
    m_playing = false;

    delete
        m_pcm;
    m_pcm = 0;

    m_file->close ();
    return false;
}


#define SF_SAMPLES (sizeof(m_pcm->samples[0])/sizeof(mad_fixed_t))

bool mgOggDecoder::start ()
{
    lock (true);
    init ();
    m_playing = true;

    if (m_file->open () /*&& info.DoScan(true) */ )
    {
// obtain from database: rate, channels
/* d(printf("ogg: open rate=%d channels=%d seek=%d\n",
   info.SampleFreq,info.Channels,file.CanSeek()))
 */
        if (m_item->getChannels () <= 2)
        {
            unlock ();
            return true;
        }
        else
        {
//      esyslog( "ERROR: cannot play ogg file %s: more than 2 channels", m_filename.c_str() );
        }
    }

    clean ();
    unlock ();

    return false;
}


bool mgOggDecoder::stop (void)
{
    lock ();

    if (m_playing)
    {
        clean ();
    }
    unlock ();

    return true;
}


struct mgDecode *
mgOggDecoder::done (eDecodeStatus status)
{
    m_ds.status = status;
    m_ds.index = m_index;
    m_ds.pcm = m_pcm;

    unlock ();                                    // release the lock from decode()

    return &m_ds;
}


struct mgDecode *
mgOggDecoder::decode (void)
{
    lock ();                                      // this is released in Done()

    if (m_playing)
    {
        short framebuff[2 * SF_SAMPLES];
        int n = m_file->stream (framebuff, SF_SAMPLES);

        if (n < 0)
        {
            return done (dsError);
        }

        if (n == 0)
        {
            return done (dsEof);
        }

// should be done during initialization
                                                  // from database
        m_pcm->samplerate = m_item->getSampleRate ();
        m_pcm->channels = m_item->getChannels (); // from database

        n /= m_pcm->channels;
        m_pcm->length = n;
        m_index = m_file->indexMs ();

        short *data = framebuff;
        mad_fixed_t *sam0 = m_pcm->samples[0], *sam1 = m_pcm->samples[1];

                                                  // shift value for mad_fixed conversion
        const int s = MAD_F_FRACBITS + 1 - (sizeof (short) * 8);

        if (m_pcm->channels > 1)
        {
            for (; n > 0; n--)
            {
                *sam0++ = (*data++) << s;
                *sam1++ = (*data++) << s;
            }
        }
        else
        {
            for (; n > 0; n--)
            {
                *sam0++ = (*data++) << s;
            }
        }
        return done (dsPlay);
    }
    return done (dsError);
}


bool mgOggDecoder::skip (int Seconds, int Avail, int Rate)
{
    lock ();
    bool
        res = false;

    if (m_playing && m_file->canSeek ())
    {
        float
            fsecs =
            (float) Seconds - ((float) Avail / (float) (Rate * (16 / 8 * 2)));
// Byte/s = samplerate * 16 bit * 2 chan

        long long
            newpos = m_file->indexMs () + (long long) (fsecs * 1000.0);

        if (newpos < 0)
        {
            newpos = 0;
        }

        newpos = m_file->seek (newpos, false);

        if (newpos >= 0)
        {
            m_index = m_file->indexMs ();
#ifdef xDEBUG
            int
                i = index / 1000;
            printf ("ogg: skipping to %02d:%02d\n", i / 60, i % 60);
#endif
            res = true;
        }
    }
    unlock ();
    return res;
}
#endif                                            //HAVE_VORBISFILE
