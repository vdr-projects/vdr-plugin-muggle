/*!
 * \file mg_selection.c
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "i18n.h"
#include "mg_selection.h"
#include "mg_setup.h"
#include "mg_tools.h"
#include "mg_keymaps.h"


mgListItem zeroitem;

mgListItem::mgListItem()
{
	m_valid=false;
	m_count=0;
}

mgListItem::mgListItem(string v,string i,unsigned int c)
{
	set(v,i,c);
}

void
mgListItem::set(string v,string i,unsigned int c)
{
	m_valid=true;
	m_value=v;
	m_id=i;
	m_count=c;
}

void 
mgListItem::operator=(const mgListItem& from)
{
	m_valid=from.m_valid;
	m_value=from.m_value;
	m_id=from.m_id;
	m_count=from.m_count;
}

void
mgListItem::operator=(const mgListItem* from)
{
	m_valid=from->m_valid;
	m_value=from->m_value;
	m_id=from->m_id;
	m_count=from->m_count;
}

bool
mgListItem::operator==(const mgListItem& other) const
{
	return m_value == other.m_value
		&& m_id == other.m_id;
}

mgListItem*
mgContentItem::getKeyItem(mgKeyTypes kt)
{
	string val;
	string id;
	if (m_trackid>=0) 
	{
		switch (kt) {
			case keyGenres:
			case keyGenre1:
			case keyGenre2:
			case keyGenre3: val = getGenre();id=m_genre1_id;break;
			case keyArtist: val = id = getArtist();break;
			case keyArtistABC: val = id = getArtist()[0];break;
			case keyAlbum: val = id = getAlbum();break;
			case keyYear: val = id = string(ltos(getYear()));break;
			case keyDecade: val = id = string(ltos(int((getYear() % 100) / 10) * 10));break;
			case keyTitle: val = id = getTitle();break;
			case keyTitleABC: val = id = getTitle()[0];break;
			case keyTrack: val = id = getTitle();break;
			case keyLanguage: val = getLanguage();id=m_language_id ; break;
			case keyRating: val = id = getRating();break;
			case keyFolder1:
			case keyFolder2:
			case keyFolder3:
			case keyFolder4: 
				{
				       char *folders[4];
				       char *fbuf=SeparateFolders(m_mp3file.c_str(),folders,4);
				       val = id = folders[int(kt)-int(keyFolder1)];
				       free(fbuf);
				       break;
				}
			default: return new mgListItem;
		}
	}
	return new mgListItem(val,id);
}


string mgContentItem::getGenre () const
{
    string result="";
    if (m_genre1!="NULL")
	    result = m_genre1;
    if (m_genre2!="NULL")
    {
	    if (!result.empty())
		    result += "/";
	    result += m_genre2;
    }
    return result;
}


string mgContentItem::getLanguage() const
{
    return m_language;
}

string mgContentItem::getBitrate () const
{
    return m_bitrate;
}


string mgContentItem::getImageFile () const
{
    return "Name of Imagefile";
}


string mgContentItem::getAlbum () const
{
    return m_albumtitle;
}


int mgContentItem::getYear () const
{
    return m_year;
}


int mgContentItem::getRating () const
{
    return m_rating;
}


int mgContentItem::getDuration () const
{
    return m_duration;
}


int mgContentItem::getSampleRate () const
{
    return m_samplerate;
}


int mgContentItem::getChannels () const
{
    return m_channels;
}

mgContentItem::mgContentItem ()
{
    m_trackid = -1;
    m_valid = true;
}

mgContentItem::mgContentItem (const mgContentItem* c)
{
    m_trackid = c->m_trackid;
    m_title = c->m_title;
    m_mp3file = c->m_mp3file;
    m_artist = c->m_artist;
    m_albumtitle = c->m_albumtitle;
    m_genre1_id = c->m_genre1_id;
    m_genre2_id = c->m_genre2_id;
    m_genre1 = c->m_genre1;
    m_genre2 = c->m_genre2;
    m_language = c->m_language;
    m_language_id = c->m_language_id;
    m_bitrate = c->m_bitrate;
    m_year = c->m_year;
    m_rating = c->m_rating;
    m_duration = c->m_duration;
    m_samplerate = c->m_samplerate;
    m_channels = c->m_channels;
}


static bool music_dir_exists[100];
static bool music_dirs_scanned=false;

bool
mgContentItem::readable(string filename) const
{
	return !access(filename.c_str(),R_OK);
}

string
mgContentItem::getSourceFile(bool AbsolutePath) const
{
	string tld = the_setup.ToplevelDir;
    	string result = m_mp3file;
	if (!Valid())
		goto not_valid;
	if (!readable(tld+result))
	{
		result.clear();
		if (!music_dirs_scanned)
		{
			for (unsigned int i =0 ; i < 100 ; i++)
			{
				struct stat stbuf;
				char *dir;
				asprintf(&dir,"%s%02d",tld.c_str(),i);
				music_dir_exists[i]=!stat(dir,&stbuf);
				free(dir);
			}
			music_dirs_scanned=true;
		}
		for (unsigned int i =0 ; i < 100 ; i++)
		{
			if (!music_dir_exists[i])
				continue;
			char *file;
			asprintf(&file,"%02d/%s",i,m_mp3file.c_str());
			if (readable(tld+file))
			{
				m_mp3file = file;
				result = m_mp3file;
			}
			free(file);
			if (!result.empty())
				break;
		}
	}
	if (result.empty())
	{
		m_valid = false;
not_valid:
		return m_mp3file;
	}	
	if (AbsolutePath)
		result = tld + result;
	return result;
}

mgContentItem::mgContentItem (const MYSQL_ROW row)
{
    m_trackid = atol (row[0]);
    if (row[1])
	m_title = row[1];
    else
	m_title = "NULL";
    if (row[2])
    	m_mp3file = row[2];
    else
    	m_mp3file = "NULL";
    if (row[3])
    	m_artist = row[3];
    else
    	m_artist = "NULL";
    if (row[4])
    	m_albumtitle = row[4];
    else
    	m_albumtitle = "NULL";
    if (row[5])
    {
	m_genre1_id = row[5];
    	m_genre1 = KeyMaps.value(keyGenres,row[5]);
    }
    else
    	m_genre1 = "NULL";
    if (row[6])
    {
	m_genre2_id = row[6];
    	m_genre2 = KeyMaps.value(keyGenres,row[6]);
    }
    else
    	m_genre2 = "NULL";
    if (row[7])
    	m_bitrate = row[7];
    else
    	m_bitrate = "NULL";
    if (row[8])
    	m_year = atol (row[8]);
    else
    	m_year = 0;
    if (row[9])
        m_rating = atol (row[9]);
    else
        m_rating = 0;
    if (row[10])
    	m_duration = atol (row[10]);
    else
    	m_duration = 0;
    if (row[11])
    	m_samplerate = atol (row[11]);
    else
    	m_samplerate = 0;
    if (row[12])
    	m_channels = atol (row[12]);
    else
    	m_channels = 0;
    if (row[13])
    {
    	m_language_id = row[13];
	m_language = KeyMaps.value(keyLanguage,row[13]);
    }
    else
    	m_language_id = "NULL";
};

