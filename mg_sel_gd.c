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
#include <assert.h>

#include "mg_sel_gd.h"
# include "mg_db.h"
#include <vdr/i18n.h>

mgSelectionGd::mgSelectionGd(const mgSelection *s) {
	InitFrom(s);
}

mgSelectionGd::mgSelectionGd(const bool fall_through)
: mgSelection(fall_through) {
}

void mgSelectionGd::InitSelection() {
	mgSelection::InitSelection();
	m_db = GenerateDB();
}

static bool iskeyGdGenre(mgKeyTypes kt) {
	return kt>=keyGdGenre1  && kt <= keyGdGenres;
}

bool
mgSelectionGd::DeduceKeyValue(mgKeyTypes new_kt,const mgSelection *s,
vector<mgListItem>& items) {
	if (!s)
		return false;
	for (unsigned int i=0;i<s->ordersize();i++) {
		mgKeyTypes old_kt = s->getKeyType(i);
		if (old_kt>new_kt
			&& iskeyGdGenre(old_kt)
		&& iskeyGdGenre(new_kt)) {
			string selid=KeyMaps.id(new_kt,
				KeyMaps.value(new_kt,s->getKeyItem(i)->id()));
			items.push_back(mgListItem(
				KeyMaps.value(new_kt,selid),selid));
			return true;
		}
	}
	return false;
}

void
mgSelectionGd::clean() {
	mgSelection::clean();
	keyvector::iterator i;
	keyvector::iterator j;
	bool collection_found = false;
	bool collitem_found = false;
	bool album_found = false;
	bool tracknb_found = false;
	bool title_found = false;
	bool is_sufficiently_unique = false;
	for (i = Keys.begin () ; i != Keys.end (); ++i) {
		mgKey* k = *i;
		if (k->Type()==keyGdUnique) {
			truncate(i-Keys.begin());
			break;
		}
		collection_found |= (k->Type()==keyGdCollection);
		collitem_found |= (k->Type()==keyGdCollectionItem);
		album_found |= (k->Type()==keyGdAlbum);
		tracknb_found |= (k->Type()==keyGdTrack);
		title_found |= (k->Type()==keyGdTitle);
		is_sufficiently_unique = tracknb_found || (album_found && title_found)
			|| (collection_found && collitem_found);
		if (is_sufficiently_unique) {
			truncate (i+1-Keys.begin());
			break;
		}
		if (k->Type()==keyGdYear) {
			for (j = i+1 ; j != Keys.end(); ++j)
			if ((*j)->Type() == keyGdDecade) {
				delete *j;
				Keys.erase(j);
				break;
			}
		}
	}
	if (!is_sufficiently_unique) {
		if (!album_found)
			Keys.push_back(ktGenerate(keyGdAlbum));
		if (!title_found)
			Keys.push_back(ktGenerate(keyGdTitle));
	}
								 // make sure we ALWAYS have a unique key
	Keys.push_back(ktGenerate(keyGdUnique));
}

vector <const char *>
mgSelectionGd::Choices(unsigned int level, unsigned int *current) const
{
	vector<const char*> result;
	if (level>ordersize()) {
		*current = 0;
		return result;
	}
	for (unsigned int ki=(unsigned int)(ktLow());ki<=(unsigned int)(ktHigh());ki++) {
		mgKeyTypes kt = mgKeyTypes(ki);
		if (kt == keyGdUnique)
			continue;
		if (kt==getKeyType(level)) {
			*current = result.size();
			result.push_back(ktName(kt));
			continue;
		}
		if (UsedBefore(kt,level))
			continue;
		if (kt==keyGdDecade) {
			if (UsedBefore(keyGdYear,level)) continue;
		}
		if (kt==keyGdGenre1) {
			if (UsedBefore(keyGdGenre2,level)) continue;
			if (UsedBefore(keyGdGenre3,level)) continue;
			if (UsedBefore(keyGdGenres,level)) continue;
		}
		if (kt==keyGdGenre2) {
			if (UsedBefore(keyGdGenre3,level)) continue;
			if (UsedBefore(keyGdGenres,level)) continue;
		}
		if (kt==keyGdGenre3) {
			if (UsedBefore(keyGdGenres,level)) continue;
		}
		if (kt==keyGdFolder1) {
			if (UsedBefore(keyGdFolder2,level)) continue;
			if (UsedBefore(keyGdFolder3,level)) continue;
			if (UsedBefore(keyGdFolder4,level)) continue;
		}
		if (kt==keyGdFolder2) {
			if (UsedBefore(keyGdFolder3,level)) continue;
			if (UsedBefore(keyGdFolder4,level)) continue;
		}
		if (kt==keyGdFolder3) {
			if (UsedBefore(keyGdFolder4,level)) continue;
		}
		if (kt==keyGdCollectionItem) {
			if (!UsedBefore(keyGdCollection,level)) continue;
		}
		if (kt==keyGdCollection)
			result.push_back(ktName(kt));
		else if (keycount(kt)>1)
			result.push_back(ktName(kt));
	}
	return result;
}

