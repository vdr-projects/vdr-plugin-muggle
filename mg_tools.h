/*******************************************************************/
/*! \file  muggle_tools.h
 * \brief  A few util functions for standalone and plugin messaging
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/01 18:22:53 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 * 
 */
/*******************************************************************/
/* makes sur we dont use parse the same declarations twice */
#ifndef _MUGGLE_TOOLS_H
#define _MUGGLE_TOOLS_H

#include "mysql/mysql.h"

#define STANDALONE 1

void mgSetDebugLevel(int new_level);
void mgDebug(int level, const char *fmt, ...);
void mgDebug( const char *fmt, ... );
void mgWarning(const char *fmt, ...);
void mgError(const char *fmt, ...);


MYSQL_RES* mgSqlReadQuery(MYSQL *db, const char *fmt, ...);
void mgSqlWriteQuery(MYSQL *db, const char *fmt, ...);

#endif
