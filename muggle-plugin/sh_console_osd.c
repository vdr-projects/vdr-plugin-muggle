/*
 * osd.c: Abstract On Screen Display layer
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sh_console_osd.c,v 1.1 2004/02/01 18:22:53 LarsAC Exp $
 */

#include "myosd.h"
#include <string.h>
//#include "device.h"
// #include "i18n.h"
//#include "status.h"

// --- cOsdItem --------------------------------------------------------------

cOsdItem::cOsdItem(eOSState State)
{
  text = NULL;
}

cOsdItem::cOsdItem(const char *Text, eOSState State)
{
  text = NULL;
  SetText(Text,true);
}

cOsdItem::~cOsdItem()
{
  free(text);
}

void cOsdItem::SetText(const char *Text, bool Copy)
{
  free(text);
  text = Copy ? strdup(Text) : (char *)Text; // text assumes ownership!
}

const char* cOsdItem::Get()
{
    return text;
}
void cOsdItem::Display()
{
    printf("%s\n", text);
}

// --- cOsdMenu --------------------------------------------------------------

cOsdMenu::cOsdMenu(const char *Title, int c0, int c1, int c2, int c3, int c4)
{
  cols[0] = c0;
  cols[1] = c1;
  cols[2] = c2;
  cols[3] = c3;
  cols[4] = c4;
  m_first = 0;
  m_current = m_marked = -1;
  subMenu = NULL;
  helpRed = helpGreen = helpYellow = helpBlue = NULL;
  status = NULL;
}

cOsdMenu::~cOsdMenu()
{
  free(title);
  delete subMenu;
  free(status);
}


void cOsdMenu::SetTitle(const char *Title, bool ShowDate)
{
     title = strdup(Title);
}

void cOsdMenu::Add(cOsdItem *Item)
{
    m_display.push_back(Item);
}

void cOsdMenu::Display(void)
{
  //Interface->Clear();
  printf("\n\n\n\n");
  printf("----------(start %d, current %d)--------------------\n",
	 m_first, m_current);
  //Interface->SetCols(cols);
  //Interface->Title(title);
  printf(" Title: %s\n", title);
  //Interface->Help(helpRed, helpGreen, helpYellow, helpBlue);
  for(int i= m_first; i < (int) m_display.size() && i < m_first+DISPLAY_SIZE; i++)
  {
      if(i == m_current)
      {
	  printf("==>");
	  
      }
      else
      {
	  printf("   ");
	  
      }
      printf("%s\n", m_display[i]->Get());
  }
  printf("----------------------------------------------\n");
  if(hasHotkeys)
  {
      printf("%15s (r) %15s (g) %15s (y) %15s (b)\n", 
	     helpRed? helpRed :"" ,
	     helpGreen? helpGreen :"" ,
	     helpYellow? helpYellow :"" ,
	     helpBlue? helpBlue :"" );
	     
  }
}

int cOsdMenu::Index(cOsdItem *Item)
{
    int pos=0;
    for(vector<cOsdItem*>::iterator iter = m_display.begin();
      iter != m_display.end(); iter++)
  {
      if((*iter) == Item)
      {
	  break;
      }
      pos++;
  }
  return pos;
}
void cOsdMenu::SetCurrent(cOsdItem *Item)
{
  m_current = Item ? Index(Item) : -1;
}
void cOsdMenu::CursorUp(void)
{
    if(m_current >0) m_current--;
    if(m_current < m_first) m_first = m_first - PAGE_JUMP;
    Display();
}
void cOsdMenu::CursorDown(void)
{
    if(m_current < (int)m_display.size()-1)
	m_current++;
    if(m_current >= m_first+DISPLAY_SIZE) m_first =m_first  + PAGE_JUMP;
    Display();
}

void cOsdMenu::PageUp(void)
{
    if(m_first >= PAGE_JUMP)
    {
	m_first -= PAGE_JUMP;
	m_current -= PAGE_JUMP;
    }
    Display();

}
void cOsdMenu::PageDown(void)
{
    if(m_first+ PAGE_JUMP< (int)m_display.size())
    {
	m_first += PAGE_JUMP;
	m_current += PAGE_JUMP;
	if(m_current <	m_first) m_current = m_first;
	if(m_current >= (int) m_display.size())
	    m_current = (int) m_display.size();
    }
    Display();
}

void cOsdMenu::Clear(void)
{
  m_first = 0;
  m_current = m_marked = -1;
  hasHotkeys = false;
  for(vector<cOsdItem*>::iterator iter = m_display.begin();
      iter != m_display.end(); iter++)
  {
      delete *iter;
  }
  m_display.clear();
}

eOSState cOsdMenu::ProcessKey(eKeys Key)
{

  switch (Key) {
    case kUp|k_Repeat:
    case kUp:   CursorUp();   break;
    case kDown|k_Repeat:
    case kDown: CursorDown(); break;
    case kLeft|k_Repeat:
    case kLeft: PageUp(); break;
    case kRight|k_Repeat:
    case kRight: PageDown(); break;
    case kBack: return osBack;
    default: if (m_marked < 0)
                return osUnknown;
    }
  return osContinue;
}

void cOsdMenu::SetHelp(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
    helpRed = Red;
    helpGreen = Green;
    helpYellow = Yellow;
    helpBlue = Blue;
}
