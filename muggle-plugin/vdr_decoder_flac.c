/*! \file vdr_decoder_flac.c
 *  \ingroup vdr
 * 
 *  The file implements a decoder which is used by the player to decode flac audio files.
 *
 *  Based on code from
 *  MP3/MPlayer plugin to VDR (C++)
 *  (C) 2001-2003 Stefan Huelswitt <huels@iname.com>
 */

#ifdef HAVE_FLAC

#define DEBUG
#include "vdr_setup.h"
#include "vdr_decoder_flac.h"

#include "mg_tools.h"
#include "mg_db.h"

#include <mad.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

static const unsigned MAX_RES_SIZE = 16384;

// --- mgFlacDecoder -------------------------------------------------------------

mgFlacDecoder::mgFlacDecoder( mgContentItem *item ) 
  : mgDecoder( item ), FLAC::Decoder::File()
{
  mgLog lg( "mgFlacDecoder::mgFlacDecoder" );

  m_filename = item->getSourceFile();
  m_pcm = 0;
  m_reservoir = 0;

  initialize();
}

mgFlacDecoder::~mgFlacDecoder()
{
  mgLog lg( "mgFlacDecoder::~mgFlacDecoder" );
  clean();
}

bool mgFlacDecoder::valid()
{
  // how to check whether this is a valid flac file?
  return is_valid();
}

mgPlayInfo *mgFlacDecoder::playInfo(void)
{
  return 0;
}

bool mgFlacDecoder::initialize()
{
  mgLog lg( "mgFlacDecoder::initialize" );
  bool state = true;

  clean();
  
  //  set_metadata_ignore_all();
  set_metadata_respond( FLAC__METADATA_TYPE_STREAMINFO );
  set_filename( m_filename.c_str() );

  m_first = true;
  m_reservoir_count = 0;
  m_current_time_ms = 0;
  m_len_decoded = 0;
  m_index = 0;
  m_pcm   = new struct mad_pcm;
  
  // init reservoir buffer; this should be according to the maximum
  // frame/sample size that we can probably obtain from metadata
  m_reservoir = new (FLAC__int32*)[2];
  m_reservoir[0] = new FLAC__int32[MAX_RES_SIZE];
  m_reservoir[1] = new FLAC__int32[MAX_RES_SIZE];

  FLAC::Decoder::File::State d = init(); // TODO: check this

  process_until_end_of_metadata(); // basically just skip metadata

  return state;
}

bool mgFlacDecoder::clean()
{
  mgLog lg( "mgFlacDecoder::clean" );
  m_playing = false;
  
  delete m_pcm; 
  m_pcm = 0;
  
  if( m_reservoir )
    {
      delete[] m_reservoir[0];
      delete[] m_reservoir[1];
    }
  delete[] m_reservoir;
  
  // why false? true?
  return true;
}

bool mgFlacDecoder::start()
{
  MGLOG( "mgFlacDecoder::start" );
  bool res = false;
  lock(true);

  // can FLAC handle more than 2 channels anyway?
  if( m_item->getChannels() <= 2 )
    {
      m_playing = true;
      res = true;
    }
  else 
    {
      mgError( "ERROR: cannot play flac file %s: more than 2 channels", m_filename.c_str() );
      clean();
    }
  
  unlock();
  return res;
}

bool mgFlacDecoder::stop(void)
{
  MGLOG( "mgFlacDecoder::stop" );
  lock();
  finish();

  if( m_playing )
  {
    clean();
  }
  unlock();

  return true;
}

struct mgDecode *mgFlacDecoder::done( eDecodeStatus status )
{
  m_ds.status = status;
  m_ds.index  = m_index;
  m_ds.pcm    = m_pcm;

  unlock(); // release the lock from Decode() !

  return &m_ds;
}

struct mgDecode *mgFlacDecoder::decode()
{
  // mgLog lg( "mgFlacDecoder::decode" );
  m_decode_status = dsPlay;

  const unsigned SF_SAMPLES = (sizeof(m_pcm->samples[0])/sizeof(mad_fixed_t));

  lock(true); // this is released in done()

