/*******************************************************************/
/*! \file  muggle_tools.h
 * \brief  A few util functions for standalone and plugin messaging
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.3 $
 * \date    $Date: 2004/05/28 15:29:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: lvw $
 * 
 */
/*******************************************************************/
/* makes sur we dont use parse the same declarations twice */
#ifndef _MUGGLE_TOOLS_H
#define _MUGGLE_TOOLS_H

#include <iostream>
#include <string>

#include "mysql/mysql.h"

#define STANDALONE 1

void mgSetDebugLevel(int new_level);
void mgDebug(int level, const char *fmt, ...);
void mgDebug( const char *fmt, ... );
void mgWarning(const char *fmt, ...);
void mgError(const char *fmt, ...);

MYSQL_RES* mgSqlReadQuery(MYSQL *db, const char *fmt, ...);
void mgSqlWriteQuery(MYSQL *db, const char *fmt, ...);

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


class mgLog
{
 public:
  enum
    {
      LOG, WARNING, ERROR, FATAL
    } mgLogLevel;

  std::ostream& getStream()
    {
      return std::cout;
    }
  
  mgLog( std::string methodname ) : m_methodname( methodname ) 
    {
      getStream() << m_methodname << " entered" << std::endl;
    };

  ~mgLog()
    {
      getStream() << m_methodname << " terminated" << std::endl;
    }

 private:
  
  std::string m_methodname;

};

/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_tools.h,v $
 * Revision 1.3  2004/05/28 15:29:18  lvw
 * Merged player branch back on HEAD branch.
 *
 * Revision 1.2.2.2  2004/04/18 14:08:41  lvw
 * Added some more logging/debugging code
 *
 * Revision 1.2.2.1  2004/04/09 16:14:48  lvw
 * Added further code for logging/debugging.
 *
 * Revision 1.2  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */
#endif /*  _MUGGLE_TOOLS_H */
