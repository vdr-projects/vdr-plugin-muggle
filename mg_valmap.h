#ifndef _MG_VALMAP_H
#define _MG_VALMAP_H

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <map>

using namespace std;

//! \brief a map for reading / writing configuration data. 
class mgValmap : public map<string,string> {
	private:
		const char *m_key;
	public:
		/*! \brief constructor
		 * \param key all names will be prefixed with key.
		 */
		mgValmap(const char *key);
		//! \brief read from file
		void Read(FILE *f);
		//! \brief write to file
		void Write(FILE *f);
		//! \brief enter a string value
		void put(string value,const char*name, ...);
		//! \brief enter a C string value
		void put(const char*value, const char* name, ...);
		//! \brief enter a long value
		void put(long value, const char*name, ...);
		//! \brief enter a int value
		void put(int value, const char*name, ... );
		//! \brief enter a unsigned int value
		void put(unsigned int value, const char*name, ...);
		//! \brief enter a bool value
		void put(bool value, const char*name, ...);
	 //! \brief return a string
	 string getstr(const char* name, ...);
	 //! \brief return a bool
	 bool getbool(const char* name, ...);
	 //! \brief return a long
	 long getlong(const char* name, ...);
	 //! \brief return an unsigned int
	 unsigned int getuint(const char* name, ...);
	private:
		void my_put(const string value, const char *name, va_list& ap);
		string my_get(const char *name, va_list& ap);
};
#endif
