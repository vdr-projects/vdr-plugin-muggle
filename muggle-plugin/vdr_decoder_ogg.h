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

#include "vdr_decoder.h"

// ----------------------------------------------------------------

class mgOggFile;

/*! 
 * \brief A decoder for Ogg Vorbis files
 *
 */
class mgOggDecoder : public mgDecoder 
{
 private:

  mgOggFile *m_file;
  struct mgDecode m_ds;
  struct mad_pcm *m_pcm;
  unsigned long long m_index;

  //
  void init(void);
  bool clean(void);
  struct mgDecode *done( eDecodeStatus status );

 public:

  mgOggDecoder( mgContentItem *item );
  ~mgOggDecoder();

  virtual mgPlayInfo *playInfo();

  virtual bool valid();

  virtual bool start();

  virtual bool stop();

  virtual bool skip(int Seconds, int Avail, int Rate);

  virtual struct mgDecode *decode(void);
};

// ----------------------------------------------------------------

#endif //HAVE_VORBISFILE
#endif //___DECODER_OGG_H
