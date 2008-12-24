/*!
 * \file mg_item.c
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <cstring>

#include "mg_item.h"
#include "mg_tools.h"
#include "mg_setup.h"

bool
mgItem::Valid(bool Silent) const
{
	if (!m_validated) {
								 // sets m_valid as a side effect
		getSourceFile(true,Silent);
		m_validated=true;
	}
	return m_valid;
}

bool
mgItem::readable(string filename) const
{
	errno=0;
	string fullname;
	fullname = the_setup.ToplevelDir + filename;
	int fd = open(fullname.c_str(),O_RDONLY);
	if (fd<0)
		return false;
	char buf[20];
	errno=0;
	int i=read(fd,buf,10);
	int err = errno;
	close(fd);
	if (i!=10) {
		errno = 123456;
		return false;
	}
	else
		errno = err;
	close(fd);
	errno = 0;
	return true;
}

void
mgItem::analyze_failure(string filename) const
{
	readable(filename);			 // sets errno
	int err = errno;
#ifdef DEBUGSEL
	if (err==ENOENT) {
		char *command;
		msprintf(&command,"cd '%s';%s/scripts/speak.sh '%s'>/dev/null 2>&1",
			the_setup.ToplevelDir,
			the_setup.ConfigDirectory.c_str(),
			filename.c_str());
		system(command);
		free(command);
		readable(filename);			 // sets errno
		err = errno;
	}
#endif
	int nsize = filename.size();
	const char *p;
	if (err==123456)
		p="File too short";
	else
		p = strerror(err);
	extern void showmessage(int duration,const char*,...);
	if (nsize<20)
		showmessage(0,"%s not readable, errno=%d",filename.c_str(),err);
	else
		showmessage(0,"%s..%s not readable, errno=%d",
			filename.substr(0,15).c_str(),filename.substr(nsize-15).c_str(),err);
	mgWarning ("cannot read %s: %s", filename.c_str (),p);
}

mgItem::mgItem() {
	m_valid = false;
	m_validated = false;
	m_itemid = -1;
	m_year = 0;
	m_rating = 0;
	m_duration = 0;
}

mgItem*
mgItem::Clone () {
	if (!this)
		return 0;
	mgItem* result = new mgItem();
	result->InitFrom(this);
	return result;
}

void
mgItem::InitFrom(const mgItem* c) {
	m_sel = c->m_sel;
	m_valid = c->m_valid;
	m_validated = c->m_validated;
	m_itemid = c->m_itemid;
	m_title = c->m_title;
	m_realfile = c->m_realfile;
	m_genre_id = c->m_genre_id;
	m_genre = c->m_genre;
	m_language = c->m_language;
	m_language_id = c->m_language_id;
	m_year = c->m_year;
	m_rating = c->m_rating;
	m_duration = c->m_duration;
}


string
mgItem::ABC(string s) const
{
	string result="";
	if (!s.size()) return " ";
	result += s[0];
	if (the_setup.utf8) { // does not recognize illegal utf8 values
		unsigned char first=(unsigned char) result[0];
		if (first>0xc0) result += s[1];
		if (first>0xe0) result += s[2];
		if (first>0xf0) result += s[3];
	}
	mgDebug(1,"ABC(%s):%s - %d",s.c_str(),result.c_str(),result.size());
	return result;
}

