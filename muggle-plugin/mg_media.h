/*******************************************************************/
/*! \file  mgmedia.h
 * \brief  Top level access to media in vdr plugin muggle
 * for the vdr muggle plugindatabase
 ******************************************************************** 
 * \version $Revision: 1.1 $
 * \date    $Date: 2004/02/01 18:22:53 $
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  file owner: $Author: LarsAC $
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
 * \class mgFilters
 *
 * \brief stores a set of search constraints to the media database
 ********************************************************************/
class mgFilters 
{
    typedef enum filterType
    {
	NUMBER,         // integer number (with upper and lower limits)
	STRING,         // any string
	REGEXP_STRING,  // string containing wildcard symbol '*'
	CHOICE       // value fro ma list of choices
    }filterType;
	
 public:
    std::vector<std::string> fields;
    std::vector<std::string> values;

    mgFilters();
    ~mgFilters();

    int getNumFilters();
    std::string getName(int filter);
    int getValue(int filter);
    filterType getType(int filter);

    // for NUMBER filters
    int getMin(int filter);
    int getMax(int filter);

    // for CHOICE
    std::vector<std::string> getChoices();
    int getCurrent(int filter);

    // check, if a value is correct
    bool checkValue(int filter, std::string value);
    bool checkValue(int filter, int value);
    
    // finally set the values
    
    bool setValue(int filter, std::string value);
    bool setValue(int filter, int value);
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
    std::string m_filter;
    int m_defaultView;

 public:
   mgMedia(contentType mediatype);
   ~mgMedia();
  
  std::string getMediaTypeName();

  mgSelectionTreeNode* getSelectionRoot();

  mgFilters getActiveFilters();

  void setFilters(mgFilters filters);
  void setFilters(std::string sql)
    {
      m_filter=sql;
    }

  // playlist management
  mgPlaylist* createTemporaryPlaylist();
  mgPlaylist* loadPlaylist( std::string name );
  std::vector<std::string> getStoredPlaylists();

  std::vector<int> getDefaultCols();
  mgTracklist* getTracks();
};
#endif  /* END  _CONTENT_INTERFACE_H */


















