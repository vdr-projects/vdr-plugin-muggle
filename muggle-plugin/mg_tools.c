/*******************************************************************/
/*! \file  muggle_tools.cpp
 * \brief  A few util functions for standalone and plugin messaging
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.2 $
 * \date    $Date: 2004/02/02 22:48:04 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: MountainMan $
 * 
 */
/*******************************************************************/

#include "mg_tools.h"

extern "C"
{
  #include <stdarg.h>
  #include <stdio.h>
}
#include <stdlib.h>


#define  MAX_BUFLEN  1024
#define  MAX_QUERY_BUFLEN  1024
static char buffer[MAX_BUFLEN];
static char querybuf[MAX_QUERY_BUFLEN];

static int DEBUG_LEVEL=3;

void mgSetDebugLevel(int new_level)
{
  DEBUG_LEVEL = new_level;
}

void mgDebug(int level, const char *fmt, ...)
{
  
  va_list ap;
  if(level <= DEBUG_LEVEL)
  {
    va_start(ap, fmt);
  
    vsnprintf(buffer, MAX_BUFLEN-1, fmt, ap);
    if(STANDALONE)
    {
      fprintf(stderr, "dbg %d: %s\n", level, buffer);
    }
    else
    {
#if !STANDALONE
      isyslog( "%s\n", buffer);
#endif
    }
  }
  va_end(ap);
}

void mgDebug( const char *fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  mgDebug( 1, fmt, ap );
}


void mgWarning(const char *fmt, ...)
{
  
  va_list ap;

  va_start(ap, fmt);
  
  vsnprintf(buffer, MAX_BUFLEN-1, fmt, ap);
  if(STANDALONE)
  {
    fprintf(stderr, "warning: %s\n",  buffer);
  }
  else
  {
#if !STANDALONE
    isyslog( "Warning: %s\n", buffer);
#endif

  }

  va_end(ap);
}

void mgError(const char *fmt, ...)
{
  
  va_list ap;

  va_start(ap, fmt);
  
  vsnprintf(buffer, MAX_BUFLEN-1, fmt, ap);
  if(STANDALONE)
  {
    fprintf(stderr, "Error: %s\n", buffer);
    exit(1);
  }
  else
  {
#if !STANDALONE
    isyslog( "Error in Muggle: %s\n", buffer);
#endif
  }

  va_end(ap);
}

MYSQL_RES* mgSqlReadQuery(MYSQL *db, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
  
    vsnprintf(querybuf, MAX_QUERY_BUFLEN-1, fmt, ap);
    mgDebug(9, "SQL-Query: '%s'",querybuf); 
    if(mysql_query(db,querybuf))
    {
	mgError("SQL error in MUGGLE\n%s\n", querybuf);
    }
    return mysql_store_result(db);
}

void mgSqlWriteQuery(MYSQL *db, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
  
    vsnprintf(querybuf, MAX_QUERY_BUFLEN-1, fmt, ap);
    if(mysql_query(db,querybuf))
    {
	mgError("SQL error in MUGGLE\n%s\n", querybuf);
    }
}

/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_tools.c,v $
 * Revision 1.2  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */
