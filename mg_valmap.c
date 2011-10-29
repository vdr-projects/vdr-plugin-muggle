//								-*- c++ -*-

#include "mg_valmap.h"
#include "mg_tools.h"

#include <stdarg.h>
#include <cstdlib>
#include <cstring>

mgValmap::mgValmap(const char *key) {
	m_key = key;
}

void mgValmap::Read(FILE *f) {
	char *line=(char*)malloc(1000);
	char *prefix=(char*)malloc(strlen(m_key)+2);
	strcpy(prefix,m_key);
	strcat(prefix,".");
	rewind(f);
	while (fgets(line,1000,f)) {
		if (strncmp(line,prefix,strlen(prefix))) continue;
		if (line[strlen(line)-1]=='\n')
			line[strlen(line)-1]=0;
		char *name = line + strlen(prefix);
		char *eq = strchr(name,'=');
		if (!eq) continue;
		*(eq-1)=0;
		char *value = eq + 2;
		(*this)[string(name)]=string(value);
	}
	free(prefix);
	free(line);
}

void mgValmap::Write(FILE *f) {
	for (mgValmap::const_iterator it=begin();it!=end();++it) {
		char b[1000];
		sprintf(b,"%s.%s = %s\n",
			m_key,it->first.c_str(),
			it->second.c_str());
		fputs(b,f);
	}
}

void mgValmap::put(const string value, const char* name, ... ) {
	va_list ap;
	va_start(ap, name);
	my_put(value,name,ap);
}

void mgValmap::put(int value, const char* name, ...) {
	va_list ap;
	va_start(ap, name);
	my_put(ltos(value),name,ap);
	va_end(ap);
}

void mgValmap::put(unsigned int value, const char* name, ...) {
	va_list ap;
	va_start(ap, name);
	my_put(ltos(value),name,ap);
	va_end(ap);
}

void mgValmap::put(long value,const char* name, ...) {
	va_list ap;
	va_start(ap, name);
	my_put(ltos(value),name,ap);
	va_end(ap);
}

void mgValmap::put(const bool value,const char* name, ...) {
	string  s;
	if (value)
		s = "true";
	else
		s = "false";
	va_list ap;
	va_start(ap, name);
	my_put(s,name,ap);
	va_end(ap);
}

void mgValmap::put(const char* value, const char* name, ...) {
	if (!value) return;
	va_list ap;
	va_start(ap, name);
	my_put(value, name, ap);
	va_end(ap);
}

string
mgValmap::getstr(const char* name, ...) {
	va_list ap;
	va_start(ap, name);
	string result = my_get(name, ap);
	va_end(ap);
	return result;
}

bool
mgValmap::getbool(const char* name, ...) {
	va_list ap;
	va_start(ap, name);
	bool result = my_get(name, ap)=="true";
	va_end(ap);
	return result;
}

long
mgValmap::getlong(const char* name, ...) {
	va_list ap;
	va_start(ap, name);
	long result = atol(my_get(name, ap).c_str());
	va_end(ap);
	return result;
}

unsigned int
mgValmap::getuint(const char* name, ...) {
	va_list ap;
	va_start(ap, name);
	unsigned int result = atol(my_get(name, ap).c_str());
	va_end(ap);
	return result;
}

void mgValmap::my_put(const string value, const char* name, va_list& ap) {
	char buffer[600];
	vsnprintf(buffer, 599, name, ap);
	(*this)[string(buffer)] = value;
}

string
mgValmap::my_get(const char *name, va_list& ap) {
	char buffer[600];
	vsnprintf(buffer, 599, name, ap);
	return (*this)[buffer];
}
