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
#include "mg_db.h"

#include "i18n.h"
#include <getopt.h>
#include <config.h>

static const char *VERSION = "0.1.2";
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
    main = NULL;
// defaults for database arguments
    the_setup.DbHost = strdup ("localhost");
    the_setup.DbSocket = NULL;
    the_setup.DbPort = 0;
    the_setup.DbName = strdup ("GiantDisc");
    the_setup.DbUser = strdup ("");
    the_setup.DbPass = strdup ("");
    the_setup.GdCompatibility = false;
    the_setup.ToplevelDir = strdup ("/mnt/music/");
}


mgMuggle::~mgMuggle ()
{
    if (main) main->SaveState();
    free(the_setup.DbHost);
    free(the_setup.DbName);
    free(the_setup.DbUser);
    free(the_setup.DbPass);
    free(the_setup.ToplevelDir);
}


const char *
mgMuggle::CommandLineHelp (void)
{
// Return a string that describes all known command line options.
    return
        "  -h HHHH,  --host=HHHH     specify database host (default is localhost)\n"
        "  -s SSSS   --socket=PATH   specify database socket (default is TCP connection)\n"
        "  -n NNNN,  --name=NNNN     specify database name (overridden by -g)\n"
        "  -p PPPP,  --port=PPPP     specify port of database server (default is )\n"
        "  -u UUUU,  --user=UUUU     specify database user (default is )\n"
        "  -w WWWW,  --password=WWWW specify database password (default is empty)\n"
        "  -t TTTT,  --toplevel=TTTT specify toplevel directory for music (default is /mnt/music)\n"
        "  -g,       --giantdisc     enable full Giantdisc compatibility mode\n";
}


bool mgMuggle::ProcessArgs (int argc, char *argv[])
{
    mgDebug (1, "mgMuggle::ProcessArgs");

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
        {"toplevel", required_argument, NULL, 't'},
        {"giantdisc", no_argument, NULL, 'g'},
        {NULL}
    };

    int
        c,
        option_index = 0;
    while ((c =
        getopt_long (argc, argv, "gh:s:n:p:t:u:w:", long_options,
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
            case 'g':
            {
                the_setup.DbName = strcpyrealloc (the_setup.DbName, "GiantDisc");
                the_setup.GdCompatibility = true;
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
    mgSetDebugLevel (99);
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
    main = new mgMainMenu (); 
    return main;
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
