
#include <mysql/mysql.h>

#include "mg_thread_sync.h"
#include "mg_sync.h"

static mgThreadSync* the_instance = NULL;

mgThreadSync* mgThreadSync::get_instance()
{
  if( !the_instance )
    {
      the_instance = new mgThreadSync();
    }

  
  if( the_instance->Active() )
    {
      return NULL;
    }
  else
    {
      return the_instance;
    }
}

void mgThreadSync::SetArguments( char * const * path_argv, bool delete_missing )
{
  m_path = path_argv;
  m_delete = delete_missing;
}

bool mgThreadSync::Sync(char * const * path_argv, bool delete_missing )
{
  mgThreadSync *s = mgThreadSync::get_instance();
  if( s )
    {
      s->SetArguments( path_argv, delete_missing );
      s->Start();
      
      return true;
    }
  else
    {
      return false;
    }  
}

void
mgThreadSync::Action()
{
  mysql_thread_init();

  if( m_path )
    {
      mgSync s;
      s.Sync( m_path, m_delete );
    }

  mysql_thread_end();
}


