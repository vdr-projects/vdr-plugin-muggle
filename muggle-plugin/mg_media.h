/*******************************************************************/
/*! \file  mgmedia.h
 * \brief  Top level access to media in vdr plugin muggle
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.8 $
 * \date    $Date: 2004/02/09 19:27:52 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: MountainMan $
 * 
 *
 */
/*******************************************************************/
/* makes sure we dont use parse the same declarations twice */
#ifndef _MG_MEDIA_H
#define _MG_MEDIA_H

#include <string>
#include <vector>

#include <mysql/mysql.h>

class mgPlaylist;
class mgTracklist;
class mgSelectionTreeNode;

/*! 
 *******************************************************************
 * \class mgFilter
 *
 * Abstract base class for representation of filter values with boundaries
 ********************************************************************/
class mgFilter
{
 public:
  typedef enum filterType { UNDEF=0, INT, STRING, BOOL }filterType;
 protected:
   filterType m_type;
   char* m_name;

 public:
   mgFilter(const char* name);
   virtual ~mgFilter();
   filterType getType();
   const char* getName();
   virtual std::string getStrVal()=0;
   virtual int getIntVal(){return 0;}
   virtual void store()=0;
   virtual void restore()=0;
   virtual void clear()=0;
   virtual bool isSet()=0;
};
 
/*! 
 *******************************************************************
 * \class mgFilterInt
 ********************************************************************/
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
 *******************************************************************
 * \class mgFilterString
 ********************************************************************/
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
 *******************************************************************
 * \class mgFilterBool
 ********************************************************************/
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
 *******************************************************************
 * \class mgFilterChoices
 ********************************************************************/
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


/*! 
 *******************************************************************
 * \class mgFilterSets
 *
 * Represents one or several sets of filters to set and memorize
 * search constraint 
 ********************************************************************/
class mgFilterSets {
 protected:
  int m_activeSetId;
   std::vector<mgFilter*> *m_activeSet;   // pointer to the active filter set
  
   std::vector< std::vector<mgFilter*>*> m_sets;
  // stores name-value pairs, even if a different set is active

   std::vector<std::string>             m_titles;
  // stores the titles for all filters

 public:
  mgFilterSets();
  // constructor, constracts a number >=1 of filter sets
  // the first set (index 0 ) is active by default

  virtual ~mgFilterSets();
  // destructor

  int numSets(); 
  // returns the number of available sets

  void nextSet();
  // proceeds to the next one in a circular fashion

  void select(int n);
  // activates a specific set by index

  virtual void clear();
  // restores the default values for all filter values in the active set
  // normally, the default values represent 'no restrictions'

  void accept();
  // stores the current filter values

   std::vector<mgFilter*> *getFilters();
  // returns the active set to the application
  // the application may temporarily modify the filter values 
  // accept() needs to be called to memorize the changed values

  virtual  std::string computeRestriction(int *viewPrt)=0;
  // computes the (e.g. sql-) restrictions specified by the active filter set
  // and returns the index of the appropriate defualt view in viewPrt

  std::string getTitle();
  // returns title of active filter set
};

/*! 
 *******************************************************************
 * \class mgMedia
 *
 * \brief main class to access content in the vdr plugin muggle
 *
 * The constructor of this class should be the only point in the plugin,
 * where the data type is explicitelymentioned. 
 * The class provides a set of objects that abstract from the data
 * type and source
 ********************************************************************/
class mgMedia
{
 
 public: 
  typedef enum contentType{
     GD_MP3
  } contentType;

 private:
    MYSQL m_db;
    contentType m_mediatype;
    std::string m_sql_filter;
    int m_defaultView;
     mgFilterSets *m_filters;

 public:
   mgMedia(contentType mediatype);
   ~mgMedia();
  
  std::string getMediaTypeName();

  mgSelectionTreeNode* getSelectionRoot();

  // playlist management
  mgPlaylist* createTemporaryPlaylist();
  mgPlaylist* loadPlaylist( std::string name );
  std::vector<std::string> *getStoredPlaylists();

  std::vector<int> getDefaultCols();
  mgTracklist* getTracks();

  // filter management

  void initFilterSet(int num=0);
  // creates FiliterSetObject for the selected media type 
  // and activates set n (if available)


  std::vector<mgFilter*> *getActiveFilters();
  // returns pointer to the activen filter set to be modified by the osd
  // Note: Modifications become only active by calling applyActiveFilter()

  std::string getActiveFilterTitle();

  void nextFilterSet();
  // proceeds to the next filter set in a cirlucar fashion
  
  void clearActiveFilter();
  // clears the current filter values and restores defaults


  mgSelectionTreeNode *applyActiveFilter();
  // Applies the active filter set and returns a root node for the 
  // selection in the default view for this filter set

};

/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_media.h,v $
 * Revision 1.8  2004/02/09 19:27:52  MountainMan
 * filter set implemented
 *
 * Revision 1.7  2004/02/02 23:33:41  MountainMan
 * impementation of gdTrackFilters
 *
 * Revision 1.6  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */
#endif  /* END  _MG_MEDIA_H */

