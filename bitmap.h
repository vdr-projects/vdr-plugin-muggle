/*
 * Borrowed from vdr-text2skin
 */

#ifdef USE_BITMAP
#ifndef VDR_MP3BITMAP_H
#define VDR_MP3BITMAP_H

#include "common.h"
#include "imagecache.h"
#include <vdr/osd.h>

struct tBitmapSpec
{
	std::string Filename;
	int         Alpha;
	int         Width;
	int         Height;
	int         Colors;

	tBitmapSpec(const std::string &filename, int alpha, int width, int height, int colors):
	Filename(filename), Alpha(alpha), Width(width), Height(height), Colors(colors) {}

	bool operator<(const tBitmapSpec &Src) const;
	bool operator==(const tBitmapSpec &Src) const;
};

inline bool tBitmapSpec::operator<(const tBitmapSpec &Src) const
{
	if (Filename == Src.Filename) {
		if (Alpha == Src.Alpha) {
			if (Width == Src.Width) {
				if (Height == Src.Height)
					return Colors < Src.Colors;
				return Height < Src.Height;
			}
			return Width < Src.Width;
		}
		return Alpha < Src.Alpha;
	}
	return Filename < Src.Filename;
}

inline bool tBitmapSpec::operator==(const tBitmapSpec &Src) const
{
	return Filename == Src.Filename
		&& Alpha == Src.Alpha
		&& Width == Src.Width
		&& Height == Src.Height
		&& Colors == Src.Colors;
}

class cMP3Bitmap;

class cBitmapCache: public cxCache<tBitmapSpec,cMP3Bitmap*>
{
	protected:
		virtual void DeleteObject(const tBitmapSpec &Key, cMP3Bitmap *&Data);
		virtual void ResetObject(cMP3Bitmap *&Data);

	public:
		cBitmapCache(uint MaxItems): cxCache<tBitmapSpec,cMP3Bitmap*>(MaxItems) {}
		virtual ~cBitmapCache() { Flush(); }
};

class cMP3Bitmap
{
	private:
		static cBitmapCache   mCache;

		std::vector<cBitmap*> mBitmaps;
		int                   mCurrent;
		time_t                mDelay;
		time_t                mLastGet;

		// disallow direct construction
		cMP3Bitmap(void);

	public:
		static cMP3Bitmap *Load(const std::string &Filename, int Alpha = 0, int height = 0,
			int width = 0, int colors = 0, bool Quiet = false);
		static bool Available(const std::string &Filename, int Alpha = 0, int height = 0,
			int width = 0, int colors = 0);
		static void ResetCache(void) { mCache.Reset(); }
		static void FlushCache(void) { mCache.Flush(); }

		virtual ~cMP3Bitmap();

		void Reset(void) { mCurrent = 0; mLastGet = 0; }
		cBitmap &Get(void);
		void SetColor(int Index, tColor Color);
		void SetAlpha(int Alpha);

		bool LoadImlib(const char *Filename,int height, int width, int colors, bool Quiet);
};

inline void cMP3Bitmap::SetColor(int Index, tColor Color) {
	mBitmaps[mCurrent]->SetColor(Index, Color);
}
#endif							 // VDR_MP3BITMAP_H
#endif
