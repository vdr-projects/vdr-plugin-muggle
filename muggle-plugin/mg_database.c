/*! \file   mg_database.c
 *  \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 */

#include "mg_database.h"


mgDB::mgDB() 
{
}

mgDB::mgDB(std::string host, std::string name, 
	   std::string user, std::string pass,
	   int port)
{
}

mgDB::~mgDB()
{
}

MYSQL mgDB::getDBHandle()
{

  return m_dbase;
}

std::string mgDB::escape_string( MYSQL *db, std::string s )
{
  char *escbuf = (char *) malloc( 2*s.size() + 1 );

  int len = mysql_real_escape_string( db, escbuf, s.c_str(), s.size() );

  std::string r = std::string( escbuf );
  free( escbuf );

  return r;
}

MYSQL_RES* mgDB::read_query( const char *fmt, ...)
{
  va_list ap;
  va_start( ap, fmt );  
  vsnprintf( querybuf, MAX_QUERY_BUFLEN-1, fmt, ap );
  
  if( mysql_query( &m_dbase, querybuf) )
    {
      mgError( "SQL error in MUGGLE:\n%s\n", querybuf );
    }
  
  MYSQL_RES *result = mysql_store_result( &m_dbase );
  
  va_end(ap);
  return result;
}

void mgDB::write_query( const char *fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  vsnprintf( querybuf, MAX_QUERY_BUFLEN-1, fmt, ap );
  
  if( mysql_query( &m_dbase, querybuf ) )
    {
      mgError( "SQL error in MUGGLE:\n%s\n", querybuf );
    }
  
  va_end(ap);
}
