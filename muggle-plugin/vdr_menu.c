/*******************************************************************/
/*! \file   vdr_menu.c
 *  \brief  Implements menu handling for browsing media libraries within VDR
 ******************************************************************** 
 * \version $Revision: 1.18 $
 * \date    $Date: 2004/02/23 16:18:15 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: RaK $
 *
 * $Id: vdr_menu.c,v 1.18 2004/02/23 16:18:15 RaK Exp $
 */
/*******************************************************************/

#include <menuitems.h>
#include <tools.h>
#include <mysql/mysql.h>

#include "vdr_menu.h"

#include "mg_content_interface.h"
#include "mg_tools.h"
#include "mg_media.h"
#include "mg_filters.h"

#include "gd_content_interface.h"

#include "i18n.h"

#include <string>
#include <vector>

using namespace std;

// ----------------------- mgMenuTreeItem ------------------

mgMenuTreeItem::mgMenuTreeItem( mgSelectionTreeNode *node )
  : m_node( node )
{
  Set();
}

mgSelectionTreeNode* mgMenuTreeItem::Node()
{
  return m_node;
}

void mgMenuTreeItem::Set()
{
  char *buffer = 0;
  asprintf( &buffer, m_node->getLabel().c_str() );
  SetText( buffer, false );  
}

// ----------------------- mgMenuTrackItem ------------------


// ----------------------- mgMainMenu ----------------------

mgMainMenu::mgMainMenu(mgMedia *media, mgSelectionTreeNode *root, mgPlaylist *playlist)
  : cOsdMenu( "" ), m_media(media), m_root(root), m_current_playlist(playlist)
{
  mgDebug( 1,  "Creating Muggle Main Menu" );
  
  SetTitle( tr("Muggle Media Database") );
  SetButtons();

  m_filtername = new char[32];
  strcpy( m_filtername, "none" );

  m_title = new char[32];
  strcpy( m_title, "none" );

  m_interpret = new char[32];
  strcpy( m_interpret, "none" );

  m_album = new char[32];
  strcpy( m_album, "none" );

  m_playlist = new char[32];
  strcpy( m_playlist, "none" );

  m_year_min   = 1900;
  m_year_max   = 2100;

  m_filter     = 0;

  m_tracklist = NULL;

  DisplayTree( m_root );
}

mgSelectionTreeNode *mgMainMenu::CurrentNode()
{
  mgMenuTreeItem *item = (mgMenuTreeItem *)Get( Current() );
  return item? item->Node(): 0;
}

mgMenuTreeItem *mgMainMenu::CurrentItem()
{
  mgMenuTreeItem *item = (mgMenuTreeItem *)Get( Current() );

  return item;
}

void mgMainMenu::SetButtons(  )
{
  SetHasHotkeys();

  if( m_state == TREE )
    {
      SetHelp( tr("Add"), tr("Cycle tree"), tr("Playlist"), tr("Submenu") );
    }
  else if( m_state == PLAYLIST )
    {
      SetHelp( tr("Edit PL"), tr("Track info"), tr("Filter"), tr("Submenu") );
    }
  else if( m_state == PLAYLIST_TRACKINFO )
    {
      SetHelp( tr("Edit PL?"), tr("Album info"), tr("Filter"), tr("Submenu") );
    }
  else if( m_state == PLAYLIST_ALBUMINFO )
    {
      SetHelp( tr("Edit PL?"), tr("Playlist"), tr("Filter"), tr("Submenu") );
    }
  else if( m_state == FILTER )
    {
      SetHelp( tr("Query"), tr("Other Search"), tr("Browser"), tr("Submenu") );
    }
  else
    {
      SetHelp( "t", "o", "d", "o" );
    }  
}

