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

TextureGL::TextureGL(unsigned int _texture, int _w, TextureSettings settings, int _samples /* = 0 */)
        : Texture(_w, settings, _samples)
{
    texture = _texture;
}

TextureGL::TextureGL(unsigned int _texture, int _w, int _h, TextureSettings settings, int _samples /* = 0 */)
        : Texture(_w, _h, settings, _samples)
{
    texture = _texture;
}

TextureGL::TextureGL(unsigned int _texture, int _w, int _h, int _d, TextureSettings settings, int _samples /* = 0 */)
        : Texture(_w, _h, _d, settings, _samples)
{
    texture = _texture;
}

TextureGL::~TextureGL()
{
    glDeleteTextures(1, &texture);
}

void TextureGL::uploadPixelData(int width, void *pixelData, PixelFormat pixelFormat)
{
    TexturePtr texturePtr = shared_from_this();
    Renderer->bindTexture(texturePtr);
    glTexSubImage1D(settings.type, 0, 0, width, pixelFormat.pixelFormat, pixelFormat.pixelType, pixelData);
}

void TextureGL::uploadPixelData(int width, int height, void *pixelData, PixelFormat pixelFormat)
{
    TexturePtr texturePtr = shared_from_this();
    Renderer->bindTexture(texturePtr);
    glTexSubImage2D(settings.type, 0, 0, 0, width, height, pixelFormat.pixelFormat, pixelFormat.pixelType, pixelData);
}

void TextureGL::uploadPixelData(int width, int height, int depth, void *pixelData, PixelFormat pixelFormat)
{
    TexturePtr texturePtr = shared_from_this();
    Renderer->bindTexture(texturePtr);
    glTexSubImage3D(settings.type, 0, 0, 0, 0, width, height, depth, pixelFormat.pixelFormat, pixelFormat.pixelType,
            pixelData);
}

TexturePtr TextureGL::createTextureView() {
    GLuint textureViewGL;
    glGenTextures(1, &textureViewGL);
    glTextureView(textureViewGL, GL_TEXTURE_2D, this->texture, settings.internalFormat, 0, 1, 0, 1);
    return TexturePtr(new TextureGL(textureViewGL, w, h, d, settings, samples));
}

}
