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

#define __STL_CONFIG_H
#include <vdr/tools.h>

//! \brief buffer for messages
#define  MAX_BUFLEN  2048

static char buffer[MAX_BUFLEN];

static int DEBUG_LEVEL = 3;

void
mgSetDebugLevel (int new_level) {
	DEBUG_LEVEL = new_level;
}

int
msprintf(char **strp, const char *fmt, ...) {
	va_list ap;
	int res;
	va_start (ap, fmt);
	res=vmsprintf(strp,fmt,ap);
	va_end (ap);
	return res;
}

int
vmsprintf(char **strp, const char *fmt, va_list &ap) {
	*strp=0;
	int res=vasprintf (strp, fmt, ap);
	if (res<0) {
		*strp=strdup("???,see logfile");
		mgError("vasprintf() returns %d. This probably means illformed UTF-8 characters."
			" Please convert your file names to UTF-8",fmt,res);
	}
	return res;
}

void
mgDebug (int level, const char *fmt, ...) {

	if (level <= DEBUG_LEVEL) {
		va_list ap;
		va_start (ap, fmt);
#ifdef HAVE_PG
		buffer[0]='P';
#elif HAVE_SQLITE
		buffer[0]='S';
#else
		buffer[0]='M';
#endif
		buffer[1]=' ';
		vsnprintf (buffer+2, sizeof(buffer)-2, fmt, ap);
		dsyslog("%s",buffer);
		va_end (ap);
	}
}

void
mgDebug (const char *fmt, ...) {
	va_list ap;
	va_start (ap, fmt);
	mgDebug (1, fmt, ap);
	va_end (ap);
}

extern void showmessage(int duration,const char*,...);

void
mgWarning (const char *fmt, ...) {

	va_list ap;
	va_start (ap, fmt);
	vsnprintf (buffer, sizeof(buffer), fmt, ap);
	isyslog("%s",buffer);
	showmessage(0,buffer);
	va_end (ap);
}

void
mgError (const char *fmt, ...) {
	va_list ap;
	va_start (ap, fmt);
	vsnprintf (buffer, sizeof(buffer), fmt, ap);
	esyslog("%s",buffer);
	showmessage(0,buffer);
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
SeparateFolders(const char *filename, char * folders[], unsigned int fcount) {
	static char empty[1];
	empty[0]=0;
	for (unsigned int i=0;i<fcount;i++)
		folders[i]=empty;
	char *fbuf=strdup(filename);
	char *slash=fbuf-1;
	for (unsigned int i=0;i<fcount;i++) {
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
addsep (string & s, string sep, string n) {
	if (!n.empty ()) {
		if (!s.empty ())
			s.append (sep);
		s.append (n);
	}
	return s;
}

string
comma (string & s, string n) {
	return addsep (s, ",", n);
}

//! \brief converts long to string
string
itos (int i) {
	std::stringstream s;
	s << i;
	return s.str ();
}

//! \brief convert long to string
string
ltos (long l) {
	std::stringstream s;
	s << l;
	return s.str ();
}

char *
extension(const char *filename) {
	char *dot = strrchr(filename,'.');
	if (!dot)
		dot = strrchr(filename,0)-1;
	return dot+1;
}

bool
notempty(const char *s) {
	if (!s)
		return false;
	else
		return strlen(s);
}

bool samedir( const char *d1, const char *d2 ) {
	int path_max;

#ifdef PATH_MAX
	path_max = PATH_MAX;
#else
	path_max = pathconf ( "/", _PC_PATH_MAX );
	if (path_max <= 0) {
		path_max = 4096;
	}
#endif

	char rp1[path_max], rp2[path_max];

	realpath(d1, rp1);
	realpath(d2, rp2);

	return (!strcmp( rp1, rp2 ) );
}

bool mkdir_p(const char *s) {
	char *slash=strrchr(s,'/');
	if (!slash) return false;
	char *sc = strdup(s);
	*strrchr(sc,'/')=0;	// cut off the filename
	char *p = sc;
	int mode;
	while (*p) {
		slash=strchr(p+1,'/');
		if (slash) *slash=0;
		mode=R_OK|X_OK|W_OK;
		if (slash) if (strchr(slash+1,'/'))
			mode=R_OK|X_OK;
		if (access(sc,mode)) {
			mkdir(sc,0755);
			mgDebug(5,"Made directory %s",sc);
		}
		if (!slash) break;
		*slash='/';
		p=slash+1;
	}
	free(sc);
	return true;
}
