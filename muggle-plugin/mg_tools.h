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
#include <string>
#include <mysql.h>

#define STANDALONE 1                              // what's this?

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

        std::ostream & getStream ()
        {
            return std::cout;
        }

        mgLog (std::string methodname):m_methodname (methodname)
        {
            getStream () << m_methodname << " entered" << std::endl;
        };

        ~mgLog ()
        {
            getStream () << m_methodname << " terminated" << std::endl;
        }

    private:

        std::string m_methodname;

};

std::string trim(std::string const& source, char const* delims = " \t\r\n");

#endif                                            /*  _MUGGLE_TOOLS_H */
