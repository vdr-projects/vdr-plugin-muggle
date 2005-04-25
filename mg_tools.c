/*!
 * \file  mg_tools.c
 * \brief  A few util functions for standalone and plugin messaging for the vdr muggle plugindatabase
 *
 * \version $Revision: 1.4 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "mg_tools.h"

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
	syslog(LOG_DEBUG,"%s\n",buffer);
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
    syslog(LOG_INFO,"Warning: %s\n",buffer);
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

    syslog (LOG_ERR,"Error in Muggle: %s\n", buffer);

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


char *
SeparateFolders(const char *filename, char * folders[],unsigned int fcount)
{
	for (unsigned int i=0;i<fcount;i++)
		folders[i]="";
	char *fbuf=strdup(filename);
	char *slash=fbuf-1;
	for (unsigned int i=0;i<fcount;i++)
	{
		char *p=slash+1;
		slash=strchr(p,'/');
		if (!slash)
			break;
		folders[i]=p;
		*slash=0;
	}
	return fbuf;
}

string&
addsep (string & s, string sep, string n)
{
	if (!n.empty ())
	{
		if (!s.empty ())
    		s.append (sep);
		s.append (n);
	}
	return s;
}


//! \brief converts long to string
string
itos (int i)
{
	std::stringstream s;
	s << i;
	return s.str ();
}

//! \brief convert long to string
string
ltos (long l)
{
	std::stringstream s;
	s << l;
	return s.str ();
}
