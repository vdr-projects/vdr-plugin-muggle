/*******************************************************************/
/*! \file   vdr_menu.h
 *  \brief  Implements menu handling for broswing media libraries within VDR
 ******************************************************************** 
 * \version $Revision: 1.2 $
 * \date    $Date: 2004/02/01 22:12:56 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: RaK $
 */
/*******************************************************************/

#ifndef _VDR_MENU_H
#define _VDR_MENU_H

#undef SHELL_TEST

#ifdef SHELL_TEST
  #include "myosd.h"
  #include "mymenuitems.h"
#else
  #include <vdr/osd.h>
#endif

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
  
  mgMainMenu();

  mgSelectionTreeNode *CurrentNode();
  
  eOSState ProcessKey(eKeys Key);
  void Move( int from, int to);

 protected:

  enum MuggleStatus
    { 
      TREE, FILTER, TRACKS,
      PLAYLIST, PLAYLIST_TRACKINFO, PLAYLIST_ALBUMINFO, 
    };

  // To be rewritten mode dependent 
  void SetButtons();

  void DisplayTree( mgSelectionTreeNode *node );
  void DisplayTreeViewSelector();

  void DisplayFilter();
  void DisplayFilterSelector();

  void DisplayPlaylist();
  void DisplayTracklist();
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
};

#endif
