/*! 
 * \file   gd_content_interface.c
 * \brief  Data Objects for content (e.g. mp3 files, movies) for the vdr muggle plugin
 * \ingroup giantdisc
 *
 * \version $Revision: 1.27 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * Implements main classes of for content items and interfaces to SQL databases
 *
 * This file implements the following classes 
 *  - GdPlaylist    a playlist
 *  - mgGdTrack     a single track (content item). e.g. an mp3 file
 *  - mgSelection   a set of tracks (e.g. a database subset matching certain criteria)
 */

#define DEBUG

#include "gd_content_interface.h"

#include "mg_tools.h"
#include "mg_database.h"
#include "vdr_setup.h"

#include "i18n.h"


#define GD_PLAYLIST_TYPE 0 //< listtype for giant disc db

// some dummies to keep the compiler happy
#define DUMMY_CONDITION true // we use that as dummy condition to satisfy C++ syntax
#define DUMMY

/*!
 * \brief initialize a database used by Giantdisc
 *
 * \todo should be a static function in some Gd class
 */
int GdInitDatabase( MYSQL *db )
{
  if( mysql_init(db) == NULL )
    {	
      return -1;
    }
    
  if (the_setup.DbSocket != NULL)
    {
      mgDebug(1,"Using sockets for connecting to Database.");

      //mgDebug(3,"Socket is: '%s'",the_setup.DbSocket);
      //mgDebug(3,"DbUser is: '%s'",the_setup.DbUser);
      //mgDebug(3,"DbPassword is: '%s'",the_setup.DbPass);

      if( mysql_real_connect( db,
			      "",
			      the_setup.DbUser,       
			      the_setup.DbPass, 
			      the_setup.DbName,
			      0,
			      the_setup.DbSocket, 0 ) == NULL )
	{
        return -2;
      } // if mysql_real_connect
    } //if DbSocket
  else
    {
      mgDebug(1,"Using TCP-host for connecting to Database.");
      if( mysql_real_connect( db,
			      the_setup.DbHost, 
			      the_setup.DbUser,       
			      the_setup.DbPass, 
			      the_setup.DbName,
			      the_setup.DbPort,
			      NULL, 0 ) == NULL )
      {
        return -2;
      } // if mysql_real_connect
    } // else (if DbSocket)
    
  return 0;
}

std::vector<std::string> *GdGetStoredPlaylists(MYSQL db)
{
    std::vector<std::string>* list = new  std::vector<std::string>();
    MYSQL_RES	*result;
    MYSQL_ROW	row;

    result = mgSqlReadQuery(&db, "SELECT title FROM playlist");

    while( (row = mysql_fetch_row(result) ) != NULL )
      {
	list->push_back(row[0]);
      }
    return list;
}

gdFilterSets::gdFilterSets()
{
  mgFilter* filter;
  std::vector<mgFilter*>* set;
  std::vector<std::string>* rating;
  m_titles.push_back( tr("Track Search") );

  // create an initial set of filters with empty values
  set = new std::vector<mgFilter*>();
  rating = new std::vector<std::string>();
  rating->push_back("-");
  rating->push_back("O");
  rating->push_back("+");
  rating->push_back("++");
 
  // year-from
  filter = new mgFilterInt(tr("year (from)"), 1901, 1900, 2100); set->push_back(filter);

  // year-to
  filter = new mgFilterInt(tr("year (to)"), 2099, 1900, 2100); set->push_back(filter);

  // title
  filter = new mgFilterString(tr("title"), ""); set->push_back(filter);

  // artist
  filter = new mgFilterString(tr("artist"), ""); set->push_back(filter);

  // genre
  filter = new mgFilterString(tr("genre"), ""); set->push_back(filter);

  // rating. TODO: Currently buggy. LVW
  // filter = new mgFilterChoice(tr("rating"), 1, rating); set->push_back(filter);

  m_sets.push_back(set);
  
  m_titles.push_back(tr("Album Search"));
  
  set = new std::vector<mgFilter*>();
  // year-from
  filter = new mgFilterInt(tr("year (from)"), 1901, 1900, 2100); set->push_back(filter);
  // year-to
  filter = new mgFilterInt(tr("year (to)"), 2099, 1900, 2100); set->push_back(filter);
  // title
  filter = new mgFilterString(tr("album title"), ""); set->push_back(filter);
  // artist
  filter = new mgFilterString(tr("album artist"), ""); set->push_back(filter);
  // genre
  filter = new mgFilterString(tr("genre"), ""); set->push_back(filter);
  // rating
  filter = new mgFilterChoice(tr("rating"), 1, rating); set->push_back(filter);

  m_sets.push_back(set);
  
  m_titles.push_back(tr("Playlist Search"));
  
  set = new std::vector<mgFilter*>();
  // year-from
  filter = new mgFilterInt(tr("year (from)"), 1901, 1900, 2100); set->push_back(filter);
  // year-to
  filter = new mgFilterInt(tr("year (to)"), 2099, 1900, 2100); set->push_back(filter);
  // title
  filter = new mgFilterString(tr("playlist title"), ""); set->push_back(filter);
  // artist
  filter = new mgFilterString(tr("playlist author"), ""); set->push_back(filter);
  // title
  filter = new mgFilterString(tr("title"), ""); set->push_back(filter);
  // artist
  filter = new mgFilterString(tr("artist"), ""); set->push_back(filter);
  // genre
  filter = new mgFilterString(tr("genre"), ""); set->push_back(filter);
  // rating
  filter = new mgFilterChoice(tr("rating"), 1, rating); set->push_back(filter);

  m_sets.push_back(set);
  
  m_activeSetId = 0;
  m_activeSet = m_sets[m_activeSetId];
}

