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
#include "i18n.h"

mgSetup the_setup;

std::string GdFindFile( std::string mp3file, std::string ToplevelDir );

// --- mgMenuSetup -----------------------------------------------------------

mgMenuSetup::mgMenuSetup ()
{
    m_data = the_setup;

    SetSection (tr ("Muggle"));

    Add (new
        cMenuEditBoolItem (tr ("Setup.Muggle$Initial loop mode"),
        &m_data.InitLoopMode));
    Add (new
        cMenuEditBoolItem (tr ("Setup.Muggle$Initial shuffle mode"),
        &m_data.InitShuffleMode));
    Add (new
        cMenuEditBoolItem (tr ("Setup.Muggle$Audio mode"), &m_data.AudioMode,
        tr ("Round"), tr ("Dither")));
    Add (new
        cMenuEditBoolItem (tr ("Setup.Muggle$Use 48kHz mode only"),
        &m_data.Only48kHz));
    Add (new
        cMenuEditIntItem (tr ("Setup.Muggle$Display mode"),
        &m_data.DisplayMode, 1, 3));
    Add (new
        cMenuEditBoolItem (tr ("Setup.Muggle$Background mode"),
        &m_data.BackgrMode, tr ("Black"), tr ("Live")));
    Add (new
        cMenuEditIntItem (tr ("Setup.Muggle$Normalizer level"),
        &m_data.TargetLevel, 0, MAX_TARGET_LEVEL));
    Add (new
        cMenuEditIntItem (tr ("Setup.Muggle$Limiter level"),
        &m_data.LimiterLevel, MIN_LIMITER_LEVEL, 100));
}


void
mgMenuSetup::Store (void)
{
    the_setup = m_data;

    SetupStore ("InitLoopMode", the_setup.InitLoopMode);
    SetupStore ("InitShuffleMode", the_setup.InitShuffleMode);
    SetupStore ("AudioMode", the_setup.AudioMode);
    SetupStore ("DisplayMode", the_setup.DisplayMode);
    SetupStore ("BackgrMode", the_setup.BackgrMode);
    SetupStore ("TargetLevel", the_setup.TargetLevel);
    SetupStore ("LimiterLevel", the_setup.LimiterLevel);
    SetupStore ("Only48kHz", the_setup.Only48kHz);
}


// --- mgSetup ---------------------------------------------------------------

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
    ToplevelDir = "/mnt/music/";
}

std::string
mgSetup::getFilename( std::string basename )
{
  if( GdCompatibility )
    {
      return GdFindFile( basename, std::string( ToplevelDir ) );
    }
  else
    {
      return std::string( ToplevelDir ) + basename;
    }
}

#define FINDCMD      "cd '%s' && find -follow -name '%s' 2> /dev/null"

std::string GdFindFile( std::string mp3file, std::string tld )
{
  std::string fullname = "";
  char *cmd = NULL;
  asprintf( &cmd, FINDCMD, tld.c_str(), mp3file.c_str() );
  FILE *p = popen( cmd, "r" );
  if (p) 
    {
      char *s;
      while( (s = readline(p) ) != NULL) 
	{
	  // printf( "Found: %s", s );
	  fullname = std::string( tld ) + std::string( s );
	}
      pclose(p);
    }

  free( cmd );

  return fullname;
}
