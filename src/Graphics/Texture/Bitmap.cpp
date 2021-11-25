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

#include "Bitmap.hpp"
#include <Utils/File/Logfile.hpp>
#include <Math/Math.hpp>
#include <Math/Geometry/Rectangle.hpp>
#include <Math/Geometry/Point2.hpp>
#include <cstring>
#include <iostream>
#include <png.h>

namespace sgl {

void Bitmap::allocate(int width, int height, int _bpp /* = 32 */) {
    if (bitmap != nullptr) {
        freeData();
    }

    w = width;
    h = height;
    bpp = _bpp;
    bitmap = new uint8_t[width * height * (bpp / 8)];
}

void Bitmap::fill(const Color &color) {
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            *getPixel(x, y) = color.getR();
            *(getPixel(x, y) + 1) = color.getG();
            *(getPixel(x, y) + 2) = color.getB();
            *(getPixel(x, y) + 3) = color.getA();
        }
    }
}

void Bitmap::memset(uint8_t data) {
    std::memset(bitmap, data, w * h * (bpp / 8));
}

void Bitmap::fromMemory(void *data, int width, int height, int _bpp /* = 32 */) {
    if (bitmap != nullptr) {
        freeData();
    }

    w = width;
    h = height;
    bpp = _bpp;
    bitmap = new uint8_t[w * h * (bpp / 8)];
    std::memcpy(bitmap, data, w * h * (bpp / 8));

}

BitmapPtr Bitmap::clone() {
    BitmapPtr clonedBitmap(new Bitmap);
    clonedBitmap->fromMemory(this->getPixel(0, 0), this->getW(), this->getH(), this->getBPP());
    return clonedBitmap;
}

void Bitmap::blit(BitmapPtr &aim, const Point2 &pos)
{
    // No area to be blit?
    if (pos.x >= aim->w || pos.x+w <= 0 || pos.y >= aim->h || pos.y+h <= 0) {
        return;
    }

    assert(this->w > 0 && aim->w && this->h && aim->h);
    int startx = clamp(pos.x, 0, aim->w-1);
    int endx = clamp(pos.x+this->w-1, 0, aim->w-1);
    int starty = clamp(pos.y, 0, aim->h-1);
    int endy = clamp(pos.y+this->h-1, 0, aim->h-1);

    // Copy the relevant scanlines
    for (int y = starty; y <= endy; ++y) {
        memcpy(
                aim->getPixel(startx, y), this->getPixel(startx - pos.x, y - pos.y),
                (endx - startx + 1) * (bpp / 8));
    }
}

void Bitmap::blit(BitmapPtr &aim, const Rectangle &sourceRectangle, const Rectangle &destinationRectangle)
{
    int sourceY = int(sourceRectangle.y);
    int sourceW = int(sourceRectangle.w);
    int sourceX = int(sourceRectangle.x);
    int sourceH = int(sourceRectangle.h);
    int destX = int(destinationRectangle.x);
    int destY = int(destinationRectangle.y);
    int destW = int(destinationRectangle.w);
    int destH = int(destinationRectangle.h);
    UNUSED(destW);
    UNUSED(destH);

    assert(sourceW == destW && sourceH == destH);
    assert(sourceX >= 0 && sourceY >= 0 && destX >= 0 && destY >= 0);
    assert(sourceX + sourceW <= this->getW() && sourceY + sourceH <= this->getH());
    assert(destX + destW <= aim->getW() && destY + destH <= aim->getH());
    assert(this->getBPP() == aim->getBPP());

    for (int y = 0; y < sourceH - sourceY; ++y) {
        int x = 0;
        memcpy(
                aim->getPixel(destX + x, destY + y), this->getPixel(sourceX + x, sourceY + y),
                sourceW * (bpp / 8));
    }
}

void Bitmap::colorize(Color color)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Color oldColor = getPixelColor(x, y);
            oldColor.setR(color.getR());
            oldColor.setG(color.getG());
            oldColor.setB(color.getB());
            setPixelColor(x, y, oldColor);
        }
    }
}

BitmapPtr Bitmap::rotated(int degree)
{
    BitmapPtr bitmap(new Bitmap);

    if (degree == 90) {
        bitmap->allocate(h, w);
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                // (x,y) -> (y,w-x-1)
                bitmap->setPixelColor(y, w-x-1, getPixelColor(x,y));
            }
        }
    } else if (degree == 180) {
        bitmap->allocate(w, h);
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                // (x,y) -> (w-x-1,h-y-1)
                bitmap->setPixelColor(w-x-1, w-x-1, getPixelColor(x,y));
            }
        }
    } else if (degree == 270) {
        bitmap->allocate(h, w);
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                // (x,y) -> (h-y-1,x)
                bitmap->setPixelColor(h-y-1, x, getPixelColor(x,y));
            }
        }
    }

    return bitmap;
}

