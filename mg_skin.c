/*
 * Music plugin to VDR (C++)
 *
 * (C) 2006 Morone
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

//#include <string>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vdr/plugin.h>

#include <vdr/i18n.h>
#include "mg_skin.h"
#include "mg_setup.h"

cmgSkin mgSkin;

// --- cmgSkin ---------------------------------------------------------------

cmgSkin::cmgSkin(void) {

	// TOP: 3 colors (one is used for transparence)
	clrTopBG1              = 0xCFF2A00C;
	clrTopTextFG1          = 0xFFBABBC0;

	// BETWEEN TOP AND LIST: 4 different colors
	clrTopBG2              = 0xEF2D435A;
	clrTopTextFG2          = 0xFFBABBC0;
	clrTopItemBG1      = 0xEF2D435A;
	clrTopItemInactiveFG   = 0xDF303F52;
	clrTopItemActiveFG     = 0xCFF2A00C;
	// TRACKLIST: 4 colors
	clrListBG1             = 0xEF2D435A;
	clrListBG2             = 0xDF303F52;
	clrListTextFG          = 0xFFBABBC0;
	clrListTextActiveFG    = 0xFFF2A00C;
	clrListTextActiveBG    = 0xDF303F52;
	clrListRating          = 0xFFCC0C0C;
	// INFO:  4 colors
	clrInfoBG1             = 0xEF2D435A;
	clrInfoBG2             = 0xDF303F52;
	clrInfoTextFG1         = 0xFFBABBC0;
	clrInfoTitleFG1        = 0xFFBABBC0;
	clrInfoTextFG2         = 0xFFBABBC0;
	// PROGRESS:  4 colors
	clrProgressBG1         = 0xEF2D435A;
	clrProgressBG2         = 0xDF303F52;
	clrProgressbarFG       = 0xEFA00404;
	clrProgressbarBG       = 0xDF000000;
	// STATUS: 16 colors , but take care about overall 16 color OSD
	clrStatusBG            = 0xCFF2A00C;
	clrStatusRed           = 0xFFC00000;
	clrStatusGreen         = 0xFF00FF00;
	clrStatusYellow        = 0xFFE0E222;
	clrStatusBlue          = 0xFF3B96FD;
	clrStatusTextFG        = 0xFF000000;
	// COMMONCOLORS
	clrMesgBG1             = 0xEF2D435A;
	clrMesgBG2             = 0xDF303F52;
	clrMesgFG1             = 0xFFBABBC0;

	// FOR MPEGBACKGROUNDCOVER
	rows                   = 7;
	mpgdif                 = 0;
	localbackground        = "";
	streambackground       = "";
	localcover             = "/music-default-cover.png";
	streamcover            = "/music-default-stream.png";
	reloadmpeg             = false;
};

cmgSkin::~cmgSkin() {
}

int cmgSkin::ParseSkin(const char *SkinName, bool ReloadMpeg) {

	using namespace std;
	ifstream filestr;
	std::string line;
	std::string Value;
	std::string datei;
	std::string skinname;

	bool result=false;

	reloadmpeg = ReloadMpeg;

	skinname = SkinName;

	localbackground = "";
	streambackground = "";

	datei = the_setup.ConfigDirectory;
	datei = datei + "/themes/";
	datei = datei + skinname;

	dsyslog("music: Load themefile '%s'\n", datei.c_str());

	filestr.open (datei.c_str());
	if(filestr) {
		while (getline(filestr, line, '\n')) {
			int len = line.length();
			string::size_type pos = line.find ("<value>",0);

			if(pos != string::npos) {
				Value = line.substr(len -10, len);

				if      (strstr(line.c_str(),"clrTopBG1"))             clrTopBG1             = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrTopTextFG1"))         clrTopTextFG1         = strtoul(Value.c_str(), NULL,16);

				else if (strstr(line.c_str(),"clrTopBG2"))             clrTopBG2             = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrTopTextFG2"))         clrTopTextFG2         = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrTopItemBG1"))         clrTopItemBG1         = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrTopItemInactiveFG"))  clrTopItemInactiveFG  = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrTopItemActiveFG"))    clrTopItemActiveFG    = strtoul(Value.c_str(), NULL,16);

				else if (strstr(line.c_str(),"clrListBG1"))            clrListBG1            = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrListBG2"))            clrListBG2            = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrListTextFG"))         clrListTextFG         = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrListTextActiveFG"))   clrListTextActiveFG   = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrListTextActiveBG"))   clrListTextActiveBG   = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrListRating"))         clrListRating         = strtoul(Value.c_str(), NULL,16);

				else if (strstr(line.c_str(),"clrInfoBG1"))            clrInfoBG1            = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrInfoBG2"))            clrInfoBG2            = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrInfoTextFG1"))        clrInfoTextFG1        = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrInfoTitleFG1"))       clrInfoTitleFG1       = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrInfoTextFG2"))        clrInfoTextFG2        = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrProgressBG1"))        clrProgressBG1        = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrProgressBG2"))        clrProgressBG2        = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrProgressbarFG"))      clrProgressbarFG      = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrProgressbarBG"))      clrProgressbarBG      = strtoul(Value.c_str(), NULL,16);

				else if (strstr(line.c_str(),"clrStatusBG"))           clrStatusBG           = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrStatusRed"))          clrStatusRed          = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrStatusGreen"))        clrStatusGreen        = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrStatusYellow"))       clrStatusYellow       = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrStatusBlue"))         clrStatusBlue         = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrStatusTextFG"))       clrStatusTextFG       = strtoul(Value.c_str(), NULL,16);

				else if (strstr(line.c_str(),"clrMesgBG1"))            clrMesgBG1            = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrMesgBG2"))            clrMesgBG2            = strtoul(Value.c_str(), NULL,16);
				else if (strstr(line.c_str(),"clrMesgFG1"))            clrMesgFG1            = strtoul(Value.c_str(), NULL,16);

				else if (strstr(line.c_str(),"localcover")) {
					pos = line.rfind ("=",len);
					if(pos != string::npos) {
						Value   = line.substr(pos +1,len);
						localcover = Value.c_str();
					}
				}
				else if (strstr(line.c_str(),"streamcover")) {
					pos = line.rfind ("=",len);
					if(pos != string::npos) {
						Value   = line.substr(pos +1,len);
						streamcover = Value.c_str();
					}
				}
				else if (strstr(line.c_str(),"localbackground")) {
					pos = line.rfind ("=",len);
					if(pos != string::npos) {
						Value   = line.substr(pos +1,len);
						localbackground = Value.c_str();
					}
				}
				else if (strstr(line.c_str(),"streambackground")) {
					pos = line.rfind ("=",len);
					if(pos != string::npos) {
						Value   = line.substr(pos +1,len);
						streambackground = Value.c_str();
					}
				}
				else if (strstr(line.c_str(),"rows")) {
					pos = line.rfind ("=",len);
					if(pos != string::npos) {
						Value   = line.substr(pos +1,len);
						rows = atoi(Value.c_str());
					}
				}
				else if (strstr(line.c_str(),"mpgdif")) {
					pos = line.rfind ("=",len);
					if(pos != string::npos) {
						Value   = line.substr(pos +1,len);
						mpgdif = atoi(Value.c_str());
					}
				}
			}
		}

		filestr.close();
		result = true;
	}

	return result;
}

int cmgSkin::StoreSkin(const char *ThemeName) {
	using namespace std;
	ifstream filestr;
	std::string line;
	std::string datei;
	std::string dateiout;
	std::string themename;

	bool res=false;

	themename = ThemeName;

	datei = the_setup.ConfigDirectory + "/themes/" + ThemeName;

	dateiout = the_setup.ConfigDirectory + "/themes/current.colors";

	if( FILE *f = fopen(dateiout.c_str(), "w")) {
		filestr.open (datei.c_str());
		if(filestr) {
			while (getline(filestr, line, '\n')) {
				line = line + "\n";
				fprintf(f, line.c_str());
			}
			filestr.close();
		}
		res = true;
		fclose(f);
	}
	else
		res = false;

	return res;
}
