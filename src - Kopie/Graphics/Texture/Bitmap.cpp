/*
 * Bitmap.cpp
 *
 *  Created on: 01.10.2016
 *      Author: Christoph Neuhauser
 */

#include "Bitmap.hpp"
#include <Math/Math.hpp>
#include <Math/Geometry/Rectangle.hpp>
#include <Math/Geometry/Point2.hpp>
#include <cstring>
#include <iostream>
#include <png.h>

namespace sgl {

void Bitmap::allocate(int width, int height, int _bpp /* = 32 */) {
	if (bitmap != NULL) {
		freeData();
	}

	w = width;
	h = height;
	bpp = _bpp;
	bitmap = new uint8_t[width * height * bpp / 8];
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
	std::memset(bitmap, data, w * h * bpp / 8);
}

void Bitmap::fromMemory(void *data, int width, int height, int _bpp /* = 32 */) {
	if (bitmap != NULL) {
		freeData();
	}

	w = width;
	h = height;
	bpp = _bpp;
	bitmap = new uint8_t[w * h * bpp / 8];
	std::memcpy(bitmap, data, w * h * bpp / 8);

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
		memcpy(aim->getPixel(startx, y), this->getPixel(startx-pos.x, y-pos.y), (endx-startx+1)*getBPP()/8);
	}
}

void Bitmap::blit(BitmapPtr &aim, const Rectangle &sourceRectangle, const Rectangle &destinationRectangle)
{
	int sourceX = sourceRectangle.x;
	int sourceY = sourceRectangle.y;
	int sourceW = sourceRectangle.w;
	int sourceH = sourceRectangle.h;
	int destX = destinationRectangle.x;
	int destY = destinationRectangle.y;
	int destW = destinationRectangle.w;
	int destH = destinationRectangle.h;

	assert(sourceW == destW && sourceH == destH);
	assert(sourceX >= 0 && sourceY >= 0 && destX >= 0 && destY >= 0);
	assert(sourceX + sourceW <= this->getW() && sourceY + sourceH <= this->getH());
	assert(destX + destW <= aim->getW() && destY + destH <= aim->getH());
	assert(this->getBPP() == aim->getBPP());

	for (int y = 0; y < sourceH - sourceY; ++y) {
		int x = 0;
		memcpy(aim->getPixel(destX+x, destY+y), this->getPixel(sourceX+x, sourceY+y), sourceW*getBPP()/8);
	}
}

