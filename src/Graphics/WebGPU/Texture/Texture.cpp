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

#include <Math/Math.hpp>
#include <Utils/File/Logfile.hpp>
#include <utility>
#include <cstring>
#include "../Utils/Device.hpp"
#include "Texture.hpp"

namespace sgl { namespace webgpu {

size_t getTextureFormatEntryByteSize(WGPUTextureFormat format) {
    // Compressed formats not supported so far.
    switch (format) {
        case WGPUTextureFormat_R8Unorm:
        case WGPUTextureFormat_R8Snorm:
        case WGPUTextureFormat_R8Uint:
        case WGPUTextureFormat_R8Sint:
        case WGPUTextureFormat_Stencil8:
            return 1;
        case WGPUTextureFormat_R16Uint:
        case WGPUTextureFormat_R16Sint:
        case WGPUTextureFormat_R16Float:
        case WGPUTextureFormat_RG8Unorm:
        case WGPUTextureFormat_RG8Snorm:
        case WGPUTextureFormat_RG8Uint:
        case WGPUTextureFormat_RG8Sint:
        case WGPUTextureFormat_Depth16Unorm:
            return 2;
            return 3;
        case WGPUTextureFormat_R32Float:
        case WGPUTextureFormat_R32Uint:
        case WGPUTextureFormat_R32Sint:
        case WGPUTextureFormat_Depth32Float:
        case WGPUTextureFormat_RG16Uint:
        case WGPUTextureFormat_RG16Sint:
        case WGPUTextureFormat_RG16Float:
        case WGPUTextureFormat_Depth24Plus:
        case WGPUTextureFormat_Depth24PlusStencil8:
        case WGPUTextureFormat_RG11B10Ufloat:
        case WGPUTextureFormat_RGBA8Unorm:
        case WGPUTextureFormat_RGBA8UnormSrgb:
        case WGPUTextureFormat_RGBA8Snorm:
        case WGPUTextureFormat_RGBA8Uint:
        case WGPUTextureFormat_RGBA8Sint:
        case WGPUTextureFormat_BGRA8Unorm:
        case WGPUTextureFormat_BGRA8UnormSrgb:
        case WGPUTextureFormat_RGB10A2Uint:
        case WGPUTextureFormat_RGB10A2Unorm:
            return 4;
        case WGPUTextureFormat_Depth32FloatStencil8:
            return 5; // Is this 8 internally? But we never use this for host -> device copies anyways...
        case WGPUTextureFormat_RG32Float:
        case WGPUTextureFormat_RG32Uint:
        case WGPUTextureFormat_RG32Sint:
        case WGPUTextureFormat_RGBA16Uint:
        case WGPUTextureFormat_RGBA16Sint:
        case WGPUTextureFormat_RGBA16Float:
            return 8;
        case WGPUTextureFormat_RGBA32Float:
        case WGPUTextureFormat_RGBA32Uint:
        case WGPUTextureFormat_RGBA32Sint:
            return 16;
        //case WGPUTextureFormat_RGB9E5Ufloat: ?
        default:
            return 0;
    }
}

size_t getTextureFormatNumChannels(WGPUTextureFormat format) {
    switch (format) {
        case WGPUTextureFormat_R8Unorm:
        case WGPUTextureFormat_R8Snorm:
        case WGPUTextureFormat_R8Uint:
        case WGPUTextureFormat_R8Sint:
        case WGPUTextureFormat_R16Uint:
        case WGPUTextureFormat_R16Sint:
        case WGPUTextureFormat_R16Float:
        case WGPUTextureFormat_RG8Unorm:
        case WGPUTextureFormat_RG8Snorm:
        case WGPUTextureFormat_RG8Uint:
        case WGPUTextureFormat_RG8Sint:
        case WGPUTextureFormat_R32Float:
        case WGPUTextureFormat_R32Uint:
        case WGPUTextureFormat_R32Sint:
        case WGPUTextureFormat_Stencil8:
        case WGPUTextureFormat_Depth16Unorm:
        case WGPUTextureFormat_Depth24Plus:
        case WGPUTextureFormat_Depth32Float:
        case WGPUTextureFormat_BC4RUnorm:
        case WGPUTextureFormat_BC4RSnorm:
        case WGPUTextureFormat_EACR11Unorm:
        case WGPUTextureFormat_EACR11Snorm:
        case WGPUTextureFormat_EACRG11Unorm:
        case WGPUTextureFormat_EACRG11Snorm:
            return 1;
        case WGPUTextureFormat_RG16Uint:
        case WGPUTextureFormat_RG16Sint:
        case WGPUTextureFormat_RG16Float:
        case WGPUTextureFormat_RG32Float:
        case WGPUTextureFormat_RG32Uint:
        case WGPUTextureFormat_RG32Sint:
        case WGPUTextureFormat_Depth24PlusStencil8:
        case WGPUTextureFormat_Depth32FloatStencil8:
        case WGPUTextureFormat_RG11B10Ufloat:
        case WGPUTextureFormat_BC5RGUnorm:
        case WGPUTextureFormat_BC5RGSnorm:
            return 2;
        case WGPUTextureFormat_BC6HRGBUfloat:
        case WGPUTextureFormat_BC6HRGBFloat:
            return 3;
        case WGPUTextureFormat_RGBA8Unorm:
        case WGPUTextureFormat_RGBA8UnormSrgb:
        case WGPUTextureFormat_RGBA8Snorm:
        case WGPUTextureFormat_RGBA8Uint:
        case WGPUTextureFormat_RGBA8Sint:
        case WGPUTextureFormat_BGRA8Unorm:
        case WGPUTextureFormat_BGRA8UnormSrgb:
        case WGPUTextureFormat_RGB10A2Uint:
        case WGPUTextureFormat_RGB10A2Unorm:
        case WGPUTextureFormat_RGB9E5Ufloat:
        case WGPUTextureFormat_RGBA16Uint:
        case WGPUTextureFormat_RGBA16Sint:
        case WGPUTextureFormat_RGBA16Float:
        case WGPUTextureFormat_RGBA32Float:
        case WGPUTextureFormat_RGBA32Uint:
        case WGPUTextureFormat_RGBA32Sint:
        case WGPUTextureFormat_BC1RGBAUnorm:
        case WGPUTextureFormat_BC1RGBAUnormSrgb:
        case WGPUTextureFormat_BC2RGBAUnorm:
        case WGPUTextureFormat_BC2RGBAUnormSrgb:
        case WGPUTextureFormat_BC3RGBAUnorm:
        case WGPUTextureFormat_BC3RGBAUnormSrgb:
        case WGPUTextureFormat_BC7RGBAUnorm:
        case WGPUTextureFormat_BC7RGBAUnormSrgb:
        case WGPUTextureFormat_ETC2RGB8Unorm:
        case WGPUTextureFormat_ETC2RGB8UnormSrgb:
        case WGPUTextureFormat_ETC2RGB8A1Unorm:
        case WGPUTextureFormat_ETC2RGB8A1UnormSrgb:
        case WGPUTextureFormat_ETC2RGBA8Unorm:
        case WGPUTextureFormat_ETC2RGBA8UnormSrgb:
        case WGPUTextureFormat_ASTC4x4Unorm:
        case WGPUTextureFormat_ASTC4x4UnormSrgb:
        case WGPUTextureFormat_ASTC5x4Unorm:
        case WGPUTextureFormat_ASTC5x4UnormSrgb:
        case WGPUTextureFormat_ASTC5x5Unorm:
        case WGPUTextureFormat_ASTC5x5UnormSrgb:
        case WGPUTextureFormat_ASTC6x5Unorm:
        case WGPUTextureFormat_ASTC6x5UnormSrgb:
        case WGPUTextureFormat_ASTC6x6Unorm:
        case WGPUTextureFormat_ASTC6x6UnormSrgb:
        case WGPUTextureFormat_ASTC8x5Unorm:
        case WGPUTextureFormat_ASTC8x5UnormSrgb:
        case WGPUTextureFormat_ASTC8x6Unorm:
        case WGPUTextureFormat_ASTC8x6UnormSrgb:
        case WGPUTextureFormat_ASTC8x8Unorm:
        case WGPUTextureFormat_ASTC8x8UnormSrgb:
        case WGPUTextureFormat_ASTC10x5Unorm:
        case WGPUTextureFormat_ASTC10x5UnormSrgb:
        case WGPUTextureFormat_ASTC10x6Unorm:
        case WGPUTextureFormat_ASTC10x6UnormSrgb:
        case WGPUTextureFormat_ASTC10x8Unorm:
        case WGPUTextureFormat_ASTC10x8UnormSrgb:
        case WGPUTextureFormat_ASTC10x10Unorm:
        case WGPUTextureFormat_ASTC10x10UnormSrgb:
        case WGPUTextureFormat_ASTC12x10Unorm:
        case WGPUTextureFormat_ASTC12x10UnormSrgb:
        case WGPUTextureFormat_ASTC12x12Unorm:
        case WGPUTextureFormat_ASTC12x12UnormSrgb:
            return 4;
        default:
            return 0;
    }
}

Texture::Texture(Device* device, TextureSettings _textureSettings)
        : device(device), textureSettings(std::move(_textureSettings)) {
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = textureSettings.dimension;
    textureDescriptor.format = textureSettings.format;
    textureDescriptor.mipLevelCount = textureSettings.mipLevelCount;
    textureDescriptor.sampleCount = textureSettings.sampleCount;
    textureDescriptor.size = textureSettings.size;
    textureDescriptor.usage = textureSettings.usage;
    if (textureSettings.viewFormats.empty()) {
        textureDescriptor.viewFormatCount = 1;
        textureDescriptor.viewFormats = &textureSettings.format;
    } else {
        textureDescriptor.viewFormatCount = textureSettings.viewFormats.size();
        textureDescriptor.viewFormats = textureSettings.viewFormats.data();
    }
    if (!textureSettings.label.empty()) {
        textureDescriptor.label = textureSettings.label.c_str();
    }
    texture = wgpuDeviceCreateTexture(device->getWGPUDevice(), &textureDescriptor);
    if (!texture) {
        sgl::Logfile::get()->throwError("Error in Texture::Texture: wgpuDeviceCreateTexture failed!");
    }
}

Texture::Texture(Device* device, TextureSettings _textureSettings, WGPUTexture texture)
        : device(device), textureSettings(std::move(_textureSettings)), texture(texture) {
    hasOwnership = false;
}

Texture::~Texture() {
    if (texture) {
        if (hasOwnership) {
            wgpuTextureDestroy(texture);
        }
        wgpuTextureRelease(texture);
        texture = {};
    }
}

void Texture::write(const TextureWriteInfo& writeInfo, WGPUQueue queue) {
    uint8_t* alignedDataPtr = nullptr;
    const void* dataPtr = writeInfo.srcPtr;
    WGPUTextureDataLayout textureDataLayout{};
    textureDataLayout.offset = writeInfo.srcOffset;
    textureDataLayout.bytesPerRow = writeInfo.srcBytesPerRow;
    textureDataLayout.rowsPerImage = writeInfo.srcRowsPerImage;

    // The WebGPU specification demands multiples of 256 bytes for row stride.
    if (writeInfo.srcBytesPerRow % 256 != 0) {
        size_t unalignedRowSize = writeInfo.srcBytesPerRow;
        size_t alignedRowSize = sgl::uiceil(writeInfo.srcBytesPerRow, 256) * 256;
        size_t numRows = writeInfo.srcRowsPerImage;
        alignedDataPtr = new uint8_t[alignedRowSize * numRows];
        const auto* srcPtr = reinterpret_cast<const uint8_t*>(writeInfo.srcPtr);
        uint8_t* dstPtr = alignedDataPtr;
        for (size_t rowIdx = 0; rowIdx < numRows; rowIdx++) {
            memcpy(dstPtr, srcPtr, unalignedRowSize);
            srcPtr += alignedRowSize;
            dstPtr += unalignedRowSize;
        }
        dataPtr = alignedDataPtr;
        textureDataLayout.bytesPerRow  = uint32_t(alignedRowSize);
        //sgl::Logfile::get()->writeError("Error in Texture::write: bytesPerRow % 256 != 0.");
    }

    WGPUImageCopyTexture imageCopyTexture{};
    imageCopyTexture.texture = texture;
    imageCopyTexture.mipLevel = writeInfo.dstMipLevel;
    imageCopyTexture.origin = writeInfo.dstOrigin;
    imageCopyTexture.aspect = writeInfo.dstAspect;

    wgpuQueueWriteTexture(
            queue, &imageCopyTexture, dataPtr, writeInfo.srcSize, &textureDataLayout, &writeInfo.dstWriteSize);

    if (alignedDataPtr) {
        delete[] alignedDataPtr;
        alignedDataPtr = nullptr;
    }
}

void Texture::write(void const* data, uint64_t dataSize, WGPUQueue queue) {
    auto bytesPerEntry = uint32_t(getTextureFormatEntryByteSize(textureSettings.format));
    TextureWriteInfo writeInfo{};
    writeInfo.srcPtr = data;
    writeInfo.srcSize = dataSize;
    writeInfo.srcOffset = 0;
    writeInfo.srcBytesPerRow = bytesPerEntry * textureSettings.size.width;
    writeInfo.srcRowsPerImage = textureSettings.size.height * textureSettings.size.depthOrArrayLayers;
    writeInfo.dstWriteSize = textureSettings.size;
    writeInfo.dstMipLevel = 0;
    writeInfo.dstOrigin = { 0, 0, 0 };
    write(writeInfo, queue);
}


TextureView::TextureView(TexturePtr _texture, TextureViewSettings _textureViewSettings)
        : texture(std::move(_texture)), textureViewSettings(std::move(_textureViewSettings)) {
    WGPUTextureViewDescriptor textureViewDescriptor{};
    if (textureViewSettings.dimension == WGPUTextureViewDimension_Undefined) {
        auto textureDimension = texture->getTextureSettings().dimension;
        if (textureDimension == WGPUTextureDimension_1D) {
            textureViewDescriptor.dimension = WGPUTextureViewDimension_1D;
        } if (textureDimension == WGPUTextureDimension_2D) {
            textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
        } if (textureDimension == WGPUTextureDimension_3D) {
            textureViewDescriptor.dimension = WGPUTextureViewDimension_3D;
        }
    } else {
        textureViewDescriptor.dimension = textureViewSettings.dimension;
    }
    if (textureViewSettings.format == WGPUTextureFormat_Undefined) {
        textureViewDescriptor.format = texture->getTextureSettings().format;
    } else {
        textureViewDescriptor.format = textureViewSettings.format;
    }
    textureViewDescriptor.baseMipLevel = textureViewSettings.baseMipLevel;
    textureViewDescriptor.mipLevelCount = textureViewSettings.mipLevelCount;
    textureViewDescriptor.baseArrayLayer = textureViewSettings.baseArrayLayer;
    textureViewDescriptor.arrayLayerCount = textureViewSettings.arrayLayerCount;
    textureViewDescriptor.aspect = textureViewSettings.aspect;
    if (!textureViewSettings.label.empty()) {
        textureViewDescriptor.label = textureViewSettings.label.c_str();
    }
    textureView = wgpuTextureCreateView(texture->getWGPUTexture(), &textureViewDescriptor);
    if (!textureView) {
        sgl::Logfile::get()->throwError("Error in TextureView::TextureView: wgpuTextureCreateView failed!");
    }
}

TextureView::~TextureView() {
    if (textureView) {
        wgpuTextureViewRelease(textureView);
        textureView = {};
    }
}

}}
