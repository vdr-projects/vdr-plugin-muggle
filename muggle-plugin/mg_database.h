/*******************************************************************/
/*! \file   mg_database.h
 *  \brief  A capsule around MySql database access
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/01 18:22:53 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 */
/*******************************************************************/


#ifndef __MG_DATABASE_H
#define __MG_DATABASE_H

#include <string>

#include <mysql/mysql.h>

class mgDB
{
 public:

  mgDB( );
  mgDB( std::string user, std::string pass );
  ~mgDB();

  MYSQL getDBHandle();
  
 private:
  //  MYSQL m_dbase;
};

#endif
