/*! 
 * \file   muggle.h
 * \brief  Implements a plugin for browsing media libraries within VDR
 *
 * \version $Revision: 1.5 $
 * \date    $Date: 2004/05/28 15:29:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: lvw $
 *
 *  $Id: muggle.h,v 1.5 2004/05/28 15:29:18 lvw Exp $
 */

#ifndef _MUGGLE_H
#define _MUGGLE_H

#include <plugin.h>

class mgMedia;
class mgSelectionTreeNode;
class mgPlaylist;

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
    
};

#endif
