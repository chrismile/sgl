/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_WEBGPU_TEXTURE_HPP
#define SGL_WEBGPU_TEXTURE_HPP

#include <memory>
#include <functional>

#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

class Device;

struct DLL_OBJECT TextureSettings {
    WGPUTextureUsageFlags usage = WGPUTextureUsage_None;
    WGPUTextureDimension dimension = WGPUTextureDimension_2D;
    WGPUExtent3D size = { 1, 1, 1 };
    WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    // If left empty, TextureSettings::format is used.
    std::vector<WGPUTextureFormat> viewFormats;
    std::string label;
};

struct DLL_OBJECT TextureWriteInfo {
    const void* srcPtr = nullptr;
    size_t srcSize = 0;

    uint64_t srcOffset = 0;
    uint32_t srcBytesPerRow = 0;
    uint32_t srcRowsPerImage = 0;

    WGPUExtent3D dstWriteSize{};
    uint32_t dstMipLevel = 0;
    WGPUOrigin3D dstOrigin{};
    WGPUTextureAspect dstAspect = WGPUTextureAspect_All;
};

DLL_OBJECT size_t getTextureFormatEntryByteSize(WGPUTextureFormat format);
DLL_OBJECT size_t getTextureFormatNumChannels(WGPUTextureFormat format);

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
class TextureView;
typedef std::shared_ptr<TextureView> TextureViewPtr;
class Sampler;
typedef std::shared_ptr<Sampler> SamplerPtr;

class DLL_OBJECT Texture {
public:
    Texture(Device* device, TextureSettings _textureSettings);
    Texture(Device* device, TextureSettings _textureSettings, WGPUTexture texture);
    ~Texture();

    void write(const TextureWriteInfo& writeInfo, WGPUQueue queue);
    // This function assumes tightly packed data and updating the whole memory of the texture.
    void write(void const* data, uint64_t dataSize, WGPUQueue queue);

    // Mipmap generation is currently not yet supported.

    [[nodiscard]] const Device* getDevice() const { return device; }
    [[nodiscard]] const TextureSettings& getTextureSettings() const { return textureSettings; }
    [[nodiscard]] const WGPUTexture& getWGPUTexture() const { return texture; }

public:
    Device* device = nullptr;
    TextureSettings textureSettings{};
    WGPUTexture texture{};
    bool hasOwnership = false; // Don't call destroy if we don't have ownership.
};

struct DLL_OBJECT TextureViewSettings {
    // Uses TextureSettings::format when set to WGPUTextureFormat_Undefined.
    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    // Tries to infer from TextureSettings::dimension when set to WGPUTextureViewDimension_Undefined
    WGPUTextureViewDimension dimension = WGPUTextureViewDimension_Undefined;
    uint32_t baseMipLevel = 0;
    uint32_t mipLevelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t arrayLayerCount = 1;
    // Alternatives: WGPUTextureAspect_StencilOnly, WGPUTextureAspect_DepthOnly
    WGPUTextureAspect aspect = WGPUTextureAspect_All;
    std::string label;
};

class DLL_OBJECT TextureView {
public:
    TextureView(TexturePtr _texture, TextureViewSettings _textureViewSettings);
    ~TextureView();

    [[nodiscard]] const Device* getDevice() const { return texture->getDevice(); }
    [[nodiscard]] const TextureSettings& getTextureSettings() const { return texture->getTextureSettings(); }
    [[nodiscard]] const TextureViewSettings& getTextureViewSettings() const { return textureViewSettings; }
    [[nodiscard]] const WGPUTexture& getWGPUTexture() const { return texture->getWGPUTexture(); }
    [[nodiscard]] const WGPUTextureView& getWGPUTextureView() const { return textureView; }

private:
    TexturePtr texture;
    TextureViewSettings textureViewSettings;
    WGPUTextureView textureView{};
};

struct SamplerSettings {
    WGPUAddressMode addressModeU = WGPUAddressMode_ClampToEdge;
    WGPUAddressMode addressModeV = WGPUAddressMode_ClampToEdge;
    WGPUAddressMode addressModeW = WGPUAddressMode_ClampToEdge;
    WGPUFilterMode magFilter = WGPUFilterMode_Linear;
    WGPUFilterMode minFilter = WGPUFilterMode_Linear;
    WGPUMipmapFilterMode mipmapFilter = WGPUMipmapFilterMode_Linear;
    float lodMinClamp = 0.0f;
    float lodMaxClamp = 1.0f;
    WGPUCompareFunction compare = WGPUCompareFunction_Undefined;
    uint16_t maxAnisotropy = 1;
    std::string label;
};

class DLL_OBJECT Sampler {
public:
    Sampler(Device* device, SamplerSettings _samplerSettings);
    ~Sampler();

    [[nodiscard]] const Device* getDevice() const { return device; }
    [[nodiscard]] const SamplerSettings& getSamplerSettings() const { return samplerSettings; }
    [[nodiscard]] const WGPUSampler& getWGPUSampler() const { return sampler; }

private:
    Device* device = nullptr;
    SamplerSettings samplerSettings;
    WGPUSampler sampler{};
};

}}

#endif //SGL_WEBGPU_TEXTURE_HPP
