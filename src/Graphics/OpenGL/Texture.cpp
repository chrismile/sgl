/*
 * Texture.cpp
 *
 *  Created on: 31.03.2015
 *      Author: Christoph Neuhauser
 */

#include <GL/glew.h>
#include "Texture.hpp"
#include <Graphics/Renderer.hpp>

namespace sgl {

TextureGL::TextureGL(int _texture, int _w, int _h, int _bpp, int _minificationFilter, int _magnificationFilter,
		int _textureWrapS, int _textureWrapT, int _samples /* = 0 */) : Texture(_w, _h,
				_bpp, _minificationFilter, _magnificationFilter, _textureWrapS, _textureWrapT, _samples)
{
	texture = _texture;
}

TextureGL::~TextureGL()
{
	glDeleteTextures(1, &texture);
}

void TextureGL::uploadPixelData(int width, int height, void *pixelData, PixelFormat pixelFormat /* = PIXEL_FORMAT_RGBA */)
{
	TexturePtr texturePtr = shared_from_this();
	Renderer->bindTexture(texturePtr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
}

}
