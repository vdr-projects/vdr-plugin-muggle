/*								-*- c++ -*-
 * MP3/MPlayer plugin to VDR (C++)
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#ifdef HAVE_SNDFILE

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "mg_setup.h"
#include "vdr_decoder_sndfile.h"
#include <vdr/i18n.h>

#define __STL_CONFIG_H
#include <vdr/tools.h>

#ifndef SNDFILE_1
#error You must use libsndfile version 1.x.x
#endif

#define CDFS_TRACK_OFF 150
#define CDFS_PROC      "/proc/cdfs"
#define CDFS_MARK_ID   "CD (discid=%x) contains %d tracks:"
#define CDFS_MARK_TR   "%*[^[][ %d - %d"
#define CDFS_TRACK     "track-"

#define CDDB_PROTO 5			 // used protocol level
#define CDDB_TOUT  30*1000		 // connection timeout (ms)

#ifdef DEBUG_CDFS
#define dc(x) { (x); }
#else
#define dc(x) ;
#endif

// --- mgSndfileDecoder -------------------------------------------------------------

#define SF_SAMPLES (sizeof(m_pcm->samples[0])/sizeof(mad_fixed_t))

mgSndfileDecoder::mgSndfileDecoder( mgItemGd *item )
: mgDecoder( item ), m_file( item ) {

	m_pcm = 0;
	m_framebuff = 0;
	m_playing = false;
	m_ready = false;
}

mgSndfileDecoder::~mgSndfileDecoder() {
	clean();
}

bool mgSndfileDecoder::valid(void) {
	bool res = false;

	if( tryLock() ) {
		if( m_file.Open(false) ) {
			res=true;
		}
		mgDecoder::unlock();
	}

	return res;
}

mgPlayInfo *
mgSndfileDecoder::playInfo () {
	mgPlayInfo *pi = NULL;

	return pi;
}

void mgSndfileDecoder::init(void) {
	clean();

	m_pcm = new struct mad_pcm;
	m_framebuff = MALLOC( int, 2*SF_SAMPLES + 8 );

#ifdef GUARD_DEBUG
	for(int i=0; i<8; i++) m_framebuff[i+(SF_SAMPLES*2)-4]=0xdeadbeaf;
#endif

	m_index = 0;
}

bool mgSndfileDecoder::clean(void) {
	m_playing = false;

	m_buffMutex.Lock();
	m_run = false;
	m_bgCond.Broadcast();
	m_buffMutex.Unlock();

	cThread::Cancel(3);

	m_buffMutex.Lock();
	if( !m_ready ) {
		m_deferedN = -1;
		m_ready=true;
	}
	m_fgCond.Broadcast();
	m_buffMutex.Unlock();

	delete m_pcm;
	m_pcm=0;

#ifdef GUARD_DEBUG
	if(m_framebuff) {
		printf("snd: bufferguard");
		for(int i=0; i<8; i++) printf(" %08x",m_framebuff[i+(SF_SAMPLES*2)-4]);
		printf("\n");
	}
#endif

	free(m_framebuff);
	m_framebuff=0;
	m_file.Close();

	return false;
}

bool mgSndfileDecoder::start(void) {
	mgDecoder::lock(true);
	init();
	m_playing=true;

	if( m_file.Open() ) {
		//      d(printf("snd: open rate=%d frames=%lld channels=%d format=0x%x seek=%d\n",
		//	       m_file.SoundfileInfo()->samplerate,m_file.SoundfileInfo()->frames,m_file.SoundfileInfo()->channels,m_file.SoundfileInfo()->format,m_file.SoundfileInfo()->seekable))

		if( m_file.SoundfileInfo()->channels <= 2 ) {
			m_ready = false;
			m_run = true;
			m_softCount=0;

			cThread::Start();
			mgDecoder::unlock();
			return true;
		}
		else {
			// esyslog("ERROR: cannot play sound file %s: more than 2 channels", m_filename.c_str() );
		}
	}
	mgDecoder::unlock();
	return clean();
}

bool mgSndfileDecoder::stop(void) {
	mgDecoder::lock();
	if( m_playing ) {
		clean();
	}
	mgDecoder::unlock();
	return true;
}

void mgSndfileDecoder::Action(void) {
	m_buffMutex.Lock();
	while( m_run ) {

		if( m_ready ) {
			m_bgCond.Wait(m_buffMutex);
		}

		if(!m_ready) {
			m_buffMutex.Unlock();
			m_deferedN = m_file.Stream( m_framebuff,SF_SAMPLES );
			m_buffMutex.Lock();
			m_ready = true;
			m_fgCond.Broadcast();
		}
	}
	m_buffMutex.Unlock();
}

struct mgDecode *mgSndfileDecoder::done(eDecodeStatus status) {
	m_ds.status = status;
	m_ds.index  = m_index*1000 / m_file.SoundfileInfo()->samplerate;
	m_ds.pcm    = m_pcm;

	mgDecoder::unlock();		 // release the lock from Decode()

	return &m_ds;
}

struct mgDecode *mgSndfileDecoder::decode() {
	mgDecoder::lock();			 // this is released in Done()
	if(m_playing) {
		cMutexLock lock(&m_buffMutex);
		while(!m_ready) {
			if( !m_softCount || !m_fgCond.TimedWait( m_buffMutex, m_softCount*5 ) ) {
				if( m_softCount < 20 ) {
					m_softCount++;
				}
				return done(dsSoftError);
			}
		}
		m_softCount = 0;
		m_ready = false;
		m_bgCond.Broadcast();

		int n = m_deferedN;
		if( n < 0 ) {
			return done(dsError);
		}

		if( n == 0 ) {
			return done(dsEof);
		}

		m_pcm->samplerate = m_file.SoundfileInfo()->samplerate;
		m_pcm->channels = m_file.SoundfileInfo()->channels;
		m_pcm->length = n;
		m_index += n;

		int *data = m_framebuff;
		mad_fixed_t *sam0 = m_pcm->samples[0], *sam1=m_pcm->samples[1];

								 // shift value for mad_fixed conversion
		const int s = (sizeof(int)*8)-1-MAD_F_FRACBITS;

		if(m_pcm->channels>1) {
			for(; n>0 ; n--) {
				*sam0++=(*data++) >> s;
				*sam1++=(*data++) >> s;
			}
		}
		else {
			for(; n>0 ; n--) {
				*sam0++=(*data++) >> s;
			}
		}
		return done(dsPlay);
	}
	return done(dsError);
}

bool mgSndfileDecoder::skip( int seconds, int avail, int rate ) {
	mgDecoder::lock();
	bool res=false;
	float bsecs = (float) avail / (float) ( rate * (16 / 8 * 2) );

	if( m_playing && m_file.SoundfileInfo()->seekable ) {
		float fsecs = (float)seconds - bsecs;
		sf_count_t frames = (sf_count_t)( fsecs*(float)m_file.SoundfileInfo()->samplerate );
		sf_count_t newpos = m_file.Seek( 0, true ) + frames;

		if( newpos > m_file.SoundfileInfo()->frames ) {
			newpos = m_file.SoundfileInfo()->frames-1;
		}

		if( newpos < 0 ) {
			newpos=0;
		}

		//    d(printf("snd: skip: secs=%d fsecs=%f frames=%lld current=%lld new=%lld\n",Seconds,fsecs,frames,m_file.Seek(0,true),newpos))

		m_buffMutex.Lock();
		frames = m_file.Seek( newpos, false );
		m_ready = false;
		m_bgCond.Broadcast();
		m_buffMutex.Unlock();

		if( frames >= 0 ) {
			m_index = frames;
#ifdef DEBUG
			int i = frames/m_file.SoundfileInfo()->samplerate;
			printf("snd: skipping to %02d:%02d (frame %lld)\n",i/60,i%60,frames);
#endif
			res=true;
		}
	}
	mgDecoder::unlock();
	return res;
}

// --- cSndfile ----------------------------------------------------------------

mgSndfile::mgSndfile( mgItemGd *item ) {
	m_filename = item->getSourceFile();
	m_sf = 0;
}

mgSndfile::~mgSndfile() {
	Close();
}

SF_INFO* mgSndfile::SoundfileInfo() {
	return &m_sfi;
}

bool mgSndfile::Open(bool log) {
	if( m_sf ) {
		return( Seek() >= 0 );
	}

	m_sf = sf_open( m_filename.c_str(), SFM_READ, &m_sfi );
	if( !m_sf && log ) {
		Error( "open" );
	}

	return ( m_sf != 0 );
}

void mgSndfile::Close(void) {
	if( m_sf ) {
		sf_close( m_sf );
		m_sf = 0;
	}
}

void mgSndfile::Error(const char *action) {
	char buff[128];
	sf_error_str( m_sf, buff, sizeof(buff) );
	// esyslog( "ERROR: sndfile %s failed on %s: %s", action, Filename, buff );
}

sf_count_t mgSndfile::Seek( sf_count_t frames, bool relative ) {
	int dir = SEEK_CUR;
	if( !relative ) {
		dir = SEEK_SET;
	}

	int n = sf_seek( m_sf, frames, dir );
	if( n < 0 ) {
		Error("seek");
	}

	return n;
}

sf_count_t mgSndfile::Stream( int *buffer, sf_count_t frames ) {
	sf_count_t n = sf_readf_int( m_sf, buffer, frames );
	if( n < 0 ) {
		Error("read");
	}
	return n;
}
#endif							 //HAVE_SNDFILE
