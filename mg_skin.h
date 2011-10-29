/*								-*- c++ -*-
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

#ifndef ___SKIN_H
#define ___SKIN_H

#include <string>

class cmgSkin
{
	private:
		//  char *localbackground, *streambackground;
	public:
		cmgSkin(void);
		virtual ~cmgSkin();
		int ParseSkin(const char *SkinName, bool ReloadMpeg);
		int StoreSkin(const char *ThemeName);
		int clrTopBG1;
		int clrTopTextFG1;
		int clrTopBG2;
		int clrTopTextFG2;
		int clrTopItemBG1;
		int clrTopItemInactiveFG;
		int clrTopItemActiveFG;
		int clrListBG1;
		int clrListBG2;
		int clrListTextFG;
		int clrListTextActiveFG;
		int clrListTextActiveBG;
		int clrListRating;
		int clrInfoBG1;
		int clrInfoBG2;
		int clrInfoTextFG1;
		int clrInfoTitleFG1;
		int clrInfoTextFG2;
		int clrProgressBG1;
		int clrProgressBG2;
		int clrProgressbarFG;
		int clrProgressbarBG;
		int clrStatusBG;
		int clrStatusRed;
		int clrStatusGreen;
		int clrStatusYellow;
		int clrStatusBlue;
		int clrStatusTextFG;
		int clrMesgBG1;
		int clrMesgBG2;
		int clrMesgFG1;
		int rows;
		int mpgdif;
		std::string localbackground;
		std::string streambackground;
		std::string localcover;
		std::string streamcover;
		bool reloadmpeg;
};

extern cmgSkin mgSkin;
#endif							 //___SKIN_H