gdFilterSets::~gdFilterSets()
{
  // everything is done in the destructor of the base class
}

std::string gdFilterSets::computeRestriction(int *viewPrt)
{
  std::string sql_str = "1";

  switch( m_activeSetId )
    {
    case 0:
      {
	// tracks (flatlist for mountain man ;-))
	*viewPrt =  100; 
      } break;
    case 1:
      {
	// album -> tracks
	*viewPrt =  101; 
      } break;
    case 2: 
      {
	// playlist -> tracks
	*viewPrt =  102;
      } break;
    default:
      {
	mgWarning( "Ignoring Filter Set %i", m_activeSetId );
      }	break;
  }

  for( std::vector<mgFilter*>::iterator iter = m_activeSet->begin();
       iter != m_activeSet->end(); 
       iter++ )
    {
      if( (*iter)->isSet() )
	{
	  if( strcmp((*iter)->getName(), tr("playlist title") ) == 0 )
	    {
	      sql_str = sql_str + " AND playlist.title like '%%" 
		+ (*iter)->getStrVal() + "%%'";
	    }
	  else if(strcmp( (*iter)->getName(), tr("playlist author") ) == 0 )
	    {
	      sql_str = sql_str + " AND playlist.author like '%%" 
		+ (*iter)->getStrVal() + "%%'";
	    }
	  else if(strcmp((*iter)->getName(), tr("album title")) == 0 )
	    {
	      sql_str = sql_str + " AND album.title like '%%" 
		+ (*iter)->getStrVal() + "%%'";
	    }
	  else if(strcmp((*iter)->getName(), tr("album artist")) == 0 )
	    { 
	      sql_str = sql_str + " AND album.artist like '%%" 
		+ (*iter)->getStrVal() + "%%'";
	    }
	  else if(strcmp((*iter)->getName(), tr("title")) == 0 )
	    {
	      sql_str = sql_str + " AND tracks.title like '%%" 
		+ (*iter)->getStrVal() + "%%'";
	    }
	  else if(strcmp((*iter)->getName(), tr("artist")) == 0 )
	    { 
	      sql_str = sql_str + " AND tracks.artist like '%%" 
		+ (*iter)->getStrVal() + "%%'";
	    }
	  else if(strcmp((*iter)->getName(), tr("genre")) == 0 )
	    { 
	      sql_str = sql_str + " AND (genre1.genre like '" 
		+ (*iter)->getStrVal() + "'";
	      sql_str = sql_str + " OR genre2.genre like '" 
		+ (*iter)->getStrVal() + "')";
	    }
	  else if(strcmp((*iter)->getName(), tr("year (from)")) == 0 )
	    { 
	      sql_str = sql_str + " AND tracks.year >= " + (*iter)->getStrVal();
	    }
	  else if(strcmp((*iter)->getName(), tr("year (to)")) == 0 )
	    { 
	      sql_str = sql_str + " AND tracks.year <= " + (*iter)->getStrVal();
	    }
	  else if(strcmp((*iter)->getName(), tr("rating")) == 0 )
	    { 
	      if ((*iter)->getStrVal() == "-") 
		{
		  sql_str = sql_str + " AND tracks.rating >= 0 ";
		} 
	      else if ((*iter)->getStrVal() == "O") 
		{
		  sql_str = sql_str + " AND tracks.rating >= 1 ";
		}
	      else if ((*iter)->getStrVal() == "+")
		{
		  sql_str = sql_str + " AND tracks.rating >= 2 ";
		} 
	      else if ((*iter)->getStrVal() == "++") 
		{
		  sql_str = sql_str + " AND tracks.rating >= 3 ";
		}
	    }
	  else
	    {
	      mgWarning( "Ignoring unknown filter %s", (*iter)->getName() );
	    }  
	}
    }  
  mgDebug(1, "Applying sql std::string %s (view=%d)", sql_str.c_str(), *viewPrt ); 
  return sql_str;
}

