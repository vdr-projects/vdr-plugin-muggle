#ifndef _MG_VALMAP_H
#define _MG_VALMAP_H

#include <stdio.h>
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
		void put(const char*name, string value);
		//! \brief enter a C string value
		void put(const char*name, const char* value);
		//! \brief enter a long value
		void put(const char*name, long value);
		//! \brief enter a int value
		void put(const char*name, int value);
		//! \brief enter a unsigned int value
		void put(const char*name, unsigned int value);
		//! \brief enter a bool value
		void put(const char*name, bool value);
	 //! \brief return a string
	 string getstr(const char* name) {
		 return (*this)[name];
	 }
	 //! \brief return a C string
	 bool getbool(const char* name) {
		 return (getstr(name)=="true");
	 }
	 //! \brief return a long
	 long getlong(const char* name) {
		 return atol(getstr(name).c_str());
	 }
	 //! \brief return an unsigned int
	 unsigned int getuint(const char* name) {
		 return (unsigned long)getlong(name);
	 }
};
#endif
