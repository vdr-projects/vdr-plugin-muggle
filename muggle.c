/*!
 * \file   muggle.c
 * \brief  Implements a plugin for browsing media libraries within VDR
 *
 * \version $Revision: 1.10 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author$
 *
 *  $Id$
 */

#include "muggle.h"

#include "vdr_menu.h"
#include "vdr_setup.h"
#include "mg_tools.h"

#include "i18n.h"
#include <vdr/tools.h>
#include <vdr/config.h>

static const char VERSION[] = "0.1.11";
static const char DESCRIPTION[] = "Media juggle plugin for VDR";
static const char MAINMENUENTRY[] = "Muggle";

const char *
mgMuggle::Version (void)
{
    return VERSION;
}


const char *
mgMuggle::Description (void)
{
    return tr(DESCRIPTION);
}


const char *
mgMuggle::MainMenuEntry (void)
{
    return tr(MAINMENUENTRY);
}


mgMuggle::mgMuggle (void)
{
}


void
mgMuggle::Stop (void)
{
	delete DbServer;
	DbServer = 0;
}


const char *
mgMuggle::CommandLineHelp (void)
{
   return the_setup.HelpText();
}


bool mgMuggle::ProcessArgs (int argc, char *argv[])
{
   return the_setup.ProcessArguments(argc,argv);
}

bool mgMuggle::Initialize (void)
{
// Initialize any background activities the plugin shall perform.
    return true;
}


bool mgMuggle::Start (void)
{
// Start any background activities the plugin shall perform.
    RegisterI18n (Phrases);
    return true;
}


void
mgMuggle::Housekeeping (void)
{
// Perform any cleanup or other regular tasks.
}


cOsdObject *
mgMuggle::MainMenuAction (void)
{
// Perform the action when selected from the main VDR menu.
    return new mgMainMenu (); 
}


cMenuSetupPage *
mgMuggle::SetupMenu (void)
{
    return new mgMenuSetup ();
}


bool mgMuggle::SetupParse (const char *Name, const char *Value)
{
    if (!strcasecmp (Name, "InitLoopMode"))
        the_setup.InitLoopMode = atoi (Value);
    else if (!strcasecmp (Name, "InitShuffleMode"))
        the_setup.InitShuffleMode = atoi (Value);
    else if (!strcasecmp (Name, "AudioMode"))
        the_setup.AudioMode = atoi (Value);
    else if (!strcasecmp (Name, "DisplayMode"))
        the_setup.DisplayMode = atoi (Value);
    else if (!strcasecmp (Name, "BackgrMode"))
        the_setup.BackgrMode = atoi (Value);
    else if (!strcasecmp (Name, "TargetLevel"))
        the_setup.TargetLevel = atoi (Value);
    else if (!strcasecmp (Name, "LimiterLevel"))
        the_setup.LimiterLevel = atoi (Value);
    else if (!strcasecmp (Name, "Only48kHz"))
        the_setup.Only48kHz = atoi (Value);
    else if (!strcasecmp (Name, "DeleteStaleReferences"))
        the_setup.DeleteStaleReferences = atoi (Value);
    else if (!strcasecmp (Name, "ImageShowDuration"))
        the_setup.ImageShowDuration = atoi (Value);
    else if (!strcasecmp (Name, "ImageCacheDir"))
        strn0cpy (the_setup.ImageCacheDir, Value, MAX_PATH);
    else if (!strcasecmp (Name, "UseStillPicture"))
        the_setup.UseDeviceStillPicture = atoi (Value);
    else
        return false;

    return true;
}

bool mgMuggle::Service( const char *Id, void *Data )
{
  bool result = false;
  
  if( !strcmp( Id, "ReplayDirectoryImages" ) )
    {
      if( Data )
	{
	  // check whether there is a current player and signal the new image playlist to it
	  mgPlayerControl *c = PlayerControl();
	  if( c )
	    {
          std::ostringstream os;
	      os << "Found running muggle player to display image playlist in " << (char *) Data;
          isyslog("%s",os.str().c_str());

	      c->NewImagePlaylist( (char*) Data );
          isyslog("New image playlist signaled.");

	      result = true;
	    }
	  // return false otherwise (never start an extra player for this)
	  else
	    {
	      result = false;
	    }
	}
      else
	{
	  // generally we can handle this
	  result = true;
	}
    }
  
  return result;
}


VDRPLUGINCREATOR (mgMuggle);                      // Don't touch this!
