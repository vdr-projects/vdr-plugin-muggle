/*! \file  mg_tools.h
 *  \ingroup muggle
 *  \brief  A few utility functions for standalone and plugin messaging for the vdr muggle plugindatabase
 *
 * \version $Revision: 1.4 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 *
 */

/* makes sure we don't use the same declarations twice */
#ifndef _MUGGLE_TOOLS_H
#define _MUGGLE_TOOLS_H

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

/*!
 * \brief Logging utilities
 *
 * \todo these could be static members in the mgLog class
 * \todo code of these functions should be compiled conditionally
 */
//@{
void mgSetDebugLevel (int new_level);
void mgDebug (int level, const char *fmt, ...);
void mgDebug (const char *fmt, ...);
void mgWarning (const char *fmt, ...);
//! \todo mgError should display the message on the OSD. How?
void mgError (const char *fmt, ...); 
//@}

#ifdef DEBUG
#define MGLOG(x) mgLog __thelog(x)
#else
#define MGLOG(x) {}
#endif

#ifdef DEBUG
#define MGLOGSTREAM __thelog.getStream()
#else
#define MGLOGSTREAM __thelog.getStream()
#endif

/*! \brief simplified logging class
 *  \ingroup muggle
 *
 *  Create a local instance at the beginning of the method
 *  and entering/leaving the function will be logged
 *  as constructors/destructors are called.
 */
class mgLog
{
    public:
        enum
        {
            LOG, WARNING, ERROR, FATAL
        } mgLogLevel;

        ostream & getStream ()
        {
            return std::cout;
        }

        mgLog (string methodname):m_methodname (methodname)
        {
            getStream () << m_methodname << " entered" << std::endl;
        };

        ~mgLog ()
        {
            getStream () << m_methodname << " terminated" << std::endl;
        }

    private:

        string m_methodname;

};

string trim(string const& source, char const* delims = " \t\r\n");

char *SeparateFolders(const char *filename, char * folders[],unsigned int fcount);

enum mgKeyTypes {
	keyGenre1=1, // the genre types must have exactly this order!
	keyGenre2,
	keyGenre3,
	keyGenres,
	keyDecade,
	keyYear,
	keyArtist,
	keyAlbum,
	keyTitle,
	keyTrack,
	keyLanguage,
	keyRating,
	keyFolder1,
	keyFolder2,
	keyFolder3,
	keyFolder4,
	keyCreated,
	keyModified,
	keyArtistABC,
	keyTitleABC,
	keyCollection,
	keyCollectionItem,
};
const mgKeyTypes mgKeyTypesLow = keyGenre1;
const mgKeyTypes mgKeyTypesHigh = keyCollectionItem;
const unsigned int mgKeyTypesNr = keyCollectionItem;

//! \brief adds string n to string s, using string sep to separate them
string& addsep (string & s, string sep, string n);

//! \brief converts long to string
string itos (int i);

//! \brief convert long to string
string ltos (long l);

char *extension (const char *filename);
#endif                                            /*  _MUGGLE_TOOLS_H */
