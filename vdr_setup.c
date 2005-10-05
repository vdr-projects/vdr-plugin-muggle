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

// --- mgMenuSetup -----------------------------------------------------------

mgMenuSetup::mgMenuSetup ()
{
    SetSection (tr ("Muggle"));

    // Audio stuff    
    Add (new
	 cMenuEditBoolItem (tr ("Setup.Muggle$Initial loop mode"),
			    &the_setup.InitLoopMode));
    Add (new
	 cMenuEditBoolItem (tr ("Setup.Muggle$Initial shuffle mode"),
			    &the_setup.InitShuffleMode));
    Add (new
	 cMenuEditBoolItem (tr ("Setup.Muggle$Audio mode"), &the_setup.AudioMode,
			    tr ("Round"), tr ("Dither")));
    Add (new
	 cMenuEditBoolItem (tr ("Setup.Muggle$Use 48kHz mode only"),
			    &the_setup.Only48kHz));
    Add (new
	 cMenuEditIntItem (tr ("Setup.Muggle$Normalizer level"),
			   &the_setup.TargetLevel, 0, MAX_TARGET_LEVEL));
    Add (new
	 cMenuEditIntItem (tr ("Setup.Muggle$Limiter level"),
			   &the_setup.LimiterLevel, MIN_LIMITER_LEVEL, 100));

    // Image/cover display
    Add (new
	 cMenuEditIntItem (tr ("Setup.Muggle$Background mode"),
			   &the_setup.BackgrMode, 1, 3 ) );
    Add (new
	 cMenuEditIntItem (tr ("Setup.Muggle$Image show duration"),
			   &the_setup.ImageShowDuration, 1, 100));
    Add (new
	 cMenuEditStrItem (tr ("Setup.Muggle$Image cache directory"),
			   the_setup.ImageCacheDir, 256, chars_allowed ) );
    Add (new
	 cMenuEditBoolItem (tr ("Setup.Muggle$Use DVB still picture"),
			    &the_setup.UseDeviceStillPicture));

    // Synchronization    
    Add (new
	 cMenuEditBoolItem (tr ("Setup.Muggle$Delete stale references"),
			    &the_setup.DeleteStaleReferences));

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

