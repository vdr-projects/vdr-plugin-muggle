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

using namespace std;

// --- mgOggFile ----------------------------------------------------------------

class mgOggFile // : public mgFileInfo
{
 private:

  bool m_opened, m_canSeek;

  OggVorbis_File vf;

  void Error( const char *action, const int err );

  string m_filename;

 public:

  mgOggFile( string filename );
  ~mgOggFile();

  bool open(bool log=true);

  void close(void);

  long long seek(long long posMs=0, bool relativ=false);

  int stream(short *buffer, int samples);

  bool canSeek(void) { return canSeek; }

  long long indexMs(void);
};

mgOggFile::mgOggFile( string filename ) :
  m_filename( filename )
{
  m_canSeek = false;
  m_opened = false;
}

mgOggFile::~mgOggFile()
{
  m_close();
}

bool mgOggFile::Open(bool log)
{
  if( m_opened )
  {
    if( m_canSeek )
      {
	return ( seek() >= 0 );
      }
    return true;
  }
  
  FILE *f = fopen( filename.c_str(), "r" );
  if( f )
    {
      int r = ov_open( f, &vf, 0, 0 );
      if( !r ) 
	{
	  canSeek = ( ov_seekable( &vf ) !=0 );
	  opened = true;
	}
      else
	{
	  fclose( f );
	  if( log ) 
	    {	
	      error( "open", r );
	    }
	}
    }
  else
    {
      if(log) 
	{ 
	  esyslog("ERROR: failed to open file %s: %s", filename.c_str(), strerror(errno) ); 
	}
    }
  return m_opened;
}
  
void mgOggFile::close()
{
  if( m_opened )
    { 
      ov_clear( &vf ); 
      m_opened = false;
    }
}

void mgOggFile::error( const char *action, const int err )
{
  char *errstr;
  switch(err) 
  {
    case OV_FALSE:      errstr = "false/no data available"; break;
    case OV_EOF:        errstr = "EOF"; break;
    case OV_HOLE:       errstr = "missing or corrupted data"; break;
    case OV_EREAD:      errstr = "read error"; break;
    case OV_EFAULT:     errstr = "internal error"; break;
    case OV_EIMPL:      errstr = "unimplemented feature"; break;
    case OV_EINVAL:     errstr = "invalid argument"; break;
    case OV_ENOTVORBIS: errstr = "no Ogg Vorbis stream"; break;
    case OV_EBADHEADER: errstr = "corrupted Ogg Vorbis stream"; break;
    case OV_EVERSION:   errstr = "unsupported bitstream version"; break;
    case OV_ENOTAUDIO:  errstr = "ENOTAUDIO"; break;
    case OV_EBADPACKET: errstr = "EBADPACKET"; break;
    case OV_EBADLINK:   errstr = "corrupted link"; break;
    case OV_ENOSEEK:    errstr = "stream not seekable"; break;
    default:            errstr = "unspecified error"; break;
  }
  esyslog( "ERROR: vorbisfile %s failed on %s: %s", action, m_filename.c_str(), errstr );
}

long long mgOggFile::indexMs(void)
{
  double p = ov_time_tell(&vf);
  if( p < 0.0 )
    {
      p = 0.0;
    }

  return (long long)( p*1000.0 );
}

long long mgOggFile::seek( long long posMs, bool relativ )
{
  if( relativ ) 
    {
      posMs += indexMs();
    }

  int r = ov_time_seek( &vf, (double) posMs/1000.0 );
  
  if(r)
    {
      error( "seek", r );
      return -1;
    }
  
  posMs = indexMs();
  return posMs;
}

int mgOggFile::stream( short *buffer, int samples )
{
  int n;
  do 
    {
      int stream;
      n = ov_read( &vf, (char *)buffer, samples*2, 0, 2, 1, &stream );
    } 
  while( n == OV_HOLE );
  
  if(n < 0) 
    {
      error( "read", n );
    }

  return (n/2);
}

// --- mgOggDecoder -------------------------------------------------------------

mgOggDecoder::mgOggDecoder( string filename ) 
  : mgDecoder( filename )
{
  m_file = new mgOggFile( filename );
  m_pcm = 0;
  init();
}

