/*! \file vdr_decoder_flac.h
 *  \ingroup vdr
 * 
 *  The file contains a decoder which is used by the player to decode flac audio files.
 *
 *  Based on code from
 *  MP3/MPlayer plugin to VDR (C++)
 *  (C) 2001-2003 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___DECODER_FLAC_H
#define ___DECODER_FLAC_H

#define DEC_FLAC     DEC_ID('F','L','A','C')
#define DEC_FLAC_STR "FLAC"

#ifdef HAVE_FLAC

#include "vdr_decoder.h"
#include <FLAC++/decoder.h>

// ----------------------------------------------------------------

class mgFlacDecoder : public mgDecoder, 
		      public FLAC::Decoder::File
{
 private:
  
  struct mgDecode m_ds;
  struct mad_pcm *m_pcm;
  unsigned long long m_index, m_current_time_ms, m_current_samples;
  unsigned int m_reservoir_count, m_len_decoded, m_samplerate;

  long m_nCurrentSampleRate, m_nCurrentChannels, m_nCurrentBitsPerSample;
  long m_nLengthMS, m_nBlockAlign, m_nAverageBitRate, m_nCurrentBitrate;
  long m_nTotalSamples, m_nBitsPerSample, m_nFrameSize, m_nBlockSize;

  bool m_state, m_first, m_continue;
  std::string m_error;

  FLAC__int32 **m_reservoir;

  bool initialize();
  bool clean();

  struct mgDecode* done(eDecodeStatus status);
  eDecodeStatus m_decode_status;

 protected:

  /*! \brief FLAC decoder callback routines */
  //@{
  virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
							  const FLAC__int32 * const buffer[]);
  virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);
  virtual void error_callback(::FLAC__StreamDecoderErrorStatus status);
  //@}

 public:

   mgFlacDecoder( mgItem *item );
  ~mgFlacDecoder();

  virtual mgPlayInfo *playInfo();

  virtual bool valid();

  virtual bool start();

  virtual struct mgDecode *decode(void);

  virtual bool skip(int Seconds, int Avail, int Rate);

  virtual bool stop();
};

// ----------------------------------------------------------------

#endif //HAVE_FLAC
#endif //___DECODER_FLAC_H
