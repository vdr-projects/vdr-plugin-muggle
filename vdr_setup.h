/*!
 * \file vdr_setup.h
 * \brief A setup class for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * $Id$
 *
 * Adapted from 
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___SETUP_MG_H
#define ___SETUP_MG_H

// #include <osd.h>
#include <menuitems.h>

#define MAX_STRING_LEN 128

#define DEFAULT_TARGET_LEVEL  25
#define MAX_TARGET_LEVEL      50
#define DEFAULT_LIMITER_LEVEL 70
#define MIN_LIMITER_LEVEL     25

/*!
 * \brief storage for setup data
 */
class mgSetup
{
public:
  int InitLoopMode;
  int InitShuffleMode;
  int AudioMode;
  int DisplayMode;
  int BackgrMode;
  int MenuMode;
  int TargetLevel;
  int LimiterLevel;
  int Only48kHz;

  char *DbHost;
  char *DbSocket;
  char *DbName;
  char *DbUser;
  char *DbPass;
  int  DbPort;
  bool GdCompatibility;
  char *ToplevelDir;

  bool trackview;
  bool progressview;

  char PathPrefix[MAX_STRING_LEN];

 public:
  mgSetup(void);
};

extern mgSetup the_setup;

/*!
 * \brief allow user to modify setup on OSD
 */
class mgMenuSetup : public cMenuSetupPage 
{
private:
  mgSetup m_data;
protected:
  virtual void Store();
public:
  mgMenuSetup();
};


#endif //___SETUP_MP3_H
