/*!
 * Texture.hpp
 *
 *  Created on: 31.03.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_TEXTURE_HPP_
#define GRAPHICS_OPENGL_TEXTURE_HPP_

#include <Graphics/Texture/Texture.hpp>

namespace sgl {

class DLL_OBJECT TextureGL : public Texture
{
public:
	TextureGL(int _texture, int _w, int _h, int _bpp, int _minificationFilter, int _magnificationFilter,
			int _textureWrapS, int _textureWrapT, int _samples = 0);
	virtual ~TextureGL();
	virtual void uploadPixelData(int width, int height, void *pixelData, PixelFormat pixelFormat = PIXEL_FORMAT_RGBA);
	inline unsigned int getTexture() const { return texture; }

protected:
	unsigned int texture;
};

}

/*! GRAPHICS_OPENGL_TEXTURE_HPP_ */
#endif
