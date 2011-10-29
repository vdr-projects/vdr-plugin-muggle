//								-*- c++ -*-

#ifndef __LYRICS_H
#define __LYRICS_H

#include "mg_menu.h"

enum LyricsState {
	lyricsNone,
	lyricsLoading,
	lyricsAsking,
	lyricsSaved
};

class mgLyrics : public mgMenu
{
	private:
		int RunCommand(string cmd);
		void LoadExternal();
		void SaveExternal();
		void ThrowTmpAway(const mgItemGd& item);
		bool HasMoreVersions();
		const mgItemGd *displayItem;
		mgItemGd *playItem;
		LyricsState state;
	public:
		mgLyrics(void);
		void BuildOsd();
		string Title() const;
		eOSState Process(eKeys key);
};
#endif							 // __LYRICS_H
