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

#ifndef GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_
#define GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_

#include <Graphics/Texture/TextureManager.hpp>

namespace sgl {

class DLL_OBJECT TextureManagerGL : public TextureManagerInterface {
public:
    TexturePtr createEmptyTexture(
            int width, const TextureSettings& settings = TextureSettings()) override;
    TexturePtr createTexture(
            void* data, int width, const PixelFormat& pixelFormat,
            const TextureSettings& settings = TextureSettings()) override;
    TexturePtr createEmptyTexture(
            int width, int height, const TextureSettings& settings = TextureSettings()) override;
    TexturePtr createTexture(
            void* data, int width, int height, const PixelFormat& pixelFormat,
            const TextureSettings& settings = TextureSettings()) override;
    TexturePtr createEmptyTexture(
            int width, int height, int depth, const TextureSettings& settings = TextureSettings()) override;
    TexturePtr createTexture(
            void* data, int width, int height, int depth, const PixelFormat& pixelFormat,
            const TextureSettings& settings = TextureSettings()) override;

    /**
     * Uses glTexStorage<x>D for creating an immutable texture.
     */
    TexturePtr createTextureStorage(
            int width, const TextureSettings& settings = TextureSettings()) override;
    TexturePtr createTextureStorage(
            int width, int height, const TextureSettings& settings = TextureSettings()) override;
    TexturePtr createTextureStorage(
            int width, int height, int depth, const TextureSettings& settings = TextureSettings()) override;

    /**
     * Only for FBOs!
     * @param fixedSampleLocations Must be true if the texture is used in combination with a renderbuffer.
     */
    TexturePtr createMultisampledTexture(
            int width, int height, int numSamples,
            int internalFormat = 0x8058 /*GL_RGBA8*/, bool fixedSampleLocations = true) override;
    /// bitsPerPixel must be 16, 24 or 32
    TexturePtr createDepthTexture(
            int width, int height, DepthTextureFormat format = DEPTH_COMPONENT16,
            int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR) override;
    TexturePtr createDepthStencilTexture(
            int width, int height, DepthStencilTextureFormat format = DEPTH24_STENCIL8,
            int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR) override;

protected:
    TexturePtr loadAsset(TextureInfo& textureInfo) override;
};

}

/*! GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_ */
#endif