mgOggDecoder::~mgOggDecoder()
{
  delete m_file;
  clean();
}

bool mgOggDecoder::valid()
{
  bool res = false;
  if( tryLock() ) 
  {
    if( m_file->open( false ) ) 
      {
	res = true;
      }
    unlock();
  }
  return res;
}

cPlayInfo *mgOggDecoder::playInfo(void)
{
  if( playing )
  {
    pi.Index = index/1000;
    pi.Total = info.Total;

    return &pi;
  }
  return 0;
}

void mgOggDecoder::init()
{
  clean();
  m_pcm   = new struct mad_pcm;
  m_index = 0;
}

bool mgOggDecoder::clean()
{
  m_playing = false;
  
  delete m_pcm; 
  m_pcm = 0;
  
  m_file->close();
  return false;
}

#define SF_SAMPLES (sizeof(pcm->samples[0])/sizeof(mad_fixed_t))

bool mgOggDecoder::start()
{
  lock(true);
  init(); 
  m_playing = true;

  if( file.open() /*&& info.DoScan(true)*/ ) 
    {
      // obtain from database: rate, channels
      /* d(printf("ogg: open rate=%d channels=%d seek=%d\n",
	       info.SampleFreq,info.Channels,file.CanSeek()))
      */
      if( info.channels <= 2 )
	{
	  unlock();
	  return true;
	}
      else 
	{
	  esyslog( "ERROR: cannot play ogg file %s: more than 2 channels", m_filename.c_str() );
	}
    }
  
  clean();
  unlock();

  return false;
}

bool mgOggDecoder::stop(void)
{
  lock();

  if( m_playing )
  {
    clean();
  }
  unlock();

  return true;
}

struct Decode *mgOggDecoder::done(eDecodeStatus status)
{
  ds.status = status;
  ds.index  = m_index;
  ds.pcm    = m_pcm;

  unlock(); // release the lock from Decode()

  return &ds;
}

struct Decode *mgOggDecoder::Decode(void)
{
  Lock(); // this is released in Done()
  if(playing)
  {
    short framebuff[2*SF_SAMPLES];
    int n = m_file->stream( framebuff, SF_SAMPLES );

    if( n < 0 ) 
      {
	return done(dsError);
      }

    if( n == 0 ) 
      {
	return done(dsEof);
      }

    // TODO
    pcm->samplerate = info.SampleFreq;  // from database
    pcm->channels   = info.Channels;    // from database

    n /= pcm->channels;
    pcm->length = n;
    index = m_file->indexMs();

    short *data = framebuff;
    mad_fixed_t *sam0 = pcm->samples[0], *sam1 = pcm->samples[1]; 

    const int s = MAD_F_FRACBITS + 1 - ( sizeof(short)*8 ); // shift value for mad_fixed conversion

    if( pcm->channels>1 ) 
      {
	for(; n > 0 ; n-- )
	  {
	    *sam0++=(*data++) << s;
	    *sam1++=(*data++) << s;
	  }
      }
    else 
      {
	for(; n>0 ; n--)
	  {
	    *sam0++=(*data++) << s;
	  }
      }
    return done(dsPlay);
  }
  return done(dsError);
}

bool mgOggDecoder::skip(int Seconds, int Avail, int Rate)
{
  lock();
  bool res = false;

  if( playing && m_file->canSeek() ) 
    {
      float fsecs = (float)Seconds - ( (float)Avail / (float)(Rate * (16/8 * 2) ) );  
      // Byte/s = samplerate * 16 bit * 2 chan

      long long newpos = m_file->indexMs() + (long long)(fsecs*1000.0);

      if( newpos < 0 ) 
	{
	  newpos=0;
	}

      newpos = m_file.Seek(newpos,false);
      
      if( newpos >= 0 )
	{
	  index = m_file->indexMs();
#ifdef DEBUG
	  int i = index/1000;
	  printf( "ogg: skipping to %02d:%02d\n", i/60, i%60 );
#endif
	  res = true;
	}
    }
  unlock();
  return res;
}

#endif //HAVE_VORBISFILE
