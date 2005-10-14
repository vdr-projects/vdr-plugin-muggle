

#ifndef MG_IMAGE_PROVIDER
#define MG_IMAGE_PROVIDER

class mgItemGd;

#include <thread.h>

#include <string>
#include <vector>

#include <id3v2frame.h>
#include <tbytevector.h>
#include <id3v2tag.h>

class mgImageProvider : public cThread
{
 public:

  /*! \brief obtain next image in list
   */
  std::string getImagePath();

  /*! \brief tell the image provider that we are replaying a different item now. Return, whether images were found.
   */
  bool updateItem( mgItemGd *item );

  /*! \brief Initialize image provider with all files in the given directory
   */
  static mgImageProvider* Create( std::string dir );

  /*! \brief Initialize image provider for use with a Giantdisc item
   */
  static mgImageProvider* Create();

  /*! \brief destroy instance and delete remaining temporary files
   */
  ~mgImageProvider();

 protected:

  /*! \brief Executes image conversion (.jpg to .mpg) in a separate thread
   */
  virtual void Action();

 private:

  mgImageProvider( std::string dir );

  mgImageProvider( );
  
  /*! \brief Obtain all images in dir in alphabetic order
   */
  void fillImageList( std::string dir );

  /*! \brief update images according to GD scheme from database entry
   */
  bool extractImagesFromDatabase( mgItemGd *item );

  /*! \brief find images for an item
   */
  void updateFromItemDirectory( mgItemGd *item );

  /*! \brief write image from an id3v2 frame to a file
   */
  void writeImage( TagLib::ByteVector &image, int num, std::string &image_cache );

  /*! \brief convert all images found in the APIC frame list 
   */
  std::string treatFrameList( TagLib::ID3v2::FrameList &l, std::string &image_cache );

  /*! \brief save images from APIC tag and save to file. returns directory where images were saved or empty string if no images were found in the APIC tag
   */
  std::string extractImagesFromTag( std::string filename );

  /*! \brief delete temporary images
   */
  void deleteTemporaryImages();

  //! \brief define various modes how images can be obtained
  enum ImageMode 
    { 
      IM_ITEM_DIR, //< Use images from the directory in which the mgItem resides
      IM_PLAYLIST  //< Use a separate playlist passed from an external plugin
    };

  //! \brief the current image delivery mode
  ImageMode m_mode;

  //! \brief index of last image delivered
  unsigned m_image_index;

  //! \brief list of all image filenames when in modes ITEM_DIR or PLAYLIST
  std::vector<std::string> m_image_list;

  //! \brief list of all converted image filenames (.mpg) when in modes ITEM_DIR or PLAYLIST
  std::vector<std::string> m_converted_images;
  
  std::string m_source_dir;  

  bool m_delete_imgs_from_tag;
};

#endif