mgGdTrack mgGdTrack::UNDEFINED =  mgGdTrack();

mgGdTrack::mgGdTrack( int sqlIdentifier, MYSQL dbase )
{
  m_uniqID = sqlIdentifier;
  m_db = dbase;
  m_retrieved = false;
}

mgGdTrack::mgGdTrack(const mgGdTrack& org)
{
  m_uniqID = org.m_uniqID;
  m_db    = org.m_db;
  m_retrieved = org.m_retrieved;
  
  if( m_retrieved )
    {
      m_artist = org.m_artist;
      m_title = org.m_title;
      m_mp3file = org.m_mp3file;
      m_album = org.m_album;
      m_genre = org.m_genre;
      m_year = org.m_year;
      m_rating = org.m_rating;
      m_length = org.m_length;
    }
}
  
mgGdTrack::~mgGdTrack()
{
  // nothing to be done
}

bool mgGdTrack::readData()
{
    MYSQL_RES	*result;
    int	nrows, nfields;

    // note: this does not work with empty album or genre fields
    result = mgSqlReadQuery( &m_db,
			     "SELECT tracks.artist, album.title, tracks.title, "
			     "tracks.mp3file, genre.genre, tracks.year, "
			     "tracks.rating, tracks.length, tracks.samplerate, tracks.channels, tracks.bitrate "
			     "FROM tracks, album, genre "
			     "WHERE tracks.id = %d "
			     "AND album.cddbid = tracks.sourceid AND "
			     "genre.id = tracks.genre1",
			     m_uniqID );
    
    nrows   = mysql_num_rows(result);
    nfields = mysql_num_fields(result);

    if( nrows == 0 )
      {
	mgWarning( "No entries found \n" );
	return false;
      }
    else 
      {
	if( nrows > 1 )
	  {
	    mgWarning("mgGdTrack::readData: More than one entry found. Using first entry.");
	  }
	
	MYSQL_ROW row = mysql_fetch_row( result );
	
	m_artist  = row[0];
	m_album   = row[1];
	m_title   = row[2];
	m_mp3file = std::string( the_setup.ToplevelDir ) + row[3];
	m_genre   = row[4];
	
	if( sscanf( row[5], "%d", &m_year) != 1 )
	  {
	    mgError("Invalid year '%s' in database", row [5]);
	  }
	
	if( row[6] && sscanf( row[6], "%d", &m_rating ) != 1 )
	  {
	    mgError( "Invalid rating '%s' in database", row [6] );
	  }
	
	if( row[7] && sscanf( row[7], "%d", &m_length) != 1 )
	  {
	    mgError( "Invalid duration '%s' in database", row [7]);
	  }

	if( row[8] && sscanf( row[8], "%d", &m_samplerate ) != 1 )
	  {
	    mgError( "Invalid samplerate '%s' in database", row [7]);
	  }
	
	if( row[9] && sscanf( row[9], "%d", &m_channels ) != 1 )
	  {
	    mgError( "Invalid channels '%s' in database", row [7]);
	  }

	m_bitrate = row[10];
	
      }
    m_retrieved = true;
    return true;
}

std::string mgGdTrack::getSourceFile()
{
  if( !m_retrieved )
    {
      readData();
    }
  return m_mp3file;
}

std::string mgGdTrack::getTitle()
{
  if( !m_retrieved )
    {
      readData();
    }
  return m_title;
}

std::string mgGdTrack::getArtist()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_artist;
}

int mgGdTrack::getLength()
{
  if( !m_retrieved )
    {
      readData();
    }
  return m_length;
}


std::string mgGdTrack::getLabel(int col)
{
  if( !m_retrieved )
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

std::vector<mgFilter*> *mgGdTrack::getTrackInfo()
{
  return new std::vector<mgFilter*>();
}

bool mgGdTrack::setTrackInfo(std::vector<mgFilter*> *info)
{
  return false;
}

std::string mgGdTrack::getAlbum()
{
  if( !m_retrieved )
    {
      readData();
    }
  return m_album;
}
  
std::string mgGdTrack::getGenre()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_genre;
}

