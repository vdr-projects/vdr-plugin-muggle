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
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fts.h>
#include <assert.h>


#include "mg_sel_gd.h"
#include "mg_db_gd.h"

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
				&& iskeyGenre(old_kt)
				&& iskeyGenre(new_kt))
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
		collection_found |= (k->Type()==keyCollection);
		collitem_found |= (k->Type()==keyCollectionItem);
		album_found |= (k->Type()==keyAlbum);
		tracknb_found |= (k->Type()==keyTrack);
		title_found |= (k->Type()==keyTitle);
		is_unique = tracknb_found || (album_found && title_found)
			|| (collection_found && collitem_found);
		if (is_unique)
		{
			truncate (i+1-Keys.begin()); // \todo test this!
			break;
		}
		if (k->Type()==keyYear)
		{
			for (j = i+1 ; j != Keys.end(); ++j)
				if ((*j)->Type() == keyDecade)
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
			Keys.push_back(ktGenerate(keyAlbum));
		if (!title_found)
			Keys.push_back(ktGenerate(keyTitle));
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
	for (unsigned int ki=int(mgKeyTypesLow);ki<=int(mgKeyTypesHigh);ki++)
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
		if (kt==keyDecade && UsedBefore(keyYear,level))
			continue;
		if (kt==keyGenre1)
		{
			if (UsedBefore(keyGenre2,level)) continue;
			if (UsedBefore(keyGenre3,level)) continue;
			if (UsedBefore(keyGenres,level)) continue;
		}
		if (kt==keyGenre2)
		{
			if (UsedBefore(keyGenre3,level)) continue;
			if (UsedBefore(keyGenres,level)) continue;
		}
		if (kt==keyGenre3)
		{
			if (UsedBefore(keyGenres,level)) continue;
		}
		if (kt==keyFolder1)
		{
		 	if (UsedBefore(keyFolder2,level)) continue;
		 	if (UsedBefore(keyFolder3,level)) continue;
		 	if (UsedBefore(keyFolder4,level)) continue;
		}
		if (kt==keyFolder2)
		{
		 	if (UsedBefore(keyFolder3,level)) continue;
		 	if (UsedBefore(keyFolder4,level)) continue;
		}
		if (kt==keyFolder3)
		{
		 	if (UsedBefore(keyFolder4,level)) continue;
		}
		if (kt==keyCollectionItem)
		{
		 	if (!UsedBefore(keyCollection,level)) continue;
		}
		if (kt==keyCollection)
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
		result.m_sql_select = string("select playlist.title,playlist.id, "
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
		if (iskeyGenre(kt))
		{
			for (unsigned int j=i+1;j<=m_level;j++)
			{
				if (j>=Keys.size())
					break;
				mgKey *kn = Keys[j];
				if (kn)
				{
					mgKeyTypes knt = kn->Type();
					if (iskeyGenre(knt) && knt>kt && !kn->id().empty())
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

