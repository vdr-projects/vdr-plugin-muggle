/*******************************************************************/
/*! \file  mgmedia.h
 * \brief  Top level access to media in vdr plugin muggle
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.7 $
 * \date    $Date: 2004/02/02 23:33:41 $
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
 * represents a filter value with boundaries or choices
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
  
 public:
  int m_intval;

  mgFilterInt(const char *name, int value, int min = 0, int max = INT_MAX);
  virtual ~mgFilterInt();

  int getVal();
  int getMin();
  int getMax();
  virtual std::string getStrVal();
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
 public:
  char* m_strval;

  mgFilterString(const char *name, const char* value, int maxlen=255,
		 std::string allowedchar="abcdefghijklmnopqrstuvwxyz0123456789-");
  
  virtual ~mgFilterString();

  int getMaxLength(); 
  std::string getAllowedChars();
  virtual std::string getStrVal();
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

 public:
  int m_bval;

  mgFilterBool(const char *name, bool value, 
	       std::string truestr="yes", std::string falsestr="no");
  virtual ~mgFilterBool();

  virtual std::string getStrVal();
  std::string getTrueString();
  std::string getFalseString();
  bool getVal();
};  

/*! 
 *******************************************************************
 * \class mgFilterChoices
 ********************************************************************/
class mgFilterChoice : public mgFilter
{
 private:
  std::vector<std::string> m_choices;
  
 public:
  int m_selval; // index of the currently selected item

  mgFilterChoice(const char *name, int val, std::vector<std::string> *choices);
  virtual ~mgFilterChoice();

  virtual std::string getStrVal();
  virtual std::vector<std::string> &getChoices();

};  

/*! 
 *******************************************************************
 * \class mgrackFilters
 *
 * Represents a set of filters to set and memorize the search 
 * constraint for a specific track
 ********************************************************************/
class mgTrackFilters
{
 protected:
  std::vector<mgFilter*> m_filters;
 public:
  mgTrackFilters();
  virtual ~mgTrackFilters();

  std::vector<mgFilter*> *getFilters();
  
  virtual std::string CreateSQL()=0;
  virtual void clear()=0;
  
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
    DUMMY =0,
    GD_MP3
  } contentType;

 private:
    MYSQL m_db;
    contentType m_mediatype;
    std::string m_sql_trackfilter;
    int m_defaultView;
    mgTrackFilters *m_trackfilter;

 public:
   mgMedia(contentType mediatype);
   ~mgMedia();
  
  std::string getMediaTypeName();

  mgSelectionTreeNode* getSelectionRoot();

  // filter management
  std::vector<mgFilter*> *getTrackFilters();
  void applyTrackFilters(std::vector<mgFilter*> *filters);

  // playlist management
  mgPlaylist* createTemporaryPlaylist();
  mgPlaylist* loadPlaylist( std::string name );
  std::vector<std::string> *getStoredPlaylists();

  std::vector<int> getDefaultCols();
  mgTracklist* getTracks();
};

/* -------------------- begin CVS log ---------------------------------
 * $Log: mg_media.h,v $
 * Revision 1.7  2004/02/02 23:33:41  MountainMan
 * impementation of gdTrackFilters
 *
 * Revision 1.6  2004/02/02 22:48:04  MountainMan
 *  added CVS $Log
 *
 *
 * --------------------- end CVS log ----------------------------------
 */
#endif  /* END  _CONTENT_INTERFACE_H */