void Bitmap::fromFile(const char *filename) {
    png_byte header[8];

    FILE *fp = fopen(filename, "rb");
    if (fp == 0) {
        sgl::Logfile::get()->writeError(
                std::string() + "ERROR: Bitmap::fromFile: Cannot load file \"" + filename + "\".");
        return;
    }

    // Read the header
    size_t numBytes = fread(header, 1, 8, fp);
    if (numBytes != 8) {
        sgl::Logfile::get()->writeError("ERROR: Bitmap::fromFile: fread failed.");
        fclose(fp);
        return;
    }

    if (png_sig_cmp(header, 0, 8)) {
        sgl::Logfile::get()->writeError(
                std::string() + "ERROR: Bitmap::fromFile: The file \"" + filename + "\" is not a PNG file.");
        fclose(fp);
        return;
    }

    png_structp png_ptr = png_create_read_struct(
            PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        sgl::Logfile::get()->writeError("ERROR: Bitmap::fromFile: png_create_read_struct returned 0.");
        fclose(fp);
        return;
    }

    // Create png_infop struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        sgl::Logfile::get()->writeError("ERROR: Bitmap::fromFile: png_create_info_struct returned 0.");
        png_destroy_read_struct(&png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
        fclose(fp);
        return;
    }

    // Create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        sgl::Logfile::get()->writeError("ERROR: Bitmap::fromFile: png_create_info_struct returned 0. (2)");
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
        fclose(fp);
        return;
    }

    // This code gets called if libpng encounters an error
    if (setjmp(png_jmpbuf(png_ptr))) {
        sgl::Logfile::get()->writeError("ERROR: Bitmap::fromFile: Error in libpng.");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return;
    }

    // Initialize png reading
    png_init_io(png_ptr, fp);

    // Let libpng know the program already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);

    // Read all the info up to the image data
    png_read_info(png_ptr, info_ptr);

    // Variables to pass to get info
    int bitDepth, colorType;
    png_uint_32 tempWidth, tempHeight;

    // Get the information about the PNG
    png_get_IHDR(
            png_ptr, info_ptr, &tempWidth, &tempHeight, &bitDepth,
            &colorType, nullptr, nullptr, nullptr);

    if (colorType != PNG_COLOR_TYPE_RGB_ALPHA && colorType != PNG_COLOR_TYPE_RGB) {
        sgl::Logfile::get()->writeError(
                "ERROR: Bitmap::fromFile: Only 32-bit RGBA or 24-bit RGB PNG images supported.");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return;
    }

    w = tempWidth;
    h = tempHeight;
    bpp = bitDepth * 4;

    // Update the png info struct.
    png_read_update_info(png_ptr, info_ptr);

    // Row size in bytes.
    size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    // Allocate the imageData as a big block
    uint8_t *dataPointer = new uint8_t[rowbytes * tempHeight];
    png_byte *imageData = (png_byte*)dataPointer;
    if (imageData == nullptr) {
        sgl::Logfile::get()->writeError("ERROR: Bitmap::fromFile: Could not allocate memory for PNG image data.");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return;
    }

    // rowPointers is pointing to imageData for reading the png with libpng
    png_bytep *rowPointers = new png_bytep[tempHeight];
    if (rowPointers == nullptr) {
        sgl::Logfile::get()->writeError("ERROR: Bitmap::fromFile: Could not allocate memory for PNG row pointers.");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        delete[] imageData;
        fclose(fp);
        return;
    }

    // Set the individual row_pointers to point at the correct offsets of imageData
    for (int i = 0; i < (int) tempHeight; i++) {
        rowPointers[i] = imageData + i * rowbytes; // tempHeight - 1 - i
    }

    // Read the png into image_data through rowPointers
    png_read_image(png_ptr, rowPointers);

    // Clean up
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    delete[] rowPointers;
    fclose(fp);

    if (colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
        bitmap = dataPointer;
    } else {
        // Convert to RGBA
        bitmap = new uint8_t[w*h*4];
        for (int i = 0; i < w*h; i++) {
            bitmap[i*4+0] = dataPointer[i*3+0];
            bitmap[i*4+1] = dataPointer[i*3+1];
            bitmap[i*4+2] = dataPointer[i*3+2];
            bitmap[i*4+3] = 255;
        }
        delete[] dataPointer;
    }
}

