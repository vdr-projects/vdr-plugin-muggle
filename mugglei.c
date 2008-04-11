/*!
 * \file mugglei.c
 * \brief implement a small utility for importing files
 *
 * \author  Lars von Wedel
 */

// #define VERBOSE

#include <unistd.h>
#include <cstring>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdarg.h>

#include <tag.h>
#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>

#include "mg_tools.h"
#include "mg_setup.h"
#include "mg_db.h"

using namespace std;

int SysLogLevel = 9;

void showmessage(int duration,const char *msg,...) {
	va_list ap;
	va_start(ap,msg);
	vfprintf(stderr,msg,ap);
	fprintf(stderr,"\n");
	va_end(ap);
}

void syslog_with_tid(int priority, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsyslog(priority, format, ap);
	va_end(ap);
}

void showimportcount(unsigned int importcount,bool final=false) {
	if (final)
		mgDebug(1,"Imported %d tracks",importcount);
}

bool
create_question() {
	return the_setup.CreateMode;
}

void
import() {
}

bool
path_within_tld() {
	char path[5000];
	if (!getcwd(path,4999)) {
		std::cout << "Path too long" << std::endl;
		exit (1);
	}
	int tldlen = strlen(the_setup.ToplevelDir);
	strcat(path,"/");
	int pathlen = strlen(path);
	if (pathlen<tldlen)
		return false;
	return !strncmp(path,the_setup.ToplevelDir,tldlen);
}

static void
usage() {
	std::cout << "mugglei -- import helper for Muggle VDR plugin" << std::endl;
	std::cout << "(C) Lars von Wedel, Wolfgang Rohdewald" << std::endl;
	std::cout << "This is free software; see the source for copying conditions." << std::endl;
	std::cout << "" << std::endl;
	std::cout << "Usage: mugglei [OPTION]... [FILE]..." << std::endl;
	std::cout << "" << std::endl;
	std::cout << "  all FILE arguments will be imported. If they are directories, their content is imported"<< std::endl;
	std::cout << "" << std::endl;
	std::cout << "Only files ending in .flac, .mp3, .ogg (ignoring case) will be imported" << std::endl;
	std::cout << "" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << the_setup.HelpText();

	exit( 2 );
}

int main( int argc, char *argv[] ) {
	setlocale(LC_ALL,"");
	the_setup.SetMugglei();
	mgSetDebugLevel(1);

	if( argc < 2 )
		usage();

	if (!the_setup.ProcessArguments(argc,argv))
		usage();

	if (!path_within_tld()) {
		std::cout << "you should be in " << the_setup.ToplevelDir
			<< " or below" << std::endl;
		exit( 2 );
	}
	if (optind==argc)
		usage();
	mgDb *sync = GenerateDB();
	sync->Sync(argv+optind);
	delete sync;
}
