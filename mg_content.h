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
#include <i18n.h>
using namespace std;

#include "mg_tools.h"
#include "mg_valmap.h"
#include "mg_order.h"

typedef vector<string> strvector;


class mgSelection;

//! \brief represents a content item like an mp3 file.
class mgContentItem
{
    public:
        mgContentItem ();

	string getKeyValue(mgKeyTypes kt);
	string getKeyId(mgKeyTypes kt);

	//! \brief copy constructor
        mgContentItem(const mgContentItem* c);

	//! \brief construct an item from an SQL row
        mgContentItem (const mgSelection* sel, const MYSQL_ROW row);
//! \brief returns track id
        long getId () const
        {
            return m_id;
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
        long m_id;
        string m_title;
        string m_mp3file;
        string m_artist;
        string m_albumtitle;
        string m_genre1_id;
        string m_genre2_id;
        string m_genre1;
        string m_genre2;
        string m_bitrate;
        int m_year;
        int m_rating;
        int m_duration;
        int m_samplerate;
        int m_channels;
};

#endif
