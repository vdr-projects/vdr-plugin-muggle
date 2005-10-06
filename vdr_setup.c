/*!
 * \file vdr_setup.c
 * \brief A setup class for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.3 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * $Id$
 *
 * Partially adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#include "vdr_setup.h"
#include "vdr_actions.h"
#include "i18n.h"

static char* chars_allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_./";

static const char *bgmodes[3];

// --- mgMenuSetup -----------------------------------------------------------

mgMenuSetup::mgMenuSetup ()
{
    SetSection (tr ("Muggle"));

    // Audio stuff    
    Add (new
	 cMenuEditBoolItem (tr ("Initial loop mode"),&the_setup.InitLoopMode,
			    tr("on"), tr("off") ) );
    Add (new
	 cMenuEditBoolItem (tr ("Initial shuffle mode"), &the_setup.InitShuffleMode,
			    tr("on"), tr("off") ) );

    Add (new
	 cMenuEditBoolItem (tr ("Audio mode"), &the_setup.AudioMode,
			    tr ("Round"), tr ("Dither")));

    Add (new
	 cMenuEditBoolItem (tr ("Use 48kHz mode only"), &the_setup.Only48kHz,
			    tr("on"), tr("off") ) );
			    
    Add (new
	 cMenuEditIntItem (tr ("Normalizer level"),
			   &the_setup.TargetLevel, 0, MAX_TARGET_LEVEL));
    Add (new
	 cMenuEditIntItem (tr ("Limiter level"),
			   &the_setup.LimiterLevel, MIN_LIMITER_LEVEL, 100));

    // Image/cover display
    bgmodes[0] = tr("Black");
    bgmodes[1] = tr("Image");
    bgmodes[2] = tr("Live");
    Add (new
	 cMenuEditStraItem (tr ("Background mode"), &the_setup.BackgrMode, 
			    3, bgmodes ) );
    Add (new
	 cMenuEditIntItem (tr ("Image show duration (secs)"),
			   &the_setup.ImageShowDuration, 1, 100));
    Add (new
	 cMenuEditStrItem (tr ("Image cache directory"),
			   the_setup.ImageCacheDir, 256, chars_allowed ) );
    Add (new
	 cMenuEditBoolItem (tr ("Use DVB still picture"), &the_setup.UseDeviceStillPicture,
			    tr("yes"), tr("no") ) );

    // Synchronization    
    Add (new
	 cMenuEditBoolItem (tr ("Delete stale references"), &the_setup.DeleteStaleReferences,
			    tr("yes"), tr("no") ));

    mgAction *a = actGenerate(actSync);
    const char *mn = a->MenuName();
    a->SetText(mn);
    free(const_cast<char*>(mn));
    Add(dynamic_cast<cOsdItem*>(a));
}


void
mgMenuSetup::Store (void)
{
    SetupStore ("InitLoopMode", the_setup.InitLoopMode);
    SetupStore ("InitShuffleMode", the_setup.InitShuffleMode);
    SetupStore ("AudioMode", the_setup.AudioMode);
    SetupStore ("DisplayMode", the_setup.DisplayMode);
    SetupStore ("BackgrMode", the_setup.BackgrMode);
    SetupStore ("TargetLevel", the_setup.TargetLevel);
    SetupStore ("LimiterLevel", the_setup.LimiterLevel);
    SetupStore ("Only48kHz", the_setup.Only48kHz);
    SetupStore ("DeleteStaleReferences", the_setup.DeleteStaleReferences);
}

