/*******************************************************************/
/*! \file   vdr_menu.h
 *  \brief  Implements menu handling for broswing media libraries within VDR
 ******************************************************************** 
 * \version $Revision: 1.9 $
 * \date    $Date: 2004/02/23 15:41:21 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: RaK $
 *
 * $Id: vdr_menu.h,v 1.9 2004/02/23 15:41:21 RaK Exp $
 *
 */
/*******************************************************************/

#ifndef _VDR_MENU_H
#define _VDR_MENU_H

#undef SHELL_TEST

#ifdef SHELL_TEST
  #include "myosd.h"
  #include "mymenuitems.h"
#else
  #include <osd.h>
#endif

#include <list>

#include "i18n.h"

class mgMedia;
class mgSelectionTreeNode;
class mgPlaylist;
class mgTracklist;

class mgMenuTreeItem : public cOsdItem
{
 public:

  mgMenuTreeItem( mgSelectionTreeNode *node );

  mgSelectionTreeNode *Node();

  void Set();

 private: 

  mgSelectionTreeNode *m_node;

};

class mgMainMenu : public cOsdMenu
{
 public:
  
  mgMainMenu(mgMedia *media, mgSelectionTreeNode *root, mgPlaylist *playlist);

  mgSelectionTreeNode *CurrentNode();
  mgMenuTreeItem *CurrentItem();
  
  eOSState ProcessKey(eKeys Key);
  void Move( int from, int to);

 protected:

  enum MuggleStatus
    { 
      TREE, TREE_SUBMENU,
      PLAYLIST, PLAYLIST_TRACKINFO, PLAYLIST_ALBUMINFO, 
      FILTER
    };

  void SetButtons();

  // Tree view handling
  void DisplayTree( mgSelectionTreeNode *node, int select = 0 );
  void DisplayTreeViewSelector();
  void DisplayTreeSubmenu();
  void TreeSubmenuAction( int n );

  // Filter view handling
  void DisplayFilter();
  void DisplayFilterSelector();

  // Playlist view handling
  void DisplayPlaylist();
  void DisplayTrackInfo();
  void DisplayAlbumInfo();  

 private:

  // content stuff
  mgMedia *m_media;
  mgSelectionTreeNode *m_root;
  mgSelectionTreeNode *m_node;
  mgPlaylist          *m_current_playlist;
  mgTracklist         *m_tracklist;
 
  // filter items
  char *m_title, *m_interpret, *m_album, *m_playlist, *m_filtername;
  int   m_year_min, m_year_max, m_filter;

  MuggleStatus m_state;

  std::list<int> m_history;
};

#endif

/************************************************************
 *
 * $Log: vdr_menu.h,v $
 * Revision 1.9  2004/02/23 15:41:21  RaK
 * - first i18n attempt
 *
 * Revision 1.8  2004/02/08 10:48:44  LarsAC
 * Made major revisions in OSD behavior
 *
 * Revision 1.7  2004/02/03 19:28:46  LarsAC
 * Playlist now created in plugin instead of in menu.
 *
 * Revision 1.6  2004/02/03 19:15:08  LarsAC
 * OSD selection now jumps back to parent when collapsing.
 *
 * Revision 1.5  2004/02/03 00:13:24  LarsAC
 * Improved OSD handling of collapse/back
 *
 *
 ************************************************************
 */