  if( m_playing )
    {
      m_pcm->samplerate = m_item->getSampleRate();  // from database
      m_pcm->channels   = m_item->getChannels();    // from database
      
      // if there is enough data in the reservoir, don't start decoding
      // PROBLEM: but we need a first time!
      bool finished;
      if( m_first )
	{
	  finished = false;
	  m_first  = false;
	}
      else
	{
	  finished = m_reservoir_count >= SF_SAMPLES;
	}
      
     while( !finished )	
       { // decode single frames until m_reservoir_count >= SF_SAMPLES or eof/error
	 m_first = false;
	 
	 // decode a single sample into reservoir_buffer (done by the write callback)
	 process_single();
	 if (get_stream_decoder_state()==FLAC__STREAM_DECODER_END_OF_STREAM)
	 {
      		m_decode_status = dsEof;
		finished = true;
	 }
	 
	 // check termination criterion
	 finished |= m_reservoir_count >= SF_SAMPLES || m_len_decoded == 0; // or error?
       }
     
     // transfer min( SF_SAMPLES, m_reservoir_count ) to pcm buffer
     
     int n = ( SF_SAMPLES <= m_reservoir_count )? SF_SAMPLES: m_reservoir_count;
     
     m_pcm->length = n;
     m_index = m_current_time_ms;

     // fill pcm container from reservoir buffer
     FLAC__int32 *data0 = m_reservoir[0];
     FLAC__int32 *data1 = m_reservoir[1];
     
     mad_fixed_t *sam0 = m_pcm->samples[0];
     mad_fixed_t *sam1 = m_pcm->samples[1]; 
     
     // determine shift value for mad_fixed conversion
     // TODO -- check for real bitsize and shit accordingly (left/right)
     const int s = MAD_F_FRACBITS + 1 - ( sizeof(short)*8 );
      // const int s = ( sizeof(int)*8 ) - 1 - MAD_F_FRACBITS;      // from libsoundfile decoder
     
     if( m_pcm->channels > 1 ) 
       {
	 for( int j=n; j > 0 ; j-- )
	   {
	     // copy buffer and transform (cf. libsoundfile decoder)
	     *sam0++ = (*data0++) << s;
	     *sam1++ = (*data1++) << s;
	   }
	 // "delete" transferred samples from reservoir buffer
	 memmove( m_reservoir[0], m_reservoir[0] + n, (m_reservoir_count - n)*sizeof(FLAC__int32) );
	 memmove( m_reservoir[1], m_reservoir[1] + n, (m_reservoir_count - n)*sizeof(FLAC__int32) );
       }
     else 
       {
	 for( int j=n; j > 0 ; j--)
	   {
	     *sam0++ = (*data0++) << s;
	   }
	 memmove( m_reservoir[0], m_reservoir[0] + n, (m_reservoir_count - n)*sizeof(FLAC__int32) );
       }    
     m_reservoir_count -= n;
     
     // and return, indicating that playing will continue (unless an error has occurred)
     return done( m_decode_status );
    }      
  
  return done(dsError);
}

::FLAC__StreamDecoderWriteStatus 
mgFlacDecoder::write_callback(const ::FLAC__Frame *frame,
			      const FLAC__int32 * const buffer[])
{
  // mgLog lg( "mgFlacDecoder::write_callback" );

  // add decoded buffer to reservoir
  m_len_decoded = frame->header.blocksize;
  m_current_samples += m_len_decoded;
  m_current_time_ms += (m_len_decoded*1000) / m_pcm->samplerate; // in milliseconds

  // cout << "Samples decoded: " << m_len_decoded << ", current time: " << m_current_time_ms << ", bits per sample: "<< m_nBitsPerSample << endl << flush;

  // append buffer to m_reservoir
  if( m_len_decoded > 0 )
  {
    memmove( m_reservoir[0] + m_reservoir_count, buffer[0], m_len_decoded*sizeof(FLAC__int32) );
    
    if( m_pcm->channels > 1 )
      {
	memmove( m_reservoir[1] + m_reservoir_count, buffer[1], m_len_decoded*sizeof(FLAC__int32) );
      }
    
    m_reservoir_count += m_len_decoded;
  }
  else
    {
      m_decode_status = dsEof;
    }
  
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void mgFlacDecoder::metadata_callback( const ::FLAC__StreamMetadata *metadata )
{
  // not needed since metadata is ignored!?
  mgLog lg( "mgFlacDecoder::metadata_callback" );

  if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) 
    {
      m_nTotalSamples = (unsigned)(metadata->data.stream_info.total_samples & 0xfffffffful);
      m_nBitsPerSample = metadata->data.stream_info.bits_per_sample;
      m_nCurrentChannels = metadata->data.stream_info.channels;
      m_nCurrentSampleRate = metadata->data.stream_info.sample_rate;
      m_nFrameSize = metadata->data.stream_info.max_framesize;
      m_nBlockSize = metadata->data.stream_info.max_blocksize;

      m_nCurrentSampleRate = (int)get_sample_rate();
      m_nCurrentChannels = (int)get_channels();
      m_nCurrentBitsPerSample = (int)get_bits_per_sample();

      // m_nLengthMS = m_nTotalSamples * 10 / (m_nCurrentSampleRate / 100);      
      m_nBlockAlign = (m_nBitsPerSample / 8) * m_nCurrentChannels;
      
      // m_nAverageBitRate = ((m_pReader->GetLength() * 8) / (m_nLengthMS / 1000) / 1000);
      // m_nCurrentBitrate = m_nAverageBitRate;
    }
}

void mgFlacDecoder::error_callback( ::FLAC__StreamDecoderErrorStatus status )
{
  mgLog lg( "mgFlacDecoder::error_callback" );

  // check status and return
  switch( status )
    {
    case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
      {
	m_error = "An error in the stream caused the decoder to lose synchronization";
      } break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
      {
	m_error = "The decoder encountered a corrupted frame header.";
      } break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
      {
	m_error = "The frame's data did not match the CRC in the footer.";
      };
    default:
      {
	m_error = "Unknown error occurred.";
      }
    }

  // cout << "Error occured: " << m_error << endl;
  m_decode_status = dsError;
}

bool mgFlacDecoder::skip(int seconds, int avail, int rate)
{
  lock();
  bool res = false;

  if( m_playing )
    {
      const long target_time_ms = ((seconds-avail)*rate / 1000) + m_current_time_ms;
      const double distance =  target_time_ms / (double)m_nLengthMS;
      const unsigned target_sample = (unsigned)(distance * (double)m_nTotalSamples);
      
      if( seek_absolute( (FLAC__uint64)target_sample) )
	{
	  m_current_time_ms = target_time_ms;
	  res = true;
	}
    }
  unlock();
  return res;
}

#endif //HAVE_FLAC