// TODO
BitmapPtr Bitmap::resizeBiCubic(int destW, int destH)
{
	BitmapPtr resizedBitmap(new Bitmap);
	resizedBitmap->allocate(destW, destH, bpp);
	uint8_t *out = resizedBitmap->getPixels();

	const float tx = float(w) / destW;
	const float ty = float(h) / destH;
	const int channels = bpp/8;
	const size_t row_stride = destW * channels;

	unsigned char C[5] = {0, 0, 0, 0, 0};

	for (int i = 0; i < destW; ++i)  {
		for (int j = 0; j < destH; ++j) {
			const int x = int(tx * j);
			const int y = int(ty * i);
			const float dx = tx * j - x;
			const float dy = ty * i - y;

			for (int k = 0; k < channels; ++k) {
				for (int jj = 0; jj < 4; ++jj)
				{
					const int z = y - 1 + jj;
					unsigned char a0 = *(getPixel(z, x) + k);
					unsigned char d0 = *(getPixel(z, x - 1) + k) - a0;
					unsigned char d2 = *(getPixel(z, x + 1) + k) - a0;
					unsigned char d3 = *(getPixel(z, x + 2) + k) - a0;
					unsigned char a1 = -1.0 / 3 * d0 + d2 - 1.0 / 6 * d3;
					unsigned char a2 = 1.0 / 2 * d0 + 1.0 / 2 * d2;
					unsigned char a3 = -1.0 / 6 * d0 - 1.0 / 2 * d2 + 1.0 / 6 * d3;
					C[jj] = a0 + a1 * dx + a2 * dx * dx + a3 * dx * dx * dx;

					d0 = C[0] - C[1];
					d2 = C[2] - C[1];
					d3 = C[3] - C[1];
					a0 = C[1];
					a1 = -1.0 / 3 * d0 + d2 -1.0 / 6 * d3;
					a2 = 1.0 / 2 * d0 + 1.0 / 2 * d2;
					a3 = -1.0 / 6 * d0 - 1.0 / 2 * d2 + 1.0 / 6 * d3;
					out[i * row_stride + j * channels + k] = a0 + a1 * dy + a2 * dy * dy + a3 * dy * dy * dy;
				}
			}
		}
	}

	return resizedBitmap;
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
		std::cerr << "ERROR: Bitmap::fromFile: Cannot load file \"" << filename
				<< "\"." << std::endl;
		return;
	}

	// Read the header
	fread(header, 1, 8, fp);

	if (png_sig_cmp(header, 0, 8)) {
		std::cerr << "ERROR: Bitmap::fromFile: The file \"" << filename
				<< "\" is not a PNG file." << std::endl;
		fclose(fp);
		return;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
			NULL, NULL);
	if (!png_ptr) {
		std::cerr << "ERROR: Bitmap::fromFile: png_create_read_struct returned 0."
				<< std::endl;
		fclose(fp);
		return;
	}

	// Create png_infop struct
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		std::cerr << "ERROR: Bitmap::fromFile: png_create_info_struct returned 0."
				<< std::endl;
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		fclose(fp);
		return;
	}

	// Create png info struct
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		std::cerr
				<< "ERROR: Bitmap::fromFile: png_create_info_struct returned 0. (2)"
				<< std::endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		fclose(fp);
		return;
	}

	// This code gets called if libpng encounters an error
	if (setjmp(png_jmpbuf(png_ptr))) {
		std::cerr << "ERROR: Bitmap::fromFile: Error in libpng." << std::endl;
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
	png_get_IHDR(png_ptr, info_ptr, &tempWidth, &tempHeight, &bitDepth,
			&colorType, NULL, NULL, NULL);

	if (colorType != PNG_COLOR_TYPE_RGB_ALPHA) {
		std::cerr << "ERROR: Bitmap::fromFile: Only 32-bit RGBA PNG images supported." << std::endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		return;
	}

	w = tempWidth;
	h = tempHeight;
	bpp = bitDepth*4;

	// Update the png info struct.
	png_read_update_info(png_ptr, info_ptr);

	// Row size in bytes.
	int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	// Allocate the imageData as a big block
	bitmap = new uint8_t[rowbytes * tempHeight];
	png_byte *imageData = (png_byte*) bitmap;
	if (imageData == NULL) {
		std::cerr << "ERROR: Bitmap::fromFile: Could not allocate memory for PNG image data." << std::endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		return;
	}

	// rowPointers is pointing to imageData for reading the png with libpng
	png_bytep *rowPointers = new png_bytep[tempHeight];
	if (rowPointers == NULL) {
		std::cerr << "ERROR: Bitmap::fromFile: Could not allocate memory for PNG row pointers." << std::endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		free(imageData);
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
}

bool Bitmap::savePNG(const char *filename, bool mirror /* = false */) {
	FILE *file = NULL;
	file = fopen(filename, "wb");
	if (!file) {
		std::cerr << "ERROR: Bitmap::savePNG: The file couldn't be saved to \""
				<< filename << "\"!" << std::endl;
		return false;
	}

	int pngPixelDataType = PNG_COLOR_TYPE_RGB;
	if (bpp == 32)
		pngPixelDataType = PNG_COLOR_TYPE_RGBA;

	png_structp pngPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING,
			NULL, NULL, NULL);
	png_infop pngInfoPointer = png_create_info_struct(pngPointer);

	png_init_io(pngPointer, file);

	png_set_IHDR(pngPointer, pngInfoPointer, w, h, 8 /* bit depth */,
			pngPixelDataType,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);

	png_write_info(pngPointer, pngInfoPointer);

	png_uint_32 pngHeight = h;
	png_uint_32 rowBytes = w * bpp / 8;

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
	if (bitmap != NULL) {
		delete[] bitmap;
		bitmap = NULL;
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
	memcpy(getPixel(x, y), color, bpp/8);
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
