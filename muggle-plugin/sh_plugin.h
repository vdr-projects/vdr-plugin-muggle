/*
 * plugin.h: The VDR plugin interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sh_plugin.h,v 1.1 2004/02/01 18:22:53 LarsAC Exp $
 */

#ifndef __PLUGIN_H
#define __PLUGIN_H

#include "myosd.h"
#include  "mymenuitems.h"
typedef cOsdMenu  cOsdObject;
#define VDRPLUGINCREATOR(PluginClass) extern "C" void *VDRPluginCreator(void) { return new PluginClass; }

class cPlugin {
private:
  static char *configDirectory;
  const char *name;
  void SetName(const char *s);
public:
  cPlugin(void);
  virtual ~cPlugin();

  const char *Name(void) { return name; }
  virtual const char *Version(void) = 0;
  virtual const char *Description(void) = 0;
  virtual const char *CommandLineHelp(void);

  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Housekeeping(void);

  virtual const char *MainMenuEntry(void);
  virtual cOsdObject *MainMenuAction(void);

  virtual cMenuSetupPage *SetupMenu(void);

  virtual bool SetupParse(const char *Name, const char *Value);

  };

#endif //__PLUGIN_H