int mgGdTrack::getYear()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_year;
}

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

int mgGdTrack::getSampleRate()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_samplerate;
}

int mgGdTrack::getChannels()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_channels;
}

std::string mgGdTrack::getBitrate()
{
    if(!m_retrieved)
    {
	readData();
    }
    return m_bitrate;
}

std::string mgGdTrack::getImageFile()
{
    return "dummyImg.jpg";
}

void mgGdTrack::setTitle(std::string new_title)
{
    m_title = new_title;
}

void mgGdTrack::setArtist(std::string new_artist)
{
    m_artist =  new_artist;
}

void mgGdTrack::setAlbum(std::string new_album)
{
    m_album = new_album;
}

void mgGdTrack::setGenre(std::string new_genre)
{
    m_genre = new_genre;
}

void mgGdTrack::setYear(int new_year)
{
  m_year = new_year;
}

void mgGdTrack::setRating(int new_rating)
{
  m_rating = new_rating;
}

bool mgGdTrack::writeData()
{
  mgSqlWriteQuery( &m_db, "UPDATE tracks "
		   "SET artist=\"%s\", title=\"%s\", year=%d, rating=%d "
		   "WHERE id=%d", 
		   m_artist.c_str(), m_title.c_str(),
		   m_year, m_rating, m_uniqID);
  return true;
}

