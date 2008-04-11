
#include "mg_thread_sync.h"
#include "mg_db.h"
#include "mg_tools.h"

static mgThreadSync* the_instance = NULL;

mgThreadSync::mgThreadSync() {
	m_path = 0;
	m_has_args = false;
}

mgThreadSync* mgThreadSync::get_instance() {
	if( !the_instance ) {
		the_instance = new mgThreadSync();
	}

	if( the_instance->Active() ) {
		return NULL;
	}
	else {
		return the_instance;
	}
}

void mgThreadSync::SetArguments(const char * const * path_argv) {
	m_path = path_argv;
	m_has_args = true;
}

bool mgThreadSync::Sync(const char * const * path_argv) {
	mgThreadSync *s = mgThreadSync::get_instance();
	if( s ) {
		s->SetArguments( path_argv);
		s->Start();
		return true;
	}
	else {
		return false;
	}
}

void
mgThreadSync::Action() {
	if( m_has_args ) {
		mgDb *s = GenerateDB(true);
		s->Sync( m_path );
		delete s;
	}
}
