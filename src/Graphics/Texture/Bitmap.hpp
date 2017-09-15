/*
 * Bitmap.hpp
 *
 *  Created on: 01.10.2016
 *      Author: Christoph Neuhauser
 */

#ifndef BITMAP_HPP_
#define BITMAP_HPP_

#include <string>
#include <cstdint>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "../Color.hpp"

namespace sgl {

class Point2;
class Rectangle;
class Bitmap;
typedef boost::shared_ptr<Bitmap> BitmapPtr;

// For now only a bit-depth of 32-bit is properly supported!
class Bitmap
{
public:
	Bitmap() : bitmap(NULL), w(0), h(0), bpp(32) {}
	Bitmap(int width, int height, int bpp = 32) : bitmap(NULL), w(0), h(0), bpp(32) { allocate(width, height, 32); }
	~Bitmap() { freeData(); }

	// Allocation, memory management, loading and saving
	void allocate(int width, int height, int bpp = 32);
	void fromMemory(void *data, int width, int height, int bpp = 32);
	void fromFile(const char *filename);
	BitmapPtr clone();
	bool savePNG(const char *filename, bool mirror = false);

	// Set color data of all pixels
	void fill(const Color &color);
	void memset(uint8_t data);

	// Operations on pixel data
	void blit(BitmapPtr &aim, const Point2 &pos);
	void blit(BitmapPtr &aim, const Rectangle &sourceRectangle, const Rectangle &destinationRectangle);
	BitmapPtr resizeBiCubic(int destW, int destH);
	void colorize(Color color);
	BitmapPtr rotated(int degree); // 90, 180 or 270

	// Floor operations
	void floorPixelPosition(int& x, int& y);
	void setPixelFloor(Color col, int x, int y);
	void setPixelFloor(BitmapPtr &img, int sourceX, int sourceY, int destX, int destY);
	void blendPixelFloor(Color col, int x, int y);
	void blendPixelFloor(BitmapPtr &img, int sourceX, int sourceY, int destX, int destY);
	void blitWrap(BitmapPtr &img, int x, int y);

	// Bitmap attributes & accessing the pixels
	const inline uint8_t *getPixelsConst() const { return bitmap; }
	inline uint8_t *getPixels() { return bitmap; }
	inline int getWidth() const { return w; }
	inline int getHeight() const { return h; }
	inline int getW() const { return w; }
	inline int getH() const { return h; }
	inline uint8_t getBPP() const { return bpp; }
	inline uint8_t getChannels() const { return bpp/8; }
	inline int getPixelIndexXY(int x, int y) { assert(x >= 0 && x < w && y >= 0 && y < h); return y * w*bpp/8 + x * bpp/8; }
	inline uint8_t *getPixel(int x, int y) { assert(x >= 0 && x < w && y >= 0 && y < h); return bitmap + y * w*bpp/8  + x * bpp/8; }
	inline const uint8_t *getPixelConst(int x, int y) const { assert(x >= 0 && x < w && y >= 0 && y < h); return bitmap + y * w*bpp/8 + x * bpp/8; }
	Color getPixelColor(int x, int y) const;
	void setPixelColor(int x, int y, const Color &color);
	void setPixel(int x, int y, const uint8_t *color);
	void blendPixelColor(int x, int y, const Color &color); // Alpha-blending

protected:
	void freeData();
	uint8_t *bitmap;
	int w, h, bpp; // bits per Pixel
};

}

#endif /* BITMAP_HPP_ */
