/*! \file vdr_decoder_ogg.h
 *  \ingroup vdr
 * 
 *  The file contains a decoder which is used by the player to decode ogg vorbis audio files.
 *
 *  Adapted from 
 *  MP3/MPlayer plugin to VDR (C++)
 *  (C) 2001-2003 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___DECODER_OGG_H
#define ___DECODER_OGG_H

#define DEC_OGG     DEC_ID('O','G','G',' ')
#define DEC_OGG_STR "OGG"

#ifdef HAVE_VORBISFILE

#include <mad.h>
#include <vorbis/vorbisfile.h>

#include "vdr_decoder.h"

// ----------------------------------------------------------------

class cOggDecoder : public cDecoder {
private:
  cOggFile file;
  cOggInfo info;
  struct Decode ds;
  struct mad_pcm *pcm;
  unsigned long long index;
  //
  void Init(void);
  bool Clean(void);
  bool GetInfo(bool keepOpen);
  struct Decode *Done(eDecodeStatus status);
public:
  cOggDecoder(const char *Filename);
  ~cOggDecoder();
  virtual bool Valid(void);
  virtual cFileInfo *FileInfo(void);
  virtual cSongInfo *SongInfo(bool get);
  virtual cPlayInfo *PlayInfo(void);
  virtual bool Start(void);
  virtual bool Stop(void);
  virtual bool Skip(int Seconds, int Avail, int Rate);
  virtual struct Decode *Decode(void);
  };

// ----------------------------------------------------------------

#endif //HAVE_VORBISFILE
#endif //___DECODER_OGG_H
