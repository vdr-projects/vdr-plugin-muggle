/*! 
 * \file   vdr_menu.c
 * \brief  Implements menu handling for browsing media libraries within VDR
 *
 * \version $Revision: 1.22 $
 * \date    $Date: 2004/07/06 00:20:51 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: MountainMan $
 *
 * $Id: vdr_menu.c,v 1.22 2004/07/06 00:20:51 MountainMan Exp $
 */

#include <string>
#include <vector>

#include <mysql/mysql.h>

#include <menuitems.h>
#include <tools.h>

#include "vdr_menu.h"
#include "vdr_player.h"
#include "i18n.h"

#include "mg_content_interface.h"
#include "mg_playlist.h"
#define DEBUG
#include "mg_tools.h"
#include "mg_media.h"
#include "mg_filters.h"

#include "gd_content_interface.h"

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

// ----------------------- mgMainMenu ----------------------

mgMainMenu::mgMainMenu(mgMedia *media, mgSelectionTreeNode *root, mgPlaylist *playlist)
  : cOsdMenu( "" ), m_media(media), m_root(root), m_current_playlist(playlist)
{
  mgDebug( 1,  "Creating Muggle Main Menu" );
  
  SetTitle( tr("Muggle Media Database") );
  SetButtons();

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
  else if( m_state == TREE_SUBMENU )
    {
      SetHelp( tr("Instant Play"), tr("2"), tr("3"), tr("Mainmenu") );
    }
  else if( m_state == PLAYLIST )
    {
      SetHelp( tr("Play"), tr("Move"), tr("Filter"), tr("Submenu") );
    }
  else if( m_state == PLAYLIST_SUBMENU )
    {
      SetHelp( tr("Load"), tr("Save"), tr("Delete"), tr("Mainmenu") );
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

void mgMainMenu::Move( int from, int to )
{
  m_current_playlist->move( from, to );

  cOsdMenu::Move( from, to );
  Display();
}

eOSState mgMainMenu::ProcessKey(eKeys key)
{
  MGLOG( "mgMainMenu::ProcessKey" );
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
			
			char *buffer = 0;
			asprintf( &buffer, "%d tracks sent to current playlist", (int) tracks->size() );
			m_current_playlist->appendList(tracks);
			Interface->Status( buffer );
			Interface->Flush();

			free( buffer );
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
		m_last_osd_index = Current();
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
	      } break;
	    case kBlue:
	      {
		m_state = TREE;
		
		// restore last selected entry
		int last = m_history.back();		
		DisplayTree( m_node, last );

		state = osContinue;
	      } break;
	    case kOk:
	      {
		state = TreeSubmenuAction( Current() );
	      } break;
	    default:
	      {
		state = osContinue;
	      } break;
	    }
	}
      else if( state == osBack )
	{
	  m_state = TREE;

	  // restore last selected entry
	  int last = m_history.back();
	  DisplayTree( m_node, last );
	  
	  state = osContinue;	  
	}
    }
  else if( m_state == PLAYLIST )
    {
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case kOk:
	      {
		// show some more information?
		state = osContinue;
	      } break;
	    case kRed:
	      {
		// TODO: what happens if the user presses play and the player is already active?
		Play( m_current_playlist );
		state = osEnd;
	      } break;
	    case kGreen:
	      {
		Mark();

		state = osContinue;
	      } break;
	    case kYellow:
	      {
		DisplayFilter();
		state = osContinue;
	      } break;
	    case kBlue: 
	      {
		// Submenu
		m_last_osd_index = Current();
		DisplayPlaylistSubmenu();
		
		state = osContinue;
	      } break;
	    default:
	      {
		state = osContinue;
	      };
	    }
	 }
    }
  else if( m_state == LOAD_PLAYLIST )
    {
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case kOk:
	      {
		// load the selected playlist
		
		m_current_playlist -> clear();
		string selected = (*m_plists)[Current()];
		m_current_playlist = m_media->loadPlaylist(selected.c_str());
		// clean the list of playlist
		m_plists->clear();
		m_last_osd_index =0;
		DisplayPlaylist(0);
		state = osContinue;
	      } break;
	    default:
	      {
		state = osContinue;
	      };
	    }
	 }
    }
  else if( m_state == PLAYLIST_SUBMENU )
    {
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case k0 ... k9:
	      {
		int n = key - k0;
		PlaylistSubmenuAction( n );
		
		state = osContinue;
	      } break;
	    case kYellow:
	      {
		PlaylistSubmenuAction( 3 );
	      } break;
	    case kBlue:
	      {
		m_state = PLAYLIST;
		DisplayPlaylist( m_last_osd_index );

		state = osContinue;
	      } break;
	    case kOk:
	      {
		state = PlaylistSubmenuAction( Current() );
	      } break;
	    default:
	      {
		state = osContinue;
	      } break;
	    }
	}
      else if( state == osBack )
	{
	  m_state = PLAYLIST;
	  DisplayPlaylist( m_last_osd_index );

	  state = osContinue;	  
	}
    }
  else if( m_state == FILTER )
    {
      if( state == osUnknown )
	{
	  switch( key )
	    {
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
		state = osContinue;			  
	      } break;
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
    // RaK: Verhindert, dass die Help Buttons verschwinden, 
    //      ist aber keine sch�ne L�sung
    //SetHelp( tr("Query"), tr("Other Search"), tr("Browser"), tr("Submenu") );
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
  Add( new cOsdItem( "1 - Instant play" ) );
  Add( new cOsdItem( "9 - Test9" ) );
  Add( new cOsdItem( "0 - Test0" ) );

  Display();
}

eOSState mgMainMenu::TreeSubmenuAction( int n )
{
  mgDebug( "mgMainMenu: TreeSubmenuAction( %d )", n );
  eOSState state = osContinue;
  
  switch( n )
    {
    case 0:
      {
	// action 0: instant play of current node, might need a security question
	
	mgSelectionTreeNode *current = CurrentNode();
	if( current )
	  {
	    // clear playlist
	    m_current_playlist->clear();

	    // append current node
	    vector<mgContentItem*> *tracks = current->getTracks();

	    if( tracks )
	      {
		m_current_playlist->appendList( tracks );
		
		// play
		Play( m_current_playlist );
		
		state = osEnd;
	      }	
	  }
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

  return state;
}

void mgMainMenu::DisplayPlaylist( int index_current )
{
  m_state = PLAYLIST;

  // make sure we have a current playlist
  Clear();
  SetButtons();

  vector<mgContentItem*>* list = m_current_playlist-> getAll();
  static char titlestr[80];
  sprintf( titlestr, "Muggle - %s (%d %s)",tr("Playlist"),
                     list->size() ,
                     tr("items") );
  SetTitle( titlestr ); 
  
  for( unsigned int i = 0; i < m_current_playlist->getNumItems(); i++)
    {
      string label =  m_current_playlist->getLabel( i, "   " );
      Add( new cOsdItem( label.c_str() ) );
    }

  if( index_current >= 0 )
    {
      cOsdItem *item = Get( m_last_osd_index );
      SetCurrent( item );
      RefreshCurrent();
      DisplayCurrent( true );
    }
    
  Display();
}

void mgMainMenu::LoadPlaylist()
{
  m_state = LOAD_PLAYLIST;
  static char titlestr[80];

  // make sure we have a current playlist
  Clear();
  SetButtons();
  sprintf( titlestr, "Muggle - %s %s ",tr("load"), tr("Playlist"));
  SetTitle( titlestr ); 
  
  // retrieve list of available playlists
  m_plists = m_media->getStoredPlaylists();
  
  for(vector<string>::iterator iter = m_plists->begin(); 
      iter != m_plists->end() ; iter++)
    {
      
      Add( new cOsdItem( iter->c_str() ) );
    }

   
  Display();
}

void mgMainMenu::SavePlaylist()
{
  if(m_current_playlist->getListname() == "")
  {
    // create dummy listname with current date and time
    time_t currentTime = time(NULL);
    m_current_playlist->setListname(ctime(&currentTime));
  }
  m_current_playlist->storePlaylist();
}

void mgMainMenu::RenamePlaylist()
{
  // dummy function. USes current date as name
  time_t currentTime = time(NULL);
  m_current_playlist->setListname(ctime(&currentTime));
}

void mgMainMenu::DisplayPlaylistSubmenu()
{
  m_state = PLAYLIST_SUBMENU;

  Clear();
  SetButtons();
  SetTitle( "Muggle - Playlist View Commands" );

  // Add items
  Add( new cOsdItem( "1 - Load playlist" ) );
  Add( new cOsdItem( "2 - Save playlist" ) );
  Add( new cOsdItem( "3 - Rename playlist" ) );
  Add( new cOsdItem( "4 - Clear playlist" ) );
  Add( new cOsdItem( "5 - Remove entry from list" ) );

  Display();
}

eOSState mgMainMenu::PlaylistSubmenuAction( int n )
{
  cout << "mgMainMenu::PlaylistSubmenuAction: " << n << endl << flush;
  eOSState state = osContinue;
  
  switch( n )
    {
    case 0:
      {
	LoadPlaylist();
	Interface->Flush();
      } break;
    case 1:
      {
	
	SavePlaylist();
	Interface->Status( "Playlist saved");
	Interface->Flush();
      } break;
    case 2:
      {	// renamer playlist
	RenamePlaylist();

	// confirmation
	Interface->Status( "Playlist renamed (dummy)" );
	Interface->Flush();

	state = osContinue;	
      } break;
    case 3:
      {	// clear playlist
	m_current_playlist->clear();

	// confirmation
	Interface->Status( "Playlist cleared" );
	Interface->Flush();

	state = osContinue;	
      } break;
    case 4:
      { // remove selected title
	m_current_playlist->remove( m_last_osd_index );
	if( m_last_osd_index > 0 )
	  {
	    m_last_osd_index --;
	  }
	DisplayPlaylist( m_last_osd_index );

	// confirmation
	Interface->Status( "Entry removed" );
	Interface->Flush();	
      }
    default:
      {
	// undefined action
      } break;
    }

  return state;
}

void mgMainMenu::DisplayFilter()
{
  m_state = FILTER;

  Clear();  
  SetButtons();

  SetTitle( m_media->getActiveFilterTitle().c_str() );
  vector<mgFilter*> *filter_list = m_media->getActiveFilters();
  
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

void mgMainMenu::Play(mgPlaylist *plist)
{
  MGLOG( "mgMainMenu::Play" );
  cControl *control = cControl::Control();

  if( control && typeid(*control) == typeid(mgPlayerControl) ) 
    { // is there a running MP3 player?
      cout << "mgMainMenu::Play: signal new playlist to existing control" << endl;
      static_cast<mgPlayerControl*>(control)->NewPlaylist(plist); // signal the running player to load the new playlist
    }
  else
    {
      cout << "mgMainMenu::Play: starting new control" << endl;
      cControl::Launch( new mgPlayerControl(plist) );
    }
}

/************************************************************
 *
 * $Log: vdr_menu.c,v $
 * Revision 1.22  2004/07/06 00:20:51  MountainMan
 * loading and saving playlists
 *
 * Revision 1.21  2004/06/02 19:29:22  lvw
 * Use asprintf to create messages
 *
 * Revision 1.20  2004/05/28 15:29:19  lvw
 * Merged player branch back on HEAD branch.
 *
 * Revision 1.19  2004/02/23 17:03:24  RaK
 * - error in filter view while trying to switch or using the colour keys
 *   workaround: first filter criteria is inttype. than it works, dont ask why ;-(
 *
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
 * Revision 1.13.2.12  2004/05/27 07:58:38  lvw
 * Removed bugs in moving and removing tracks from playlists
 *
 * Revision 1.13.2.11  2004/05/26 14:31:04  lvw
 * Added submenu for playlist view
 *
 * Revision 1.13.2.10  2004/05/25 21:58:45  lvw
 * Handle submenus for views
 *
 * Revision 1.13.2.9  2004/05/25 00:10:45  lvw
 * Code cleanup and added use of real database source files
 *
 * Revision 1.13.2.8  2004/05/11 06:35:16  lvw
 * Added debugging while hunting stop bug.
 *
 * Revision 1.13.2.7  2004/05/04 16:51:53  lvw
 * Debugging aids added.
 *
 * Revision 1.13.2.6  2004/04/29 06:48:21  lvw
 * Output statements to aid debugging added
 *
 * Revision 1.13.2.5  2004/04/25 18:44:07  lvw
 * Removed bugs in menu handling
 *
 * Revision 1.13.2.4  2004/03/14 12:30:56  lvw
 * Menu now calls player
 *
 * Revision 1.13.2.3  2004/03/11 07:22:32  lvw
 * Added setup menu
 *
 * Revision 1.13.2.2  2004/03/08 22:28:40  lvw
 * Added documentation headers.
 *
 * Revision 1.13.2.1  2004/03/02 07:07:27  lvw
 * Initial adaptations from MP3 plugin added (untested)
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