bool
mgSelectionGd::NeedKey(unsigned int i) const
{
	assert(m_level<ordersize());
	mgKey *k = Keys[i];
	mgKeyTypes kt = k->Type();
	for (unsigned int j=i+1;j<=m_level;j++) {
		mgKey *kn = Keys[j];
		if (kn && !kn->id().empty()) {
			mgKeyTypes knt = kn->Type();
			if (knt==keyGdUnique)
				return false;
			if (iskeyGdGenre(kt) && iskeyGdGenre(knt) && knt>kt)
				return false;
			if (kt==keyGdDecade && knt==keyGdYear)
				return false;
		}
	}
	return true;
}

mgParts
mgSelectionGd::SelParts(bool distinct, bool deepsort) const
{
	if (distinct && !deepsort && m_level==0 &&  isCollectionOrder()) {
		mgParts result;
		result.orderByCount = m_orderByCount;
		// sql command contributed by jarny
		result.special_statement = string("SELECT COUNT(*) * "
			"CASE WHEN playlistitem.playlist IS NULL THEN 0 ELSE 1 END, "
			"1,playlist.id,playlist.title "
			" FROM playlist "
			"LEFT JOIN playlistitem ON playlist.id = playlistitem.playlist "
			"GROUP BY playlist.title,playlistitem.playlist,playlist.id");
		return result;
	}
	else
		return mgSelection::SelParts(distinct, deepsort);
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
	switch (kt) {
		case keyGdGenres: result = tr("Genre");break;
		case keyGdGenre1: result = tr("Genre1");break;
		case keyGdGenre2: result = tr("Genre2");break;
		case keyGdGenre3: result = tr("Genre3");break;
		case keyGdFolder1: result = tr("Folder1");break;
		case keyGdFolder2: result = tr("Folder2");break;
		case keyGdFolder3: result = tr("Folder3");break;
		case keyGdFolder4: result = tr("Folder4");break;
		case keyGdArtist: result = tr("Artist");break;
		case keyGdArtistABC: result = tr("ArtistABC");break;
		case keyGdTitle: result = tr("Title");break;
		case keyGdTitleABC: result = tr("TitleABC");break;
		case keyGdTrack: result = tr("Track");break;
		case keyGdDecade: result = tr("Decade");break;
		case keyGdAlbum: result = tr("Album");break;
		case keyGdCreated: result = tr("Created");break;
		case keyGdModified: result = tr("Modified");break;
		case keyGdCollection: result = tr("Collection");break;
		case keyGdCollectionItem: result = tr("Collection item");break;
		case keyGdLanguage: result = tr("Language");break;
		case keyGdRating: result = tr("Rating");break;
		case keyGdYear: result = tr("Year");break;
		case keyGdUnique: result = tr("ID");break;
		default: result=tr("not implemented");break;
	}
	return result;
}

void
mgSelectionGd::MakeCollection() {
	clear();
	setKey(keyGdCollection);
	setKey(keyGdCollectionItem);
	setKey(keyGdUnique);
}

bool
mgSelectionGd::isCollectionOrder() const
{
	return (ordersize()==3
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
	for (unsigned int idx = 0 ; idx <= orderlevel(); idx++) {
		if (idx==ordersize()) break;
		if (getKeyType(idx) == keyGdCollection)
			if (!Name.empty() && getKeyItem(idx)->value() != Name)
				break;
		if (getKeyType(idx) == keyGdCollectionItem) {
			result = true;
			break;
		}
	}
	return result;
}

bool
mgSelectionGd::InitDefaultOrder(unsigned int i) {
	clear();
	switch (i) {
		case 1:
			setKey(keyGdArtist);
			setKey(keyGdAlbum);
			setKey(keyGdTrack);
			break;
		case 2:
			setKey(keyGdAlbum);
			setKey(keyGdTrack);
			break;
		case 3:
			setKey(keyGdGenres);
			setKey(keyGdArtist);
			setKey(keyGdAlbum);
			setKey(keyGdTrack);
			break;
		case 4:
			setKey(keyGdArtist);
			setKey(keyGdTrack);
			break;
		default:
			return false;
	}
	setKey(keyGdUnique);
	return true;
}
