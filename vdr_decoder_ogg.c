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

#include <stdlib.h>
#include <stdio.h>

#include "vdr_decoder_ogg.h"

class cOggFile : public cFileInfo
{
friend class cOggInfo;
private:
  bool opened, canSeek;
  OggVorbis_File vf;
  //
  void Error(const char *action, const int err);
public:
  cOggFile(const char *Filename);
  ~cOggFile();
  bool Open(bool log=true);
  void Close(void);
  long long cOggFile::Seek(long long posMs=0, bool relativ=false);
  int Stream(short *buffer, int samples);
  bool CanSeek(void) { return canSeek; }
  long long IndexMs(void);
  };

// --- cOggFile ----------------------------------------------------------------

cOggFile::cOggFile(const char *Filename)
:cFileInfo(Filename)
{
  canSeek=opened=false;
}

cOggFile::~cOggFile()
{
  Close();
}

bool cOggFile::Open(bool log)
{
  if( opened )
  {
    if( canSeek )
	{
		return (Seek()>=0);
	}
    return true;
  }

  if( FileInfo(log) ) 
  {
    FILE *f=fopen(Filename,"r");
    if(f) 
	{
      int r=ov_open(f,&vf,0,0);
      if(!r) 
	  {
        canSeek=(ov_seekable(&vf)!=0);
        opened=true;
      }
      else
	  {
        fclose(f);
        if(log) 
		{	
			Error("open",r);
		}
      }
    }
    else
	{
		if(log) 
		{ 
			esyslog("ERROR: failed to open file %s: %s",Filename,strerror(errno)); 
		}
    }
  return opened;
}

void cOggFile::Close(void)
{
	if(opened) 
	{ 
		ov_clear(&vf); 
		opened = false;
	}
}

void cOggFile::Error(const char *action, const int err)
{
  char *errstr;
  switch(err) 
  {
    case OV_FALSE:      errstr="false/no data available"; break;
    case OV_EOF:        errstr="EOF"; break;
    case OV_HOLE:       errstr="missing or corrupted data"; break;
    case OV_EREAD:      errstr="read error"; break;
    case OV_EFAULT:     errstr="internal error"; break;
    case OV_EIMPL:      errstr="unimplemented feature"; break;
    case OV_EINVAL:     errstr="invalid argument"; break;
    case OV_ENOTVORBIS: errstr="no Ogg Vorbis stream"; break;
    case OV_EBADHEADER: errstr="corrupted Ogg Vorbis stream"; break;
    case OV_EVERSION:   errstr="unsupported bitstream version"; break;
    case OV_ENOTAUDIO:  errstr="ENOTAUDIO"; break;
    case OV_EBADPACKET: errstr="EBADPACKET"; break;
    case OV_EBADLINK:   errstr="corrupted link"; break;
    case OV_ENOSEEK:    errstr="stream not seekable"; break;
    default:            errstr="unspecified error"; break;
  }
  esyslog("ERROR: vorbisfile %s failed on %s: %s",action,Filename,errstr);
}

long long cOggFile::IndexMs(void)
{
	double p = ov_time_tell(&vf);
	if( p < 0.0 )
	{
		p=0.0;
	}
	return (long long)( p*1000.0 );
}

long long cOggFile::Seek(long long posMs, bool relativ)
{
	if(relativ) 
	{
		posMs += IndexMs();
	}
	int r = ov_time_seek( &vf, (double) posMs/1000.0 );
	if(r)
	{
		Error("seek",r);
		return -1;
    }

	posMs = IndexMs();
	return posMs;
}

int cOggFile::Stream(short *buffer, int samples)
{
  int n;
  do {
    int stream;
    n=ov_read(&vf,(char *)buffer,samples*2,0,2,1,&stream);
    } while(n==OV_HOLE);
  if(n<0) Error("read",n);
  return (n/2);
}

// --- cOggDecoder -------------------------------------------------------------

cOggDecoder::cOggDecoder( string filename ) : cDecoder(Filename)
	,file(filename)
{
  pcm = 0;
  Init();
}

cOggDecoder::~cOggDecoder()
{
  Clean();
}

bool cOggDecoder::Valid(void)
{
  bool res = false;
  if(TryLock()) 
  {
    if( file.Open(false) ) 
	{
		res = true;
	}
    Unlock();
  }
  return res;
}

cPlayInfo *cOggDecoder::PlayInfo(void)
{
  if(playing)
  {
    pi.Index = index/1000;
    pi.Total = info.Total;
    return &pi;
  }
  return 0;
}

void cOggDecoder::Init(void)
{
  Clean();
  pcm   = new struct mad_pcm;
  index = 0;
}

bool cOggDecoder::Clean(void)
{
  playing = false;
  
  delete pcm; 
  pcm = 0;
  
  file.Close();
  return false;
}

#define SF_SAMPLES (sizeof(pcm->samples[0])/sizeof(mad_fixed_t))

bool cOggDecoder::Start(void)
{
  Lock(true);
  Init(); 
  playing = true;

	if( file.Open() /*&& info.DoScan(true)*/ ) 
	{
		// obtain from database: rate, channels
		d(printf("ogg: open rate=%d channels=%d seek=%d\n",
				info.SampleFreq,info.Channels,file.CanSeek()))
		if( info.Channels <= 2 )
		{
			Unlock();
			return true;
		}
		else esyslog("ERROR: cannot play ogg file %s: more than 2 channels",filename);
	}
	
	Clean();
	Unlock();
	return false;
}

bool cOggDecoder::Stop(void)
{
  Lock();
  if( playing )
  {
	  Clean();
  }
  Unlock();
  return true;
}

struct Decode *cOggDecoder::Done(eDecodeStatus status)
{
  ds.status=status;
  ds.index=index;
  ds.pcm=pcm;
  Unlock(); // release the lock from Decode()
  return &ds;
}

struct Decode *cOggDecoder::Decode(void)
{
  Lock(); // this is released in Done()
  if(playing)
  {
    short framebuff[2*SF_SAMPLES];
    int n = file.Stream(framebuff,SF_SAMPLES);

    if( n < 0 ) 
	{
		return Done(dsError);
	}

    if( n == 0 ) 
	{
		return Done(dsEof);
	}

    pcm->samplerate = info.SampleFreq;  // from database
    pcm->channels   = info.Channels;    // from database
    n /= pcm->channels;                 // from database
    pcm->length = n;
    index = file.IndexMs();

    short *data = framebuff;
    mad_fixed_t *sam0 = pcm->samples[0], *sam1 = pcm->samples[1]; 

	const int s=MAD_F_FRACBITS+1-(sizeof(short)*8); // shift value for mad_fixed conversion

	if(pcm->channels>1) 
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
        *sam0++=(*data++) << s;
    }
    return Done(dsPlay);
  }
  return Done(dsError);
}

bool cOggDecoder::Skip(int Seconds, int Avail, int Rate)
{
  Lock();
  bool res=false;
  if(playing && file.CanSeek()) {
    float fsecs=(float)Seconds - ((float)Avail / (float)(Rate * (16/8 * 2)));  // Byte/s = samplerate * 16 bit * 2 chan
    long long newpos=file.IndexMs()+(long long)(fsecs*1000.0);
    if(newpos<0) newpos=0;
    d(printf("ogg: skip: secs=%d fsecs=%f current=%lld new=%lld\n",Seconds,fsecs,file.IndexMs(),newpos))

    newpos=file.Seek(newpos,false);
    if(newpos>=0) {
      index=file.IndexMs();
#ifdef DEBUG
      int i=index/1000;
      printf("ogg: skipping to %02d:%02d\n",i/60,i%60);
#endif
      res=true;
      }
    }
  Unlock();
  return res;
}

#endif //HAVE_VORBISFILE
