/*******************************************************************/
/*! \file  content_interface.cpp
 * \brief  Data Objects for content (e.g. mp3 files, movies)
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.2 $
 * \date    $Date: 2004/02/02 02:01:11 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: MountainMan $
 *
 * DUMMY
 * 
 * 
 * Dummy artists: Artist_1, Artist_2, Artist_3
 * Dummy albums by ArtistX: Album_X1, .... AlbumX3
 * Dummy Tracks On Album_XY: Track_XY1,... Track_XY5
 *
 */
/*******************************************************************/
#define DEBUG

#include "sh_dummy_content.h"
#include "mg_tools.h"

#include <strstream>
#include <string>
#define GD_PLAYLIST_TYPE 0 // listtype for giant disc db

// some dummies to keep the compiler happy
#define DUMMY_CONDITION true // we use that as dummy condition to satisfy C++ syntax
#define DUMMY
#define NUM_ARTISTS 5
#define NUM_ALBUMS_PER_ARTIST 3
#define NUM_TRACKS_PER_ALBUM 9

int DummyInitDatabase(MYSQL *db){return 0;}
vector<string> *DummyGetStoredPlaylists(MYSQL db){ return new vector<string>();}

/*******************************************************************/
/* class mgTack                                                    */
/********************************************************************/
DummyTrack DummyTrack::UNDEFINED =  DummyTrack();

/*!
 *****************************************************************************
 * \brief Constructor: creates a DummyTrack obect
 *
 * \param sqlIdentifier identifies a unique row in the track database
 * \param dbase  database which stores the track table
 *
 * On creation, the object contains only the idea. The actual data fields
 * are filled when readData() is called for the first time.
 ****************************************************************************/
DummyTrack::DummyTrack(int sqlIdentifier, MYSQL dbase)
{
  m_uniqID = sqlIdentifier;
  char buf[255];

  mgDebug(9, "Creating dumy track %d", sqlIdentifier);
  // create dummy value based on the id
  sprintf(buf, "ArtistName_%d", (int)  m_uniqID / 100);
  m_artist  = buf;
  sprintf(buf,"Album_%d",  (int)  m_uniqID / 10); 
  m_album   =  buf;
  sprintf(buf,"Track_%d",m_uniqID);
  m_title   =  buf;
  sprintf(buf,"sourcefile_%d", m_uniqID);
  m_mp3file =  buf;
  m_genre   =  "unknown_genre";
  m_year = 1970 + (m_uniqID%35); 
  m_rating = 2;
  m_length = 180;
}

/*!
 *****************************************************************************
 * \brief copy constructor
 *
 ****************************************************************************/
DummyTrack::DummyTrack(const DummyTrack& org)
{
  m_uniqID = org.m_uniqID;
  m_db    = org.m_db;

  mgDebug(9, 
	"Creating a TrackCopy for track '%s' (is this really necessary?",
	org.m_title.c_str());

  m_artist = org.m_artist;
  m_title = org.m_title;
  m_mp3file = org.m_mp3file;
  m_album = org.m_album;
  m_genre = org.m_genre; 
  m_year = org.m_year; 
  m_rating = org.m_rating;  

}
  

/*!
 *****************************************************************************
 * \brief destructor
 *
 ****************************************************************************/
DummyTrack::~DummyTrack()
{
  // nothing to be done
}

/*!
 *****************************************************************************
 * \brief returns value for _mp3file
 ****************************************************************************/
string DummyTrack::getSourceFile()
{
    return m_mp3file;
}

/*!
 *****************************************************************************
 * \brief returns value for m_title
 ****************************************************************************/
string DummyTrack::getTitle()
{
    return m_title;
}

/*!
 *****************************************************************************
 * \brief returns value for m_artist
 ****************************************************************************/
string DummyTrack::getArtist()
{
    return m_artist;
}
string DummyTrack::getLabel(int col)
{
    switch(col)
    {
	case 0:
	  return m_artist;
	  break;
        case 1:
	  return  m_title;
	  break;
    default:
      return "";
    }
}
string DummyTrack::getDescription()
{
    return m_artist + " - " +  m_title;
}

/*!
 *****************************************************************************
 * \brief returns value for m_album
 ****************************************************************************/
string DummyTrack::getAlbum()
{
    return m_album;
}
  
/*!
 *****************************************************************************
 * \brief returns value for m_genre
 ****************************************************************************/
string DummyTrack::getGenre()
{
    return m_genre;
}

/*!
 *****************************************************************************
 * \brief returns value for m_year
 ****************************************************************************/
int DummyTrack::getYear()
{
    return m_year;
}

/*!
 *****************************************************************************
 * \brief returns value for m_rating
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
int DummyTrack::getRating()
{
    return m_rating;
}

int DummyTrack::getDuration()
{
    return m_rating;
}

/*!
 *****************************************************************************
 * \brief sets the field for m_title to the specified value
 *
 * Note: The new value is not stored in the database. 
 * This is only done, when writeData() is called
 ****************************************************************************/
