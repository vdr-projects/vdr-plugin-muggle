/*! 
 * \file   vdr_menu.h
 * \brief  Implements menu handling for broswing media libraries within VDR
 *
 * \version $Revision: 1.13 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 *  $Id$
 */

#ifndef _VDR_MENU_H
#define _VDR_MENU_H

#include <list>
#include <vector>

#include <osd.h>

#include "i18n.h"

class cCommands;

class mgMedia;
class mgSelectionTreeNode;
class mgPlaylist;
class mgTracklist;

/*!
 * \brief a special menu item
 */
class mgMenuTreeItem : public cOsdItem
{
 public:

  mgMenuTreeItem( mgSelectionTreeNode *node );

  mgSelectionTreeNode *Node();

  void Set();

 private: 

  mgSelectionTreeNode *m_node;

};

/*!
 * \brief the muggle main OSD
 */
class mgMainMenu : public cOsdMenu
{
 public:
  
  mgMainMenu(mgMedia *media, mgSelectionTreeNode *root, 
	     mgPlaylist *playlist, cCommands *playlist_commands );

  mgSelectionTreeNode *CurrentNode();
  mgMenuTreeItem *CurrentItem();
  
  eOSState ProcessKey(eKeys Key);

 protected:

  enum MuggleStatus
    { 
      TREE, TREE_SUBMENU,
      PLAYLIST, LOAD_PLAYLIST, SAVE_PLAYLIST,
      PLAYLIST_SUBMENU, PLAYLIST_COMMANDS,
      FILTER, FILTER_SUBMENU
    };

  void SetButtons();
  void Move( int from, int to );

  // Tree view handling
  void DisplayTree( mgSelectionTreeNode *node, int select = 0 );
  void DisplayTreeViewSelector();

  void DisplayTreeSubmenu();
  eOSState TreeSubmenuAction( int n );

  // Playlist view handling
  void DisplayPlaylist( int index_current = -1 );
  void DisplayTrackInfo();
  void DisplayAlbumInfo();  

  void LoadPlaylist();
  void SavePlaylist();
  void RenamePlaylist();
  void DisplayPlaylistSubmenu();
  eOSState PlaylistSubmenuAction( int n );
  void DisplayPlaylistCommands();
  eOSState ExecutePlaylistCommand( int current );

  // Filter view handling
  void DisplayFilter();
  void DisplayFilterSelector();


 private:
  void Play(mgPlaylist *plist);

  // content stuff
  mgMedia *m_media;
  mgSelectionTreeNode *m_root;
  mgSelectionTreeNode *m_node;
  mgPlaylist          *m_current_playlist;
  std::vector<std::string> *m_plists;

  MuggleStatus m_state;
  std::list<int> m_history;

  cCommands *m_playlist_commands;

  int      m_last_osd_index;
};

#endif

/************************************************************
 *
 * $Log: vdr_menu.h,v $
 * Revision 1.13  2004/07/25 21:33:35  lvw
 * Removed bugs in finding track files and playlist indexing.
 *
 * Revision 1.12  2004/07/09 12:22:00  LarsAC
 * Untested extensions for exporting plalists
 *
 * Revision 1.11  2004/07/06 00:20:51  MountainMan
 * loading and saving playlists
 *
 * Revision 1.10  2004/05/28 15:29:19  lvw
 * Merged player branch back on HEAD branch.
 *
 * Revision 1.9  2004/02/23 15:41:21  RaK
 * - first i18n attempt
 *
 * Revision 1.8.2.7  2004/05/27 07:58:38  lvw
 * Removed bugs in moving and removing tracks from playlists
 *
 * Revision 1.8.2.6  2004/05/26 14:31:04  lvw
 * Added submenu for playlist view
 *
 * Revision 1.8.2.5  2004/05/25 21:58:54  lvw
 * Handle submenus for views
 *
 * Revision 1.8.2.4  2004/03/14 12:30:56  lvw
 * Menu now calls player
 *
 * Revision 1.8.2.3  2004/03/11 07:22:32  lvw
 * Added setup menu
 *
 * Revision 1.8.2.2  2004/03/08 22:28:40  lvw
 * Added documentation headers.
 *
 * Revision 1.8.2.1  2004/03/02 07:07:27  lvw
 * Initial adaptations from MP3 plugin added (untested)
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
 */
