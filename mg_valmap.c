#include "mg_valmap.h"
#include "mg_order.h"

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

void mgValmap::put(const char* name, const string value) {
	if (value.empty() || value==EMPTY) return;
	(*this)[string(name)] = value;
}

void mgValmap::put(const char* name, const char* value) {
	if (!value || *value==0) return;
	(*this)[string(name)] = value;
}

void mgValmap::put(const char* name, const int value) {
	put(name,ltos(value));
}

void mgValmap::put(const char* name, const unsigned int value) {
	put(name,ltos(value));
}

void mgValmap::put(const char* name, const long value) {
	put(name,ltos(value));
}

void mgValmap::put(const char* name, const bool value) {
	string s;
	if (value)
		s = "true";
	else
		s = "false";
	put(name,s);
}