eOSState mgMainMenu::ProcessKey(eKeys key)
{
  mgDebug( 1,  "mgMainMenu::ProcessKey" );
  eOSState state = cOsdMenu::ProcessKey(key);

  if( m_state == TREE )
    {
      mgDebug( 1,  "mgMainMenu: in state TREE" );
      // Navigate with up/dn, left/right (pgup, pgdn)
      // Expand with OK, Collapse with Back
      if( state == osUnknown )
	  {
	  switch( key )
	    {
	    case kOk:
	      {
		mgDebug( 1,  "mgMainMenu: expand and descend" );

		m_history.push_back( Current() );
		mgDebug( 1, "Remember current node #%i", Current() );

		mgSelectionTreeNode *child = CurrentNode();		
		DisplayTree( child );

		state = osContinue;
	      } break;
	    case kRed:
	      {
		mgSelectionTreeNode *current = CurrentNode();
		if( current )
		  {
		    mgDebug( 1,  "mgMainMenu: add selection %s to playlist", current->getLabel().c_str() );
		    // Add selection to Play		
		    vector<mgContentItem*> *tracks = current->getTracks();

		    if( tracks )
		      {
			m_current_playlist->appendList(tracks);
			
			char buffer[256];
			sprintf( buffer, "%d tracks sent to current playlist", (int) tracks->size() );
			Interface->Status( buffer );
			Interface->Flush();
		      }
		    else
		      {
			mgDebug(1, "No tracks for current selection" );
		      }
		  }
		else
		  {
		    mgDebug(1, "Cannot find currently selected node!" );
		  }
		state = osContinue;
	      } break;
	    case kGreen:
	      {
		mgDebug( 1,  "mgMainMenu: cycle treeviews (todo)" );

		state = osContinue;	    
	      } break;
	    case kYellow:
	      {
		mgDebug( 1,  "mgMainMenu: cycle to playlist view" );

		DisplayPlaylist();
	      } break;
	    case kBlue:
	      {
		mgDebug( 1,  "mgMainMenu: submenu (todo)" );

		DisplayTreeSubmenu();

		state = osContinue;
	      } break;
	    default:
	      {
		state = osContinue;
	      } break;
	    }
	  }
      else if( state == osBack )
	{
	  mgSelectionTreeNode *parent = m_node->getParent();
	  
	  if( parent )
	    {
	      mgDebug( 1,  "mgMainMenu: collapse current node" );
	      
	      m_node->collapse();

	      // restore last selected entry
	      int last = m_history.back();
	      m_history.pop_back();

	      DisplayTree( parent, last );
	      
	      state = osContinue;
	    }
	  else
	    {
	      // Back pressed on root level... Go back to Main VDR menu
	      
	      // state = osBack;
	      state = osContinue;
	    }
	}      
    }
  else if( m_state == TREE_SUBMENU )
    {
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case k0 ... k9:
	      {
		int n = key - k0;
		
		TreeSubmenuAction( n );
		
		state = osContinue;
	      }
	    case kOk:
	      {
		TreeSubmenuAction( Current() );
		
		state = osContinue;
	      } break;
	    default:
	      {
		state = osContinue;
	      } break;
	    }
	}
      else if( state == osBack )
	{
	  mgDebug( 1,  "mgMainMenu: return from tree view submenu" );
	  
	  // restore last selected entry
	  int last = m_history.back();
	  
	  DisplayTree( m_node, last );
	  
	  state = osContinue;	  
	}
    }
  else if( m_state == PLAYLIST )
    {
      mgDebug( 1,  "mgMainMenu: in state PLAYLIST" );
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case kOk:
	      {
		mgDebug( 1,  "mgMainMenu: playlist ok" );
		state = osContinue;
	      } break;
	    case kRed:
	      {
		mgDebug( 1,  "mgMainMenu: edit playlist" );
		Mark(); // Mark (to move), moving done by VDR, calls Move
	      }
	    case kGreen:
	      {
		if( m_state == PLAYLIST )
		  {
		    mgDebug( 1,  "mgMainMenu: switch to TrackInfo" );
		    DisplayTrackInfo();
		  }
		else if( m_state == PLAYLIST_TRACKINFO )
		  {
		    mgDebug( 1,  "mgMainMenu: switch to AlbumInfo" );
		    DisplayAlbumInfo();
		  }
		else if( m_state == PLAYLIST_ALBUMINFO )
		  {
		    mgDebug( 1,  "mgMainMenu: switch to Playlist" );
		    DisplayPlaylist();
		  }
		state = osContinue;
	      } break;
	    case kYellow:
	      {
		mgDebug( 1,  "mgMainMenu: cycle playlist to  filter" );
		DisplayFilter();
		state = osContinue;
	      } break;
	    case kBlue: 
	      {
		// Submenu
		mgDebug( 1,  "mgMainMenu: playlist submenu (todo)" );

		state = osContinue;
	      } break;
	    default:
	      {
		mgDebug( 1,  "mgMainMenu: default" );
		state = osContinue;
	      };
	    }
	 }
    }
  else if( m_state == FILTER )
    {
      mgDebug( 1,  "mgMainMenu: in state FILTER" );
      
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case kOk:
	      {
		// OK: Create filter and selection tree and display
		mgDebug( 1,  "mgMainMenu: create and apply filter" );
		// m_media->applyFilters();
		state = osContinue;
	      } break;
	    case kRed: // 
	      {
		mgDebug( 1,  "mgMainMenu: query and display results" );

		if( m_root )
		  {
		    delete m_root;
		  }

		m_root = m_media->applyActiveFilter();
		// collapse all?
		DisplayTree( m_root );

		state = osContinue;
	      } break;
	    case kGreen:
	      {
		// cycle FILTER -> TREE
		mgDebug( 1,  "mgMainMenu: next filters " );

		m_media->nextFilterSet();
		DisplayFilter();

		state = osContinue;
	      } break;
	    case kYellow:
	      {
		// Green: treeview
		mgDebug( 1,  "mgMainMenu: switch to treeview" );

		DisplayTreeViewSelector();

		state = osContinue;			  
	      } break;
	    case kBlue:
	      {
		mgDebug( 1,  "mgMainMenu: submenu" );
	      }
	    default:
	      {
		state = osContinue;
	      }
	    }
	}
      else if( state == osBack )
	{
	  // m_media->resetFilters();?
	}
    }
  else
    {
      mgDebug(1, "Process key: else");
      mgDebug(1, "Process key: %d", (int) state);
      
    }
  
  return state;
}

