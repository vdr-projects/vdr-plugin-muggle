/*!
 * \file  vdr_stream.c
 * \brief Implementation of media stream classes
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/statfs.h>
#include <iostream>

#include <interface.h>

#include "mg_tools.h"

// #include "setup-mp3.h"
#include "vdr_stream.h"
#include "vdr_network.h"
#include "vdr_config.h"
// #include "i18n.h"
// #include "version.h"

//#define tr(x) x

#ifdef USE_MMAP
#include <sys/mman.h>
#endif

#define DEFAULT_PORT 80                           // default port for streaming (HTTP)

// --- mgStream -----------------------------------------------------------------

mgStream::mgStream (std::string filename):m_filename (filename)
{
    m_fd = -1;
    m_ismmap = false;
    m_buffer = 0;
}


mgStream::~mgStream ()
{
    close ();
}


bool mgStream::open (bool log)
{
    if (m_fd >= 0)
    {
        return seek ();
    }

// just check, whether file exists?
    if (fileinfo (log))
    {
//        printf ("mgStream::open: fileinfo == true\n");

        if ((m_fd =::open (m_filename.c_str (), O_RDONLY)) >= 0)
        {
            //printf ("mgStream::open: file opened\n");

            m_buffpos = m_readpos = 0;
            m_fill = 0;

            //printf ("mgStream::open: buffpos, readpos, fill set\n");

/*
   #ifdef USE_MMAP
   if( m_filesize <= MAX_MMAP_SIZE )
   {
   m_buffer = (unsigned char*)mmap( 0, m_filesize, PROT_READ,
   MAP_SHARED, m_fd, 0 );
   if( m_buffer != MAP_FAILED )
   {
   m_ismmap = true;
   return true;
   }
else
{
dsyslog("mmap() failed for %s: %s", m_filename.c_str(), strerror(errno) );
}
}
#endif
*/
            //printf ("mgStream::open: allocating buffer: %d\n", MP3FILE_BUFSIZE);
            m_buffer = new unsigned char[MP3FILE_BUFSIZE];
            //printf ("mgStream::open: buffer allocated\n");

            if (m_buffer)
            {
                //printf ("mgStream::open: buffer allocated, returning true\n");

                return true;
            }
            else
            {
                esyslog ("ERROR: not enough memory for buffer: %s",
                    m_filename.c_str ());
            }
        }
        else
        {
            if (log)
            {
                esyslog ("ERROR: failed to open file %s: %s",
                    m_filename.c_str (), strerror (errno));
            }
        }
    }

    close ();
    //printf ("mgStream::open: returning false\n");
    return false;
}


void
mgStream::close (void)
{
#ifdef USE_MMAP
    if (m_ismmap)
    {
        munmap (m_buffer, m_filesize);
        m_buffer = 0;
        m_ismmap = false;
    }
    else
    {
#endif
        delete[] m_buffer;
        m_buffer = 0;
#ifdef USE_MMAP
    }
#endif
    if (m_fd >= 0)
    {
        ::close (m_fd);
        m_fd = -1;
    }
}


bool mgStream::seek (unsigned long long pos)
{
    //printf ("mgStream::seek\n");
    if (m_fd >= 0 && pos >= 0 && pos <= m_filesize)
    {
        //printf ("mgStream::seek valid file and position detected\n");

        m_buffpos = 0;
        m_fill = 0;

        if (m_ismmap)
        {
            m_readpos = pos;

            //printf ("mgStream::seek: returning true\n");
            return true;
        }
        else
        {
            if ((m_readpos = lseek64 (m_fd, pos, SEEK_SET)) >= 0)
            {
                if (m_readpos != pos)
                {
                    dsyslog ("seek mismatch in %s, wanted %lld, got %lld",
                        m_filename.c_str (), pos, m_readpos);
                }
                //printf ("mgStream::seek: returning true\n");
                return true;
            }
            else
            {
                esyslog ("ERROR: seeking failed in %s: %d,%s",
                    m_filename.c_str (), errno, strerror (errno));
            }
        }
    }
    else
    {
        //printf ("mp3: bad seek call fd=%d pos=%lld name=%s\n", m_fd, pos,
            //m_filename.c_str ());
    }

    //printf ("mgStream::seek: returning false\n");
    return false;
}


bool
mgStream::stream (unsigned char *&data,
unsigned long &len, const unsigned char *rest)
{
    if (m_fd >= 0)
    {
        if (m_readpos < m_filesize)
        {
            if (m_ismmap)
            {
                if (rest && m_fill)
                {
                    m_readpos = (rest - m_buffer);// take care of remaining data
                }
                m_fill = m_filesize - m_readpos;
                data = m_buffer + m_readpos;
                len = m_fill;
                m_buffpos = m_readpos;
                m_readpos += m_fill;

                return true;
            }
            else
            {
                if (rest && m_fill)
                {                                 // copy remaining data to start of buffer
                    m_fill -= (rest - m_buffer);  // remaing bytes
                    memmove (m_buffer, rest, m_fill);
                }
                else
                {
                    m_fill = 0;
                }

                int r;
                do
                {
                    r = read (m_fd, m_buffer + m_fill,
                        MP3FILE_BUFSIZE - m_fill);
                }
                while (r == -1 && errno == EINTR);

                if (r >= 0)
                {
                    m_buffpos = m_readpos - m_fill;
                    m_readpos += r;
                    m_fill += r;
                    data = m_buffer;
                    len = m_fill;

                    return true;
                }
                else
                {
                    esyslog ("ERROR: read failed in %s: %d,%s",
                        m_filename.c_str (), errno, strerror (errno));
                }
            }
        }
        else
        {
            len = 0;
            return true;
        }
    }
    return false;
}


bool mgStream::removable ()
{
// we do not handle removable media at this time
    return false;
}


bool mgStream::fileinfo (bool log)
{
    struct stat64
        ds;

    if (!stat64 (m_filename.c_str (), &ds))
    {
        //printf ("mgStream::fileinfo: stat64 == 0\n");

        if (S_ISREG (ds.st_mode))
        {
            m_fsID = "";
            m_fsType = 0;

            struct statfs64
                sfs;

            if (!statfs64 (m_filename.c_str (), &sfs))
            {
                if (removable ())
                {
                    char *
                        tmpbuf;
                    asprintf (&tmpbuf, "%llx:%llx", sfs.f_blocks, sfs.f_files);
                    m_fsID = tmpbuf;
                    free (tmpbuf);
                }
                m_fsType = sfs.f_type;
            }
            else
            {
                if (errno != ENOSYS && log)
                {
                    esyslog ("ERROR: can't statfs %s: %s", m_filename.c_str (),
                        strerror (errno));
                }
            }

            m_filesize = ds.st_size;
            m_ctime = ds.st_ctime;

#ifdef CDFS_MAGIC
            if (m_fsType == CDFS_MAGIC)
            {
                m_ctime = 0;                      // CDFS returns mount time as ctime
            }
#endif
// infodone tells that info has been read, like a cache flag
//      InfoDone();
            return true;
        }
        else
        {
            if (log)
            {
                esyslog ("ERROR: %s is not a regular file",
                    m_filename.c_str ());
            }
        }
    }
    else
    {
        if (log)
        {
            esyslog ("ERROR: can't stat %s: %s", m_filename.c_str (),
                strerror (errno));
        }

        //printf ("mgStream::fileinfo: stat64 != 0 for %s\n",
         //   m_filename.c_str ());
    }

    return false;
}
