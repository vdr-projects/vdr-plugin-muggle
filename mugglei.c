/*!
 * \file mugglei.c
 * \brief implement a small utility for importing files
 *
 * \author  Lars von Wedel
 */

// #define VERBOSE

#include <unistd.h>
#include <string>

#include <stdlib.h>
#include <stdio.h>
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

int SysLogLevel = 1;

void showmessage(int duration,const char *msg,...)
{
	va_list ap;
	va_start(ap,msg);
	vfprintf(stderr,msg,ap);
	fprintf(stderr,"\n");
	va_end(ap);
}

void showimportcount(unsigned int importcount,bool final=false)
{
	if (final)
		mgDebug(1,"Imported %d tracks",importcount);
}

bool
create_question()
{
    return the_setup.CreateMode;
}

void
import()
{
}

const char *I18nTranslate(const char *s,const char *Plugin)
{
	return s;
}

int main( int argc, char *argv[] )
{
	the_setup.SetMugglei();
	mgSetDebugLevel(1);
      
  if( argc < 2 )
    { // we need at least a filename!
      std::cout << "mugglei -- import helper for Muggle VDR plugin" << std::endl;
      std::cout << "(C) Lars von Wedel" << std::endl;
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

      exit( 1 );
    }

  the_setup.ProcessArguments(argc,argv);
  if (optind<argc)
  {
  	mgDb *sync = GenerateDB();
	sync->Sync(argv+optind);
  	sync->ServerEnd();
  	delete sync;
  }
  return 0;
}

