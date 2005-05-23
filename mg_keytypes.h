/*! \file  mg_keytypes.h
 *  \ingroup muggle
 *  \brief  key type definitions for all backends
 *
 * \version $Revision: 1.4 $
 * \author  Wolfgang Rohdewald
 * \author  file owner: $Author: wr61 $
 *
 */

/* makes sure we don't use the same declarations twice */
#ifndef _MUGGLE_KEYTYPES_H
#define _MUGGLE_KEYTYPES_H

enum mgKeyTypes {
	keyGdGenre1=1, // the genre types must have exactly this order!
	keyGdGenre2,
	keyGdGenre3,
	keyGdGenres,
	keyGdDecade,
	keyGdYear,
	keyGdArtist,
	keyGdAlbum,
	keyGdTitle,
	keyGdTrack,
	keyGdLanguage,
	keyGdRating,
	keyGdFolder1,
	keyGdFolder2,
	keyGdFolder3,
	keyGdFolder4,
	keyGdCreated,
	keyGdModified,
	keyGdArtistABC,
	keyGdTitleABC,
	keyGdCollection,
	keyGdCollectionItem,
	// here come other backends: keyXXTitle = 50, (reserve)
	keyTypesHigh
};
const mgKeyTypes mgGdKeyTypesLow = keyGdGenre1;
const mgKeyTypes mgGdKeyTypesHigh = keyGdCollectionItem;
const mgKeyTypes mgKeyTypesLow = keyGdGenre1;
const mgKeyTypes mgKeyTypesHigh = keyGdCollectionItem;

//! \brief at least the number of enum elements:
const unsigned int mgKeyTypesNr = keyGdCollectionItem;

enum mgSortBy {
	mgSortNone,
	mgSortByValue,
	mgSortById,
	mgSortByIdNum
};

#endif
