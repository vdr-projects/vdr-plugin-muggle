/*******************************************************************/
/*! \file   mg_filters.c
 *  \brief  
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/12 09:15:07 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
 */
/*******************************************************************/

/* makes sure we dont parse the same declarations twice */
#include "mg_filters.h"
#include "mg_tools.h"

using namespace std;

//-------------------------------------------------------------------
//                         mgFilter
//-------------------------------------------------------------------
mgFilter::mgFilter(const char* name)
{
  m_name = strdup(name);
}
mgFilter::~mgFilter()
{
  free(m_name);
}

const char* mgFilter::getName()
{
  return m_name;
}

mgFilter::filterType mgFilter::getType()
{
  return m_type;
}

//-------------------------------------------------------------------
//        mgFilterInt
//-------------------------------------------------------------------
mgFilterInt::mgFilterInt(const char *name, int value, int min, int max)
  : mgFilter(name)
{
  m_type = INT;
  m_intval = value;
  m_default_val = value;
  m_stored_val = value;
  m_max = max;
  m_min = min;
}
mgFilterInt::~mgFilterInt()
{
}

string mgFilterInt::getStrVal()
{
  char buffer[20];
  sprintf(buffer, "%d", m_intval);

  return (string)buffer;
}

int mgFilterInt::getIntVal()
{
  return (int) m_intval;
}

int mgFilterInt::getVal()
{
  return m_intval;
}

int mgFilterInt::getMin()
{
  return m_min;
}

int mgFilterInt::getMax()
{
  return m_max;
}

void mgFilterInt::store()
{
  m_stored_val = m_intval;
}
void mgFilterInt::restore()
{
  m_intval = m_stored_val;
}
void mgFilterInt::clear()
{
  m_stored_val = m_default_val;
  m_intval     =  m_default_val;
}

bool mgFilterInt::isSet()
{
  if(m_stored_val == m_default_val)
  {
    return false;
  }
  return true;
}

//-------------------------------------------------------------------
//       mgFilterString
//-------------------------------------------------------------------
mgFilterString::mgFilterString(const char *name, const char* value,
			       int maxlen, string allowedchar)
  : mgFilter(name)
{
  m_type = STRING;
  m_strval = strdup(value);
  m_default_val = strdup(value);
  m_stored_val  = strdup(value);
  m_allowedchar = allowedchar;
  m_maxlen = maxlen;
}
mgFilterString::~mgFilterString()
{
  if(m_strval)
    {
      free(m_strval);
    }
}
  
int mgFilterString::getMaxLength()
{
  return m_maxlen;
} 

string mgFilterString::getAllowedChars()
{
  return m_allowedchar;
}
string mgFilterString::getStrVal()
{
 
  return (string) m_strval;
}
void mgFilterString::store()
{
  if(m_stored_val) free(m_stored_val);
  m_stored_val = strdup(m_strval);
}
void mgFilterString::restore()
{
  if(m_strval) free(m_strval);
  m_strval = strdup(m_stored_val);
}
void mgFilterString::clear()
{
  if(m_stored_val) free(m_stored_val);
  if(m_strval) free(m_strval);

  m_stored_val = strdup(m_default_val);
  m_strval = strdup(m_default_val);
}

bool mgFilterString::isSet()
{
  if(strlen(m_stored_val) == 0)
  {
    return false;
  }
  return true;
}
//-------------------------------------------------------------------
//        mgFilterBool
//-------------------------------------------------------------------
mgFilterBool::mgFilterBool(const char *name, bool value,
			   string truestr, string falsestr)
  : mgFilter(name)
{
  m_type = BOOL;
  m_bval  = (int) value;
  m_default_val = value;
  m_stored_val  = value;
  m_truestr = truestr;
  m_falsestr = falsestr;
  
}

mgFilterBool::~mgFilterBool()
{
}

string mgFilterBool::getStrVal()
{
  if(m_bval)
    return "true";
  else
    return "false";
}

int mgFilterBool::getIntVal()
{
  return (int) m_bval;
}

string mgFilterBool::getTrueString()
{
  return m_truestr;
}

string mgFilterBool::getFalseString()
{
  return m_falsestr;
}

bool mgFilterBool::getVal()
{
  return (bool) m_bval;
}

void mgFilterBool::store()
{
  m_stored_val = (bool) m_bval;
}

void mgFilterBool::restore()
{
  m_bval = (int) m_stored_val;
}

void mgFilterBool::clear()
{
  m_stored_val = (int) m_default_val;
  m_bval       = (int) m_default_val;
}

bool mgFilterBool::isSet()
{
  if(m_stored_val == m_default_val )
  {
    return false;
  }
  return true;
}
//-------------------------------------------------------------------
//        mgFilterChoice
//-------------------------------------------------------------------
mgFilterChoice::mgFilterChoice(const char *name, int value, vector<string> *choices)
  : mgFilter(name)
{
  m_choices = *choices;
  m_selval = value;
  m_default_val = value;
  if( m_selval < 0 || m_selval >= (int) m_choices.size() )
  {
    mgError("mgFilterChoice::mgFilterChoice(..): Illegal index %d", m_selval);
  }
}
mgFilterChoice::~mgFilterChoice()
{
  m_choices.clear();
}

string mgFilterChoice::getStrVal()
{
  if( m_selval < 0 || m_selval >= (int) m_choices.size() )
  {
    mgError("mgFilterChoice::getStrVal(): Illegal index %d", m_selval);
  }
  return m_choices[m_selval];
}
vector<string> &mgFilterChoice::getChoices()
{
  return m_choices;
}
void mgFilterChoice::store()
{
  m_stored_val = m_selval;
  
}
void mgFilterChoice::restore()
{
  m_selval =  m_stored_val;
}
void mgFilterChoice::clear()
{
  m_stored_val =  m_default_val;
  m_selval       =  m_default_val;
}

bool mgFilterChoice::isSet()
{
  if(m_stored_val == m_default_val)
  {
    return false;
  }
  return true;
}
