/*
 * menuitems.c: General purpose menu items
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sh_console_osd_menuitems.c,v 1.1 2004/02/01 18:22:53 LarsAC Exp $
 */

#include "mymenuitems.h"

const char *FileNameChars = " abcdefghijklmnopqrstuvwxyz0123456789-.#~";

// --- cMenuEditItem ---------------------------------------------------------

cMenuEditItem::cMenuEditItem(const char *Name)
{
  name = strdup(Name);
  value = NULL;
}

cMenuEditItem::~cMenuEditItem()
{
  free(name);
  free(value);
}

void cMenuEditItem::SetValue(const char *Value)
{
  free(value);
  value = strdup(Value);
  char *buffer = NULL;
  asprintf(&buffer, "%s:\t%s", name, value);
  SetText(buffer, false);
  Display();
}

// --- cMenuEditIntItem ------------------------------------------------------

cMenuEditIntItem::cMenuEditIntItem(const char *Name, int *Value, int Min, int Max)
:cMenuEditItem(Name)
{
  value = Value;
  min = Min;
  max = Max;
  Set();
}

void cMenuEditIntItem::Set(void)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", *value);
  SetValue(buf);
}

eOSState cMenuEditIntItem::ProcessKey(eKeys Key)
{
  return osContinue;
}

// --- cMenuEditBoolItem -----------------------------------------------------

cMenuEditBoolItem::cMenuEditBoolItem(const char *Name, int *Value, const char *FalseString, const char *TrueString)
:cMenuEditIntItem(Name, Value, 0, 1)
{
  falseString = FalseString ? FalseString : "no";
  trueString = TrueString ? TrueString : "yes";
  Set();
}

void cMenuEditBoolItem::Set(void)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%s", *value ? trueString : falseString);
  SetValue(buf);
}

// --- cMenuEditNumItem ------------------------------------------------------

cMenuEditNumItem::cMenuEditNumItem(const char *Name, char *Value, int Length, bool Blind)
:cMenuEditItem(Name)
{
  value = Value;
  length = Length;
  blind = Blind;
  Set();
}

void cMenuEditNumItem::Set(void)
{
  if (blind) {
     char buf[length + 1];
     int i;
     for (i = 0; i < length && value[i]; i++)
         buf[i] = '*';
     buf[i] = 0;
     SetValue(buf);
     }
  else
     SetValue(value);
}

eOSState cMenuEditNumItem::ProcessKey(eKeys Key)
{
  return osContinue;
}

// --- cMenuEditChrItem ------------------------------------------------------

cMenuEditChrItem::cMenuEditChrItem(const char *Name, char *Value, const char *Allowed)
:cMenuEditItem(Name)
{
  value = Value;
  allowed = strdup(Allowed);
  current = strchr(allowed, *Value);
  if (!current)
     current = allowed;
  Set();
}

cMenuEditChrItem::~cMenuEditChrItem()
{
  free(allowed);
}

void cMenuEditChrItem::Set(void)
{
  char buf[2];
  snprintf(buf, sizeof(buf), "%c", *value);
  SetValue(buf);
}

eOSState cMenuEditChrItem::ProcessKey(eKeys Key)
{
  return  osContinue;
}

// --- cMenuEditStrItem ------------------------------------------------------

cMenuEditStrItem::cMenuEditStrItem(const char *Name, char *Value, int Length, const char *Allowed)
:cMenuEditItem(Name)
{
  value = Value;
  length = Length;
  allowed = strdup(Allowed);
  pos = -1;
  insert = uppercase = false;
  newchar = true;
  ieCurChr = 0;
  lastKey = kNone;
  Set();
}

cMenuEditStrItem::~cMenuEditStrItem()
{
  free(allowed);
}

void cMenuEditStrItem::SetHelpKeys(void)
{
}

void cMenuEditStrItem::Set(void)
{
}

