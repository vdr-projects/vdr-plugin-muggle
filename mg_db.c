/*! 
 * \file   mg_db.c
 * \brief  A capsule around database access
 *
 * \version $Revision: 1.2 $
 * \date    $Date: 2005-04-13 17:42:54 +0100 (Thu, 13 Apr 2005) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wolfgang61 $
 */

#include <string>

using namespace std;

#include "mg_db_gd.h"
#include "mg_setup.h"

#include <sys/stat.h>
#include <fts.h>

bool UsingEmbeddedMySQL();

mgDb* GenerateDB(bool SeparateThread)
{
	// \todo should return different backends according to the_setup.Variant
	return new mgDbGd(SeparateThread);
}

mgDb::mgDb(bool SeparateThread)
{
	m_database_found=false;
	m_hasfolderfields=false;
	m_separate_thread=SeparateThread;
	m_connect_time=0;
	m_create_time=0;
}

mgDb::~mgDb()
{
}

string
mgDb::sql_string( const string s )
{
  char *b = sql_Cstring(s);
  string result = string( b);
  free( b);
  return result;
}

char*
mgDb::sql_Cstring( const string s, char *buf )
{
  return sql_Cstring(s.c_str(),buf);
}

string
optimize (string & spar)
{
	string s = spar;
	string::size_type tmp = s.find (" WHERE");
	if (tmp != string::npos)
		s.erase (tmp, 9999);
	tmp = s.find (" ORDER");
	if (tmp != string::npos)
		s.erase (tmp, 9999);
	string::size_type frompos = s.find (" FROM ") + 6;
	if (s.substr (frompos).find (",") == string::npos)
	{
		string from = s.substr (frompos, 999) + '.';
		string::size_type tbl;
		while ((tbl = spar.find (from)) != string::npos)
		{
    			spar.erase (tbl, from.size ());
		}
	}
	return spar;
}

void
mgDb::Sync(char * const * path_argv, bool delete_missing)
{
	if (!SyncStart())
		return;
  	extern void showimportcount(unsigned int,bool final=false);

	unsigned int importcount=0;
	chdir(the_setup.ToplevelDir);
	FTS *fts;
	FTSENT *ftsent;
	fts = fts_open( path_argv, FTS_LOGICAL, 0);
	if (fts)
	{
		while ( (ftsent = fts_read(fts)) != NULL)
		{
			if (!((ftsent->fts_statp->st_mode)||S_IFREG))
				continue;
			SyncFile(ftsent->fts_path);
			importcount++;
			if (importcount%1000==0)
				showimportcount(importcount);
		}
		fts_close(fts);
	}
	SyncEnd();
	showimportcount(importcount,true);
}

