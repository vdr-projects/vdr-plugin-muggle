/*! \file   mg_database.c
 *  \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 */

#include "mg_database.h"

using namespace std;

mgDB::mgDB() 
{
}

mgDB::mgDB(string host, string name, 
	   string user, string pass,
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

string mgDB::escape_string( MYSQL *db, string s )
{
  char *escbuf = (char *) malloc( 2*s.size() + 1 );

  int len = mysql_real_escape_string( db, escbuf, s.c_str(), s.size() );

  string r = string( escbuf );
  free( escbuf );

  return r;
}