GdTracklist::GdTracklist(MYSQL db_handle, std::string restrictions)
{
    MYSQL_RES	*result;
    MYSQL_ROW	row;
    int trackid;
    
    result = mgSqlReadQuery( &db_handle,
			     "SELECT  tracks.id "
			     " FROM tracks, album, genre WHERE %s"
			     " AND album.cddbid=tracks.sourceid "
			     " AND genre.id=tracks.genre1", 
			     restrictions.c_str());
    
    while( ( row = mysql_fetch_row(result) ) != NULL )
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

GdPlaylist::GdPlaylist(std::string listname, MYSQL db_handle)
{
    MYSQL_RES	*result;
    MYSQL_ROW	row;
    int	nrows;

    m_db = db_handle;

    //
    // check, if the playlist already exists 
    //
    result = mgSqlReadQuery(&m_db, 
			    "SELECT id,author FROM playlist where title=\"%s\"",
			    listname.c_str());
    nrows   = mysql_num_rows(result);

    if( nrows == 0 )
      {
	mgDebug(3, "No playlist with name %s found. Creating new playlist\n",
		listname.c_str());
	
	// create new database entry
	mgSqlWriteQuery( &m_db, "INSERT into playlist "
			 "SET title=\"%s\", author=\"%s\"",
			 listname.c_str(), 
			 "VDR", // default author
			 "");  // creates current time as timestamp
	m_author = "VDR";
	m_listname = listname;
	
	// now read thenew list to get the id
	result = mgSqlReadQuery( &m_db, 
				 "SELECT id,author FROM playlist where title=\"%s\"",
				 listname.c_str() );
	nrows   = mysql_num_rows(result);
	row     = mysql_fetch_row(result);
	
	if( sscanf(row [0], "%d", & m_sqlId) !=1 )
	  {
	    mgError("Invalid id '%s' in database", row [5]);
	  }
      }
    else 
      { // playlist exists, read data
	row = mysql_fetch_row(result);
	
	if( sscanf(row[0], "%d", & m_sqlId) !=1 )
	  {
	    mgError("Invalid id '%s' in database", row [5]);
	  }

	m_author = row[1];
	m_listname = listname;

	// now read allentries of the playlist and 
	// write them into the tracklist
 	insertDataFromSQL();

      } // end 'else (playlist exists)
    
    m_listtype = GD_PLAYLIST_TYPE; // GiantDB list type for playlists 
}

GdPlaylist::~GdPlaylist()
{    
}

void GdPlaylist::setListname(std::string name)
{
  m_listname = name;
  m_sqlId = -1;
}

int GdPlaylist::insertDataFromSQL()
{
  MYSQL_RES	*result;
  MYSQL_ROW	row;
  mgGdTrack* trackptr;
  int id;
  int nrows;
  
  result = mgSqlReadQuery( &m_db, 
			   "SELECT tracknumber, trackid FROM playlistitem "
			   "WHERE playlist = %d ORDER BY tracknumber",
			   m_sqlId);
  nrows   = mysql_num_rows(result);
  while( (row = mysql_fetch_row(result) ) != NULL )
    {
      // add antry to tracklist
      if( sscanf( row[1], "%d", &id ) !=1 )
	  {
	  mgWarning( "Track id '%s' is not an integer. Ignoring \n", row[1] );
	  }
      else
	  {
	  trackptr = new mgGdTrack( id, m_db );
	  m_list.push_back( trackptr );
	  }
      }
  return nrows;
}

bool GdPlaylist::storePlaylist()
{
    std::vector<mgContentItem*>::iterator iter;
    int num;
    MYSQL_RES	*result;
    MYSQL_ROW	row;
    int	nrows;

    if( m_listname == " " )
      {
	mgWarning("Can not store Tracklist without name");
	return false;
      }

    if(m_sqlId >= 0)
      {
	// playlist alreay exists in SQL database
	// remove old items first
	// cout << " GdPlaylist::storePlaylist: removing items from " << m_sqlId << flush;

	// remove old playlist items from db
	mgSqlWriteQuery(&m_db, 
			"DELETE FROM playlistitem WHERE playlist = %d",
			m_sqlId);
      }
    else
      {
	// create new database entry
	mgSqlWriteQuery(&m_db, "INSERT into playlist "
			"SET title=\"%s\", author=\"%s\"",
			m_listname.c_str(), 
			"VDR", // default author
			"");  // creates current time as timestamp
	m_author = "VDR";
	
	// now read thenew list to get the id
	result=mgSqlReadQuery(&m_db, 
			      "SELECT id,author FROM playlist where title=\"%s\"",
			      m_listname.c_str());
	nrows   = mysql_num_rows(result);
	row = mysql_fetch_row(result);
	
	if( sscanf( row [0], "%d", & m_sqlId ) !=1 )
	  {
	    mgError("Invalid id '%s' in database", row [5]);
	  }
      }
    // add new playlist items to db
    
    for( iter=m_list.begin(), num=0; 
	 iter != m_list.end(); 
	 iter++, num++)
      {
	mgSqlWriteQuery(&m_db, 
			"INSERT into playlistitem  "
			"SET tracknumber=\"%d\", trackid=\"%d\", playlist=%d",
			num, (*iter)->getId(), m_sqlId);
      }
    return true;
}

/*!
 * \brief returns the total duration of all songs in the list in seconds 
 */
int GdPlaylist::getPlayTime()
{
    //DUMMY
    // go over all entries in the playlist and accumulate their playtime

    return 0;
}

/*!
 * \brief returns the duration of all remaining songs in the list in seconds
 */
int GdPlaylist::getPlayTimeRemaining()
{
  //DUMMY
  // go over all remaining entries in the playlist and accumulate their
  // playtime
  // The remaining playtime of the current song is only known by the mplayer
  return 0; // dummy 
}

/*!
 * \brief constructor
 */
GdTreeNode::GdTreeNode(MYSQL db, int view, std::string filters)
  :  mgSelectionTreeNode(db, view)
{
  // create a root node
  // everything is done in the parent class
  m_restriction = filters;
  m_view = view;
  m_label = tr("Browser");
}

GdTreeNode::GdTreeNode( mgSelectionTreeNode* parent,
			std::string id, 
			std::string label, 
			std::string restriction )
  :  mgSelectionTreeNode(parent, id, label)
{
  m_restriction = restriction;
  // everything else is done in the parent class
}

/*!
 * \brief destructor
 */
GdTreeNode::~GdTreeNode()
{
  // _children.clear();
}

/*!
 * \brief checks if this node can be further expandded or not
 * \true, if node ia leaf node, false if node can be expanded
 */
bool GdTreeNode::isLeafNode()
{
  if( m_level == 0 )
    {
      return false;
    }
    
  switch(m_view)
    {
    case 1: // artist -> album -> title
      if( m_level <= 3 )
	{
	  return false;
	}
      break;
    case 2: // genre -> artist -> album -> track
      if( m_level <= 3 )
	{
	  return false;
	}
      break;
    case 3: // Artist -> Track
      if( m_level <= 2 )
	{
	  return false;
	}
      break;
    case 4:
      if( m_level <= 2 )
	{
	  return false;
	}
      break;
    case 5:
      if( m_level <= 1 )
	{
	  return false;
	}
      break;
    case 100:
      if( m_level <= 0 )
	{
	  return false;
	}
      break;
    case 101:
      if( m_level <= 1 )
	{
	  return false;
	}
      break;
    case 102:
      if( m_level <= 1 )
	{
	  return false;
	}
      break;
    default:
      mgError("View '%d' not yet implemented", m_view);
    }
    return true;
}

/*!
 * \brief compute children on the fly 
 *
 * \return: true, if the node could be expanded (or was already), false,of
 *          node can not be expanded any further
 *
 * retrieves all entries for the next level that satisfy the restriction of
 * the current level and create a child-arc for each distinct entry
 * 
 * \todo use asnprintf!
 */
bool GdTreeNode::expand()
{    
    MYSQL_ROW	row;
    MYSQL_RES	*result;
    int	nrows;
    int	nfields;
    char sqlbuff[1024];	/* hope it's big enough ! */
    char idbuf[255];
    int numchild;

    std::string labelfield;            // human readable db field for the column to be expanded
    std::string idfield;               // unique id field for the column to be expanded
    std::string new_restriction_field; // field to be restricted by the new level
    std::string new_restriction;       // complete restriction str for the current child
    std::string new_label;
    GdTreeNode* new_child;

    std::string tables;  // stores the db tables used
    
#define  FROMJOIN " FROM tracks, genre as genre1, genre as genre2, album WHERE tracks.sourceid=album.cddbid AND genre1.id=tracks.genre1 AND genre2.id=tracks.genre2 AND %s "
    
    if( m_expanded )
      {
	mgWarning("Node already expanded\n");
	return true;
      }
    
    if( m_level == 1 && m_view < 100 ) 
      {
	m_view = atoi( m_id.c_str() );
      }
    
    mgDebug( 5, "Expanding level %d view %d\n", m_level, m_view );
    if( m_level > 0 )
      {
	switch( m_view )
	  {
	  case 1:
	    { // artist -> album -> title
	      if( m_level == 1 ) 
		{
		  sprintf( sqlbuff,
			   "SELECT DISTINCT album.artist,album.artist" 
			   FROMJOIN
			   " ORDER BY album.artist"
			   , m_restriction.c_str() );
		  idfield = "album.artist";
		}
	      else if( m_level == 2 ) 
		{ // artist -> album 
		  sprintf(sqlbuff, 
			  "SELECT DISTINCT album.title,album.cddbid"
			  FROMJOIN
			  "  ORDER BY album.title"
			  , m_restriction.c_str() );
		  idfield = "album.cddbid";
		} 
	      else if(m_level == 3) 
		{ // album -> title 
		  sprintf(sqlbuff, 
			  "SELECT tracks.title,tracks.id"
			  FROMJOIN
			  "  ORDER BY tracks.tracknb"
			  , m_restriction.c_str() );
		  idfield = "tracks.id";
		} 
	      else 
		{
		  mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		  m_expanded = false;
		  return false;
		}
	      } break;
	  case 2: 
	    { // genre -> artist -> album -> track
	      if( m_level == 1 ) 
		{ // genre 
		  sprintf(sqlbuff,
			  "SELECT DISTINCT genre1.genre,tracks.genre1"
			  FROMJOIN
			  "  ORDER BY genre1.id"
			  , m_restriction.c_str());
		  idfield = "tracks.genre1";
		} 
	      else if( m_level == 2 )
		{ // genre -> artist
		  sprintf(sqlbuff,
			  "SELECT DISTINCT album.artist,album.artist"
			  FROMJOIN
			  "  ORDER BY album.artist",
			  m_restriction.c_str());
		  idfield = "album.artist";
		}
	      else if( m_level == 3 )
		{ // genre -> artist -> album
		  sprintf(sqlbuff,
			  "SELECT DISTINCT album.title,tracks.sourceid"
			  FROMJOIN
			  "  ORDER BY album.title"
			  , m_restriction.c_str());
		  idfield = "tracks.sourceid";
		}
	      else if( m_level == 4 ) 
		{ // genre -> artist -> album -> track
		  sprintf(sqlbuff,
			  "SELECT DISTINCT tracks.title, tracks.id"
			  FROMJOIN
			  "  ORDER BY tracks.tracknb"
			  , m_restriction.c_str());
		  idfield = "tracks.id";
		} 
	      else 
		{
		  mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		  m_expanded = false;
		  return false;
		}
	     } break;
	  case 3:
	    { // Artist -> Track
	      if( m_level ==1 )
		{
		  sprintf( sqlbuff,
			   "SELECT DISTINCT tracks.artist,tracks.artist"
			   FROMJOIN
			   "  ORDER BY tracks.artist",
			   m_restriction.c_str());
		  idfield = "tracks.artist";
		} 
	      else if( m_level == 2) 
		{ // Track
		  sprintf(sqlbuff,
			  "SELECT DISTINCT tracks.title,tracks.id"
			  FROMJOIN
			  "  ORDER BY tracks.title",
			  m_restriction.c_str());
		  idfield = "tracks.id";
		} 
	      else 
		{
		  mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		  m_expanded = false;
		  return false;
		}
	    } break;
        case 4:
	  { // Genre -> Year -> Track
              if( m_level == 1 ) 
		{ // Genre
		  sprintf(sqlbuff,
			  "SELECT DISTINCT genre1.genre,tracks.genre1"
			  FROMJOIN
			  "  ORDER BY genre1.genre",
			  m_restriction.c_str());
		  idfield = "tracks.genre1";
		}
	      else if (m_level == 2)
		{ // Year
		  sprintf(sqlbuff,
			  "SELECT DISTINCT tracks.year,tracks.year"
			  FROMJOIN
			  "  ORDER BY tracks.year"
			  , m_restriction.c_str());
		  idfield = "tracks.year";
		} 
	      else if( m_level == 3 ) 
		{ // Track
		  sprintf(sqlbuff,
			  "SELECT DISTINCT"
			  "    CONCAT(tracks.artist,' - ',tracks.title) AS title"
			  "          ,tracks.id"
			  FROMJOIN
			  "  ORDER BY tracks.title",
			  m_restriction.c_str());
		  idfield = "tracks.id";
		} 
	      else 
		{
		  mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		  m_expanded = false;
		  return false;
		}
	  } break;
        case 5: // Album -> Tracks
              if( m_level == 1 ) 
		{ // Album
		  sprintf(sqlbuff,
			  "SELECT DISTINCT"
			  "    CONCAT(album.artist,' - ',album.title) AS title,"
			  "    album.cddbid"
			  FROMJOIN
			  "  ORDER BY title"
			  , m_restriction.c_str());
		  idfield = "tracks.sourceid";
		} 
	      else if (m_level == 2) 
		{ // Track 
		  sprintf(sqlbuff,
			  "SELECT DISTINCT tracks.title, tracks.id"
			  FROMJOIN
			  "  ORDER BY tracks.tracknb",
			  m_restriction.c_str());
		  idfield = "tracks.id";
		} 
	      else 
		{
		  mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		  m_expanded = false;
		  return false;
		}
	      break;
	  case 100:
	    if (m_level == 1) 
	      {
		sprintf(sqlbuff,
			"SELECT CONCAT(tracks.artist,' - ',tracks.title),"
			"       tracks.id"
			FROMJOIN
			"  ORDER BY CONCAT(tracks.artist,' - ',tracks.title)"
			, m_restriction.c_str());
		idfield = "tracks.id";
	      } 
	    else 
	      {
		mgWarning( "View #%d level %d' not yet implemented", m_view, m_level );
		m_expanded = false;
		return false;
	      }
	    break;
	  case 101: 
	    { // Albumsearch result
	      if( m_level == 1 ) 
		{
		  sprintf(sqlbuff,
			  "SELECT DISTINCT"
			  "    CONCAT(album.artist,' - ',album.title) as title,"
			  "    album.cddbid"
			  FROMJOIN
			  "  ORDER BY CONCAT(album.artist,' - ',album.title)",
			  m_restriction.c_str());
		  idfield = "tracks.sourceid";
		} 
	      else if( m_level == 2 ) 
		{
		  sprintf(sqlbuff,
			  "SELECT tracks.title,tracks.id"
			  FROMJOIN
			  "  ORDER BY tracks.tracknb",
			  m_restriction.c_str());
		  idfield = "tracks.id";
		} 
	      else 
		{
		  mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		  m_expanded = false;
		  return false;
		}
	      } break;
        case 102:
	  { 
	    if (m_level == 1) 
	      {
		sprintf(sqlbuff,
			"SELECT DISTINCT playlist.title,"
			"    playlist.id"
			"  FROM playlist,playlistitem,tracks,genre as genre1,genre as genre2"
			"  WHERE playlist.id=playlistitem.playlist AND"
			"        playlistitem.trackid=tracks.id AND"
			"        genre1.id=tracks.genre1 AND"
			"        genre2.id=tracks.genre2 AND"
			"        %s"
			"  ORDER BY playlist.title,",
			m_restriction.c_str());
		idfield = "playlist.id";
	      } 
	    else if (m_level == 2) 
	      {
		sprintf(sqlbuff,
			"SELECT CONCAT(tracks.artist,' - ',tracks.title),"
			"    tracks.id"
			"  FROM playlist,playlistitem,tracks"
			"  WHERE playlist.id=playlistitem.playlist AND"
			"        playlistitem.trackid=tracks.id AND"
			"        %s"
			"  ORDER BY playlistitem.tracknumber",
			m_restriction.c_str());
		idfield = "tracks.id";
	      } 
	    else 
	      {
		mgWarning("View #%d level %d' not yet implemented", m_view, m_level);
		m_expanded = false;
		return false;
	      }
          } break;
	  default:
	    {
	      mgError("View '%d' not yet implemented", m_view);
	    }
	  }
	
	// now get all childrean of the current node fromthe database
	result = mgSqlReadQuery( &m_db, sqlbuff );
	nrows   = mysql_num_rows( result );
	nfields = mysql_num_fields(result);
	
	numchild = 1;
	while( (row = mysql_fetch_row(result) ) != NULL )
	  { 
	    // row[0] is the printable label for the new child
	    // row[1] is the unique id for the new child
	    sprintf( idbuf, "%s_%03d",  m_id.c_str(), numchild );
	    
	    // Zweite ebene zeigt alle Tracks des Albums und nicht nur 
	    // diese die den Filterkriterien entsprechen.
	    // das betrifft nur die Search Views!

	    std::string row0 = mgDB::escape_string( &m_db, std::string( row[0] ) );
	    std::string row1 = mgDB::escape_string( &m_db, std::string( row[1] ) );

	    if( m_view < 100 ) 
	      {
		new_restriction = m_restriction + " AND " 
		  + idfield + "='" + row1 + "'";
	      } 
	    else 
	      {
		new_restriction = idfield + "='" + row1 + "'";
	      }
	    
	    new_child = new GdTreeNode(this, // parent
				       (std::string) idbuf, // id
				       // row[0], // label,
				       row0,
				       new_restriction);
	    m_children.push_back(new_child);
	    numchild++;
	  }
      } 
    else if (m_view <100) 
      {
	new_child = new GdTreeNode(this, // parent
				   "1" , // id
				   tr("Artist -> Album -> Track"), // label,
				   m_restriction);
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "2" , // id
				   tr("Genre -> Artist -> Album -> Track") , // label,
				   m_restriction);
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "3" , // id
				   tr("Artist -> Track") , // label,
				   m_restriction);
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "4" , // id
				   tr("Genre -> Year -> Track") , // label,
				   m_restriction);
	m_children.push_back(new_child);
	new_child = new GdTreeNode(this, // parent
				   "5" , // id
				   tr("Album -> Track") , // label,
				   m_restriction);
	m_children.push_back(new_child);
      } 
    else 
      {
        new_child = new GdTreeNode(this, // parent
                                   "" , // id
                                   tr("Search Result"), // label,
                                   m_restriction);
        m_children.push_back(new_child);
      }
    
    m_expanded = true;
    mgDebug(5, "%d children expanded\n", m_children.size());
    return true;
}

