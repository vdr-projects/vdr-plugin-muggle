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

#ifndef ___DECODER_MP3_H
#define ___DECODER_MP3_H

#define DEC_MP3     DEC_ID('M','P','3',' ')
#define DEC_MP3_STR "MP3"

#include <mad.h>
#include <string>

#include "vdr_decoder.h"

#if MAD_F_FRACBITS != 28
#warning libmad with MAD_F_FRACBITS != 28 not tested
#endif

class mgStream;

// ----------------------------------------------------------------

/*!
 * \brief A class to decode mp3 songs into PCM using libmad
 */
class mgMP3Decoder : public mgDecoder
{
private:
  struct mgDecode m_ds;

  //
  struct mad_stream m_madstream;
  struct mad_frame *m_madframe;
  struct mad_synth *m_madsynth;
  mad_timer_t m_playtime, m_skiptime;

  //
  struct FrameInfo
  {
    unsigned long long Pos;
    mad_timer_t Time;
  } *m_frameinfo;

  int m_framenum, m_framemax, m_errcount, m_mute;

  void init();

  void clean();

  struct mgDecode *done( eDecodeStatus status );

  virtual mgPlayInfo *playInfo();

  eDecodeStatus decodeError(bool hdr);
  
  void makeSkipTime(mad_timer_t *skiptime, mad_timer_t playtime,
		    int secs, int avail, int dvbrate);

protected:
  mgStream *m_stream;
  bool m_isStream;

public:

  /*! 
   * \brief construct a decoder from a filename
   */
  mgMP3Decoder( mgContentItem *item, bool preinit = true );

  /*!
   * \brief the destructor
   */
  virtual ~mgMP3Decoder();

  /*!
   * \brief check, whether the file contains useable MP3 content
   */
  virtual bool valid();

  /*!
   * \brief start the decoding process
   */
  virtual bool start();

  /*!
   * \brief stop the decoding process
   */
  virtual bool stop();

  /*!
   * \brief skip an amount of seconds
   */
  virtual bool skip( int seconds, int avail, int rate );

  /*!
   * \brief the actual decoding function (uses libmad)
   */
  virtual struct mgDecode *decode();
};

#endif //___DECODER_MP3_H
