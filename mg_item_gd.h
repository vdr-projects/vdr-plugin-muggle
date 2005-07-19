/*!
 * \file mg_item.h
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_ITEM_GD_H
#define _MG_ITEM_GD_H
#include <stdlib.h>
#include <string>

using namespace std;

#include "mg_tools.h"
#include "mg_item.h"

//! \brief represents a GiantDisc content item 
class mgItemGd : public mgItem
{
    public:
	mgListItem* getKeyItem(mgKeyTypes kt) const;

	//! \brief copy constructor
        mgItemGd(const mgItemGd* c);

	//! \brief construct an item from an SQL row
        mgItemGd (char **row);

	mgItemGd* Clone();

//! \brief returns filename
        string getSourceFile (bool AbsolutePath=true,bool Silent=false) const;

//! \brief returns artist
        string getArtist () const;

//! \brief returns the name of the album
        string getAlbum () const;

//! \brief returns the name of genre
        string getGenre () const;

//! \brief returns the bitrate
        string getBitrate () const;

//! \brief returns samplerate
        int getSampleRate () const;

//! \brief returns # of channels
        int getChannels () const;

//! \brief returns tracknb
        int getTrack () const;

	string getImagePath(bool AbsolutePath=true) const;

    protected:
	void InitFrom(const mgItemGd* c);
    private:
        mutable string m_mp3file;
        string m_artist;
        string m_albumtitle;
        string m_genre2_id;
        string m_genre2;
        string m_bitrate;
	mutable string m_coverimg;
        int m_samplerate;
        int m_channels;
	int m_tracknb;
	mutable int m_covercount;
};

#endif
