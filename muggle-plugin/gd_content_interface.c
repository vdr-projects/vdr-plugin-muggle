/*******************************************************************/
/*! \file  content_interface.cpp
 * \brief  Data Objects for content (e.g. mp3 files, movies)
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.3 $
 * \date    $Date: 2004/02/01 22:35:23 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: RaK $
 *
 * DUMMY
 * Implements main classes of for content items and interfaces to SQL databases
 *
 * This file implements the following classes 
 * - GdPlaylist    a playlist
 * - mgGdTrack       a single track (content item). e.g. an mp3 file
 * - mgSelection   a set of tracks (e.g. a database subset matching certain criteria)
 *
 */
/*******************************************************************/
#define DEBUG

#include "gd_content_interface.h"
#include "mg_tools.h"

using namespace std;

#define GD_PLAYLIST_TYPE 0 // listtype for giant disc db

// some dummies to keep the compiler happy
#define DUMMY_CONDITION true // we use that as dummy condition to satisfy C++ syntax
#define DUMMY

// non-member function
int GdInitDatabase(MYSQL *db)
{
    if(mysql_init(db) == NULL)
    {	
	return -1;
    }
    
    if(mysql_real_connect(db,"localhost","root","",
			  "GiantDisc2",0,NULL,0) == NULL)
    {
	return -2;
    }
    return 0;
}

vector<string> GdGetStoredPlaylists(MYSQL db)
{
    vector<string> list;
    MYSQL_RES	*result;
    MYSQL_ROW	row;

    result=mgSqlReadQuery(&db, "SELECT title FROM playlist");

    while((row = mysql_fetch_row(result)) != NULL)
    {
	list.push_back(row[0]);
    }
    return list;
}

/*******************************************************************/
/* class mgTack                                                    */
/********************************************************************/
mgGdTrack mgGdTrack::UNDEFINED =  mgGdTrack();

/*!
 *****************************************************************************
 * \brief Constructor: creates a mgGdTrack obect
 *
 * \param sqlIdentifier identifies a unique row in the track database
 * \param dbase  database which stores the track table
 *
 * On creation, the object contains only the idea. The actual data fields
 * are filled when readData() is called for the first time.
 ****************************************************************************/
mgGdTrack::mgGdTrack(int sqlIdentifier, MYSQL dbase)
{
  m_uniqID = sqlIdentifier;
  m_db = dbase;
  m_retrieved = false;

}

/*!
 *****************************************************************************
 * \brief copy constructor
 *
 ****************************************************************************/
mgGdTrack::mgGdTrack(const mgGdTrack& org)
{
  m_uniqID = org.m_uniqID;
  m_db    = org.m_db;
  m_retrieved = org.m_retrieved;
  if(m_retrieved)
  {
   m_artist = org.m_artist;
   m_title = org.m_title;
   m_mp3file = org.m_mp3file;
   m_album = org.m_album;
   m_genre = org.m_genre; 
   m_year = org.m_year; 
   m_rating = org.m_rating;  
  }

}
  

/*!
 *****************************************************************************
 * \brief destructor
 *
 ****************************************************************************/
mgGdTrack::~mgGdTrack()
{
  // nothing to be done
}

/*!
 *****************************************************************************
 * \brief accesses the database to fill the actual data fields
 *
 * In order to avoid innecessary queries to the database, the content fields
 * of the mgGdTrack object may not be filled upon creation.
 * As soon as the first content field is needed, this private function 
 * is called to fill all content fields at once
 ****************************************************************************/
