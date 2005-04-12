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
#include <getopt.h>
#include <config.h>

static const char *VERSION = "0.1.6";
static const char *DESCRIPTION = "Media juggle plugin for VDR";
static const char *MAINMENUENTRY = "Muggle";

const char *
mgMuggle::Version (void)
{
    return VERSION;
}


const char *
mgMuggle::Description (void)
{
    return DESCRIPTION;
}


const char *
mgMuggle::MainMenuEntry (void)
{
    return MAINMENUENTRY;
}


mgMuggle::mgMuggle (void)
{
#ifndef HAVE_ONLY_SERVER
    char *buf;
    asprintf(&buf,"%s/.muggle",getenv("HOME"));
    set_datadir(buf);
    free(buf);
#endif
}


void
mgMuggle::Stop (void)
{
}


const char *
mgMuggle::CommandLineHelp (void)
{
// Return a string that describes all known command line options.
    return
#ifdef HAVE_ONLY_SERVER
        "  -h HHHH,  --host=HHHH     specify database host (default is localhost)\n"
#else
        "  -h HHHH,  --host=HHHH     specify database host (default is mysql embedded)\n"
#endif
        "  -s SSSS   --socket=PATH   specify database socket\n"
        "  -n NNNN,  --name=NNNN     specify database name (default is GiantDisc)\n"
        "  -p PPPP,  --port=PPPP     specify port of database server (default is )\n"
        "  -u UUUU,  --user=UUUU     specify database user (default is )\n"
        "  -w WWWW,  --password=WWWW specify database password (default is empty)\n"
        "  -t TTTT,  --toplevel=TTTT specify toplevel directory for music (default is /mnt/music)\n"
#ifndef HAVE_ONLY_SERVER
        "  -d DIRN,  --datadir=DIRN  specify directory for embedded sql data (default is $HOME/.muggle)\n"
#endif
        "  -v,       --verbose       specify debug level. The higher the more. Default is 1\n"
	"\n"
	"if the specified host is localhost, sockets will be used if possible.\n"
	"Otherwise the -s parameter will be ignored";
}


bool mgMuggle::ProcessArgs (int argc, char *argv[])
{
    mgSetDebugLevel (1);
    char b[1000];
    sprintf(b,"mgMuggle::ProcessArgs ");
    for (int i=1;i<argc;i++)
    {
	if (strlen(b)+strlen(argv[i]+2)>1000) break;;
    	strcat(b,"  ");
    	strcat(b,argv[i]);
    }
    mgDebug(1,b);

// Implement command line argument processing here if applicable.
    static struct option
        long_options[] =
    {
        {"host", required_argument, NULL, 'h'},
        {"socket", required_argument, NULL, 's'},
        {"name", required_argument, NULL, 'n'},
        {"port", required_argument, NULL, 'p'},
        {"user", required_argument, NULL, 'u'},
        {"password", required_argument, NULL, 'w'},
#ifndef HAVE_ONLY_SERVER
        {"datadir", required_argument, NULL, 'd'},
#endif
        {"toplevel", required_argument, NULL, 't'},
        {NULL}
    };

    int
        c,
        option_index = 0;
    while ((c =
#ifndef HAVE_ONLY_SERVER
        getopt_long (argc, argv, "h:s:n:p:t:u:w:d:v:", long_options,
#else
        getopt_long (argc, argv, "h:s:n:p:t:u:w:v:", long_options,
#endif
        &option_index)) != -1)
    {
        switch (c)
        {
            case 'h':
            {
                the_setup.DbHost = strcpyrealloc (the_setup.DbHost, optarg);
            }
            break;
            case 's':
            {
                the_setup.DbSocket = strcpyrealloc (the_setup.DbSocket, optarg);
            }
            break;
            case 'n':
            {
                the_setup.DbName = strcpyrealloc (the_setup.DbName, optarg);
            }
            break;
            case 'p':
            {
                the_setup.DbPort = atoi (optarg);
            }
            break;
            case 'u':
            {
                the_setup.DbUser = strcpyrealloc (the_setup.DbUser, optarg);
            }
            break;
            case 'w':
            {
                the_setup.DbPass = strcpyrealloc (the_setup.DbPass, optarg);
            }
            break;
#ifndef HAVE_ONLY_SERVER
            case 'd':
            {
	        set_datadir(optarg);
            }
	    break;
#endif
            case 'v':
            {
    		mgSetDebugLevel (atol(optarg));
            }
            break;
            case 't':
            {
                if (optarg[strlen (optarg) - 1] != '/')
                {
                    std::string res = std::string (optarg) + "/";
                    the_setup.ToplevelDir = strdup (res.c_str ());
                }
                else
                {
                    the_setup.ToplevelDir =
                        strcpyrealloc (the_setup.ToplevelDir, optarg);
                }
            }
            break;
            default:
                return false;
        }
    }

    return true;
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
    else
        return false;

    return true;
}


VDRPLUGINCREATOR (mgMuggle);                      // Don't touch this!
