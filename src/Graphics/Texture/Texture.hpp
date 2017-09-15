/*
 * Texture.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_TEXTURE_TEXTURE_HPP_
#define GRAPHICS_TEXTURE_TEXTURE_HPP_

#include <Defs.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace sgl {

enum PixelFormat {
	PIXEL_FORMAT_RGBA
};

#if !defined(GL_NEAREST) && !defined(GL_CLAMP_TO_BORDER)

#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703

#define GL_CLAMP 0x2900
#define GL_CLAMP_TO_BORDER 0x812F
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
#define GL_REPEAT 0x2901

#endif

class DLL_OBJECT Texture : public boost::enable_shared_from_this<Texture>
{
public:
	Texture(int _w, int _h, int _bpp, int _minificationFilter, int _magnificationFilter,
			int _textureWrapS, int _textureWrapT, int _samples = 0) : w(_w), h(_h), bpp(_bpp),
			minificationFilter(_minificationFilter), magnificationFilter(_magnificationFilter),
			textureWrapS(_textureWrapS), textureWrapT(_textureWrapT), samples(_samples) {}
	virtual ~Texture() {}
	virtual void uploadPixelData(int width, int height, void *pixelData, PixelFormat pixelFormat = PIXEL_FORMAT_RGBA)=0;
	inline int getBPP() const { return bpp; }
	inline int getW() const { return w; }
	inline int getH() const { return h; }
	inline int getMinificationFilter() const { return minificationFilter; }
	inline int getMagnificationFilter() const { return magnificationFilter; }
	inline int getWrapS() const { return textureWrapS; }
	inline int getWrapT() const { return textureWrapT; }
	inline bool isMultisampledTexture() const { return samples > 0; }
	inline int getNumSamples() const { return samples; }

protected:
	int w;
	int h;
	int bpp;
	int minificationFilter, magnificationFilter, textureWrapS, textureWrapT;
	int samples; // For MSAA
};

typedef boost::shared_ptr<Texture> TexturePtr;
typedef boost::weak_ptr<Texture> WeakTexturePtr;

}

#endif /* GRAPHICS_TEXTURE_TEXTURE_HPP_ */
