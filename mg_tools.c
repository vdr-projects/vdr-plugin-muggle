/*!
 * \file  mg_tools.c
 * \brief  A few util functions for standalone and plugin messaging for the vdr muggle plugindatabase
 *
 * \version $Revision: 1.4 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 */

#include "mg_tools.h"

/*extern "C"
{*/
#include <stdarg.h>
#include <stdio.h>
/*}
 */
#include <stdlib.h>

//! \brief buffer for messages
#define  MAX_BUFLEN  2048

static char buffer[MAX_BUFLEN];

static int DEBUG_LEVEL = 3;

void
mgSetDebugLevel (int new_level)
{
    DEBUG_LEVEL = new_level;
}


void
mgDebug (int level, const char *fmt, ...)
{

    va_list ap;
    if (level <= DEBUG_LEVEL)
    {
        va_start (ap, fmt);

        vsnprintf (buffer, MAX_BUFLEN - 1, fmt, ap);
        if (STANDALONE)
        {
            fprintf (stderr, "dbg %d: %s\n", level, buffer);
        }
        else
        {
#if !STANDALONE
            isyslog ("%s\n", buffer);
#endif
        }
    }
    va_end (ap);
}


void
mgDebug (const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    mgDebug (1, fmt, ap);
}


void
mgWarning (const char *fmt, ...)
{

    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, MAX_BUFLEN - 1, fmt, ap);

    if (STANDALONE)
    {
        fprintf (stderr, "warning: %s\n", buffer);
    }
    else
    {
#if !STANDALONE
        isyslog ("Warning: %s\n", buffer);
#endif
    }
    extern void showmessage(const char*,int duration=0);
    showmessage(buffer);
    va_end (ap);
}


void
mgError (const char *fmt, ...)
{

    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, MAX_BUFLEN - 1, fmt, ap);

    if (STANDALONE)
    {
        fprintf (stderr, "Error: %s\n", buffer);
        exit (1);
    }
    else
    {
#if !STANDALONE
        isyslog ("Error in Muggle: %s\n", buffer);
#endif
    }

    va_end (ap);
}


std::string trim(std::string const& source, char const* delims ) {
  std::string result(source);
  std::string::size_type index = result.find_last_not_of(delims);
  if(index != std::string::npos)
    result.erase(++index);
  index = result.find_first_not_of(delims);
  if(index != std::string::npos)
    result.erase(0, index);
  else
    result.erase();
  return result;
}
