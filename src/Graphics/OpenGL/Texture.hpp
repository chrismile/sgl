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

#ifndef GRAPHICS_OPENGL_TEXTURE_HPP_
#define GRAPHICS_OPENGL_TEXTURE_HPP_

#include <Graphics/Texture/Texture.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Image/Image.hpp>
#endif

namespace sgl {

class DLL_OBJECT TextureGL : public Texture {
public:
    TextureGL(unsigned int _texture, int _w, TextureSettings settings, int _samples = 0);
    TextureGL(unsigned int _texture, int _w, int _h, TextureSettings settings, int _samples = 0);
    TextureGL(unsigned int _texture, int _w, int _h, int _d, TextureSettings settings, int _samples = 0);
    ~TextureGL() override;
    void uploadPixelData(int width, const void* pixelData, PixelFormat pixelFormat = PixelFormat()) override;
    void uploadPixelData(int width, int height, const void* pixelData, PixelFormat pixelFormat = PixelFormat()) override;
    void uploadPixelData(int width, int height, int depth, const void* pixelData, PixelFormat pixelFormat = PixelFormat()) override;
    /// Do NOT access a texture view anymore after the reference count of the base texture has reached zero!
    TexturePtr createTextureView() override;
    inline unsigned int getTexture() const { return texture; }

protected:
    unsigned int texture;
};

#if defined(SUPPORT_VULKAN) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
/**
 * An OpenGL texture object created from external Vulkan memory.
 */
class DLL_OBJECT TextureGLExternalMemoryVk : public TextureGL {
public:
    explicit TextureGLExternalMemoryVk(vk::TexturePtr& vulkanTexture);
    ~TextureGLExternalMemoryVk() override;

protected:
    vk::ImagePtr vulkanImage;
    GLuint memoryObject = 0;

#ifdef _WIN32
    void* handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};
#endif

}

/*! GRAPHICS_OPENGL_TEXTURE_HPP_ */
#endif
