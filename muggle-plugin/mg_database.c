/*******************************************************************/
/*! \file   mg_database.c
 *  \brief  A capsule around MySql database access
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/01 18:22:53 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 */
/*******************************************************************/

#include "mg_database.h"

using namespace std;

mgDB::mgDB() 
{
}

mgDB::mgDB(string user, string pass) 
{
}

mgDB::~mgDB()
{
}

MYSQL mgDB::getDBHandle()
{
  MYSQL m;

  return m;
}


