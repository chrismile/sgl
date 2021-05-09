/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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
