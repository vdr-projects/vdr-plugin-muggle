/*! 
 * \file   muggle.c
 * \brief  Implements a plugin for browsing media libraries within VDR
 *
 * \version $Revision: 1.7 $
 * \date    $Date: 2004/05/28 15:29:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: lvw $
 *
 *  $Id: muggle.c,v 1.7 2004/05/28 15:29:18 lvw Exp $
 */

#include <getopt.h>

#include "muggle.h"

#include "vdr_menu.h"
#include "vdr_setup.h"
#include "mg_tools.h"
#include "mg_content_interface.h"
#include "mg_media.h"

#include "i18n.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Media juggle plugin for VDR";
static const char *MAINMENUENTRY  = "Muggle";

using namespace std;

const char* mgMuggle::Version(void)
{ 
  return VERSION;
}

const char* mgMuggle::Description(void)
{ 
  return DESCRIPTION;
}

const char* mgMuggle::MainMenuEntry(void)
{ 
  return MAINMENUENTRY;
}

mgMuggle::mgMuggle(void)
{
  // defaults for database arguments
  the_setup.DbHost = "localhost";
  the_setup.DbPort = 0;
  the_setup.DbName = "GiantDisc2";
  the_setup.DbUser = "";
  the_setup.DbPass = "" ;
  the_setup.GdCompatibility = false;
  the_setup.ToplevelDir = "";
}

mgMuggle::~mgMuggle()
{
  // Clean up after yourself!
}

const char *mgMuggle::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return 
    "  -h HHHH,  --host=HHHH     specify database host (default is localhost)\n"
    "  -n NNNN,  --name=NNNN     specify database name (overridden by -g)\n"
    "  -p PPPP,  --port=PPPP     specify port of database server (default is )\n"
    "  -u UUUU,  --user=UUUU     specify database user (default is )\n"
    "  -w WWWW,  --password=WWWW specify database password (default is empty)\n"
    "  -t TTTT,  --toplevel=TTTT specify toplevel directory for music\n"
    "  -g,       --giantdisc     enable full Giantdisc compatibility mode\n";
}

bool mgMuggle::ProcessArgs(int argc, char *argv[])
{
  cout << "mgMuggle::ProcessArgs" << endl << flush;

  // Implement command line argument processing here if applicable.
  static struct option long_options[] = 
    {
      { "host",      required_argument, NULL, 'h' },
      { "name",      required_argument, NULL, 'n' },
      { "port",      required_argument, NULL, 'p' },
      { "user",      required_argument, NULL, 'u' },
      { "password",  required_argument, NULL, 'w' },
      { "toplevel",  required_argument, NULL, 't' },
      { "giantdisc", no_argument,       NULL, 'g' },
      { NULL }
    };

  int c, option_index = 0;
  while( ( c = getopt_long( argc, argv, "h:p:u:w:g:", long_options, &option_index ) ) != -1 ) 
    {
      switch (c) 
	{
	case 'h': 
	  {
	    the_setup.DbHost = optarg;
	  } break;
	case 'n':
	  {
	    the_setup.DbName = optarg;
	  }
	case 'p': 
	  {
	    the_setup.DbPort = atoi( optarg );
	  } break;
	case 'u': 
	  {
	    the_setup.DbUser = optarg;
	  } break;
	case 'w': 
	  {
	    the_setup.DbPass = optarg;
	  } break;
	case 't': 
	  {
	    the_setup.ToplevelDir = optarg;
	  } break;
	case 'g': 
	  {
	    the_setup.GdCompatibility = true;
	  } break;
	default:  return false;
	}
    }

  return true;
}

bool mgMuggle::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool mgMuggle::Start(void)
{
  // Start any background activities the plugin shall perform.
  mgSetDebugLevel( 99 );

  RegisterI18n( Phrases );
  m_media    = new mgMedia( mgMedia::GD_MP3 );
  m_root     = m_media->getSelectionRoot();
  m_playlist = m_media->createTemporaryPlaylist();
  m_media->initFilterSet();
  return true;
}

void mgMuggle::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *mgMuggle::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  cOsdObject* osd = new mgMainMenu( m_media, m_root, m_playlist );

  return osd;
}

cMenuSetupPage *mgMuggle::SetupMenu(void)
{
  return new mgMenuSetup();
}

bool mgMuggle::SetupParse(const char *Name, const char *Value)
{
  if      (!strcasecmp(Name, "InitLoopMode"))     the_setup.InitLoopMode    = atoi(Value);
  else if (!strcasecmp(Name, "InitShuffleMode"))  the_setup.InitShuffleMode = atoi(Value);
  else if (!strcasecmp(Name, "AudioMode"))        the_setup.AudioMode       = atoi(Value);
  else if (!strcasecmp(Name, "DisplayMode"))      the_setup.DisplayMode     = atoi(Value);
  else if (!strcasecmp(Name, "BackgrMode"))       the_setup.BackgrMode      = atoi(Value);
  else if (!strcasecmp(Name, "TargetLevel"))      the_setup.TargetLevel     = atoi(Value);
  else if (!strcasecmp(Name, "LimiterLevel"))     the_setup.LimiterLevel    = atoi(Value);
  else if (!strcasecmp(Name, "Only48kHz"))        the_setup.Only48kHz       = atoi(Value);
  else return false;

  return true;
}

VDRPLUGINCREATOR(mgMuggle); // Don't touch this!