bool Bitmap::savePNG(const char *filename, bool mirror /* = false */) {
    FILE *file = nullptr;
    file = fopen(filename, "wb");
    if (!file) {
        std::cerr << "ERROR: Bitmap::savePNG: The file couldn't be saved to \""
                << filename << "\"!" << std::endl;
        return false;
    }

    int pngPixelDataType = PNG_COLOR_TYPE_RGB;
    if (bpp == 32)
        pngPixelDataType = PNG_COLOR_TYPE_RGBA;

    png_structp pngPointer = png_create_write_struct(
            PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop pngInfoPointer = png_create_info_struct(pngPointer);

    png_init_io(pngPointer, file);

    png_set_IHDR(
            pngPointer, pngInfoPointer, w, h, 8 /* bit depth */,
            pngPixelDataType,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

    png_write_info(pngPointer, pngInfoPointer);

    png_uint_32 pngHeight = h;
    png_uint_32 rowBytes = w * (bpp / 8);

    //png_byte *image = new png_byte[pngHeight * rowBytes];
    png_bytep *rowPointers = new png_bytep[pngHeight];

    // Does the program need to reverse the direction?
    if (mirror) {
        for (unsigned int i = 0; i < pngHeight; i++) {
            rowPointers[i] = bitmap + ((pngHeight-i-1) * rowBytes);
        }
    } else {
        for (unsigned int i = 0; i < pngHeight; i++) {
            rowPointers[i] = bitmap + (i * rowBytes);
        }
    }

    png_write_image(pngPointer, rowPointers);
    png_write_end(pngPointer, pngInfoPointer);
    png_destroy_write_struct(&pngPointer, &pngInfoPointer);

    delete[] rowPointers;
    fclose(file);

    return true;
}

void Bitmap::freeData() {
    if (bitmap != nullptr) {
        delete[] bitmap;
        bitmap = nullptr;
    }
}

Color Bitmap::getPixelColor(int x, int y) const
{
    assert(x >= 0 && x < w && y >= 0 && y < h);
    const uint8_t *pixels = getPixelConst(x, y);
    return Color(pixels[0], pixels[1], pixels[2], pixels[3]);
}

void Bitmap::setPixelColor(int x, int y, const Color &color)
{
    assert(x >= 0 && x < w && y >= 0 && y < h);
    uint8_t *pixels = getPixel(x, y);
    pixels[0] = color.getR();
    pixels[1] = color.getG();
    pixels[2] = color.getB();
    pixels[3] = color.getA();
}

void Bitmap::setPixel(int x, int y, const uint8_t *color)
{
    memcpy(getPixel(x, y), color, bpp / 8);
}

void Bitmap::blendPixelColor(int x, int y, const Color &color)
{
    assert(x >= 0 && x < w && y >= 0 && y < h);

    int a = color.getA();
    int ia = 255 - a;

    Color destCol = getPixelColor(x, y);
    int destr = ((int)color.getR() * a) / 255 + ((int)destCol.getR() * ia) / 255;
    int destg = ((int)color.getG() * a) / 255 + ((int)destCol.getG() * ia) / 255;
    int destb = ((int)color.getB() * a) / 255 + ((int)destCol.getB() * ia) / 255;
    int desta = a + ((int)destCol.getA() * ia) / 255;
    setPixelColor(x, y, Color(destr, destg, destb, desta));
}



// -------------- Floor -----------------

void Bitmap::floorPixelPosition(int& x, int& y) {
    x = floorMod(x, w);
    y = floorMod(y, h);
}

void Bitmap::setPixelFloor(Color col, int x, int y) {
    floorPixelPosition(x,y);
    setPixelColor(x, y, col);
}

void Bitmap::setPixelFloor(BitmapPtr &img, int sourceX, int sourceY, int destX, int destY) {
    setPixelFloor(img->getPixelColor(sourceX, sourceY), destX, destY);
}

void Bitmap::blendPixelFloor(Color col, int x, int y) {
    floorPixelPosition(x,y);
    blendPixelColor(x, y, col);
}

void Bitmap::blendPixelFloor(BitmapPtr &img, int sourceX, int sourceY, int destX, int destY) {
    blendPixelFloor(img->getPixelColor(sourceX, sourceY), destX, destY);
}

void Bitmap::blitWrap(BitmapPtr &img, int x, int y) {
    for (int sourceY = 0; sourceY < img->getHeight(); ++sourceY) {
        int destY = sourceY + y;
        for (int sourceX = 0; sourceX < img->getWidth(); ++sourceX) {
            int destX = sourceX + x;
            blendPixelFloor(img, sourceX, sourceY, destX, destY);
        }
    }
}

}
