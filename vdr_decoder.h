/*!								-*- c++ -*-
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
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___DECODER_H
#define ___DECODER_H

#include <string>

#include <vdr/thread.h>
#include <string>

#define DEC_ID(a,b,c,d) (((a)<<24)+((b)<<16)+((c)<<8)+(d))

#include "mg_item_gd.h"

// --------From decoder_core.h ------------------------------------

/*!
 * \brief The current status of the decoder
 * \ingroup vdr
 */
enum eDecodeStatus
{
	dsOK = 0, dsPlay, dsSkip, dsEof, dsError, dsSoftError
};

// ----------------------------------------------------------------

/*!
 * \brief A data structure to put decoded PCM data
 * \ingroup vdr
 */
struct mgDecode
{
	eDecodeStatus status;
	int index;
	struct mad_pcm *pcm;
};

// ----------------------------------------------------------------

/*!
 * \brief Information about ???
 * \ingroup vdr
 */
class mgPlayInfo
{
	public:
		int m_index, m_total;
};

// ----------------------------------------------------------------

/*!
 * \brief Media types
 * \ingroup vdr
 */
enum mgMediaType
{
	MT_MP3, MT_MP3_STREAM, MT_OGG, MT_FLAC, MT_SND, MT_UNKNOWN
};

/*!
 * \brief A generic decoder class to handle conversion into PCM format
 * \ingroup vdr
 */
class mgDecoder
{
	protected:

		/*! \brief database handle to the track being decoded */
		mgItemGd * m_item;

		/*! \brief The currently playing file */
		std::string m_filename;

		/*! \brief Mutexes to coordinate threads */
		cMutex m_lock, m_locklock;
		int m_locked;
		bool m_urgentLock;

		/*! \brief Whether the decoder is currently active */
		bool m_playing;

		/*! \brief ??? */
		mgPlayInfo m_playinfo;

		/*! \brief Place a lock */
		virtual void lock (bool urgent = false);

		/*! \brief Release a lock */
		virtual void unlock (void);

		/*! \brief Try to obtain a lock */
		virtual bool tryLock (void);

	public:

		//@{
		/*! \brief The constructor */
		mgDecoder (mgItemGd * item);

		/*! \brief The destructor */
		virtual ~ mgDecoder ();
		//@}

		/*! \brief Whether a decoder instance is able to play the given file */
		virtual bool valid () = 0;

		/*! \brief Whether a stream (i.e. from the network is being decoded */
		virtual bool isStream () {
			return false;
		}

		/*! \brief Start decoding */
		virtual bool start () = 0;

		/*! \brief Stop decoding */
		virtual bool stop () = 0;

		/*! \brief Skip an amount of time. Impossible by default */
		virtual bool skip (int seconds, int avail, int rate) {
			return false;
		}

		/*! \brief Return decoded data */
		virtual struct mgDecode *decode () = 0;

		/*! \brief Information about the current playback status */
		virtual mgPlayInfo *playInfo () {
			return 0;
		}
};

// ----------------------------------------------------------------

/*!
 * \brief A generic decoder manager class to handle different decoders
 */
class mgDecoders
{
	public:

		/*! \brief Try to find a valid decoder for a file
		 */
		static mgDecoder *findDecoder (mgItemGd * item);

		/*! \brief determine the media type for a given source
		 */
		static mgMediaType getMediaType (std::string filename);

};
#endif							 //___DECODER_H
