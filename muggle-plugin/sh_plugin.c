/*
 * plugin.c: The VDR plugin interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sh_plugin.c,v 1.1 2004/02/01 18:22:53 LarsAC Exp $
 */

#include "shell_plugin.h"

#define LIBVDR_PREFIX  "libvdr-"
#define SO_INDICATOR   ".so."

#define MAXPLUGINARGS  1024
#define HOUSEKEEPINGDELTA 10 // seconds

// --- cPlugin ---------------------------------------------------------------

char *cPlugin::configDirectory = NULL;

cPlugin::cPlugin(void) 
{
  name = NULL;
}

cPlugin::~cPlugin()
{
    // I18nRegister(NULL, Name());
}

void cPlugin::SetName(const char *s)
{
  name = s;
}

const char *cPlugin::CommandLineHelp(void)
{
  return NULL;
}

bool cPlugin::ProcessArgs(int argc, char *argv[])
{
  return true;
}

bool cPlugin::Initialize(void)
{
  return true;
}

bool cPlugin::Start(void)
{
  return true;
}

void cPlugin::Housekeeping(void)
{
}

const char *cPlugin::MainMenuEntry(void)
{
  return NULL;
}

cOsdObject *cPlugin::MainMenuAction(void)
{
  return NULL;
}
bool cPlugin::SetupParse(const char *Name, const char *Value)
{
    return false;
}
cMenuSetupPage *cPlugin::SetupMenu(void)
{
    return NULL;
}
