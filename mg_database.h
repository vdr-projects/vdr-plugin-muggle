/*! 
 * \file   mg_database.h
 * \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2004/05/28 15:29:18 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author: lvw $
 */

#ifndef __MG_DATABASE_H
#define __MG_DATABASE_H

#include <string>
#include <mysql/mysql.h>

class mgDB
{
 public:

  /*! \brief default constructor 
   */
  mgDB( );

  /*! \brief constructor 
   *
   *  \param host
   *  \param name
   *  \param user
   *  \param pass
   *  \param port
   */
  mgDB( std::string host, std::string name, 
	std::string user, std::string pass,
	int port );
  
  /*! \brief constructor */
  ~mgDB();

  /*! \brief obtain database handle*/
  MYSQL getDBHandle();
  
 private:
  MYSQL m_dbase;
};

#endif
