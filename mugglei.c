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
#include <mysql/mysql.h>
#include <getopt.h>
/*extern "C"
{*/
  #include <stdarg.h>
  #include <stdio.h>
/*}
*/
#include <stdlib.h>

#include <tag.h>
#include <mpegfile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <fileref.h>

#include "mg_tools.h"
#include "mg_setup.h"
#include "mg_sync.h"


using namespace std;

int SysLogLevel = 1;

static char *server_args[] = 
{
  "this_program",       /* this string is not used */
  "--datadir=.",
  "--key_buffer_size=32M"
};

static char *server_groups[] = 
{
  "embedded",
  "server",
  "this_program_SERVER",
  (char *)NULL
};

string host, user, pass, dbname, sck;
bool import_assorted, delete_mode, create_mode;

void showmessage(const char *msg)
{
}

const char *I18nTranslate(const char *s,const char *Plugin)
{
	return s;
}

int main( int argc, char *argv[] )
{
	mgSetDebugLevel(1);
  if( mysql_server_init(sizeof(server_args) / sizeof(char *),
                        server_args, server_groups) )
    {
      exit(1);
    }
  char *filename;
      
  if( argc < 2 )
    { // we need at least a filename!
      std::cout << "mugglei -- import helper for Muggle VDR plugin" << std::endl;
      std::cout << "(C) Lars von Wedel" << std::endl;
      std::cout << "This is free software; see the source for copying conditions." << std::endl;
      std::cout << "" << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  -h <hostname>       - specify host of mySql database server (default is 'localhost')" << std::endl;
      std::cout << "  -s <socket>         - specify a socket for mySQL communication (default is TCP)" << std::endl;
      std::cout << "  -n <database>       - specify database name (default is 'GiantDisc')" << std::endl;
      std::cout << "  -u <username>       - specify user of mySql database (default is empty)" << std::endl;
      std::cout << "  -p <password>       - specify password of user (default is empty password)" << std::endl;
      std::cout << "  -t <topleveldir>    - name of music top level directory" << std::endl;
      std::cout << "  -f <filename>       - name of music file or directory to import or update relative to topleveldir" << std::endl;
      std::cout << "  -z                  - scan all database entries and delete entries for files not found" << std::endl;
      std::cout << "  -c                  - create a new database entry deleting existing entries" << std::endl;

      exit( 1 );
    }

  // option defaults
  import_assorted = false;
  delete_mode = false;
  create_mode = false;
  filename = "";

  // parse command line options
  while( 1 )
    {
      int c = getopt(argc, argv, "h:s:n:u:p:t:f:z");

      if (c == -1)
	break;
      
      switch (c) 
	{
	case 0:
	  { // long option
	    
	  } break;
	case 'h':
	  {
	    the_setup.DbHost = optarg;
	  } break;
	case 'n':
	  {
	    the_setup.DbName = optarg;
	  } break;
	case 'u':
	  {
	    the_setup.DbUser = optarg;
	  } break;
	case 'p':
	  {
	    the_setup.DbPass = optarg;
	  } break;
	case 's':
	  {
	    the_setup.DbSocket = optarg;
	  } break;
	case 't':
	  {
	    the_setup.ToplevelDir = optarg;
	  } break;
	case 'f':
	  {
	    filename = optarg;
	  } break;
        case 'z':
          {
            delete_mode = true;
          } break;
        case 'c':
          {
            create_mode = true;
          } break;
	}
    }

  mgSync sync;
  sync.Sync(filename,false);
  return 0;
}