void mgMainMenu::DisplayTree( mgSelectionTreeNode* node, int select )
{
  m_state = TREE;

  if( node->expand( ) )
    {
      Clear();
      
      char buffer[256];
      sprintf( buffer, "Muggle - %s", node->getLabel().c_str() );

      SetTitle( buffer ); 
      SetButtons();

      m_node = node;
      mgDebug( 1,  "mgBrowseMenu::DisplaySelection: node %s received", node->getLabel().c_str() );
      vector<mgSelectionTreeNode*> children = node->getChildren();
      
      mgDebug( 1, "mgBrowseMenu::DisplaySelection: %d elements received", children.size() );
      
      for( vector<mgSelectionTreeNode*>::iterator iter = children.begin();
	   iter != children.end();
	   iter ++ )
	{
	  Add( new mgMenuTreeItem( *iter ) );
	}

      cOsdItem *item = Get( select );
      SetCurrent( item );
      
      RefreshCurrent();	      
      DisplayCurrent(true);
    }
  Display();
}

void mgMainMenu::DisplayTreeViewSelector()
{
  m_history.clear();
  // collapse all!
  DisplayTree( m_root );
}

void mgMainMenu::DisplayTreeSubmenu()
{
  m_state = TREE_SUBMENU;

  Clear();
  
  mgDebug( 1, "Creating Muggle tree view submenu" );
  SetButtons();

  SetTitle( strcat("Muggle - ",tr("Tree View Commands") ) );

  // Add items
  Add( new cOsdItem( "1 - Test1" ) );
  Add( new cOsdItem( "9 - Test9" ) );
  Add( new cOsdItem( "0 - Test0" ) );

  Display();
}

void mgMainMenu::TreeSubmenuAction( int n )
{
  mgDebug( "mgMainMenu: TreeSubmenuAction( %d )", n );
  
  switch( n )
    {
    case 0:
      {
	// action 0
      } break;
    case 1:
      {
	// action 0
      } break;
    default:
      {
	// undefined action
      } break;
    }
}

void mgMainMenu::DisplayPlaylist()
{
  m_state = PLAYLIST;
  mgDebug( 1,  "mgBrowseMenu::DisplayPlaylist");

  // make sure we have a current playlist
  Clear();
  
  SetButtons();

  vector<mgContentItem*>* list = m_current_playlist-> getAll();
  static char titlestr[80];
  sprintf( titlestr, "Muggle - %s (%d %s)",tr("Playlist"),
                     list->size() ,
                     tr("items") );
  SetTitle( titlestr ); 
  
  mgDebug( 1, "mgBrowseMenu::DisplayPlaylist: %d elements received", 
	   list->size() );

  for( unsigned int i = 0; i < m_current_playlist->getNumItems(); i++)
    {
      string label =  m_current_playlist->getLabel( i, "   " );
      Add( new cOsdItem( label.c_str() ) );
    }
    
  Display();
}

void mgMainMenu::DisplayTrackInfo()
{
  m_state = PLAYLIST_TRACKINFO;
  SetButtons();

  // show info of the currently playing track
}

void mgMainMenu::DisplayAlbumInfo()
{
  m_state = PLAYLIST_ALBUMINFO;
  SetButtons();

  // show info of the currently playing album
}

void mgMainMenu::Move( int from, int to )
{
  // check current view, perform move in the content view
  if( m_state == PLAYLIST )
    {
      // resort
    }

  // what now?
}