/*!
 * \brief iterate all children recursively to find the tracks
 */
std::vector<mgContentItem*>* GdTreeNode::getTracks()
{
    MYSQL_ROW	row;
    MYSQL_RES	*result;
    int	nrows;
    int	nfields;
    std::vector<mgContentItem*>* retlist;
    int trackid;

    retlist = new std::vector<mgContentItem*>();
    
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


/*!
 *****************************************************************************
 * \brief returns the first track matchin the restrictions of this node
 * assuming we are in a leaf node, this returns the track represented by the
 * the leaf 
 ****************************************************************************/
mgContentItem* GdTreeNode::getSingleTrack()
{
    MYSQL_ROW	row;
    MYSQL_RES	*result;
    int	nrows;
    int	nfields;
    mgContentItem* track = NULL;
    int trackid;

    // get all tracks satisying the restrictions of this node
    mgDebug(5, "getTracks(): query '%s'", m_restriction.c_str());
    
    result = mgSqlReadQuery(&m_db,
			"SELECT  tracks.id FROM tracks, album, genre WHERE %s"
	       " AND album.cddbid=tracks.sourceid AND genre.id=tracks.genre1", 
			    m_restriction.c_str());
    nrows   = mysql_num_rows(result);
    nfields = mysql_num_fields(result);
    
    if( nrows != 1 )
    {
	mgWarning( "GdTreeNode::getSingleTrack() :SQL call returned %d tracks, using only the first",
		   nrows );
    }
    // get the first row 
    if( ( row = mysql_fetch_row(result)) != NULL )
    { 
	// row[0] is the trackid
	if(sscanf(row[0], "%d", &trackid) != 1)
	{
	    mgError("Can not extract integer track id from '%s'",
		    row[0]);
	}
	track = new mgGdTrack(trackid, m_db);
    }
    return  track;
}

