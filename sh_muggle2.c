#include <stdio.h>
#include <stdlib.h>
#include "mysql/mysql.h"

#include "myosd.h"
#include "muggle.h"
#include "mg_media.h"
#include "mg_tools.h"
#include <unistd.h>

eKeys waitForKey()
{
    char buf[2];
    read(0, buf, 2);
    mgDebug(9, "Key '%c' read\n",buf[0]); 
    switch(buf[0])
    {
      case '8':
	return kUp;
      case '2':
	return kDown;
      case '1':
	return  kMenu;
      case '5':
	return kOk;
      case '3':
	return kBack;
      case '4':
	return kLeft;
      case '6':
	return kRight;
      case 'r':
	return kRed;
      case 'g':
	return kGreen;
      case 'y':
	return kYellow;
      case 'b':
	return kBlue;
       default:
	return kNone;

    }
}
int main (int argc, char **argv)
{

    cOsdObject *mainMenu;
    eKeys key;
    eOSState state;
    bool loop=true;
    mgMuggle muggle;
    muggle.Initialize();
    muggle.Start();
    mgSetDebugLevel(8);

    mainMenu = muggle.MainMenuAction();

    while(loop)
    {
	key = waitForKey();
	state = mainMenu->ProcessKey(key);
	if(state == osEnd)
	    loop = false;
    }
}
