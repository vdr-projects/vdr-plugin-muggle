/*!
 * \file mg_selection.h
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_CONTENT_H
#define _MG_CONTENT_H
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string>
#include <list>
#include <vector>
#include <map>
using namespace std;

#include "mg_tools.h"
#include "mg_valmap.h"

typedef vector<string> strvector;


class mgSelection;

class mgListItem
{
	public:
		mgListItem();
		mgListItem(string v,string i,unsigned int c=0);
		void set(string v,string i,unsigned int c=0);
		void operator=(const mgListItem& from);
		void operator=(const mgListItem* from);
		bool operator==(const mgListItem& other) const;
		string value() const { return m_value; } 
		string id() const { return m_id; } 
		unsigned int count() const { return m_count; } 
		bool valid() const { return m_valid; }
	private:
		bool m_valid;
		string m_value;
		string m_id;
		unsigned int m_count;
};

//! \brief represents a content item like an mp3 file.
class mgContentItem
{
    public:
        mgContentItem ();

	mgListItem* getKeyItem(mgKeyTypes kt);

	//! \brief copy constructor
        mgContentItem(const mgContentItem* c);

	//! \brief construct an item from an SQL row
        mgContentItem (const MYSQL_ROW row);
//! \brief returns track id
        long getTrackid () const
        {
            return m_trackid;
        }

//! \brief returns title
        string getTitle () const
        {
            return m_title;
        }

//! \brief returns filename
        string getSourceFile (bool AbsolutePath=true) const;

//! \brief returns artist
        string getArtist () const
        {
            return m_artist;
        }

//! \brief returns the name of the album
        string getAlbum () const;

//! \brief returns the name of genre
        string getGenre () const;

//! \brief returns the name of the language
        string getLanguage () const;

//! \brief returns the bitrate
        string getBitrate () const;

//! \brief returns the file name of the album image
        string getImageFile () const;

//! \brief returns year
        int getYear () const;

//! \brief returns rating
        int getRating () const;

//! \brief returns duration
        int getDuration () const;

//! \brief returns samplerate
        int getSampleRate () const;

//! \brief returns # of channels
        int getChannels () const;
        
    private:
        long m_trackid;
        string m_title;
        string m_mp3file;
        string m_artist;
        string m_albumtitle;
        string m_genre1_id;
        string m_genre2_id;
        string m_genre1;
        string m_genre2;
        string m_bitrate;
        string m_language_id;
        string m_language;
        int m_year;
        int m_rating;
        int m_duration;
        int m_samplerate;
        int m_channels;
};

extern mgListItem zeroitem;
#endif
