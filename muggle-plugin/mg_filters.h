/*! \file   mg_filters.h
 *  \brief  Top level access to media in vdr plugin muggle
 *          for the vdr muggle plugindatabase
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author$
 */

#ifndef _MG_FILTERS_H
#define _MG_FILTERS_H

#include <string>
#include <vector>

/*! 
 * \brief abstract base class for representation of filter values with boundaries
 */
class mgFilter
{
 public:
  
  typedef enum filterType 
    { 
      UNDEF = 0, 
      INT, 
      STRING, 
      BOOL, 
      CHOICE 
    } filterType;
  
 protected:
  filterType m_type;
  char* m_name;
  
 public:
  
  mgFilter(const char* name);
  
  virtual ~mgFilter();

  filterType getType();

  const char* getName();

  virtual std::string getStrVal() = 0;

  virtual int getIntVal()
    { return 0; }

  virtual void store() = 0;

  virtual void restore() = 0;

  virtual void clear() = 0;

  virtual bool isSet() = 0;
};
 
/*! 
 * \class mgFilterInt
 */
class mgFilterInt : public mgFilter
{
 private:
  int m_min;
  int m_max; 
  int m_stored_val;
  int m_default_val;
  
 public:
  int m_intval;

  mgFilterInt(const char *name, int value, int min = 0, int max = 9999);
  virtual ~mgFilterInt();

  int getVal();
  int getMin();
  int getMax();
  virtual std::string getStrVal();
  virtual int getIntVal();
  virtual void store();
  virtual void restore();
  virtual void clear();
   virtual bool isSet();
};  
  
/*! 
 * \class mgFilterString
 */
class mgFilterString : public mgFilter
{
 private:
  std::string m_allowedchar;
  int m_maxlen;
  char* m_stored_val;
  char* m_default_val;

 public:
  char* m_strval;

  mgFilterString(const char *name, const char* value, int maxlen=255,
		 std::string allowedchar="abcdefghijklmnopqrstuvwxyz0123456789-");
  
  virtual ~mgFilterString();

  int getMaxLength(); 
  std::string getAllowedChars();
  virtual std::string getStrVal();
  virtual void store();
  virtual void restore();
  virtual void clear();
  virtual bool isSet();
};  

/*! 
 * \class mgFilterBool
 */
class mgFilterBool : public mgFilter
{
 private:
  std::string m_truestr;
  std::string m_falsestr;
  bool m_stored_val;
  bool m_default_val;

 public:
  int m_bval;

  mgFilterBool(const char *name, bool value, 
	       std::string truestr="yes", std::string falsestr="no");
  virtual ~mgFilterBool();

  virtual std::string getStrVal();
  virtual int getIntVal();
  std::string getTrueString();
  std::string getFalseString();
  bool getVal();
  virtual void store();
  virtual void restore();
  virtual void clear();
  virtual bool isSet();
};  

/*! 
 * \class mgFilterChoice
 */
class mgFilterChoice : public mgFilter
{
 private:
  std::vector<std::string> m_choices;
  int m_stored_val; 
  int m_default_val;
 
 public:
  int m_selval; // index of the currently selected item

  mgFilterChoice(const char *name, int val, std::vector<std::string> *choices);
  virtual ~mgFilterChoice();

  virtual std::string getStrVal();
  virtual std::vector<std::string> &getChoices();
  virtual void store();
  virtual void restore();
  virtual void clear();
  virtual bool isSet();
};  


#endif
