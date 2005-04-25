
#include "mg_thread_sync.h"
#include "mg_db.h"

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

static char *dot_args[] = { ".", 0 };

void mgThreadSync::SetArguments( char * const * path_argv, bool delete_missing )
{
  if (path_argv)
	  m_path = path_argv;
  else
	  m_path = dot_args;
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
  if( m_path )
    {
      mgDb *s = GenerateDB(true);
      s->Sync( m_path, m_delete );
      delete s;
    }
}