void mgMainMenu::DisplayFilter()
{
  m_state = FILTER;
  Clear();
  
  mgDebug( 1, "Creating Muggle filter view" );
  SetButtons();

  SetTitle( m_media->getActiveFilterTitle().c_str() );
  mgDebug( 1, "filter view2" );

  vector<mgFilter*> *filter_list = m_media->getActiveFilters();
  
  mgDebug( 1, "filter view3" );
  int i=0;
  for( vector<mgFilter*>::iterator iter = filter_list->begin();
       iter != filter_list->end();
       iter ++ )
    {
	  mgDebug( 1, "Filter %d/%dint filter %s='%s'",
		   i, filter_list->size(),
		   (*iter)->getName(), 
		   (*iter)->getStrVal().c_str());
      switch( (*iter)->getType() )
	{
	case mgFilter::INT:
	  {
	    mgFilterInt *fi = (mgFilterInt *) (*iter);
	    
	    Add( new cMenuEditIntItem(  fi->getName(), 
					&(fi->m_intval), 
					fi->getMin(), fi->getMax() ) );
	  } break;
	case mgFilter::STRING:
	  {
	    mgFilterString *fs = (mgFilterString *) (*iter);

	    // BUG: This might be buggy as fs->getAllowedChars() may become
	    // invalid while VDR is still trying to access it
	    Add( new cMenuEditStrItem( fs->getName(), fs->m_strval,
				       fs->getMaxLength(), 
				       fs->getAllowedChars().c_str() ) );
	    
	  } break;
	case mgFilter::BOOL:
	  {
	    mgFilterBool *fb = (mgFilterBool *) (*iter);
	    Add( new cMenuEditBoolItem(  fb->getName(), &( fb->m_bval), 
					 fb->getTrueString().c_str(), 
					 fb->getFalseString().c_str() ) );
	  } break;
	case mgFilter::CHOICE:
	  {
	    mgFilterChoice *fc = (mgFilterChoice *) (*iter);
	    vector<string> choices = fc->getChoices();

	    char **choices_str = new (char *)[ choices.size() ];

	    int j = 0;
	    for( vector<string>::iterator iter = choices.begin();
		 iter != choices.end();
		 iter ++, j ++ )
	      {
		// BUG: Is this a big memory leak!? When to delete and who?
                // RaK: zweiter Iterator war "i" richtig: "j"
		// choices_str[j] = strndup( choices[i].c_str(), 128 );
		choices_str[j] = strndup( choices[j].c_str(), 128 );
	      }
	    
	    Add( new cMenuEditStraItem(  fc->getName(), &( fc->m_selval ),
					 choices.size(), choices_str ) );

	    // delete all choices_str elements!
	    // delete[] choices_str; // ???

	  } break;
	default:
	case mgFilter::UNDEF:
	  {
	  } break;
	}
      i++;
    }

  Display();
}

void mgMainMenu::DisplayFilterSelector()
{
  // show available filters, load on OK?
}

/************************************************************
 *
 * $Log: vdr_menu.c,v $
 * Revision 1.18  2004/02/23 16:18:15  RaK
 * - i18n
 *
 * Revision 1.17  2004/02/23 15:56:19  RaK
 * - i18n
 *
 * Revision 1.16  2004/02/23 15:41:21  RaK
 * - first i18n attempt
 *
 * Revision 1.15  2004/02/14 22:02:45  RaK
 * - mgFilterChoice Debuged
 *   fehlendes m_type = CHOICE in mg_filters.c
 *   falscher iterator in vdr_menu.c
 *
 * Revision 1.14  2004/02/12 09:08:48  LarsAC
 * Added handling of filter choices (untested)
 *
 * Revision 1.13  2004/02/09 19:27:52  MountainMan
 * filter set implemented
 *
 * Revision 1.12  2004/02/08 10:48:44  LarsAC
 * Made major revisions in OSD behavior
 *
 * Revision 1.11  2004/02/03 21:53:32  RaK
 * beak = break in l 212
 *
 * Revision 1.10  2004/02/03 20:24:29  LarsAC
 * Clear index history when jumping to root node in order to avoid overflow
 *
 * Revision 1.9  2004/02/03 19:34:51  LarsAC
 * Back on root level now jumps back to VDR main menu.
 *
 * Revision 1.8  2004/02/03 19:28:46  LarsAC
 * Playlist now created in plugin instead of in menu.
 *
 * Revision 1.7  2004/02/03 19:15:08  LarsAC
 * OSD selection now jumps back to parent when collapsing.
 *
 * Revision 1.6  2004/02/03 00:13:24  LarsAC
 * Improved OSD handling of collapse/back
 *
 *
 ************************************************************
 */
