/*!
 * \file mg_sync.h
 * \brief synchronization between SQL and filesystem
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_THREADSYNC_H
#define _MG_THREADSYNC_H

#include <thread.h>

class mgThreadSync : public cThread
{
 public:

  static mgThreadSync* get_instance();

  bool Sync(char * const * path_argv=0);

 protected:
  /*! \brief Runs the import routine as a separate thread
   */
  virtual void Action();

 private:

  void SetArguments( char * const * path_argv);
  
  char * const *m_path;
  bool m_delete;

};

#endif
