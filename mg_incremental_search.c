/*!								-*- c++ -*-
 *  \file  mg_incremental_search.c
 *  \ingroup muggle
 *  \brief  A class that encapsulates incremental search
 *
 *  \version $Revision: $
 *  \date    $Date: $
 *  \author  Lars von Wedel
 *  \author  file owner: $Author: $
 *
 */

#include "mg_incremental_search.h"

#include <cstring>
#include <iostream>

using namespace std;

static const char* keys[] = {
	" 0",
	".-_1",
	"abc2",
	"def3",
	"ghi4",
	"jkl5",
	"mno6",
	"pqrs7",
	"tuv8",
	"wxyz9"
};

mgIncrementalSearch::mgIncrementalSearch()
: m_position(-1), m_repeats(0), m_last_key(100), m_last_keypress(0.0)
{ }

string mgIncrementalSearch::KeyStroke( unsigned key ) {
	struct timeval now ;
	gettimeofday( &now, NULL );

	double current_t = now.tv_sec + (double)now.tv_usec / 1000000.0;
	double delta_t = current_t - m_last_keypress;
	m_last_keypress = current_t;

								 // 1 second
	const double IS_TIMEOUT = 1.0;
	if( delta_t > IS_TIMEOUT || key != m_last_key ) {
		m_position ++;

		char tmp[2];
		tmp[0] = (keys[key])[0];
		tmp[1] = '\0';

		m_buffer += string( tmp );

		m_repeats = 0;
		m_last_key = key;
	}
	else {
		// within timeout and have the same key
		// position remains
		m_repeats ++;

		if( (unsigned) m_repeats >= strlen( keys[key] ) ) {
			// wrap around to first char
			m_repeats = 0;
		}
		m_buffer[m_position] = (keys[key])[m_repeats];
	}
	return m_buffer;
}

string mgIncrementalSearch::Backspace() {
	if( !m_buffer.empty() ) {
		m_buffer.erase( m_buffer.size()-1, 1 );
		m_position--;
		m_last_key=100;
		m_last_keypress=0.0;
		m_repeats=0;
	}
	return m_buffer;
}
