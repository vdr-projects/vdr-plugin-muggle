/*! \file  mg_incremental_search.h
 *  \ingroup muggle
 *  \brief  A class that encapsulates incremental search
 *
 *  \version $Revision: $
 *  \date    $Date: $
 *  \author  Lars von Wedel
 *  \author  file owner: $Author: $
 *
 */

/* makes sure we don't use the same declarations twice */
#ifndef _MUGGLE_INCSEARCH_H
#define _MUGGLE_INCSEARCH_H

#include <string>
#include <sys/time.h>

class mgIncrementalSearch
{
 public:
  mgIncrementalSearch();

  std::string KeyStroke( unsigned key );

  std::string Backspace();

 private:
  std::string m_buffer;
  int m_position;
  unsigned m_repeats, m_last_key;
 
  double m_last_keypress;
};

#endif
