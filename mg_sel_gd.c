/*!
 * \file mg_sel_gd.c
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fts.h>
#include <assert.h>


#include "mg_sel_gd.h"
#include "mg_db_gd.h"

#include "i18n.h"

mgSelectionGd::mgSelectionGd(const mgSelection *s)
{
	InitFrom(s);
}

mgSelectionGd::mgSelectionGd(const bool fall_through)
	: mgSelection(fall_through)
{
}

void mgSelectionGd::InitSelection() {
	mgSelection::InitSelection();
	m_db = new mgDbGd;
}


static bool iskeyGdGenre(mgKeyTypes kt)
{
	return kt>=keyGdGenre1  && kt <= keyGdGenres;
}

void
mgSelectionGd::DeduceKeyValue(mgKeyTypes new_kt,const mgSelection *s,
		vector<mgListItem>& items)
{
	// \todo this is no generic code, move to mgSelectionGd
	if (!s) 
		return;
	for (unsigned int i=0;i<s->ordersize();i++)
	{
		mgKeyTypes old_kt = s->getKeyType(i);
		if (old_kt>new_kt
				&& iskeyGdGenre(old_kt)
				&& iskeyGdGenre(new_kt))
		{
			string selid=KeyMaps.id(new_kt,
				KeyMaps.value(new_kt,s->getKeyItem(i)->id()));
			items.push_back(mgListItem(
				KeyMaps.value(new_kt,selid),selid));
			break;
		}
	}
}




void
mgSelectionGd::clean()
{
	mgSelection::clean();
	keyvector::iterator i;
	keyvector::iterator j;
	bool collection_found = false;	
	bool collitem_found = false;	
	bool album_found = false;	
	bool tracknb_found = false;	
	bool title_found = false;	
	bool is_unique = false;
	for (i = Keys.begin () ; i != Keys.end (); ++i)
	{
		mgKey* k = *i;
		collection_found |= (k->Type()==keyGdCollection);
		collitem_found |= (k->Type()==keyGdCollectionItem);
		album_found |= (k->Type()==keyGdAlbum);
		tracknb_found |= (k->Type()==keyGdTrack);
		title_found |= (k->Type()==keyGdTitle);
		is_unique = tracknb_found || (album_found && title_found)
			|| (collection_found && collitem_found);
		if (is_unique)
		{
			truncate (i+1-Keys.begin()); // \todo test this!
			break;
		}
		if (k->Type()==keyGdYear)
		{
			for (j = i+1 ; j != Keys.end(); ++j)
				if ((*j)->Type() == keyGdDecade)
				{
					delete *j;
					Keys.erase(j);
					break;
				}
		}
	}
	if (!is_unique)
	{
		if (!album_found)
			Keys.push_back(ktGenerate(keyGdAlbum));
		if (!title_found)
			Keys.push_back(ktGenerate(keyGdTitle));
	}
}


vector <const char *>
mgSelectionGd::Choices(unsigned int level, unsigned int *current) const
{
	vector<const char*> result;
	if (level>ordersize())
	{
		*current = 0;
		return result;
	}
	for (unsigned int ki=(unsigned int)(ktLow());ki<=(unsigned int)(ktHigh());ki++)
	{
		mgKeyTypes kt = mgKeyTypes(ki);
		if (kt==getKeyType(level))
		{
			*current = result.size();
			result.push_back(ktName(kt));
			continue;
		}
		if (UsedBefore(kt,level))
			continue;
		if (kt==keyGdDecade)
		{
			if (UsedBefore(keyGdYear,level)) continue;
		}
		if (kt==keyGdGenre1)
		{
			if (UsedBefore(keyGdGenre2,level)) continue;
			if (UsedBefore(keyGdGenre3,level)) continue;
			if (UsedBefore(keyGdGenres,level)) continue;
		}
		if (kt==keyGdGenre2)
		{
			if (UsedBefore(keyGdGenre3,level)) continue;
			if (UsedBefore(keyGdGenres,level)) continue;
		}
		if (kt==keyGdGenre3)
		{
			if (UsedBefore(keyGdGenres,level)) continue;
		}
		if (kt==keyGdFolder1)
		{
		 	if (UsedBefore(keyGdFolder2,level)) continue;
		 	if (UsedBefore(keyGdFolder3,level)) continue;
		 	if (UsedBefore(keyGdFolder4,level)) continue;
		}
		if (kt==keyGdFolder2)
		{
		 	if (UsedBefore(keyGdFolder3,level)) continue;
		 	if (UsedBefore(keyGdFolder4,level)) continue;
		}
		if (kt==keyGdFolder3)
		{
		 	if (UsedBefore(keyGdFolder4,level)) continue;
		}
		if (kt==keyGdCollectionItem)
		{
		 	if (!UsedBefore(keyGdCollection,level)) continue;
		}
		if (kt==keyGdCollection)
			result.push_back(ktName(kt));
		else if (keycount(kt)>1)
			result.push_back(ktName(kt));
	}
	return result;
}


mgParts
mgSelectionGd::Parts(mgDb *db,bool orderby) const
{
	mgParts result;
	result.orderByCount = m_orderByCount;
	if (m_level==0 &&  isCollectionOrder())
	{
		// sql command contributed by jarny
		result.special_statement = string("select playlist.title,playlist.id, "
				"count(*) * (playlistitem.playlist is not null) from playlist "
				"left join playlistitem on playlist.id = playlistitem.playlist "
				"group by playlist.title");
		return result;
	}
	for (unsigned int i=0;i<=m_level;i++)
	{
		if (i==Keys.size()) break;
		mgKey *k = Keys[i];
		mgKeyTypes kt = k->Type();
		if (iskeyGdGenre(kt))
		{
			for (unsigned int j=i+1;j<=m_level;j++)
			{
				if (j>=Keys.size())
					break;
				mgKey *kn = Keys[j];
				if (kn)
				{
					mgKeyTypes knt = kn->Type();
					if (iskeyGdGenre(knt) && knt>kt && !kn->id().empty())
						goto next;
				}
			}
		}
		result += k->Parts(db,orderby && (i==m_level));
next:
		continue;
	}
	return result;
}

mgKeyTypes
mgSelectionGd::ktLow() const
{
	return mgGdKeyTypesLow;
}

mgKeyTypes
mgSelectionGd::ktHigh() const
{
	return mgGdKeyTypesHigh;
}

const char * const
mgSelectionGd::ktName(const mgKeyTypes kt) const
{
	const char * result = "";
	switch (kt)
	{
		case keyGdGenres: result = "Genre";break;
		case keyGdGenre1: result = "Genre1";break;
		case keyGdGenre2: result = "Genre2";break;
		case keyGdGenre3: result = "Genre3";break;
		case keyGdFolder1: result = "Folder1";break;
		case keyGdFolder2: result = "Folder2";break;
		case keyGdFolder3: result = "Folder3";break;
		case keyGdFolder4: result = "Folder4";break;
		case keyGdArtist: result = "Artist";break;
		case keyGdArtistABC: result = "ArtistABC";break;
		case keyGdTitle: result = "Title";break;
		case keyGdTitleABC: result = "TitleABC";break;
		case keyGdTrack: result = "Track";break;
		case keyGdDecade: result = "Decade";break;
		case keyGdAlbum: result = "Album";break;
		case keyGdCreated: result = "Created";break;
		case keyGdModified: result = "Modified";break;
		case keyGdCollection: result = "Collection";break;
		case keyGdCollectionItem: result = "Collection item";break;
		case keyGdLanguage: result = "Language";break;
		case keyGdRating: result = "Rating";break;
		case keyGdYear: result = "Year";break;
		default: result="not implemented";break;
	}
	return tr(result);
}

void
mgSelectionGd::MakeCollection()
{
	clear();
	setKey(keyGdCollection);
	setKey(keyGdCollectionItem);
}

bool
mgSelectionGd::isCollectionOrder() const
{
	return (ordersize()==2
		&& (Keys[0]->Type()==keyGdCollection)
		&& (Keys[1]->Type()==keyGdCollectionItem));
}

bool
mgSelectionGd::isLanguagelist() const
{
    return (getKeyType(0) == keyGdLanguage);
}

bool
mgSelectionGd::isCollectionlist () const
{
    if (ordersize()==0) return false;
    return (getKeyType(0) == keyGdCollection && orderlevel() == 0);
}

bool
mgSelectionGd::inCollection(const string Name) const
{
    bool result = false;
    for (unsigned int idx = 0 ; idx <= orderlevel(); idx++)
    {
	    if (idx==ordersize()) break;
	    if (getKeyType(idx) == keyGdCollection)
		    if (!Name.empty() && getKeyItem(idx)->value() != Name)
			    break;
	    if (getKeyType(idx) == keyGdCollectionItem)
	    {
		    result = true;
		    break;
	    }
    }
    return result;
}

bool
mgSelectionGd::InitDefaultOrder(unsigned int i)
{
	clear();
	switch (i) {
		case 1:
			setKey(keyGdArtist);
			setKey(keyGdAlbum);
			setKey(keyGdTrack);
			return true;
		case 2:
			setKey(keyGdAlbum);
			setKey(keyGdTrack);
			return true;
		case 3:
			setKey(keyGdGenres);
			setKey(keyGdArtist);
			setKey(keyGdAlbum);
			setKey(keyGdTrack);
			return true;
		case 4:
			setKey(keyGdArtist);
			setKey(keyGdTrack);
			return true;
		default:
			return false;
	}
}
