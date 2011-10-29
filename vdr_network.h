/*								-*- c++ -*-
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___NETWORK_H
#define ___NETWORK_H

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>
#include <vdr/config.h>

class cRingBufferLinear;

// ----------------------------------------------------------------

class mgNet:public cRingBufferLinear, cThread
{
	private:
		int m_fd;
		bool m_connected, m_netup;
		int m_deferedErrno;
		int m_rwTimeout, m_conTimeout;
		unsigned char m_lineBuff[4096];
		int m_count;
		//
		void close (void);
		int ringRead (unsigned char *dest, int len);
		void copyFromBuff (unsigned char *dest, int n);
	protected:
		virtual void action (void);
	public:
		mgNet (int size, int ConTimeoutMs, int RwTimeoutMs);
		~mgNet ();
		bool connect (const char *hostname, const int port);
		void disconnect (void);
		bool connected (void) {
			return m_connected;
		}
		int gets (char *dest, int len);
		int puts (char *dest);
		int read (unsigned char *dest, int len);
		int write (unsigned char *dest, int len);
};
#endif							 //___NETWORK_H
