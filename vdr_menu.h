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

#include <string>
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
  mgSelectionTreeNode *m_menu_item;
  mgPlaylist          *m_current_playlist;
  std::vector< std::string > *m_plists;

  MuggleStatus m_state;
  std::list< int > m_history;

  cCommands *m_playlist_commands;

  int      m_last_osd_index;
};

#endif
