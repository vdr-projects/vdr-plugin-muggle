/*******************************************************************/
/*! \file   muggle.c
 *  \brief  Implements a plugin for browsing media libraries within VDR
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/01 18:22:53 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 */
/*******************************************************************/

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Access GiantDisc database contents";
static const char *MAINMENUENTRY  = "Muggle";

#include "muggle.h"
#include "vdr_menu.h"
#include "mg_tools.h"

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
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

mgMuggle::~mgMuggle()
{
  // Clean up after yourself!
}

const char *mgMuggle::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool mgMuggle::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
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

  return true;
}

void mgMuggle::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *mgMuggle::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  cOsdObject* osd = new mgMainMenu();

  return osd;
}

cMenuSetupPage *mgMuggle::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool mgMuggle::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

VDRPLUGINCREATOR(mgMuggle); // Don't touch this!
