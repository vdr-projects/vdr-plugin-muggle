/*!
 * \file   muggle.h
 * \ingroup vdr
 * \brief  Implements a plugin for browsing media libraries within VDR
 *
 * \version $Revision: 1.6 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 *
 *  $Id$
 */

// Some notes about the general structure of the plugin

/* \defgroup giantdisc GiantDisc integration layer
 *    The GiantDisc integration layer contains functions and classes
 *    which enable interoperability with a database schema that conforms
 *    to the GiantDisc layout.
 *
 * \defgroup vdr       VDR integration layer
 *    The VDR integration layer contains components which allow the
 *    plugin functionality to be accessed by VDR. These are mainly
 *    related to the OSD and a player/control combination.
 *
 * \defgroup muggle    Main muggle business
 *    The core of the plugin is an abstract representation of information
 *    organized in trees (thus suitable for OSD navigation) as well as
 *    means to organize
 */

#ifndef _MUGGLE_H
#define _MUGGLE_H
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <vdr/plugin.h>

class mgMainMenu;

class mgMuggle:public cPlugin
{
    public:

        mgMuggle (void);

        virtual const char *Version (void);

        virtual const char *Description (void);

        virtual const char *CommandLineHelp (void);

        virtual bool ProcessArgs (int argc, char *argv[]);

        virtual bool Initialize (void);

        virtual bool Start (void);

        virtual void  Stop (void);

        virtual void Housekeeping (void);

        virtual const char *MainMenuEntry (void);

        virtual cOsdObject *MainMenuAction (void);

        virtual cMenuSetupPage *SetupMenu (void);

        virtual bool SetupParse (const char *Name, const char *Value);

	virtual bool Service(const char *Id, void *Data = NULL);

};
#endif