bool mgGdTrack::readData()
{
    MYSQL_RES	*result;
    int	nrows;
    int	nfields;

    // note: this does not work with empty album or genre fields
    result = mgSqlReadQuery(&m_db,
			    "SELECT tracks.artist, album.title, tracks.title, "
			    " tracks.mp3file, genre.genre, tracks.year, "
			    " tracks.rating, tracks.length "
			    "FROM tracks, album,  genre "
			    "WHERE tracks.id=%d "
			    "AND album.cddbid=tracks.sourceid AND "
			    " genre.id=tracks.genre1",
			    m_uniqID);

    nrows   = mysql_num_rows(result);
    nfields = mysql_num_fields(result);
    if(nrows == 0)
    {
      mgWarning("No entries found \n");
      return false;
    }
    else 
    {
	if (nrows >1 )
	{
	  mgWarning("More than one entry found");
	}
	MYSQL_ROW	row;
	
	row = mysql_fetch_row(result);
	m_artist = row[0];
	m_album = row[1];
	m_title = row [2];
	m_mp3file =  row [3];
	m_genre = row [4]; 
	if(sscanf(row [5], "%d", &m_year) !=1)
	{
	  mgError("Invalid year '%s' in database", row [5]);
	}
	if(sscanf(row [6], "%d", &m_rating) !=1)
	{
	  mgError("Invalid rating '%s' in database", row [6]);
	}
	if(sscanf(row [7], "%d", &m_length) !=1)
	{
	  mgError("Invalid duration '%s' in database", row [7]);
	}
    }
    m_retrieved = true;
    return true;
}

/*!
 *****************************************************************************
 * \brief returns value for _mp3file
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
string mgGdTrack::getSourceFile()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_mp3file;
}

/*!
 *****************************************************************************
 * \brief returns value for m_title
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
string mgGdTrack::getTitle()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_title;
}

/*!
 *****************************************************************************
 * \brief returns value for m_artist
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
string mgGdTrack::getArtist()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_artist;
}

/*!
 *****************************************************************************
 * \brief returns a string for one field of the item
 *
 * This is a generic function that shozld work for all content items
 ****************************************************************************/
string mgGdTrack::getLabel(int col)
{
    if(!m_retrieved)
    {
	readData();
    }
    switch(col)
    {
	case 0: 
	    return m_title;
	case 1: 
	    return m_artist;
	case 2: 
	    return m_album;
	case 3: 
	    return m_genre;
	default:
	    return "";
    }
}

string mgGdTrack::getDescription()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_artist + " - " +  m_title;
    
}

/*!
 *****************************************************************************
 * \brief returns value for m_album
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
string mgGdTrack::getAlbum()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_album;
}
  
/*!
 *****************************************************************************
 * \brief returns value for m_genre
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
string mgGdTrack::getGenre()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_genre;
}

/*!
 *****************************************************************************
 * \brief returns value for m_year
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
int mgGdTrack::getYear()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_year;
}

/*!
 *****************************************************************************
 * \brief returns value for m_rating
 *
 * If value has not been retrieved from the database, radData() is called first
 ****************************************************************************/
int mgGdTrack::getRating()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_rating;
}

int mgGdTrack::getDuration()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_rating;
}
string mgGdTrack::getImageFile()
{
    return "dummyImg.jpg";
}

/*!
 *****************************************************************************
 * \brief sets the field for m_title to the specified value
 *
 * Note: The new value is not stored in the database. 
 * This is only done, when writeData() is called
 ****************************************************************************/
void mgGdTrack::setTitle(string new_title)
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
void mgGdTrack::setArtist(string new_artist)
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
void mgGdTrack::setAlbum(string new_album)
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
void mgGdTrack::setGenre(string new_genre)
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
void mgGdTrack::setYear(int new_year)
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
void mgGdTrack::setRating(int new_rating)
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
bool mgGdTrack::writeData()
{
    mgSqlWriteQuery(&m_db, "UPDATE tracks "
		    "SET artist=\"%s\", title=\"%s\", year=%d, rating=%d "
		    "WHERE id=%d", 
		    m_artist.c_str(), m_title.c_str(),
		    m_year, m_rating, m_uniqID);
    return true;
}

GdTracklist::GdTracklist(MYSQL db_handle, string restrictions)
{
    MYSQL_RES	*result;
    MYSQL_ROW	row;
    int trackid;

    result = mgSqlReadQuery(&db_handle,
			    "SELECT  tracks.id "
			    " FROM tracks, album, genre WHERE %s"
			    " AND album.cddbid=tracks.sourceid "
			    " AND genre.id=tracks.genre1", 
			    restrictions.c_str());
    while((row = mysql_fetch_row(result)) != NULL)
    { 
      // row[0] is the trackid
	if(sscanf(row[0], "%d", &trackid) != 1)
	{
	    mgError("Can not extract integer track id from '%s'",
		    row[0]);
	}
	m_list.push_back(new mgGdTrack(trackid, db_handle));
    }
    
}
    

