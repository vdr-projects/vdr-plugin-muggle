/*								-*- c++ -*-
 * Muggle plugin to VDR (C++)
 *
 * (C) 2005 Lars von Wedel, Wolfgang Rohdewald, mainly copied from
 * (C) 2001-2005 Stefan Huelswitt <s.huelswitt@gmx.de>
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#ifndef ___DECODER_SND_H
#define ___DECODER_SND_H

#define DEC_SND     DEC_ID('S','N','D',' ')
#define DEC_SND_STR "SND"

#ifdef HAVE_SNDFILE

#include <string>

#include <time.h>
#include <mad.h>
#include <sndfile.h>

#include <vdr/thread.h>

#include "vdr_decoder.h"

#define CDFS_MAGIC 0xCDDA		 // cdfs filesystem-ID

// ----------------------------------------------------------------

class mgSndfile
{
	private:
		SNDFILE *m_sf;
		std::string m_filename;

		void Error(const char *action);

		SF_INFO m_sfi;
	public:

		mgSndfile( mgItemGd *item );
		~mgSndfile();

		SF_INFO* SoundfileInfo();
		bool Open( bool log = true );
		void Close();
		sf_count_t Seek( sf_count_t frames = 0, bool relative = false );
		sf_count_t Stream( int *buffer, sf_count_t frames );
};

// ----------------------------------------------------------------

class mgSndfileDecoder : public mgDecoder, public cThread
{
	private:
		mgSndfile m_file;

		struct mgDecode m_ds;
		struct mad_pcm *m_pcm;
		unsigned long long m_index;
		//
		cMutex m_buffMutex;
		cCondVar m_fgCond, m_bgCond;
		bool m_run, m_ready;
		int *m_framebuff, m_deferedN, m_softCount;
		//
		void init();
		bool clean();
		struct mgDecode *done( eDecodeStatus status );

	protected:
		virtual void Action(void);

	public:
		mgSndfileDecoder( mgItemGd *item );
		~mgSndfileDecoder();

		virtual bool valid(void);
		virtual bool start(void);
		virtual bool stop(void);
		virtual bool skip(int seconds, int avail, int rate);
		virtual struct mgDecode *decode();
		virtual mgPlayInfo *playInfo();
};

// ----------------------------------------------------------------
#endif							 //HAVE_SNDFILE
#endif							 //___DECODER_SND_H
