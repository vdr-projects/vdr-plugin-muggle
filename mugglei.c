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

bool import_assorted, delete_mode, create_mode;

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
}

bool
create_question()
{
    return create_mode;
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
#ifdef HAVE_ONLY_SERVER
      std::cout << "  -h <hostname>       - specify host of database server (default is 'localhost')" << std::endl;
#else
      std::cout << "  -h <hostname>       - specify host of database server (default is embedded')" << std::endl;
#endif
      std::cout << "  -s <socket>         - specify a socket for communication with database server (default is TCP)" << std::endl;
      std::cout << "  -n <database>       - specify database name (default is 'GiantDisc')" << std::endl;
      std::cout << "  -u <username>       - specify user of database (default is empty)" << std::endl;
      std::cout << "  -p <password>       - specify password of user (default is empty password)" << std::endl;
      std::cout << "  -t <topleveldir>    - name of music top level directory" << std::endl;
      std::cout << "  -z                  - scan all database entries and delete entries for files not found" << std::endl;
      std::cout << "                        -z is not yet implemented" << std::endl;
      std::cout << "  -c                  - delete the entire database and recreate a new empty one" << std::endl;
      std::cout << "  -d <datadir>        - the data directory for embedded sql. Defaults to ./.muggle" << std::endl;
      std::cout << "  -v                  - the wanted log level, the higher the more. Default is 1" << std::endl;
      std::cout << std::endl << std::endl;
      std::cout << "if the specified host is localhost, sockets will be used if possible." << std::endl;
      std::cout << "Otherwise the -s parameter will be ignored" << std::endl;

      exit( 1 );
    }

  // option defaults
  import_assorted = false;
  delete_mode = false;
  create_mode = false;

  // parse command line options
  while( 1 )
    {
      int c = getopt(argc, argv, "h:s:n:u:p:t:zcv:d:");

      if (c == -1)
	break;
      
      switch (c) 
	{
	case 0:
	  { // long option
	    
	  } break;
	case 'h':
	  {
	    the_setup.DbHost = strdup(optarg);
	  } break;
	case 'n':
	  {
	    the_setup.DbName = strdup(optarg);
	  } break;
	case 'u':
	  {
	    the_setup.DbUser = strdup(optarg);
	  } break;
	case 'p':
	  {
	    the_setup.DbPass = strdup(optarg);
	  } break;
	case 's':
	  {
	    the_setup.DbSocket = strdup(optarg);
	  } break;
	case 't':
	  {
	    the_setup.ToplevelDir = strdup(optarg);
	  } break;
        case 'z':
          {
            delete_mode = true;
          } break;
        case 'c':
          {
            create_mode = true;
          } break;
        case 'v':
          {
	    mgSetDebugLevel(atol(optarg));
          } break;
        case 'd':
          {
	    the_setup.DbDatadir = strdup(optarg);
          } break;
	}
    }
  if (optind<argc)
  {
  	mgDb *sync = GenerateDB();
	sync->Sync(argv+optind);
  	sync->ServerEnd();
  	delete sync;
  }
  return 0;
}

