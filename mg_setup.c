/*!
 * \file mg_setup.c
 * \brief A setup class for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.3 $
 * \date    $Date: 2005-01-24 15:45:30 +0100 (Mon, 24 Jan 2005) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: wr61 $
 *
 * $Id: mg_setup.c 399 2005-01-24 14:45:30Z wr61 $
 *
 * Partially adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */


#include "mg_setup.h"
#include "mg_tools.h"
#include "mg_db.h"
#include <stdio.h>
#include <string>
#include <getopt.h>
#include <tools.h>

mgSetup the_setup;


mgSetup::mgSetup ()
{
    m_mugglei=false;
    InitLoopMode = 0;
    InitShuffleMode = 0;
    AudioMode = 1;
    DisplayMode = 3;
    BackgrMode = 2;
    TargetLevel = DEFAULT_TARGET_LEVEL;
    LimiterLevel = DEFAULT_LIMITER_LEVEL;
    Only48kHz = 0;

    DbHost = 0;
    DbSocket = 0;
    DbPort = 0;
    DbName = strdup ("GiantDisc");
    DbUser = 0;
    DbPass = 0;
    asprintf(&DbDatadir,"%s/.muggle",getenv("HOME"));
    ToplevelDir = strdup("/mnt/music/");
    CreateMode = false;
    DeleteStaleReferences = false;

    // stuff related to cover image display
    ImageCacheDir = strdup( "/tmp" );
    UseDeviceStillPicture = true;
}

bool
mgSetup::IsMugglei() const
{
	return m_mugglei;
}

void
mgSetup::SetMugglei()
{
	m_mugglei = true;
}

mgSetup::~mgSetup ()
{
    free(DbHost);
    free(DbSocket);
    free(DbName);
    free(DbUser);
    free(DbPass);
    free(DbDatadir);
    free(ToplevelDir);
    free(ImageCacheDir);
}

bool
mgSetup::NoHost() const
{
    return !DbHost || strlen(DbHost)==0;
}

bool mgSetup::ProcessArguments (int argc, char *argv[])
{
    mgSetDebugLevel (1);
    char b[1000];
    sprintf(b,"mgSetup::ProcessArgs ");
    for (int i=1;i<argc;i++)
    {
	if (strlen(b)+strlen(argv[i]+2)>1000) break;;
    	strcat(b,"  ");
    	strcat(b,argv[i]);
    }
    mgDebug(1,b);

    struct option long_options[50];
    char short_options[100];
    memset(short_options,0,sizeof(short_options));
    memset(long_options,0,sizeof(long_options));
    static struct option all_options[] =
    {
        {"host", required_argument, 0, 'h'},
        {"socket", required_argument, 0, 's'},
        {"port", required_argument, 0, 'p'},
        {"user", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'w'},
        {"name", required_argument, 0, 'n'},
        {"datadir", required_argument, 0, 'd'},
        {"toplevel", required_argument, 0, 't'},
        {"verbose", required_argument, 0, 'v'},
        {"create", no_argument, 0, 'c'},
        {"delete", no_argument, 0, 'z'},
        {0,0,0}
    };
    char wanted_opts[50];
    strcpy(wanted_opts,"ndtv");
    if (IsMugglei())
	    strcat(wanted_opts,"cz");
    mgDb* db = GenerateDB();
    strcat(wanted_opts,db->Options());
    delete db;
    char *p = wanted_opts;
    char *s = short_options;
    int li = 0;
    while (*p)
    {
	    for (unsigned int idx = 0; all_options[idx].name;idx++)
	    {
		    if (*p==all_options[idx].val)
		    {
			*s++ = *p;
			if (all_options[idx].has_arg==required_argument)
				*s++ = ':';
			long_options[li++]=all_options[idx];
			break;
		    }
	    }
	    p++;
    }
    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, short_options,long_options,&option_index)) != -1)
    {
        switch (c)
        {
            case 'h':
            {
		free(DbHost);
                DbHost = strdup (optarg);
            }
            break;
            case 's':
            {
                free(DbSocket);
		DbSocket = strdup(optarg);
            }
            break;
            case 'p':
            {
                DbPort = atoi (optarg);
            }
            break;
            case 'u':
            {
                free(DbUser);
		DbUser = strdup(optarg);
            }
            break;
            case 'w':
            {
                free(DbPass);
		DbPass = strdup(optarg);
            }
            break;
            case 'n':
            {
                free(DbName);
		DbName = strdup(optarg);
            }
            break;
            case 'd':
            {
                free(DbDatadir);
		DbDatadir = strdup(optarg);
            }
	    break;
            case 'v':
            {
    		mgSetDebugLevel (atol(optarg));
            }
            break;
            case 't':
            {
		free(ToplevelDir);
                if (optarg[strlen (optarg) - 1] != '/')
                {
                    std::string res = std::string (optarg) + "/";
                    ToplevelDir = strdup (res.c_str ());
                }
                else
                    ToplevelDir = strdup(optarg);
            }
            break;
            case 'z':
            {
            	DeleteStaleReferences = true;
            }
	    break;
            case 'c':
            {
            	CreateMode = true;
            }
	    break;
	}
    }
    return true;
}

const char*
mgSetup::HelpText()
{
    static char buf[2000];
    strcpy(buf,
    "  -n NNNN,  --name=NNNN     specify database name (default is GiantDisc)\n"
    "  -t TTTT,  --toplevel=TTTT specify toplevel directory for music (default is /mnt/music)\n"
    "  -d DIRN,  --datadir=DIRN  specify directory for embedded sql data (default is $HOME/.muggle)\n"
    "  -v,       --verbose       specify debug level. The higher the more. Default is 1\n");
    if (IsMugglei())
	    strcat(buf,
    "  -z        --delete        scan all data base entries and delete entries if their file is not found\n"
    "  -c        --create        delete the entire data base and create a new one\n");
    mgDb* db = GenerateDB();
    strcat(buf,db->HelpText());
    delete db;
    return buf;
}
