/*******************************************************************/
/*! \file   vdr_menu.c
 *  \brief  Implements menu handling for broswing media libraries within VDR
 ******************************************************************** 
 * \version $Revision: 1.8 $
 * \date    $Date: 2004/02/03 19:28:46 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 *
 * $Id: vdr_menu.c,v 1.8 2004/02/03 19:28:46 LarsAC Exp $
 */
/*******************************************************************/

#include <menuitems.h>
#include <tools.h>
#include <mysql/mysql.h>

#include "vdr_menu.h"

#include "mg_content_interface.h"
#include "mg_tools.h"
#include "mg_media.h"

#include "gd_content_interface.h"

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
  
  SetTitle( "Muggle Media Database" );
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
      SetHelp( "Add", "Filter", "Playlist", "Change View" );
    }
  else if( m_state == PLAYLIST )
    {
      SetHelp( "Tracks", "Filter", "Edit PL", "Tree" );
    }
  else if( m_state == PLAYLIST_TRACKINFO )
    {
      SetHelp( "Play/Pause", "Album Info", "Filter", "Tree" );
    }
  else if( m_state == PLAYLIST_ALBUMINFO )
    {
      SetHelp( "Play/Pause", "Filter", "Playlist", "Tree" );
    }
  else if( m_state == FILTER )
    {
      SetHelp( "Add", "Playlist", "Filter", "Tree" );
    }
  else if( m_state == TRACKS )
    {
      SetHelp( "Add", "Filter", "Playlist", "Tree" );
    }
}

eOSState mgMainMenu::ProcessKey(eKeys key)
{
  mgDebug( 1,  "mgMainMenu::ProcessKey" );
  eOSState state = cOsdMenu::ProcessKey(key);
  
  if( m_state == PLAYLIST )
    {
      mgDebug( 1,  "mgMainMenu: in state PLAYLIST" );
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case kOk:
	      {
		mgDebug( 1,  "mgMainMenu: Mark" );
		Mark(); // Mark (to move), moving done by VDR, calls Move
		state = osContinue;
	      } break;
	    case kRed:
	      {
		mgDebug( 1,  "mgMainMenu: " );
		// Shuffle?
	      }
	    case kYellow:
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
	    case kBlue: 
	      {
		mgDebug( 1,  "mgMainMenu: switch to TreeView" );
		DisplayTree( m_root );
		state = osContinue;
	      } break;
	    case kGreen:
	      {
		mgDebug( 1,  "mgMainMenu: switch to Filter" );
		DisplayFilter();
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
	    } break;
	  case kRed: // ???
	  case kYellow:
	    {
	      // Yellow always goes to playlist view
	      mgDebug( 1,  "mgMainMenu: switch to playlist" );
	      DisplayPlaylist();
	      state = osContinue;	    
	    } break;
	  case kGreen:
	    {
	      mgDebug( 1,  "mgMainMenu: switch to filter" );
	      // Green: select other filters
	      DisplayFilterSelector();			  
	    } break;
	  case kBlue:
	    {
	      // Blue: treeview
	      mgDebug( 1,  "mgMainMenu: switch to treeview" );
	      DisplayTree( m_root );
	      state = osContinue;			  
	    }
	  default:
	    {
	      state = osContinue;
	    }
	  }
      }
    else if( state == osBack )
      {
	// m_media->resetFilters();	
      }
  }
  else if( m_state == TREE )
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
		mgDebug( 1,  "mgMainMenu: switch to filter" );

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
	    case kYellow:
	      {
		mgDebug( 1,  "mgMainMenu: display playlist" );
		// Yellow always goes to playlist view
		DisplayPlaylist();
		state = osContinue;	    
	      } break;
	    case kGreen:
	      {
		mgDebug( 1,  "mgMainMenu: display filter" );
		DisplayFilter();
		state = osContinue;	    
	      } break;
	    case kBlue:
	      {
		mgDebug( 1,  "mgMainMenu: select other tree view" );
		// Select other views -> Select other Media
		DisplayTreeViewSelector();
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
	    }
	  state = osContinue;
	}
    }
  else if( m_state == TRACKS )
    {
      // Navigate with up/dn, left/right (pgup, pgdn)
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case kOk:
	      {
		// Show Song Info
		state = osContinue;
	      } break;
	    case kRed:
	      {
		// Add item to Playlist
		if(Current() >= 0 && Current() < (int) m_tracklist->getNumItems())
		  {
		    mgContentItem* item = m_tracklist->getItem( Current() );
		    mgDebug(3, "Ading item %s to playlist",
			    item->getTitle().c_str() );
		    
		    m_current_playlist->append( item );
		  }
		state = osContinue;
	      } break;
	    case kGreen:
	      {
		// Green always goes to the filter view
		DisplayFilter();
		state = osContinue;	    
	      } break;
	    case kYellow:
	      {
		// Yellow always goes to playlist view
		DisplayPlaylist();
		state = osContinue;	    
	      } break;
	    case kBlue:
	      {
		DisplayTree( m_root );
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
	  // where to go on back?
	  state = osContinue;
	}
    }
  else
    {
      mgDebug(1, "Process key: else");
    }
  
  return state;
}

void mgMainMenu::Move( int from, int to )
{
  // check current view, perform move in the content view
  if( m_state == PLAYLIST )
    {
      // resort
    }
}

void mgMainMenu::DisplayTracklist()
{
    m_state = TRACKS;
    mgDebug( 1,  "mgBrowseMenu::DisplayTracklist");

    Clear();
    SetButtons();

    mgDebug( 1, "mgBrowseMenu::DisplayTracklist: %d elements received", 
	     m_tracklist->getNumItems() );
    
    static char titlestr[80];
    sprintf( titlestr, "Muggle Tracklist (%d items)", m_tracklist->getNumItems()  );
    SetTitle( titlestr ); 

    // create tracklist with the current filters
    if( m_tracklist ) 
      {
	delete m_tracklist;
      }
    
    m_tracklist = m_media->getTracks();

    for( unsigned int i = 0; i < m_tracklist->getNumItems(); i++)
      {
	string label =  m_tracklist->getLabel( i, "   " );
	Add( new cOsdItem( label.c_str() ) );
      }
    
    Display();
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
  sprintf( titlestr, "Muggle Playlist (%d items)", list->size() );
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
  // show info of the currently playing track
}

void mgMainMenu::DisplayAlbumInfo()
{
  m_state = PLAYLIST_ALBUMINFO;
  // show info of the currently playing track
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

      mgDebug( 1, "Setting current to #%d", select );

      cOsdItem *item = Get( select );
      SetCurrent( item );
      
      RefreshCurrent();	      
      DisplayCurrent(true);
      
      // Interface->Flush();
      
      mgDebug( 1,  "mgBrowseMenu::DisplayNode: Children added to OSD" );
    }
  Display();
}

void mgMainMenu::DisplayTreeViewSelector()
{
}

void mgMainMenu::DisplayFilter()
{
  m_state = FILTER;
  Clear();
  
  mgDebug( 1, "Creating Muggle filter view" );
  SetButtons();

  SetTitle( "Muggle Filter View" );

  vector<mgFilter*> *filter_list = m_media->getTrackFilters();
  
  for( vector<mgFilter*>::iterator iter = filter_list->begin();
       iter != filter_list->end();
       iter ++ )
    {
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
	default:
	case mgFilter::UNDEF:
	  {
	  } break;
	}
    }

  Display();
}

void mgMainMenu::DisplayFilterSelector()
{

}

/************************************************************
 *
 * $Log: vdr_menu.c,v $
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
