//								-*- c++ -*-

#ifndef __COMMANDS_H
#define __COMMANDS_H

#include <string>
#include <vdr/osdbase.h>

#include "config.h"

#include "vdr_menu.h"

//class cMP3Player;
class cFileSources;
class cFileSource;
class cFileObj;
class cFileObjItem;

class mgPlayOsd : public mgOsd
{
	public:
		void SaveState() {}
};

class mgPlayerCommands : public mgMenu
{
	private:
		cCommands commands;
		eOSState Execute(void);
	protected:
		void BuildOsd();
		string Title(void) const;
	public:
		mgPlayerCommands(void);
		virtual ~mgPlayerCommands();
		void LoadCommands();
		bool DeleteList(void);
};
#endif							 // __COMMANDS_H
