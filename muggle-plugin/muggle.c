/*******************************************************************/
/*! \file   muggle.c
 *  \brief  Implements a plugin for browsing media libraries within VDR
 ******************************************************************** 
 * \version $Revision: 1.5 $
 * \date    $Date: 2004/02/23 15:17:51 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: RaK $
 */
/*******************************************************************/

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Access GiantDisc database contents";
static const char *MAINMENUENTRY  = "Muggle";

#include "muggle.h"

#include "vdr_menu.h"

#include "mg_tools.h"
#include "mg_content_interface.h"
#include "mg_media.h"

#include "i18n.h"

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

  m_media = new mgMedia( mgMedia::GD_MP3 );
  m_root  = m_media->getSelectionRoot();
  m_playlist = m_media->createTemporaryPlaylist();
  m_media->initFilterSet();
  RegisterI18n(Phrases);
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
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool mgMuggle::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

VDRPLUGINCREATOR(mgMuggle); // Don't touch this!