eOSState cMenuEditStrItem::ProcessKey(eKeys Key)
{
#if 0
  const char c1[] = "-.#~,/_@1";
  const char c2[] = "abcäåá2";
  const char c3[] = "defé3";
  const char c4[] = "ghi4";
  const char c5[] = "jkl5";
  const char c6[] = "mnoöñó6";
  const char c7[] = "pqrs7";
  const char c8[] = "tuvüú8";
  const char c9[] = "wxyz9";
  const char c0[] = " 0";

  switch (Key) {
    case kRed:   // Switch between upper- and lowercase characters
                 if (pos >= 0 && (!insert || !newchar)) {
                    uppercase = !uppercase;
                    value[pos] = uppercase ? toupper(value[pos]) : tolower(value[pos]);
                    }
                 break;
    case kGreen: // Toggle insert/overwrite modes
                 if (pos >= 0) {
                    insert = !insert;
                    newchar = true;
                    }
                 SetHelpKeys();
                 lastKey = Key;
                 break;
    case kYellow|k_Repeat:
    case kYellow: // Remove the character at current position; in insert mode it is the character to the right of cursor
                 if (pos >= 0) {
                    if (strlen(value) > 1) {
                       if (!insert || pos < int(strlen(value)) - 1)
                          memmove(value + pos, value + pos + 1, strlen(value) - pos);
                       // reduce position, if we removed the last character
                       if (pos == int(strlen(value)))
                          pos--;
                       }
                    else if (strlen(value) == 1)
                       value[0] = ' '; // This is the last character in the string, replace it with a blank
                    if (isalpha(value[pos]))
                       uppercase = isupper(value[pos]);
                    newchar = true;
                    }
                 lastKey = Key;
                 break;
    case kLeft|k_Repeat:
    case kLeft:  if (pos > 0) {
                    if (!insert || newchar)
                       pos--;
                    newchar = true;
                    }
                 if (!insert && isalpha(value[pos]))
                    uppercase = isupper(value[pos]);
                 lastKey = Key;
                 break;
    case kRight|k_Repeat:
    case kRight: if (pos < length - 2 && pos < int(strlen(value)) ) {
                    if (++pos >= int(strlen(value))) {
                       if (pos >= 2 && value[pos - 1] == ' ' && value[pos - 2] == ' ')
                          pos--; // allow only two blanks at the end
                       else {
                          value[pos] = ' ';
                          value[pos + 1] = 0;
                          }
                       }
                    }
                 newchar = true;
                 if (!insert && isalpha(value[pos]))
                    uppercase = isupper(value[pos]);
                 if (pos == 0)
                    SetHelpKeys();
                 lastKey = Key;
                 break;
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:  if (pos >= 0) {
                    if (insert && newchar) {
                       // create a new character in insert mode
                       if (int(strlen(value)) < length - 1) {
                          memmove(value + pos + 1, value + pos, strlen(value) - pos + 1);
                          value[pos] = ' ';
                          }
                       }
                    if (uppercase)
                       value[pos] = toupper(Inc(tolower(value[pos]), NORMALKEY(Key) == kUp));
                    else
                       value[pos] =         Inc(        value[pos],  NORMALKEY(Key) == kUp);
                    newchar = false;
                    }
                 else
                    return cMenuEditItem::ProcessKey(Key);
                 lastKey = Key;
                 break;
    case k0|k_Repeat ... k9|k_Repeat:
    case k0 ... k9: if (Key != lastKey) {
                    ieCurChr = 0;
                    if (!newchar) {
                       // kRight
                       if (pos < length - 2 && pos < int(strlen(value)) ) {
                          if (++pos >= int(strlen(value))) {
                             if (pos >= 2 && value[pos - 1] == ' ' && value[pos - 2] == ' ')
                                pos--; // allow only two blanks at the end
                             else {
                                value[pos] = ' ';
                                value[pos + 1] = 0;
                                }
                             }
                          }
                       newchar = true;
                       if (!insert && isalpha(value[pos]))
                          uppercase = isupper(value[pos]);
                       }
                    }
                 // kUp/kDown
                 if (pos >= 0) {
                    if (insert && newchar) {
                       // create a new character in insert mode
                       if (int(strlen(value)) < length - 1) {
                          memmove(value + pos + 1, value + pos, strlen(value) - pos + 1);
                          value[pos] = ' ';
                          }
                       }
                    }
                 else
                    return cMenuEditItem::ProcessKey(Key);
                 switch (Key) {
                    case k1:
                         if (uppercase)
                            value[pos] = toupper(c1[ieCurChr]);
                         else
                            value[pos] =         c1[ieCurChr];
                         if (c1[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k2:
                         if (uppercase)
                            value[pos] = toupper(c2[ieCurChr]);
                         else
                            value[pos] =         c2[ieCurChr];
                         if (c2[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k3:
                         if (uppercase)
                            value[pos] = toupper(c3[ieCurChr]);
                         else
                            value[pos] =         c3[ieCurChr];
                         if (c3[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k4:
                         if (uppercase)
                            value[pos] = toupper(c4[ieCurChr]);
                         else
                            value[pos] =         c4[ieCurChr];
                         if (c4[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k5:
                         if (uppercase)
                            value[pos] = toupper(c5[ieCurChr]);
                         else
                            value[pos] =         c5[ieCurChr];
                         if (c5[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k6:
                         if (uppercase)
                            value[pos] = toupper(c6[ieCurChr]);
                         else
                            value[pos] =         c6[ieCurChr];
                         if (c6[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k7:
                         if (uppercase)
                            value[pos] = toupper(c7[ieCurChr]);
                         else
                            value[pos] =         c7[ieCurChr];
                         if (c7[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k8:
                         if (uppercase)
                            value[pos] = toupper(c8[ieCurChr]);
                         else
                            value[pos] =         c8[ieCurChr];
                         if (c8[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k9:
                         if (uppercase)
                            value[pos] = toupper(c9[ieCurChr]);
                         else
                            value[pos] =         c9[ieCurChr];
                         if (c9[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    case k0:
                         if (uppercase)
                            value[pos] = toupper(c0[ieCurChr]);
                         else
                            value[pos] =         c0[ieCurChr];
                         if (c0[ieCurChr+1] == 0)
                            ieCurChr = 0;
                         else
                            ieCurChr++;
                         break;
                    default:
                         break;
                    }
                 lastKey = Key;
                 newchar = false;
                 break;
    case kOk:    if (pos >= 0) {
                    pos = -1;
                    lastKey = Key;
                    newchar = true;
                    stripspace(value);
                    SetHelpKeys();
                    break;
                    }
                 // run into default
    default:     if (pos >= 0 && BASICKEY(Key) == kKbd) {
                    int c = KEYKBD(Key);
                    if (c <= 0xFF) {
                       const char *p = strchr(allowed, tolower(c));
                       if (p) {
                          int l = strlen(value);
                          if (insert && l < length - 1)
                             memmove(value + pos + 1, value + pos, l - pos + 1);
                          value[pos] = c;
                          if (pos < length - 2)
                             pos++;
                          if (pos >= l) {
                             value[pos] = ' ';
                             value[pos + 1] = 0;
                             }
                          }
                       else {
                          switch (c) {
                            case 0x7F: // backspace
                                       if (pos > 0) {
                                          pos--;
                                          return ProcessKey(kYellow);
                                          }
                                       break;
                            }
                          }
                       }
                    else {
                       switch (c) {
                         case kfHome: pos = 0; break;
                         case kfEnd:  pos = strlen(value) - 1; break;
                         case kfIns:  return ProcessKey(kGreen);
                         case kfDel:  return ProcessKey(kYellow);
                         }
                       }
                    }
                 else
                    return cMenuEditItem::ProcessKey(Key);
    }
  Set();
#endif
  return osContinue;
}

// --- cMenuEditStraItem -----------------------------------------------------

cMenuEditStraItem::cMenuEditStraItem(const char *Name, int *Value, int NumStrings, const char * const *Strings)
:cMenuEditIntItem(Name, Value, 0, NumStrings - 1)
{
  strings = Strings;
  Set();
}

void cMenuEditStraItem::Set(void)
{
  SetValue(strings[*value]);
}

// --- cMenuTextItem ---------------------------------------------------------

cMenuTextItem::cMenuTextItem(const char *Text, int X, int Y, int W, int H, eDvbColor FgColor, eDvbColor BgColor, eDvbFont Font)
{

  text = strdup(Text);
}

cMenuTextItem::~cMenuTextItem()
{
  free(text);
}

void cMenuTextItem::Clear(void)
{
}

void cMenuTextItem::Display(int Offset, eDvbColor FgColor, eDvbColor BgColor)
{
}

void cMenuTextItem::ScrollUp(bool Page)
{
}

void cMenuTextItem::ScrollDown(bool Page)
{
}

eOSState cMenuTextItem::ProcessKey(eKeys Key)
{
  return osContinue;
}

// --- cMenuSetupPage --------------------------------------------------------

cMenuSetupPage::cMenuSetupPage(void)
:cOsdMenu("", 33)
{
  plugin = NULL;
}

void cMenuSetupPage::SetSection(const char *Section)
{
}

eOSState cMenuSetupPage::ProcessKey(eKeys Key)
{
  return osContinue;
}

void cMenuSetupPage::SetPlugin(cPlugin *Plugin)
{
}

void cMenuSetupPage::SetupStore(const char *Name, const char *Value)
{
}

void cMenuSetupPage::SetupStore(const char *Name, int Value)
{
}
