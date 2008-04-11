
#ifndef MG_IMAGE_PROVIDER
#define MG_IMAGE_PROVIDER

class mgItemGd;

#include <vdr/thread.h>

#include <string>
#include <vector>

using namespace std;

#include <id3v2frame.h>
#include <tbytevector.h>
#include <id3v2tag.h>

#include <vdr/osd.h>

class mgImageProvider
{
	public:

		/*! \brief obtain next image in list
		 */
		virtual string getImagePath( std::string &source );

		/*! \brief tell the image provider that we are replaying a different item now. Return, whether images were found.
		 */
		virtual bool updateItem( mgItemGd *newitem );

		mgImageProvider(string dir);

		mgImageProvider(tArea area);

		~mgImageProvider() {};

	protected: // TODO was davon ist private?

		/*! \brief Obtain all images in dir in alphabetic order
		 */
		void fillImageList( std::string dir );

		/*! \brief update images according to GD scheme from database entry
		 */
		bool extractImagesFromDatabase();

		/*! \brief find images for an item
		 */
		void updateFromItemDirectory();

		/*! \brief write image from an id3v2 frame to a file
		 */
		void writeImage( TagLib::ByteVector &image, int num, std::string &image_cache );

		/*! \brief convert all images found in the APIC frame list
		 */
		std::string treatFrameList( TagLib::ID3v2::FrameList &l, std::string &image_cache );

		/*! \brief save images from APIC tag and save to file. returns directory where images were saved or empty string if no images were found in the APIC tag
		 */
		std::string extractImagesFromTag( std::string filename );

		//! \brief define various modes how images can be obtained
		enum ImageMode
		{
			IM_ITEM_DIR,		 //< Use images from the directory in which the mgItem resides
			IM_PLAYLIST			 //< Use a separate playlist passed from an external plugin
		};

		ImageMode m_mode;
		mgItemGd *currItem;

		//! \brief index of last image delivered
		unsigned m_image_index;

		//! \brief list of all image filenames when in modes ITEM_DIR or PLAYLIST
		std::vector<std::string> m_image_list;

		std::vector<std::string> m_need_conversion;

		//! \brief list of all converted image filenames (.mpg) when in modes ITEM_DIR or PLAYLIST
		std::vector<std::string> m_converted_images;
		
		std::string m_source_dir;

		bool m_delete_imgs_from_tag;

		tArea coverarea;
		bool CollectImages();
		string getCachedMPGFile(mgItemGd *item,string f);
};

class mgMpgImageProvider : public mgImageProvider, public cThread
{
	public:

		/*! \brief Executes image conversion (.jpg to .mpg) in a separate thread
		 */
		mgMpgImageProvider(string dir);
		mgMpgImageProvider(tArea area);
		~mgMpgImageProvider();	
		string getImagePath( std::string &source );
		void Action();
		bool updateItem( mgItemGd *newitem );

};

#endif