void DummyTrack::setTitle(string new_title)
{
    m_title = new_title;
}

/*!
 *****************************************************************************
 * \brief sets the field for m_artist to the specified value
 *
 * Note: The new value is not stored in the database. 
 * This is only done, when writeData() is called
 ****************************************************************************/
void DummyTrack::setArtist(string new_artist)
{
    m_artist =  new_artist;
}


/*!
 *****************************************************************************
 * \brief sets the field for m_album to the specified value
 *
 * Note: The new value is not stored in the database. 
 * This is only done, when writeData() is called
 ****************************************************************************/
void DummyTrack::setAlbum(string new_album)
{
    m_album = new_album;
}


/*!
 *****************************************************************************
 * \brief sets the field for m_genre to the specified value
 *
 * Note: The new value is not stored in the database. 
 * This is only done, when writeData() is called
 ****************************************************************************/
void DummyTrack::setGenre(string new_genre)
{
    m_genre = new_genre;
}


/*!
 *****************************************************************************
 * \brief sets the field for m_year to the specified value
 *
 * Note: The new value is not stored in the database. 
 * This is only done, when writeData() is called
 ****************************************************************************/
void DummyTrack::setYear(int new_year)
{
    m_year = new_year;
}


/*!
 *****************************************************************************
 * \brief sets the field for m_rating to the specified value
 *
 * Note: The new value is not stored in the database. 
 * This is only done, when writeData() is called
 ****************************************************************************/
void DummyTrack::setRating(int new_rating)
{
    m_rating = new_rating;
}


/*!
 *****************************************************************************
 * \brief stores current values in the sql database
 *
 * Note: At the moment, only the values stored directly in the 'tracks' 
 * database are updated 
 ****************************************************************************/
bool DummyTrack::writeData()
{
    return true;
}

DummyTracklist::DummyTracklist(MYSQL db_handle, string restrictions)
{
}

/*******************************************************************/
/* class DummyPlaylist                                              */
/********************************************************************/

/*!
 *****************************************************************************
 * \brief Constructor: opens playlist by name 
 *
 * \param listname user-readable identifier of the paylist
 * \param db_handl database which stores the playlist
 *
 * If the playlist does not yet exist, an empty playlist is created
 ****************************************************************************/
DummyPlaylist::DummyPlaylist(string listname, MYSQL db_handle)
{
    m_db = db_handle;

    //
    // check, if the database really exists 
    //
    if(listname =="EXISTS")
    {
	
	m_author = "DUMMY_author";
	m_listname = listname;
	createDummyPlaylist(1);
    }
    else
    {
	// new Playlist 	
	m_author = "VDR";
	m_listname = listname;

    }
    	
}

/*!
 *****************************************************************************
 * \brief Constructor: construct playlist object from existing sql playlist
 *
 * \param sql_identifier: sql internal identifier for the playlist 
 * \param db_handl database which stores the playlist
 *
 * This constructor is typically used when a playlist is selected from an
 * internal list of playlists 
 ****************************************************************************/
DummyPlaylist::DummyPlaylist(unsigned int sql_identifier, MYSQL db_handle)
{
    m_db = db_handle;
    char buf[256];

    m_author = "DUMMY_author";
    sprintf(buf, "Dummylist_%d", sql_identifier);
    m_listname =  buf;
    createDummyPlaylist(sql_identifier);
}

void DummyPlaylist::createDummyPlaylist(int start)
{
    DummyTrack* trackptr;
    for(int id=start; id < start+20; id++)
    {  
	trackptr = new DummyTrack(id, m_db);
	m_list.push_back(trackptr);
    }
}

/*!
 *****************************************************************************
 * \brief empty destructor
 *
 * Nothing to be done. The actual data is stored in the sql db
 ****************************************************************************/
DummyPlaylist::~DummyPlaylist()
{
    // nothing to be done
}

bool DummyPlaylist::storePlaylist()
{
    mgDebug(1, "Storing Playlist #%s'", m_listname.c_str());
    return true;
}
/*!
 *****************************************************************************
 * \brief returns the total duration of all songs in the list in seconds 
 *
 ****************************************************************************/
int DummyPlaylist::getPlayTime()
{
    return m_list.size()* 180;
}

/*!
 *****************************************************************************
 * \brief returns the duration of all remaining songs in the list in seconds
 *
 ****************************************************************************/
int DummyPlaylist::getPlayTimeRemaining()
{
  return 0; // dummy 
}

/*******************************************************************/
/* class DummyTreeNode                                       */
/*******************************************************************/

DummyTreeNode::DummyTreeNode(MYSQL db, int view)
  :  mgSelectionTreeNode(db, view)
{
    // create a root node
    // everything is done in the parent class
    m_restriction ="1";
}

DummyTreeNode::DummyTreeNode(mgSelectionTreeNode* parent,
			     string id, string label, string restriction)
  :  mgSelectionTreeNode(parent, id, label)
{
    m_restriction = restriction;
    // everything else is done in the parent class
}


/*!
 *****************************************************************************
 * \brief destructor
 *
 ****************************************************************************/
