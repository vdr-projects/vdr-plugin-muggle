/*!
 * \file mg_item_gd.c
 * \brief An interface to GiantDisc data items
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
#include "mg_item_gd.h"
#include "mg_setup.h"
#include "mg_tools.h"
#include "mg_db.h"

// this one last because of swap() redefinition:
#include <tools.h>

static bool gd_music_dir_exists[100];
static bool gd_music_dirs_scanned=false;

mgListItem*
mgItemGd::getKeyItem(mgKeyTypes kt)
{
	string val;
	string id;
	if (m_itemid>=0) 
	{
		switch (kt) {
			case keyGdGenres:
			case keyGdGenre1:
			case keyGdGenre2:
			case keyGdGenre3: val = getGenre();id=m_genre_id;break;
			case keyGdArtist: val = id = getArtist();break;
			case keyGdArtistABC: val = id = getArtist()[0];break;
			case keyGdAlbum: val = id = getAlbum();break;
			case keyGdYear: val = id = string(ltos(getYear()));break;
			case keyGdDecade: val = id = string(ltos(int((getYear() % 100) / 10) * 10));break;
			case keyGdTitle: val = id = getTitle();break;
			case keyGdTitleABC: val = id = getTitle()[0];break;
			case keyGdTrack: val = id = getTitle();break;
			case keyGdLanguage: val = getLanguage();id=m_language_id ; break;
			case keyGdRating: val = id = getRating();break;
			case keyGdFolder1:
			case keyGdFolder2:
			case keyGdFolder3:
			case keyGdFolder4: 
				{
				       char *folders[4];
				       char *fbuf=SeparateFolders(m_mp3file.c_str(),folders,4);
				       val = id = folders[int(kt)-int(keyGdFolder1)];
				       free(fbuf);
				       break;
				}
			default: return new mgListItem;
		}
	}
	return new mgListItem(val,id);
}


string mgItemGd::getGenre () const
{
    string result="";
    if (m_genre!="NULL")
	    result = m_genre;
    if (m_genre2!="NULL" && m_genre2.size()>0)
    {
	    if (!result.empty())
		    result += "/";
	    result += m_genre2;
    }
    return result;
}


string mgItemGd::getBitrate () const
{
    return m_bitrate;
}


string mgItemGd::getImageFile () const
{
    return "Name of Imagefile";
}


string mgItemGd::getArtist () const
{
    return m_artist;
}


string mgItemGd::getAlbum () const
{
    return m_albumtitle;
}


int mgItemGd::getSampleRate () const
{
    return m_samplerate;
}


int mgItemGd::getChannels () const
{
    return m_channels;
}

mgItemGd::mgItemGd(const mgItemGd* c)
{
     InitFrom(c);
}

mgItemGd*
mgItemGd::Clone ()
{
    if (!this)
	    return 0;
    else
	    return new mgItemGd(this);
}

void
mgItemGd::InitFrom(const mgItemGd* c)
{
    mgItem::InitFrom(c);
    m_mp3file = c->m_mp3file;
    m_artist = c->m_artist;
    m_albumtitle = c->m_albumtitle;
    m_genre2_id = c->m_genre2_id;
    m_genre2 = c->m_genre2;
    m_bitrate = c->m_bitrate;
    m_samplerate = c->m_samplerate;
    m_channels = c->m_channels;
}


string
mgItemGd::getSourceFile(bool AbsolutePath) const
{
	int access_errno=0;
	string tld = the_setup.ToplevelDir;
    	string result = m_mp3file;
	if (m_validated && !m_valid)
		return m_mp3file;
	if (!readable(tld+"/"+result))
	{
		access_errno=errno;
		result="";
		if (!gd_music_dirs_scanned)
		{
			for (unsigned int i =0 ; i < 100 ; i++)
			{
				struct stat stbuf;
				char *dir;
				asprintf(&dir,"%s%02d",tld.c_str(),i);
				gd_music_dir_exists[i]=!stat(dir,&stbuf);
				free(dir);
			}
			gd_music_dirs_scanned=true;
		}
		for (unsigned int i =0 ; i < 100 ; i++)
		{
			if (!gd_music_dir_exists[i])
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
	m_validated=true;
	if (result.empty())
	{
		analyze_failure(tld + "/" + m_mp3file);
		m_valid = false;
		return m_mp3file;
	}	
	if (AbsolutePath)
		result = tld +"/" + result;
	return result;
}

mgItemGd::mgItemGd (char **row)
{
    m_valid = true;
    m_validated = false;
    m_itemid = atol (row[0]);
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
    if (row[5] && row[5][0])
	m_genre_id = row[5];
    else
    	m_genre_id = "NULL";
    m_genre = KeyMaps.value(keyGdGenres,m_genre_id);
    if (row[6] && row[6][0])
	m_genre2_id = row[6];
    else
    	m_genre2_id = "NULL";
    m_genre2 = KeyMaps.value(keyGdGenres,m_genre2_id);
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
    	m_language_id = row[13];
    else
    	m_language_id = "NULL";
     m_language = KeyMaps.value(keyGdLanguage,m_language_id);
};

