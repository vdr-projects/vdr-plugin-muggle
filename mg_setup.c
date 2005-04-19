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
#include <string>

mgSetup the_setup;


mgSetup::mgSetup ()
{
    InitLoopMode = 0;
    InitShuffleMode = 0;
    AudioMode = 1;
    DisplayMode = 3;
    BackgrMode = 1;
    TargetLevel = DEFAULT_TARGET_LEVEL;
    LimiterLevel = DEFAULT_LIMITER_LEVEL;
    Only48kHz = 0;

    DbHost = 0;
    DbSocket = 0;
    DbPort = 0;
    DbName = strdup ("GiantDisc");
    DbUser = 0;
    DbPass = 0;
    ToplevelDir = strdup("/mnt/music/");

    DeleteStaleReferences = false;
}

mgSetup::~mgSetup ()
{
    free(DbHost);
    free(DbSocket);
    free(DbName);
    free(DbUser);
    free(DbPass);
    free(ToplevelDir);
}

bool
mgSetup::NoHost() const
{
    return !DbHost || strlen(DbHost)==0;
}