DummyTreeNode::~DummyTreeNode()
{
  m_children.clear();
}

/*!
 *****************************************************************************
 * \brief checks if this node can be further expandded or not
 * \true, if node ia leaf node, false if node can be expanded
 *
 ****************************************************************************/
bool DummyTreeNode::isLeafNode()
{
    switch(m_view)
     {
	 case 1: // artist -> album -> title
	     if( m_level <= 3 )
	     {
		 return true;
	     }
	     break;
	 default:
	     mgError("View '%d' not yet implemented", m_view);
     }
    return false;
}


/*!
 *****************************************************************************
 * \brief compute children on the fly 
 *
 ****************************************************************************/
bool DummyTreeNode::expand()
{
  char buf[20];
  if (m_expanded)
  {
      mgWarning("Node already expanded\n");
      return true;
  }
  m_expanded = true;

  mgDebug(5, "Expanding level %d\n", m_level);
  switch(m_view)
  {
    case 1: // artist -> album -> title
     if(m_level == 0) // root, expand artist
     {
       for(int artnum=1; artnum <= NUM_ARTISTS; artnum++)
       {
	 sprintf(buf, "%d", artnum);
	 m_children.push_back(new DummyTreeNode(this, m_id+ (string) buf,
					       "Artist "+ (string)buf,
					       m_restriction + " AND album.artist='Artist "+(string) buf+"'"));
       }

     }
     else if(m_level == 1) // artist, expand album
     {
	 // create album names in the form Album_x1, ... Album_x3
	 // where x is the artist ID
	 // for normal usage: _restrictions should now hold:
	 // album.artist = XYZ
       for(int albnum=1; albnum <= NUM_ALBUMS_PER_ARTIST; albnum++)
       {
	 sprintf(buf, "%d", albnum);
	 
	 m_children.push_back(new DummyTreeNode(this, m_id+ (string) buf,
					       "Album_"+ m_id +  (string) buf,
					       m_restriction + " AND track.sourceid='0x00"+ m_id + (string) buf +"'"));
       }
       break;
     }
     else if(m_level == 2) // artist -> album, expand title
     {
	 // create track names in the form Track_xy1, ... Track_xy5
	 // where x is the artist ID and y is the album id
	 // for normal usage: _restrictions should now hold:
	 // album.artist = ... AND track.sourceid='0x00XY
       for(int tracknum=1; tracknum <= NUM_TRACKS_PER_ALBUM; tracknum++)
       {
	 sprintf(buf, "%d", tracknum);
	 m_children.push_back(new DummyTreeNode(this, m_id+ (string) buf,
					       "Track_"+m_id+ (string) buf,
					       m_restriction + " AND track.id=tr"+ m_id + (string) buf + "'" ));
       }
       break;
     }
     else
     {
	 mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
	 m_expanded = false;
	 return false;
     }
     break;
   default:
       mgError("View '%d' not yet implemented", m_view);
  }
  mgDebug(5, "%d children expanded\n", m_children.size());
  return true;
}

/*!
 *****************************************************************************
 * \brief go over all children recursively to find the tracks
 *
 ****************************************************************************/
vector<mgContentItem*>* DummyTreeNode::getTracks()
{
  vector<mgContentItem*>* dummy;
  string sql;
 
  int artistnum;
  int albumnum;
  int tracknum;
  dummy = new  vector<mgContentItem*>();
  
  sql = m_restriction;
  mgDebug(5, "QUERY:\n%s\n", sql.c_str());

  artistnum = 0;
  do  // artist_loop
  {
      if(m_level >=1)
	  artistnum = m_id[0]-'0'; // we have a unique artist
      else
	  artistnum++; // we are in a loop

      albumnum = 0;
      do  // album_loop
      { 
	  if(m_level >=2)
	      albumnum = m_id[1]-'0';  // we have a unique album
	  else
	      albumnum++; // we are in a loop
        
	  tracknum =0;
	  do  // track_loop
	  {
	      if(m_level ==3)
		  tracknum = m_id[2]-'0';  // we have a unique track
	      else
		  tracknum++; // we are in a loop
	      dummy->push_back(new DummyTrack(artistnum*100+albumnum*10+tracknum, m_db));
	  }while ((m_level < 3) && (tracknum<  NUM_TRACKS_PER_ALBUM));
      }while ((m_level < 2) && (albumnum < NUM_ALBUMS_PER_ARTIST));
  }while ((m_level < 1) && (artistnum < NUM_ARTISTS ));

  return  dummy;
}

/*!
 *****************************************************************************
 * \brief returns the first track matchin the restrictions of this node
 * assuming we are in a leaf node, this returns the track represented by the
 * the leaf 
 ****************************************************************************/
mgContentItem* DummyTreeNode::getSingleTrack()
{
    // get all tracks satisying the restrictions of this node
    mgDebug(5, "getTracks(): query '%s'", m_restriction.c_str());
    
    mgContentItem* track = new DummyTrack(atoi(m_id.c_str()), m_db);

    return  track;
}




























