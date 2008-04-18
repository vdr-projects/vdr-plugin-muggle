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
#include <cstring>
#include <string>
#include <getopt.h>
#include <vdr/plugin.h>

mgSetup the_setup;

mgSetup::mgSetup () {
	m_mugglei=false;
	InitLoopMode = 0;
	InitShuffleMode = 0;
	AudioMode = 1;
	DisplayMode = 3;
	BackgrMode = backgrCoverSmall;
	TargetLevel = DEFAULT_TARGET_LEVEL;
	LimiterLevel = DEFAULT_LIMITER_LEVEL;
	Only48kHz = 0;

	DbHost = 0;
	DbSocket = 0;
	DbPort = 0;
	DbName = strdup ("GiantDisc");
	DbUser = 0;
	DbPass = 0;
	msprintf(&DbDatadir,"%s/.muggle",getenv("HOME"));
	ToplevelDir = strdup(MUSICDIR "/");
	CreateMode = false;
	utf8 = false;
	const char *lang = getenv("LANG");
	if (lang) {
		const char *dot = strchr(lang, '.');
		if (dot)
			utf8 = strcmp(dot+1,"UTF-8")==0;
	}
	if (utf8)
		mgWarning("muggle running in UTF-8 mode");

	DeleteStaleReferences = false;

	//Player:
	ArtistFirst = 0;
#ifdef USE_BITMAP
	ImgAlpha = 255;
#endif
	Jumptime = 30;

	// stuff related to cover image display
	msprintf(&CacheDir,"%s/.muggle/cache",getenv("HOME"));
	UseDeviceStillPicture = 1;
	ImageShowDuration = 10;
}

bool
mgSetup::IsMugglei() const
{
	return m_mugglei;
}

void
mgSetup::SetMugglei() {
	m_mugglei = true;
}

mgSetup::~mgSetup () {
	free(DbHost);
	free(DbSocket);
	free(DbName);
	free(DbUser);
	free(DbPass);
	free(DbDatadir);
	free(ToplevelDir);
	free(CacheDir);
}

bool
mgSetup::NoHost() const
{
	return !DbHost || strlen(DbHost)==0;
}

bool mgSetup::ProcessArguments (int argc, char *argv[]) {
	mgSetDebugLevel (1);
	char ArgsMessage[1000];
	sprintf(ArgsMessage,"mgSetup::ProcessArgs ");
	for (int i=1;i<argc;i++) {
		if (strlen(ArgsMessage)+strlen(argv[i]+2)>1000) break;;
		strcat(ArgsMessage,"  ");
		strcat(ArgsMessage,argv[i]);
	}

	struct option long_options[50];
	char short_options[100];
	memset(short_options,0,sizeof(short_options));
	memset(long_options,0,sizeof(long_options));
	static struct option all_options[] = {
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
	strcpy(wanted_opts,"ntv");
	if (IsMugglei())
		strcat(wanted_opts,"cz");
	mgDb* db = GenerateDB();
	strcat(wanted_opts,db->Options());
	delete db;
	char *p = wanted_opts;
	char *s = short_options;
	int li = 0;
	while (*p) {
		for (unsigned int idx = 0; all_options[idx].name;idx++) {
			if (*p==all_options[idx].val) {
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
	while ((c = getopt_long(argc, argv, short_options,long_options,&option_index)) != -1) {
		switch (c) {
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
			case '?':
				return false;
		}
	}
// we want to get the absolute path with symlinks resolved
	char prev[1000];
	if (!getcwd(prev,1000))
		mgError("current path too long");
	if (chdir(the_setup.ToplevelDir))
		mgError("cannnot change to directory %s",the_setup.ToplevelDir);
	char ndir[1000];
	if (!getcwd(ndir,1000))
		mgError("new path too long");
	free(ToplevelDir);
	msprintf(&the_setup.ToplevelDir,"%s/",ndir);
	chdir(prev);
	mgDebug(1,ArgsMessage);
	return true;
}

const char*
mgSetup::HelpText() {
	static char buf[2000];
	strcpy(buf,
		"  -n NNNN,  --name=NNNN     specify database name (default is GiantDisc)\n"
		"  -t TTTT,  --toplevel=TTTT specify toplevel directory for music (default is " MUSICDIR ")\n"
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
