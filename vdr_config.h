/*!
 * \file   vdr_config.h
 * \brief  Implements menu handling for browsing media libraries within VDR
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

// This files contains some option switches/values for compile time
// configuration of the MP3/MPlayer plugin. You should only alter
// this file, if you have understand the source code parts which deal
// with the changed value.

// After changing this file you should do a "make plugins-clean ; make plugins"
// to recompile vdr.

#ifndef ___VDR_CONFIG_H
#define ___VDR_CONFIG_H

// The buffer size in bytes for the decoded audio data which is about to
// be send to the dvb driver. Should not be made to big, as this delays the
// pause function too much.
#define MP3BUFSIZE (100*1024)

// The number of consecutive frame decoding error that may occure during
// decoding before aborting the file.
#define MAX_FRAME_ERR 10

// Defines the min. gain required to launch the normalizer. Don't bother normalizing
// songs which wouldn't change much in volumen.
#define MIN_GAIN 0.03

// Comment this out to disable the fast limiter function in the normalizer. The fast function
// uses a lookup table and is 12-14 times faster than the normal function, but less
// accurate (rel. error about 1e-7).
#define USE_FAST_LIMITER

// Defines the filename extention to use for playlist files.
#define PLAYLISTEXT ".m3u"

// Defines the text to identify WinAmp-Style playlists.
#define WINAMPEXT "#EXTM3U"

// Comment this out, if you don't want to use mmap() to access the data files.
// Normaly there is no need to disable mmap(), as the code switches to normal
// file i/o if mmap() is not available for a specific file/filesystem.
#define USE_MMAP

// Defines the max. memory size used for mmapping. This should not exceed the
// available free memory on your machine.
#define MAX_MMAP_SIZE (32*1024*1024)

// The buffer size in bytes to use for normal file i/o if a file cannot be
// mmap()ed. If set to large on slow media, may cause audio drop outs as the
// audio buffer may underrun while filling this buffer.
#define MP3FILE_BUFSIZE (32*1024)

// The filename to save the cached id3 information. The file is located in the
// video directory and this definition must not contain any path (no "/" )
#define CACHEFILENAME "id3info.cache"

// The interval in seconds in which the id3 cache is saved to disk (only
// if any changes occured).
#define CACHESAVETIMEOUT 120

// How many days to keep unused entries in the id3 cache. Unused entries from
// undefined mp3sources are purged after timeout/10 days.
#define CACHEPURGETIMEOUT 120

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// Defines the timeout in seconds for functions which use a single key
// (e.g. openning the playlist window). If the key is repressed during
// the timeout, the secondary function is activated.
#define MULTI_TIMEOUT 3

// Defines the timeout in ms for entering the single digits in direct song
// selection.
#define SELECT_TIMEOUT 1000

// If the progress display is closed on direct song selection, the display
// is opend temporarily. This defines the time in seconds after the display
// is closed again.
#define SELECTHIDE_TIMEOUT 3

// Defines the time in seconds to jump inside a song with left/right.
#define JUMPSIZE 3

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// DEBUGGING OPTIONS
// (not needed for normal operation)

// Uncomment to enable generic debugging messages to the console. This may slow
// down operation in some cases.
#define DEBUG

// Uncomment to disable audio output to the dvb driver. The audio data is
// simply discarded.
//#define DUMMY_OUTPUT

// Uncomment to enable dumping the normalize limiter lookup table to the file
// "/tmp/limiter". The generated file will be about 3MB in size. This option shouldn't
// be enabled for day-by-day operation.
//#define ACC_DUMP
#endif                                            //___VDR_CONFIG_H
