/*! 
 * \file   mg_database.h
 * \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 */

#ifndef __MG_DATABASE_H
#define __MG_DATABASE_H

#include <string>
#include <mysql/mysql.h>

/*!
 * \brief an abstract database class
 * 
 * \todo Currently unused. This class could abstract database connection and query handling, but this will be tedious as an abstract representation of results would be needed.
 */
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
	std::string user="", std::string pass="",
	int port = 0 );

  // add constructor for sockets
  
  /*! \brief constructor */
  ~mgDB();

  /*! 
   * \brief obtain database handle
   */
  MYSQL getDBHandle();

  /*! 
   * \brief database initialization
   */
  // int initialize();

  /*! 
   * \brief helper function to execute read queries
   *
   * \todo Could be a member of mgDatabase?
   */
  MYSQL_RES* read_query( const char *fmt, ... );

  /*! 
   * \brief helper function to execute write queries
   *
   * \todo Could be a member of mgDatabase?
   */
  void write_query( const char *fmt, ... );  

  /*!
   * \brief escape arguments to be contained in a query
   *
   * \todo use m_dbase member of this class
   */
  static std::string escape_string( MYSQL *db, std::string s );

  
  
 private:
  MYSQL m_dbase;
};

#endif
