/*******************************************************************/
/*! \file   muggle.h
 *  \brief  Implements a plugin for browsing media libraries within VDR
 ******************************************************************** 
 * \version $Revision: 1.4 $
 * \date    $Date: 2004/02/03 19:28:46 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 */
/*******************************************************************/

#ifndef _MUGGLE_H
#define _MUGGLE_H

#undef SHELL_TEST

#ifdef SHELL_TEST
  #include "shell_plugin.h"
#else
  #include <plugin.h>
#endif

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
