/*******************************************************************/
/*! \file  mgmedia.h
 * \brief  Top level access to media in vdr plugin muggle
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.2 $
 * \date    $Date: 2004/02/02 02:01:11 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: MountainMan $
 * 
 *
 */
/*******************************************************************/
/* makes sur we dont use parse the same declarations twice */
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

class mgFilterInt : public mgFilter
{
 private:
  int m_min;
  int m_max; 
  int m_intval;
  
 public:
  mgFilterInt(const char *name, int value, int min = 0, int max = INT_MAX);
  virtual ~mgFilterInt();

  int getVal();
  int getMin();
  int getMax();
  virtual std::string getStrVal();

  void setVal(int value);
};  
  
class mgFilterString : public mgFilter
{
 private:
  char* m_strval;
  
 public:
  mgFilterString(const char *name, const char* value);
  virtual ~mgFilterString();

  const char* getVal();
  virtual std::string getStrVal();

  void setVal(const char* val);
};  

class mgFilterBool : public mgFilter
{
 private:
  bool m_bval;
  
 public:
  mgFilterBool(const char *name, bool value);
  virtual ~mgFilterBool();

  bool getVal();
  virtual std::string getStrVal();

  void setVal(bool val);
};  

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

 public:
   mgMedia(contentType mediatype);
   ~mgMedia();
  
  std::string getMediaTypeName();

  mgSelectionTreeNode* getSelectionRoot();

  std::vector<mgFilter*> *getTrackFilters();

  void setTrackFilters(std::vector<mgFilter*> *filters);

  // playlist management
  mgPlaylist* createTemporaryPlaylist();
  mgPlaylist* loadPlaylist( std::string name );
  std::vector<std::string> *getStoredPlaylists();

  std::vector<int> getDefaultCols();
  mgTracklist* getTracks();
};
#endif  /* END  _CONTENT_INTERFACE_H */


















