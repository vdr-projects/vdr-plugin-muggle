/*! 
 * \file   muggle.h
 * \brief  Implements a plugin for browsing media libraries within VDR
 *
 * \version $Revision: 1.6 $
 * \date    $Date: 2004/07/09 12:22:00 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 *
 *  $Id: muggle.h,v 1.6 2004/07/09 12:22:00 LarsAC Exp $
 */

#ifndef _MUGGLE_H
#define _MUGGLE_H

#include <plugin.h>

class mgMedia;
class mgSelectionTreeNode;
class mgPlaylist;

class cCommands;

class mgMuggle : public cPlugin
{
public:

  mgMuggle(void);

  virtual ~mgMuggle();

  virtual const char *Version(void);

  virtual const char *Description(void);

  virtual const char *CommandLineHelp(void);

  virtual bool ProcessArgs(int argc, char *argv[]);

  virtual bool Initialize(void);

  virtual bool Start(void);

  virtual void Housekeeping(void);

  virtual const char *MainMenuEntry(void);

  virtual cOsdObject *MainMenuAction(void);

  virtual cMenuSetupPage *SetupMenu(void);

  virtual bool SetupParse(const char *Name, const char *Value);

private:

  mgMedia             *m_media;
  mgSelectionTreeNode *m_root;
  mgPlaylist          *m_playlist;
  cCommands           *m_playlist_commands;

};

#endif