/*******************************************************************/
/* class GdPlaylist                                                */
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
GdPlaylist::GdPlaylist(string listname, MYSQL db_handle)
{
    MYSQL_RES	*result;
    MYSQL_ROW	row;
    int	nrows;

    m_db = db_handle;

    //
    // check, if the database really exists 
    //
    result=mgSqlReadQuery(&m_db, 
			  "SELECT id,author FROM playlist where title=\"%s\"",
			  listname.c_str());
    nrows   = mysql_num_rows(result);
    if(nrows == 0)
    {
	mgDebug(3, "No playlist with name %s found. Creating new playlist\n",
		listname.c_str());
	
	// create new database entry
	mgSqlWriteQuery(&m_db, "INSERT into playlist "
			"SET title=\"%s\", author=\"%s\"",
			listname.c_str(), 
			"VDR", // default author
			"");  // creates current time as timestamp
	m_author = "VDR";
	m_listname = listname;
    }
    else // playlist exists, read data
    {
	row = mysql_fetch_row(result);
	
	if(sscanf(row [0], "%d", & m_sqlId) !=1)
	{
	  mgError("Invalid id '%s' in database", row [5]);
	}
	m_author = row[1];
	m_listname = listname;
	// now read allentries of the playlist and 
	// write them into the tracklist
 	insertDataFromSQL();
    }// end 'else (playlist exists)
    	
    m_listtype = GD_PLAYLIST_TYPE; // GiantDB list type for playlists 
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
GdPlaylist::GdPlaylist(unsigned int sql_identifier, MYSQL db_handle)
{
    MYSQL_RES	*result;
    int	nrows;

    m_db = db_handle;

    // check, if the database really exists 
    result = mgSqlReadQuery(&m_db, 
			    "SELECT title,author FROM playlist where id=%d", 
			    sql_identifier);
    nrows   = mysql_num_rows(result);
    if(nrows == 0)
    {
	mgDebug(3, "No playlist with id %d found. Creating new playlist\n", 
		sql_identifier);
	
	// create new database entry
	// DUMMY
    }
    else // playlist exists, read data
    {
	MYSQL_ROW	row;
	row = mysql_fetch_row(result);
	
	m_listname  = row[0];
	m_author = row[1];
 	m_sqlId = sql_identifier;
	// now read allentries of the playlist and 
	// write them into the tracklist
 	insertDataFromSQL();
   }
  m_listtype = GD_PLAYLIST_TYPE; // GiantDB list type for playlists 
}

/*!
 *****************************************************************************
 * \brief empty destructor
 *
 * Nothing to be done. Constructor of parent class takes care
 ****************************************************************************/
GdPlaylist::~GdPlaylist()
{
    
}

/*!
 *****************************************************************************
 * \brief reads the track list from the sql database into a locallist
 ****************************************************************************/
int GdPlaylist::insertDataFromSQL()
{
    MYSQL_RES	*result;
    MYSQL_ROW	row;
    mgGdTrack* trackptr;
    int id;
    int nrows;

    result = mgSqlReadQuery(&m_db, 
			    "SELECT tracknumber, trackid FROM playlistitem "
			    "WHERE playlist = %d ORDER BY tracknumber",
			    m_sqlId);
    nrows   = mysql_num_rows(result);
    while((row = mysql_fetch_row(result)) != NULL)
    {
	// add antry to tracklist
	if(sscanf(row[1], "%d", &id) !=1)
	{
	    mgWarning("Track id '%s' is not an integer. Ignoring \n", row[1]);
	}
	else
	{
	    trackptr = new mgGdTrack(id, m_db);
	    m_list.push_back(trackptr);
	}
    }
    return nrows;
}
bool GdPlaylist::storePlaylist()
{
    vector<mgContentItem*>::iterator iter;
    int num;


    if(m_listname =="")
    {
	mgWarning("Can not store Tracklist without name");
	return false;
    }
    // remove old playlist items from db
    mgSqlWriteQuery(&m_db, 
		    "DELETE FROM playlistitem WHERE playlist = %d",
		    m_sqlId);

    // add new playlist items to db
   
    for(iter= m_list.begin(), num=0; iter != m_list.end(); iter++, num++)
    {

	mgSqlWriteQuery(&m_db, 
			"INSERT into playlistitem  "
			"SET tracknumber=\"%s\", trackid=\"%s\", playlist=%d",
			num, (*iter)->getId(), m_sqlId);
    }
    return true;
}
/*!
 *****************************************************************************
 * \brief returns the total duration of all songs in the list in seconds 
 *
 ****************************************************************************/
int GdPlaylist::getPlayTime()
{
    //DUMMY
    // go over all entries in the playlist and accumulate their playtime

    return 0;
}

/*!
 *****************************************************************************
 * \brief returns the duration of all remaining songs in the list in seconds
 *
 ****************************************************************************/
int GdPlaylist::getPlayTimeRemaining()
{
  //DUMMY
  // go over all remaining entries in the playlist and accumulate their
  // playtime
  // The remaining playtime of the current song is only known by the mplayer
  return 0; // dummy 
}

/*******************************************************************/
/* class GdTreeNode                                       */
/*******************************************************************/

/*!
 *****************************************************************************
 * \brief constructor
 *
 ****************************************************************************/
GdTreeNode::GdTreeNode(MYSQL db, int view, string filters)
  :  mgSelectionTreeNode(db, view)
{
    // create a root node
    // everything is done in the parent class
    m_restriction = filters;
    m_view = view;
}
GdTreeNode::GdTreeNode(mgSelectionTreeNode* parent,
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
GdTreeNode::~GdTreeNode()
{
    // _children.clear();
}



/*!
 *****************************************************************************
 * \brief compute children on the fly 
 *
 * \return: true, if the node could be expanded (or was already), false,of
 *           node can not be expanded any further
 *
 * retrieves all entries for the next level that satisfy the restriction of
 * the current level and create a child-arc for each distinct entry
 * 
 ****************************************************************************/
bool GdTreeNode::expand()
{
    
    MYSQL_ROW	row;
    MYSQL_RES	*result;
    int	nrows;
    int	nfields;
    char sqlbuff[1024];	/* hope it's big enough ! */
    char idbuf[255];
    int numchild;

    string labelfield; // human readable db field for the column to be expanded
    string idfield;  // unique id field for the column to be expanded
    string tables;  // stores the db tables used
    string new_restriction_field; // field to be restricted by the new level
    string new_restriction; // complete restriction str for the current child
    string new_label; 
    GdTreeNode* new_child;

    if (m_expanded)
    {
	mgWarning("Node already expanded\n");
	return true;
    }
    if (m_level == 1 && m_view < 100) 
    {
      m_view = atoi(m_id.c_str());
    }
    mgDebug(5, "Expanding level %d view %d\n", m_level,m_view);
    if (m_level > 0) 
    {
     switch(m_view)
     {
	 case 1: // artist -> album -> title
	     if(m_level == 1) {
		 sprintf(sqlbuff,
			 "SELECT DISTINCT album.artist,album.artist"
			 "  FROM album"
			 "  WHERE %s"
			 "  ORDER BY album.artist"
			 , m_restriction.c_str() );
		 idfield = "album.artist";
	} else if(m_level == 2) { // artist -> album 
	    sprintf(sqlbuff, 
		    "SELECT DISTINCT album.title,album.cddbid"
		    "  FROM album"
 		    "  WHERE %s"
		    "  ORDER BY album.title"
		    , m_restriction.c_str() );
	    idfield = "album.cddbid";
	} else if(m_level == 3) { // album -> title 
	    sprintf(sqlbuff, 
		    "SELECT tracks.title,tracks.id"
		    "  FROM tracks,album"
		    "  WHERE %s AND tracks.sourceid=album.cddbid"
		    "  ORDER BY tracks.tracknb"
		    , m_restriction.c_str() );
	    idfield = "tracks.id";
	} else {
	    mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
	    m_expanded = false;
	    return false;
	}
	     break;
	 case 2: // genre -> artist -> album -> track
	     if(m_level == 1) { // genre 
		 sprintf(sqlbuff,
			 "SELECT DISTINCT genre.genre,tracks.genre1"
			 "  FROM tracks,genre"
			 "  WHERE (genre.id=tracks.genre1) AND"
			 "        %s"
			 "  ORDER BY genre.id"
			 , m_restriction.c_str());
		 idfield = "tracks.genre1";
	     } else if(m_level == 2) { // genre -> artist
		 sprintf(sqlbuff,
			 "SELECT DISTINCT album.artist,album.artist"
			 "  FROM tracks,genre,album"
			 "  WHERE (genre.id=tracks.genre1) AND"
			 "        (album.cddbid=tracks.sourceid) AND"
			 "         %s"
			 "  ORDER BY album.artist",
			  m_restriction.c_str());
		 idfield = "album.artist";
	     } else if(m_level == 3) { // genre -> artist -> album
		 sprintf(sqlbuff,
			 "SELECT DISTINCT"
                         "   album.title,"
                         "   tracks.sourceid"
			 "  FROM tracks,genre,album"
			 "  WHERE (genre.id=tracks.genre1) AND"
			 "        (album.cddbid=tracks.sourceid) AND"
			 "         %s"
			 "  ORDER BY album.title"
			 , m_restriction.c_str());
		 idfield = "tracks.sourceid";
	     } else if(m_level == 4) { // genre -> artist -> album -> track
		 sprintf(sqlbuff,
			 "SELECT DISTINCT tracks.title, tracks.id"
			 "  FROM tracks,genre,album"
			 "  WHERE (genre.id=tracks.genre1) AND"
			 "        (album.cddbid=tracks.sourceid) AND"
			 "         %s"
			 "  ORDER BY tracks.tracknb"
			 , m_restriction.c_str());
		 idfield = "tracks.id";
	     } else {
		 mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		 m_expanded = false;
		 return false;
	     }
	     break;
     case 3: // Artist -> Track
             if(m_level ==1)
	     {
		 sprintf(sqlbuff,
			 "SELECT DISTINCT tracks.artist,tracks.artist"
			 "  FROM tracks"
			 "  WHERE "
			 "        %s"
			 "  ORDER BY tracks.artist" 
			 , m_restriction.c_str());
		 idfield = "tracks.artist";
	     } else if (m_level == 2) { // Track
		 sprintf(sqlbuff,
			 "SELECT DISTINCT tracks.title,tracks.id"
			 "  FROM tracks, album"
			 "  WHERE"
			 "         %s AND tracks.sourceid=album.cddbid"
			 "  ORDER BY tracks.title"
			 , m_restriction.c_str());
		 idfield = "tracks.id";
	     } else {
		 mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		 m_expanded = false;
		 return false;
	     }
	     break;
        case 4: // Genre -> Year -> Track
              if(m_level == 1) { // Genre
                sprintf(sqlbuff,
                        "SELECT DISTINCT genre.genre,tracks.genre1"
                        "  FROM genre,tracks"
                        "  WHERE (genre.id=tracks.genre1) AND"
                        "        %s"
                        "  ORDER BY genre.genre"
                        , m_restriction.c_str());
                idfield = "tracks.genre1";
              } else if (m_level == 2) { // Year
                sprintf(sqlbuff,
                        "SELECT DISTINCT tracks.year,tracks.year"
                        "  FROM genre,tracks"
                        "  WHERE (genre.id=tracks.genre1) AND"
                        "        %s"
                        "  ORDER BY tracks.year"
                        , m_restriction.c_str());
                idfield = "tracks.year";
              } else if (m_level == 3) { // Track
                sprintf(sqlbuff,
                        "SELECT DISTINCT"
                          "  CONCAT(tracks.artist,' - ',tracks.title) AS title"
                          "        ,tracks.id"
                        "  FROM tracks,genre"
                        "  WHERE (genre.id=tracks.genre1) AND"
                        "         %s"
                        "  ORDER BY tracks.title"
                        , m_restriction.c_str());
                idfield = "tracks.id";
            } else {
                mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
                m_expanded = false;
                return false;
            }
            break;
        case 5: // Album -> Tracks
              if(m_level == 1) { // Album
                sprintf(sqlbuff,
                        "SELECT DISTINCT"
                        "    CONCAT(album.artist,' - ',album.title) AS title,"
                        "    album.cddbid"
                        "  FROM album"
                        "  WHERE 1 AND"
                        "        %s"
                        "  ORDER BY title"
                        , m_restriction.c_str());
                idfield = "tracks.genre1";
              } else if (m_level == 2) { // 
		 sprintf(sqlbuff,
			 "SELECT DISTINCT tracks.title, tracks.id"
			 "  FROM tracks,genre,album"
			 "  WHERE (genre.id=tracks.genre1) AND"
			 "        (album.cddbid=tracks.sourceid) AND"
			 "         %s"
			 "  ORDER BY tracks.tracknb"
			 , m_restriction.c_str());
		 idfield = "tracks.id";
              } else {
                mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
                m_expanded = false;
                return false;
	      }
         case 100:
           if (m_level == 1) {
             sprintf(sqlbuff,
                         "SELECT DISTINCT"
                         "    CONCAT(album.artist,' - ',album.title) as title,"
                         "    album.cddbid"
                         "  FROM album,genre"
                         "  WHERE "
                         "         %s"
                         "  ORDER BY title"
                         , m_restriction.c_str());
             idfield = "tracks.sourceid";
           } else if (m_level == 2) {
                 sprintf(sqlbuff,
                         "SELECT tracks.title,tracks.id"
                         "  FROM tracks,genre,album"
                         "  WHERE (genre.id=tracks.genre1) AND"
                         "        (album.cddbid=tracks.sourceid) AND"
                         "         %s"
                         "  ORDER BY tracks.tracknb"
                         , m_restriction.c_str());
                 idfield = "tracks.id";
           } else {
                 mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
                 m_expanded = false;
                 return false;
           }
           break;
default:
	     mgError("View '%d' not yet implemented", m_view);
     }
     
     // now get all childrean ofthe current node fromthe database
     result = mgSqlReadQuery(&m_db, sqlbuff);
     nrows   = mysql_num_rows(result);
     nfields = mysql_num_fields(result);
     
     numchild=1;
     while((row = mysql_fetch_row(result)) != NULL)
     { 
	 // row[0] is the printable label for the new child
	 // row[1] is the unique id for the new child
	 sprintf(idbuf, "%s_%03d",  m_id.c_str(), numchild);
	 
	 new_restriction = m_restriction + " AND " 
	     + idfield + "= '" + row[1] + "'";
	 
	 new_child = new GdTreeNode(this, // parent
				    (string) idbuf, // id
				    row[0], // label,
				    new_restriction);
	 m_children.push_back(new_child);
	 numchild++;
     }
    } else if (m_view <100) {
	new_child = new GdTreeNode(this, // parent
				   "1" , // id
				   "Artist -> Album -> Title", // label,
				   "1");
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "2" , // id
				   "Genre -> Artist -> Album -> Track" , // label,
				   "1");
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "3" , // id
				   "Artist -> Track" , // label,
				   "1");
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "4" , // id
				   "Genre -> Year -> Track" , // label,
				   "1");
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "5" , // id
				   "Album -> Track" , // label,
				   "1");
	m_children.push_back(new_child);
    } else {
        new_child = new GdTreeNode(this, // parent
                                   "100" , // id
                                   "Search Result", // label,
                                   m_restriction);
        m_children.push_back(new_child);
    }

  m_expanded = true;
  mgDebug(5, "%d children expanded\n", m_children.size());
  return true;
}

/*!
 *****************************************************************************
 * \brief go over all children recursively to find the tracks
 *
 ****************************************************************************/
vector<mgContentItem*>* GdTreeNode::getTracks()
{
    MYSQL_ROW	row;
    MYSQL_RES	*result;
    int	nrows;
    int	nfields;
    vector<mgContentItem*>* retlist;
    int trackid;

    retlist = new  vector<mgContentItem*>();
    
    // get all tracks satisying the restrictions of this node
    mgDebug(5, "getTracks(): query '%s'", m_restriction.c_str());
    
    result = mgSqlReadQuery(&m_db,
			"SELECT  tracks.id FROM tracks, album, genre WHERE %s"
	       " AND album.cddbid=tracks.sourceid AND genre.id=tracks.genre1", 
			    m_restriction.c_str());
    nrows   = mysql_num_rows(result);
    nfields = mysql_num_fields(result);
    
    while((row = mysql_fetch_row(result)) != NULL)
    { 
      // row[0] is the trackid
	if(sscanf(row[0], "%d", &trackid) != 1)
	{
	    mgError("Can not extract integer track id from '%s'",
		    row[0]);
	}
	retlist->push_back(new mgGdTrack(trackid, m_db));
    }
    return  retlist;
}









