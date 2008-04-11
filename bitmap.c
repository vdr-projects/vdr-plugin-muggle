/*
 * borrowed from vdr-text2skin
 */

#include "bitmap.h"
#include "quantize.h"
#include <vdr/tools.h>

#define X_DISPLAY_MISSING

#include <Imlib2.h>
cBitmapCache cMP3Bitmap::mCache(1);

#include <glob.h>

void cBitmapCache::DeleteObject(const tBitmapSpec &Key, cMP3Bitmap *&Data) {
	delete Data;
}

void cBitmapCache::ResetObject(cMP3Bitmap *&Data) {
	Data->Reset();
}

cMP3Bitmap *cMP3Bitmap::Load(const std::string &Filename, int Alpha, int height, int width, int colors, bool Quiet) {
	tBitmapSpec spec(Filename, Alpha, height, width, colors);
	//  d(printf("checking image with spec %s_%d_%d_%d_%d..", Filename.c_str(),Alpha,height,width,colors));
	std::string fname = Filename;

	cMP3Bitmap *res = NULL;
	if (mCache.Contains(spec)) {
		res = mCache[spec];
		//	d(printf("..cache ok\n"));
	}
	else {
		int pos;
		if ((pos = fname.find('*')) != -1) {
			glob_t gbuf;
			if (glob(fname.c_str(), 0, NULL, &gbuf) == 0) {
				//  d(printf("GLOB: FOUND %s\n", gbuf.gl_pathv[0]));
				fname = gbuf.gl_pathv[0];
			}
			else {
				if (!Quiet) d(printf("music: bitmap: ERROR: No match for wildcard filename %s", Filename.c_str()));
				fname = "";
			}
			globfree(&gbuf);
		}

		res = new cMP3Bitmap;
		bool result = false;

		result = res->LoadImlib(fname.c_str(),height,width,colors, Quiet);

		if (result) {
			res->SetAlpha(Alpha);
		}
		else {
			d(printf("music: bitmap: ERROR: filename %s too short to identify format", fname.c_str()));
			DELETENULL(res);
		}
		mCache[spec] = res;
	}

	return res;
}

bool cMP3Bitmap::Available(const std::string &Filename, int Alpha, int height, int width, int colors) {
	if ((int)Filename.find('*') != -1) {
		bool result = false;
		glob_t gbuf;
		if (glob(Filename.c_str(), 0, NULL, &gbuf) == 0)
			result = true;
		globfree(&gbuf);
		return result;
	}
	else
		return access(Filename.c_str(), F_OK) == 0;
}

cMP3Bitmap::cMP3Bitmap(void) {
	mCurrent = 0;
	mLastGet = 0;
}

cMP3Bitmap::~cMP3Bitmap() {
	for (int i = 0; i < (int)mBitmaps.size(); ++i)  delete mBitmaps[i];
	mBitmaps.clear();
}

cBitmap &cMP3Bitmap::Get(void) {
	if (mBitmaps.size() == 1)  return *mBitmaps[0];

	/*	time_t upd;
		int diff;
		if (mLastGet == 0) {
			mLastGet = Now;
			upd = mDelay;
		} else if ((diff = Now - mLastGet) >= mDelay) {
			mCurrent = (mCurrent + 1) % mBitmaps.size();
			mLastGet = Now;
			upd = mDelay - diff > 1 ? mDelay - diff : 1;
		} else {
			upd = mDelay - diff;
		}

		if (UpdateIn == 0 || UpdateIn > (uint)upd)
			UpdateIn = upd;
	*/
	mCurrent = (mCurrent + 1) % mBitmaps.size();
	return *mBitmaps[mCurrent];
}

void cMP3Bitmap::SetAlpha(int Alpha) {
	if (Alpha > 0) {
		std::vector<cBitmap*>::iterator it = mBitmaps.begin();
		for (; it != mBitmaps.end(); ++it) {
			int count;
			if ((*it)->Colors(count)) {
				for (int i = 0; i < count; ++i) {
					int alpha = (((*it)->Color(i) & 0xFF000000) >> 24) * Alpha / 255;
					(*it)->SetColor(i, ((*it)->Color(i) & 0x00FFFFFF) | (alpha << 24));
				}
			}
		}
	}
}

bool cMP3Bitmap::LoadImlib(const char *Filename, int height, int width, int colors, bool Quiet) {
	Imlib_Image buffer;
	int h,w;
	unsigned char * outputImage = NULL;
	unsigned int * outputPalette = NULL;
	cQuantizeWu* quantizer = new cQuantizeWu();
	cBitmap *bmp = NULL;

	buffer = imlib_load_image(Filename);

	imlib_context_set_image(buffer);
	w= imlib_image_get_width();
	h= imlib_image_get_height();
	imlib_context_set_image(buffer);

	if (!buffer)
		return false;

	Imlib_Image image;
	image = imlib_create_cropped_scaled_image(0, 0, w, h ,width , height);

	imlib_context_set_image(buffer);
	imlib_free_image_and_decache();

	imlib_context_set_image(image);
	w= imlib_image_get_width();
	h= imlib_image_get_height();
	imlib_context_set_image(image);

	bmp = new cBitmap(w, h, 8);
	uint8_t *data = (uint8_t*)imlib_image_get_data_for_reading_only();

	if ( colors != 0 ) {
		quantizer->Quantize(data, w* h, colors);
		outputImage = quantizer->OutputImage();
		outputPalette = quantizer->OutputPalette();
	}
	imlib_free_image();

	int pos = 0;
	for (int y = 0; y < bmp->Height(); ++y) {
		for (int x = 0; x < bmp->Width(); ++x) {
			if ( colors != 0 ) {
				bmp->DrawPixel(x, y ,  outputPalette[outputImage[y * bmp->Width() + x]] | 0xFF000000 );
			}
			else {
				tColor col = (data[pos + 3] << 24) | (data[pos + 2] << 16) | (data[pos + 1] << 8) | data[pos + 0];
				bmp->DrawPixel(x, y, col);
				pos += 4;
			}
		}
	}

	mBitmaps.push_back(bmp);
	delete(quantizer);
	return true;
}
