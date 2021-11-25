/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2016, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BITMAP_HPP_
#define BITMAP_HPP_

#include <string>
#include <cstdint>
#include <memory>
#include "../Color.hpp"

namespace sgl {

class Point2;
class Rectangle;
class Bitmap;
typedef std::shared_ptr<Bitmap> BitmapPtr;

//! For now only a bit-depth of 32-bit is properly supported!
class DLL_OBJECT Bitmap
{
public:
    Bitmap() : bitmap(NULL), w(0), h(0), bpp(32) {}
    Bitmap(int width, int height, int bpp = 32) : bitmap(NULL), w(0), h(0), bpp(32) { allocate(width, height, 32); }
    ~Bitmap() { freeData(); }

    //! Allocation, memory management, loading and saving
    void allocate(int width, int height, int bpp = 32);
    void fromMemory(void *data, int width, int height, int bpp = 32);
    void fromFile(const char *filename);
    BitmapPtr clone();
    bool savePNG(const char *filename, bool mirror = false);

    //! Set color data of all pixels
    void fill(const Color &color);
    void memset(uint8_t data);

    //! Operations on pixel data
    void blit(BitmapPtr &aim, const Point2 &pos);
    void blit(BitmapPtr &aim, const Rectangle &sourceRectangle, const Rectangle &destinationRectangle);
    void colorize(Color color);
    //! 90, 180 or 270
    BitmapPtr rotated(int degree);

    //! Floor operations
    void floorPixelPosition(int& x, int& y);
    void setPixelFloor(Color col, int x, int y);
    void setPixelFloor(BitmapPtr &img, int sourceX, int sourceY, int destX, int destY);
    void blendPixelFloor(Color col, int x, int y);
    void blendPixelFloor(BitmapPtr &img, int sourceX, int sourceY, int destX, int destY);
    void blitWrap(BitmapPtr &img, int x, int y);

    //! Bitmap attributes & accessing the pixels
    const inline uint8_t *getPixelsConst() const { return bitmap; }
    inline uint8_t *getPixels() { return bitmap; }
    inline int getWidth() const { return w; }
    inline int getHeight() const { return h; }
    inline int getW() const { return w; }
    inline int getH() const { return h; }
    inline uint8_t getBPP() const { return bpp; }
    inline uint8_t getChannels() const { return bpp / 8; }
    inline int getPixelIndexXY(int x, int y) {
        assert(x >= 0 && x < w && y >= 0 && y < h);
        return y * w * (bpp / 8) + x * (bpp / 8);
    }
    inline uint8_t *getPixel(int x, int y) {
        assert(x >= 0 && x < w && y >= 0 && y < h);
        return bitmap + y * w * (bpp / 8)  + x * (bpp / 8);
    }
    inline const uint8_t *getPixelConst(int x, int y) const {
        assert(x >= 0 && x < w && y >= 0 && y < h);
        return bitmap + y * w * (bpp / 8) + x * (bpp / 8);
    }
    Color getPixelColor(int x, int y) const;
    void setPixelColor(int x, int y, const Color &color);
    void setPixel(int x, int y, const uint8_t *color);
    //! Alpha-blending
    void blendPixelColor(int x, int y, const Color &color);

protected:
    void freeData();
    uint8_t *bitmap;
    //! bits per Pixel
    int w, h, bpp;
};

}

/*! BITMAP_HPP_ */
#endif
