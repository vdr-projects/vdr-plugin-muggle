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

#include <stdio.h>
#include "i18n.h"
#include "mg_selection.h"
#include "vdr_setup.h"
#include "mg_tools.h"


string
mgContentItem::getKeyId(mgKeyTypes kt)
{
	if (m_id<0) 
		return "";
	switch (kt) {
		case keyGenres:
		case keyGenre1:
		case keyGenre2:
		case keyGenre3: return m_genre1_id;
		default: return getKeyValue(kt);
	}
}

string
mgContentItem::getKeyValue(mgKeyTypes kt)
{
	if (m_id<0) 
		return "";
	switch (kt) {
		case keyGenres:
		case keyGenre1:
		case keyGenre2:
		case keyGenre3: return getGenre();
		case keyArtist: return getArtist();
		case keyAlbum: return getAlbum();
		case keyYear: return string(ltos(getYear()));
		case keyDecade: return string(ltos(int((getYear() % 100) / 10) * 10));
		case keyTitle: return getTitle();
		case keyTrack: return getTitle();
		default: return "";
	}
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
    m_id = -1;
}

mgContentItem::mgContentItem (const mgContentItem* c)
{
    m_id = c->m_id;
    m_title = c->m_title;
    m_mp3file = c->m_mp3file;
    m_artist = c->m_artist;
    m_albumtitle = c->m_albumtitle;
    m_genre1_id = c->m_genre1_id;
    m_genre2_id = c->m_genre2_id;
    m_genre1 = c->m_genre1;
    m_genre2 = c->m_genre2;
    m_bitrate = c->m_bitrate;
    m_year = c->m_year;
    m_rating = c->m_rating;
    m_duration = c->m_duration;
    m_samplerate = c->m_samplerate;
    m_channels = c->m_channels;
}

static char *mg_readline(FILE *f)
{
  static char buffer[MAXPARSEBUFFER];
  if (fgets(buffer, sizeof(buffer), f) > 0) {
     int l = strlen(buffer) - 1;
     if (l >= 0 && buffer[l] == '\n')
        buffer[l] = 0;
     return buffer;
     }
  return 0;
}

static const char *FINDCMD = "cd '%s' 2>/dev/null && find -follow -name '%s' -print 2>/dev/null";

static string
GdFindFile( const char* tld, string mp3file )
{
  string result = "";
  char *cmd = 0;
  asprintf( &cmd, FINDCMD, tld, mp3file.c_str() );
  FILE *p = popen( cmd, "r" );
  if (p) 
    {
      char *s;
      if( (s = mg_readline(p) ) != 0) 
	  result = string(s);
      pclose(p);
    }

  free( cmd );

  return result;
}

string
mgContentItem::getSourceFile(bool AbsolutePath) const
{
	const char* tld = the_setup.ToplevelDir;
	string result="";
	if (AbsolutePath) result = tld;
	if (the_setup.GdCompatibility)
		result += GdFindFile(tld,m_mp3file);
	else
    		result += m_mp3file;
	return result;
}

mgContentItem::mgContentItem (const mgSelection* sel,const MYSQL_ROW row)
{
    m_id = atol (row[0]);
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
    	m_genre1 = sel->value(keyGenres,row[5]);
    }
    else
    	m_genre1 = "NULL";
    if (row[6])
    {
	m_genre2_id = row[6];
    	m_genre2 = sel->value(keyGenres,row[6]);
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
};
