/*******************************************************************/
/*! \file   mg_database.c
 *  \brief  A capsule around MySql database access
 ******************************************************************** 
 * \version $Revision: 1.2 $
 * \date    $Date: 2004/05/28 15:29:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: lvw $
 */
/*******************************************************************/

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


