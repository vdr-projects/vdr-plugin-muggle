/*
 * osd.h: Abstract On Screen Display layer
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sh_console_osd.h,v 1.1 2004/02/01 18:22:53 LarsAC Exp $
 */
#ifndef __OSD_H
#define __OSD_H
#define DEBUG_OSD
#if defined(DEBUG_OSD)
#include <ncurses.h>
#include <stdlib.h>
#include <vector.h>
#endif
//#include "config.h"
// #include "interface.h"
//#include "osdbase.h"
//#include "tools.h"
#define MaxCols 5
#define  Setup_OSDheight 12
#define  Setup_OSDwidth 40
#define  Setup_Symbol false
#define  Setup_MenuScrollPage true
#define MAXOSDITEMS (Setup.OSDheight - 4)
enum eDvbColor {
  clrBackground,
  clrTransparent = clrBackground,
  clrBlack = clrBackground,
  clrRed,
  clrGreen,
  clrYellow,
  clrBlue,
  clrMagenta,
  clrCyan,
  clrWhite,
  };

extern int bgbackground, bgchannelname, bgepgtime, bgtitleline, bgscrolline,
           bghelpred, bghelpgreen, bghelpyellow, bghelpblue;

#define clrBackground    ((eDvbColor)bgbackground)
#define clrChannelName   ((eDvbColor)bgchannelname)
#define clrEpgTime       ((eDvbColor)bgepgtime)
#define clrTitleLine     ((eDvbColor)bgtitleline)
#define clrScrolLine     ((eDvbColor)bgscrolline)
#define clrHelpRed       ((eDvbColor)bghelpred)
#define clrHelpGreen     ((eDvbColor)bghelpgreen)
#define clrHelpYellow    ((eDvbColor)bghelpyellow)
#define clrHelpBlue      ((eDvbColor)bghelpblue)

#define DISPLAY_SIZE 12
#define  PAGE_JUMP 11

extern int fgchannelname, fgdatetime, fgepgtime, fgtitle, fgsubtitle, fgtitleline, fgscrolline, 
         fgmenufont, volumebar, timebar1, timebar2, fgsymbolon, fgsymboloff;

enum eOSState { osUnknown,
                osContinue,
                osSchedule,
                osChannels,
                osDirector,
                osTimers,
                osRecordings,
                osPlugin,
                osSetup,
                osCommands,
                osPause,
                osRecord,
                osReplay,
                osStopRecord,
                osStopReplay,
                osCancelEdit,
                osSwitchDvb,
                osBack,
                osEnd,
                os_User, // the following values can be used locally
                osUser1,
                osUser2,
                osUser3,
                osUser4,
                osUser5,
                osUser6,
                osUser7,
                osUser8,
                osUser9,
                osUser10,
                osUser11,
              };
enum eDvbFont {
  fontOsd,
  fontOsd2,
  fontFix,
  fontSym,
/* TODO as soon as we have the font files...
  fontTtxSmall,
  fontTtxLarge,
*/
  };
enum eKeys { // "Up" and "Down" must be the first two keys!
             kUp,
             kDown,
             kMenu,
             kOk,
             kBack,
             kLeft,
             kRight,
             kRed,
             kGreen,
             kYellow,
             kBlue,
             k0, k1, k2, k3, k4, k5, k6, k7, k8, k9,
             kPlay,
             kPause,
             kStop,
             kRecord,
             kFastFwd,
             kFastRew,
             kPower,
             kChanUp,
             kChanDn,
             kVolUp,
             kVolDn,
             kMute,
             kSchedule,
             kChannels,
             kTimers,
             kRecordings,
             kDirector,
             kSetup,
             kCommands,
             kUser1, kUser2, kUser3, kUser4, kUser5, kUser6, kUser7, kUser8, kUser9,
             kNone,
             kKbd,
             // The following codes are used internally:
             k_Plugin,
             k_Setup,
             // The following flags are OR'd with the above codes:
             k_Repeat  = 0x8000,
             k_Release = 0x4000,
             k_Flags   = k_Repeat | k_Release,
           };

class cOsdItem {
private:
  char *text;
 public:
  cOsdItem(eOSState State = osUnknown);
  cOsdItem(const char *Text, eOSState State = osUnknown);
  void SetText(const char *Text, bool Copy);
  virtual ~cOsdItem();
  int Index();  
  const char* Get();
  void Display();
};

class cOsdMenu {
private:
  char *title;
  int cols[MaxCols];
  int m_first, m_current, m_marked;
  cOsdMenu *subMenu;
  const char *helpRed, *helpGreen, *helpYellow, *helpBlue;
  char *status;
  int digit;
  bool hasHotkeys;
  vector<cOsdItem*> m_display;
protected:
  bool visible;
  void CursorUp(void);
  void CursorDown(void);
  void PageUp(void);
  void PageDown(void);
  int Index(cOsdItem *Item);
public:
  cOsdMenu(const char *Title, int c0 = 0, int c1 = 0, int c2 = 0, int c3 = 0, int c4 = 0);
  virtual ~cOsdMenu();
  void SetTitle(const char *Title, bool ShowDate=false);
  int Current(void) { return m_current; }
  void Add(cOsdItem *Item);
//  void Ins(cOsdItem *Item, bool Current = false, cOsdItem *Before = NULL);
  virtual void Display(void);
  virtual void SetCurrent(cOsdItem *Item);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Clear(void);
  virtual void SetHasHotkeys(){ hasHotkeys = true; }
  virtual void SetHelp(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL);
};

#endif //__OSD_H
