/*! 
 * \file   mg_mysql.h
 * \brief  A capsule around MySql database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-02-10 17:42:54 +0100 (Thu, 10 Feb 2005) $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner, Wolfgang Rohdewald
 * \author  Responsible author: $Author: LarsAC $
 */

#ifndef __MG_MYSQL_H
#define __MG_MYSQL_H

#include <string>
#include <mysql/mysql.h>

using namespace std;

void database_end();		// must be done explicitly
void set_datadir(char *datadir);

/*!
 * \brief an abstract database class
 * 
 */
class mgmySql
{
 public:

  /*! \brief default constructor 
   */
  mgmySql( );

  ~mgmySql();

  /*! 
   * \brief helper function to execute queries
   *
   */
  MYSQL_RES* exec_sql( const string query);

  /*!
   * \brief escape arguments to be contained in a query
   */
  string sql_string( string s );

  char* sql_Cstring( const string s,char *buf=0);
  char* sql_Cstring( const char *s,char *buf=0);

  string get_col0( const string query);
  
/*! \brief executes a query and returns the integer value from
 * the first column in the first row. The query shold be a COUNT query
 * returning only one row.
 * \param query the SQL query to be executed
 */
  unsigned long exec_count (string query);

  long thread_id() { return mysql_thread_id(m_db);}
  long affected_rows() { return mysql_affected_rows(m_db);}
  bool ServerConnected() const;
  bool Connected() const;
  bool HasFolderFields() const { return m_hasfolderfields;}
  void Connect();
  //! \brief create database and tables
  void Create();
  void FillTables();
  void CreateFolderFields();

 private:
  MYSQL *m_db;
  bool m_database_found;
  bool m_hasfolderfields;
};

#endif
