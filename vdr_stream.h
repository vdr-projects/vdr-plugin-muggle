/*!								-*- c++ -*-
 * \file  vdr_stream.h
 * \brief Definitions of media streams
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
 * (C) 2001-2003 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___STREAM_H
#define ___STREAM_H

#include <string>

#include "vdr_decoder.h"

class cNet;

// ----------------------------------------------------------------

class mgStream					 // : public mgFileInfo
{
	private:
		int m_fd;
		bool m_ismmap;

		// from cFileInfo
		std::string m_filename, m_fsID;
		unsigned long long m_filesize;
		time_t m_ctime;
		long m_fsType;

		bool fileinfo (bool log);
		bool removable ();

	protected:
		unsigned char *m_buffer;
		unsigned long long m_readpos, m_buffpos;
		unsigned long m_fill;
	public:
		mgStream (std::string filename);
		virtual ~ mgStream ();
		virtual bool open (bool log = true);
		virtual void close ();
		virtual bool stream (unsigned char *&data, unsigned long &len,
			const unsigned char *rest = NULL);
		virtual bool seek (unsigned long long pos = 0);
		virtual unsigned long long bufferPos () {
			return m_buffpos;
		}
};
#endif							 //___STREAM_H
