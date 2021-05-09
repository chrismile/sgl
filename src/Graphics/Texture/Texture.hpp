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

#ifndef GRAPHICS_TEXTURE_TEXTURE_HPP_
#define GRAPHICS_TEXTURE_TEXTURE_HPP_

#include <Defs.hpp>
#include <memory>

#if !defined(GL_NEAREST) && !defined(GL_CLAMP_TO_BORDER)

#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703

#define GL_CLAMP 0x2900
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
#define GL_REPEAT 0x2901

#endif

namespace sgl {

struct PixelFormat {
    PixelFormat() {
        pixelFormat = 0x1908; // GL_RGBA
        pixelType = 0x1401; // GL_UNSIGNED_BYTE
    }
    PixelFormat(int pixelFormat, int pixelType) {
        this->pixelFormat = pixelFormat;
        this->pixelType = pixelType;
    }

    int pixelFormat; // OpenGL: Format of pixel data, e.g. RGB, RGBA, BGRA, Depth, Stencil, ...
    int pixelType; // OpenGL: Type of one pixel data, e.g. Unsigned Byte, Float, ...
};

enum TextureType {
    TEXTURE_1D = 0x0DE0, TEXTURE_2D = 0x0DE1, TEXTURE_3D = 0x806F,
    TEXTURE_1D_ARRAY = 0x8C18, TEXTURE_2D_ARRAY = 0x8C1A,
    TEXTURE_2D_MULTISAMPLE = 0x9100
};

struct TextureSettings {
    TextureSettings() {
        type = TEXTURE_2D;

        textureMinFilter = GL_LINEAR;
        textureMagFilter = GL_LINEAR;
        textureWrapS = GL_CLAMP_TO_EDGE;
        textureWrapT = GL_CLAMP_TO_EDGE;
        textureWrapR = GL_CLAMP_TO_EDGE;
        anisotropicFilter = false;

        internalFormat = 0x1908; // GL_RGBA
        pixelFormat = 0x1908; // GL_RGBA
        pixelType = 0x1401; // GL_UNSIGNED_BYTE
    }
    TextureSettings(TextureType _type, int _textureMinFilter, int _textureMagFilter,
                    int _textureWrapS, int _textureWrapT, int _textureWrapR = GL_CLAMP_TO_EDGE) : TextureSettings() {
        type = _type;
        textureMinFilter = _textureMinFilter;
        textureMagFilter = _textureMagFilter;
        textureWrapS = _textureWrapS;
        textureWrapT = _textureWrapT;
        textureWrapR = _textureWrapR;
    }
    TextureSettings(int _textureMinFilter, int _textureMagFilter,
                    int _textureWrapS, int _textureWrapT, int _textureWrapR = GL_CLAMP_TO_EDGE) : TextureSettings() {
        type = TEXTURE_2D;
        textureMinFilter = _textureMinFilter;
        textureMagFilter = _textureMagFilter;
        textureWrapS = _textureWrapS;
        textureWrapT = _textureWrapT;
        textureWrapR = _textureWrapR;
    }

    TextureType type;
    int textureMinFilter;
    int textureMagFilter;
    int textureWrapS;
    int textureWrapT;
    int textureWrapR;
    bool anisotropicFilter;

    int internalFormat; // OpenGL: Format of data on GPU
    int pixelFormat; // OpenGL: Format of pixel data, e.g. RGB, RGBA, BGRA, Depth, Stencil, ...
    int pixelType; // OpenGL: Type of one pixel data, e.g. Unsigned Byte, Float, ...
};

// For binding both the depth and stencil part of a depth-stencil texture to a shader.
enum DepthStencilMode {
    DEPTH_STENCIL_TEXTURE_MODE_NO_MODE_SET,
    DEPTH_STENCIL_TEXTURE_MODE_DEPTH_COMPONENT, DEPTH_STENCIL_TEXTURE_MODE_STENCIL_COMPONENT
};

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
typedef std::weak_ptr<Texture> WeakTexturePtr;

class DLL_OBJECT Texture : public std::enable_shared_from_this<Texture>
{
public:
    Texture(int _w, TextureSettings settings, int _samples = 0) : w(_w), h(0), d(0), settings(settings),
                                                                  samples(_samples) {}
    Texture(int _w, int _h, TextureSettings settings, int _samples = 0) : w(_w), h(_h), d(0), settings(settings),
                                                                          samples(_samples) {}
    Texture(int _w, int _h, int _d, TextureSettings settings, int _samples = 0) : w(_w), h(_h), d(_d),
            settings(settings), samples(_samples) {}
    virtual ~Texture() {}
    virtual void uploadPixelData(int width, void *pixelData, PixelFormat pixelFormat = PixelFormat())=0;
    virtual void uploadPixelData(int width, int height, void *pixelData, PixelFormat pixelFormat = PixelFormat())=0;
    virtual void uploadPixelData(int width, int height, int depth, void *pixelData, PixelFormat pixelFormat = PixelFormat())=0;
    /// Do NOT access a texture view anymore after the reference count of the base texture has reached zero!
    virtual TexturePtr createTextureView()=0;
    inline int getW() const { return w; }
    inline int getH() const { return h; }
    inline int getMinificationFilter() const { return settings.textureMinFilter; }
    inline int getMagnificationFilter() const { return settings.textureMagFilter; }
    inline int getWrapS() const { return settings.textureWrapS; }
    inline int getWrapT() const { return settings.textureWrapT; }
    inline int getWrapR() const { return settings.textureWrapR; }
    inline TextureSettings getSettings() const { return settings; }
    inline TextureType getTextureType() const { return settings.type; }
    inline bool isMultisampledTexture() const { return samples > 0; }
    inline int getNumSamples() const { return samples; }
    inline bool hasManualDepthStencilComponentMode() const { return depthStencilMode != DEPTH_STENCIL_TEXTURE_MODE_NO_MODE_SET; }
    inline bool hasDepthComponentMode() const { return depthStencilMode == DEPTH_STENCIL_TEXTURE_MODE_DEPTH_COMPONENT; }
    inline bool hasStencilComponentMode() const { return depthStencilMode == DEPTH_STENCIL_TEXTURE_MODE_STENCIL_COMPONENT; }
    inline void setDepthStencilComponentMode(DepthStencilMode _depthStencilMode) { depthStencilMode = _depthStencilMode; }

protected:
    int w;
    int h;
    int d;
    TextureSettings settings;
    /// For MSAA
    int samples;
    DepthStencilMode depthStencilMode = DEPTH_STENCIL_TEXTURE_MODE_NO_MODE_SET;
};

}

/*! GRAPHICS_TEXTURE_TEXTURE_HPP_ */
#endif
