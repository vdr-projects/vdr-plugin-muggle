/*! 
 * \file   vdr_menu.c
 * \brief  Implements menu handling for browsing media libraries within VDR
 *
 * \version $Revision: 1.27 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * $Id$
 */

#include <typeinfo>
#include <string>
#include <vector>

// #include <cstring>

#include <mysql/mysql.h>

#include <menuitems.h>
#include <tools.h>
#include <config.h>
#include <plugin.h>

#if VDRVERSNUM >= 10307
#include <vdr/interface.h>
#include <vdr/skins.h>
#endif

#include "muggle.h"
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

mgMainMenu::mgMainMenu(mgMedia *media, mgSelectionTreeNode *root, 
		       mgPlaylist *playlist, cCommands *playlist_commands)
  : cOsdMenu( "" ), m_media(media), m_root(root), 
     m_current_playlist(playlist), m_playlist_commands(playlist_commands)
{
  mgDebug( 1,  "Creating Muggle Main Menu" );
  
  SetTitle( tr("Muggle Media Database") );
  SetButtons();

  DisplayTree( m_root );

  strncpy( m_listname, playlist->getListname().c_str(), 31 );
  m_listname[31] = '\0';
  m_editing_listname = false;
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
		m_history.push_back( Current() );
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
		    std::vector<mgContentItem*> *tracks = current->getTracks();

		    if( tracks )
		      {
			
			char *buffer = 0;
			asprintf( &buffer, tr("%d tracks sent to current playlist"), (int) tracks->size() );
			m_current_playlist->appendList(tracks);
#if VDRVERSNUM >= 10307
			Skins.Message(mtInfo,buffer);
			Skins.Flush();
#else
			Interface->Status( buffer );
			Interface->Flush();
#endif      
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
		m_menu_item = CurrentItem()->Node();
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
	      state = osBack;

	      // state = osContinue;
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
	    case kRed:
	      {
		state = TreeSubmenuAction( 0 );
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
		// start replay at selected index
		unsigned idx = Current();
		Play( m_current_playlist, idx );
		state = osContinue;
	      } break;
	    case kRed:
	      {
		// TODO: what happens if the user presses play and the player is already active?
		// TODO: resume?
		unsigned resume = mgMuggle::getResumeIndex();
		Play( m_current_playlist, resume );
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
		delete m_current_playlist;

		std::string selected = (*m_plists)[ Current() ];
		m_current_playlist = m_media->loadPlaylist( selected.c_str() );

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
		if( Current() != 0 )
		  { // not editing playlist name
		    state = PlaylistSubmenuAction( Current() );
		  }
		else
		  { // editing playlist name
		    m_editing_listname = !m_editing_listname;
		    if( m_editing_listname = false )
		      { // we just changed from true to false so editing was terminated
			RenamePlaylist( std::string( m_listname ) );
			
		      }
		  }
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
  else if( m_state == PLAYLIST_COMMANDS )
    {
      if( state == osUnknown )
	{
	  switch( key )
	    {
	    case kOk:
	      {
		state = ExecutePlaylistCommand( Current() );
	      } break;
	    default:
	      {
	      }
	    }
	}
      else if( state == osBack )
	{
	  m_state = PLAYLIST_SUBMENU;
	  DisplayPlaylistSubmenu();
	  
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
    //      ist aber keine schöne Lösung
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
      std::vector<mgSelectionTreeNode*> children = node->getChildren();
      
      for( std::vector<mgSelectionTreeNode*>::iterator iter = children.begin();
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
  SetButtons();

  char *buffer;
  asprintf( &buffer, "Muggle - %s", tr("Tree View Commands") );
  SetTitle( buffer );
  free( buffer );

  // Add items
  Add( new cOsdItem( "Instant play" ) );

  Display();
}

eOSState mgMainMenu::TreeSubmenuAction( int n )
{
  eOSState state = osContinue;
  
  switch( n )
    {
    case 0:
      {
	// action 0: instant play of current node, might need a security question
	
	mgSelectionTreeNode *current = CurrentNode();
	if( current )
	  {
	    // append current node
	    std::vector<mgContentItem*> *tracks = m_menu_item->getTracks();

	    if( tracks )
	      {
		// clear playlist
		m_current_playlist->clear();
		m_current_playlist->appendList( tracks );
		
		// play
		mgMuggle::setResumeIndex( 0 );
		Play( m_current_playlist, 0 );
		
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

  std::vector<mgContentItem*>* list = m_current_playlist-> getAll();
  static char titlestr[80];
  sprintf( titlestr, "Muggle - %s (%d %s)",tr("Playlist"),
                     list->size() ,
                     tr("items") );
  SetTitle( titlestr ); 

  for( unsigned int i = 0; i < m_current_playlist->getNumItems(); i++)
    {
      std::string label =  m_current_playlist->getLabel( i, "   " );
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
  
  for(std::vector<std::string>::iterator iter = m_plists->begin(); 
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

void mgMainMenu::RenamePlaylist( std::string name )
{
  // dummy function. USes current date as name
  m_current_playlist->setListname( name );

  // confirmation
#if VDRVERSNUM >= 10307
  Skins.Message(mtInfo, "Playlist renamed" );
  Skins.Flush();
#else
  Interface->Status( "Playlist renamed" );
  Interface->Flush();
#endif
}

void mgMainMenu::DisplayPlaylistSubmenu()
{
  static const char allowed[] = { "abcdefghijklmnopqrstuvwxyz0123456789-_" };

  m_state = PLAYLIST_SUBMENU;

  Clear();
  SetButtons();
  SetTitle( "Muggle - Playlist View Commands" );

  // Add items
  Add( new cMenuEditStrItem( tr("Playlist name"), m_listname, 31, allowed ) );
  Add( new cOsdItem( tr("Load playlist" ) ) );
  Add( new cOsdItem( tr("Save playlist" ) ) );
  Add( new cOsdItem( tr("Clear playlist" ) ) );
  Add( new cOsdItem( tr("Remove entry from list" ) ) );
  Add( new cOsdItem( tr("Export playlist" ) ) );

  if( m_playlist_commands )
    {
      Add( new cOsdItem( tr("External playlist commands" ) ) );
    }

  Display();
}

void mgMainMenu::DisplayPlaylistCommands()
{
  m_state = PLAYLIST_COMMANDS;

  cCommand *command;
  int i = 0;

  Clear();
  SetTitle( "Muggle - External Playlist Commands" );

  while( ( command = m_playlist_commands->Get(i) ) != NULL )
    {
      Add( new cOsdItem( hk( command->Title() ) ) );
      i++;
    }

  Display();
}

eOSState mgMainMenu::ExecutePlaylistCommand( int current )
{
  cCommand *command = m_playlist_commands->Get( current );
  if( command )
    {
      char *buffer = NULL;
      bool confirmed = true;
      if( command->Confirm() ) 
	{
	  asprintf( &buffer, "%s?", command->Title() );
//#if VDRVERSNUM < 10307
	  confirmed = Interface->Confirm( buffer );
//#else
//#endif
	  free( buffer );
        }
      if( confirmed )
       {
	 asprintf( &buffer, "%s...", command->Title() );
#if VDRVERSNUM >= 10307
	 Skins.Message(mtInfo,buffer);
	 Skins.Flush();
#else
	 Interface->Status( buffer );
	 Interface->Flush();
#endif
	 free( buffer );

	 std::string tmp_m3u_file = (char *) AddDirectory( cPlugin::ConfigDirectory("muggle"), "current.m3u" );
	 m_current_playlist->exportM3U( tmp_m3u_file );

	 char *result = (char *)command->Execute( tmp_m3u_file.c_str() );

	 /* What to do? Recode cMenuText (not much)?
	 if( result )
	   {
	     return AddSubMenu( new cMenuText( command->Title(), result ) );
	   }
	 */
	 
	 free( result );

	 return osEnd;
       }
    }
  return osContinue;
}

eOSState mgMainMenu::PlaylistSubmenuAction( int n )
{
  std::cout << "mgMainMenu::PlaylistSubmenuAction: " << n << std::endl << std::flush;
  eOSState state = osContinue;
  
  switch( n )
    {
    case 0:
      {	// rename playlist - should never get here!
	state = osContinue;	
      } break;
      
    case 1:
      {
	LoadPlaylist();
#if VDRVERSNUM < 10307
	Interface->Flush();
#else
  Skins.Flush();
#endif
	// jump to playlist view from here?
      } break;
    case 2:
      {	
	SavePlaylist();
#if VDRVERSNUM >= 10307
	Skins.Message(mtInfo,"Playlist saved");
	Skins.Flush();
#else
	Interface->Status( "Playlist saved");
	Interface->Flush();
#endif
      } break;
    case 3:
      {	// clear playlist

	cControl *control = cControl::Control();
	std::string buffer;

	if( control && typeid(*control) == typeid(mgPlayerControl) ) 
	  {
	    buffer = "Cannot clear playlist while playing.";
	  }
	else
	  {
	    m_current_playlist->clear();

	    buffer = "Playlist cleared";
	  }
	
	// confirmation
#if VDRVERSNUM >= 10307
	Skins.Message( mtInfo, buffer.c_str() );
	Skins.Flush();
#else
	Interface->Status( buffer.c_str() );
	Interface->Flush();
#endif
	
	state = osContinue;	
      } break;
    case 4:
      { // remove selected title
	bool res = m_current_playlist->remove( m_last_osd_index );

	if( m_last_osd_index > 0 )
	  {
	    m_last_osd_index --;
	  }
	DisplayPlaylist( m_last_osd_index );

	// confirmation
	std::string confirm = res? "Entry deleted": "Cannot delete entry";

#if VDRVERSNUM >= 10307
	Skins.Message( mtInfo, confirm.c_str() );
	Skins.Flush();
#else
	Interface->Status( confirm.c_str() );
	Interface->Flush();	
#endif
      } break;
    case 5:
      {
	std::string m3u_file = AddDirectory( cPlugin::ConfigDirectory("muggle"),
					m_current_playlist->getListname().c_str() );
	m_current_playlist->exportM3U( m3u_file );
      } break;
    case 6:
      {
	DisplayPlaylistCommands();
      } break;
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
  std::vector<mgFilter*> *filter_list = m_media->getActiveFilters();
  
  int i=0;
  for( std::vector<mgFilter*>::iterator iter = filter_list->begin();
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
	    std::vector<std::string> choices = fc->getChoices();

	    char **choices_str = new (char *)[ choices.size() ];

	    int j = 0;
	    for( std::vector<std::string>::iterator iter = choices.begin();
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

void mgMainMenu::Play( mgPlaylist *plist, unsigned first )
{
  MGLOG( "mgMainMenu::Play" );
  cControl *control = cControl::Control();

  if( control && typeid(*control) == typeid(mgPlayerControl) ) 
    { // is there a running MP3 player?
      static_cast<mgPlayerControl*>(control)->NewPlaylist(plist, first); // signal the running player to load the new playlist
    }
  else
    {
      cControl::Launch( new mgPlayerControl(plist, first) );
    }
}
