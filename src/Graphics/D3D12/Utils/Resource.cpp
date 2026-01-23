/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#include "../Render/CommandList.hpp"
#include "Device.hpp"
#include "Resource.hpp"

#include "Math/Math.hpp"

namespace sgl { namespace d3d12 {

size_t getDXGIFormatNumChannels(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D32_FLOAT:
            return 1;
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return 2;
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return 3;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return 4;
        default:
            return 0;
    }
}

size_t getDXGIFormatChannelSizeInBytes(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return 1;
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return 2;
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return 4;
        default:
            return 0;
    }
}

size_t getDXGIFormatSizeInBytes(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
            return 1;
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
            return 2;
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return 4;
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return 8;
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return 12;
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return 16;
        default:
            return 0;
    }
}

ChannelFormat getDXGIFormatChannelFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return ChannelFormat::UNORM8;
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        return ChannelFormat::SNORM8;
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return ChannelFormat::UNORM16;
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        return ChannelFormat::SNORM16;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return ChannelFormat::FLOAT16;
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return ChannelFormat::FLOAT32;
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8B8A8_UINT:
        return ChannelFormat::UINT8;
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16B16A16_UINT:
         return ChannelFormat::UINT16;
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
         return ChannelFormat::UINT32;
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return ChannelFormat::SINT8;
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return ChannelFormat::SINT16;
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return ChannelFormat::SINT32;
    default:
        return ChannelFormat::UNDEFINED;
    }
}

ChannelCategory getDXGIFormatChannelCategory(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return ChannelCategory::FLOAT;
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return ChannelCategory::UINT;
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return ChannelCategory::SINT;
        default:
            return ChannelCategory::UNDEFINED;
    }
}

FormatInfo getDXGIFormatInfo(DXGI_FORMAT format) {
    FormatInfo formatInfo{};
    formatInfo.numChannels = getDXGIFormatNumChannels(format);
    formatInfo.channelSizeInBytes = getDXGIFormatChannelSizeInBytes(format);
    formatInfo.formatSizeInBytes = getDXGIFormatSizeInBytes(format);
    formatInfo.channelCategory = getDXGIFormatChannelCategory(format);
    formatInfo.channelFormat = getDXGIFormatChannelFormat(format);
    return formatInfo;
}


std::string getDXGIFormatHLSLStructuredTypeString(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
            return "float";
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R32_UINT:
            return "uint";
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R32_SINT:
            return "int";
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R32G32_UINT:
            return "uint2";
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32G32_SINT:
            return "int2";
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R32G32_FLOAT:
            return "float2";
        case DXGI_FORMAT_R32G32B32_UINT:
            return "uint3";
        case DXGI_FORMAT_R32G32B32_SINT:
            return "int3";
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return "float3";
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return "uint4";
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return "int4";
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return "float4";
        default:
            return "UNKNOWN";
    }
}

std::string convertDXGIFormatToString(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return "DXGI_FORMAT_R32G32B32A32_FLOAT";
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return "DXGI_FORMAT_R32G32B32A32_UINT";
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return "DXGI_FORMAT_R32G32B32A32_SINT";
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return "DXGI_FORMAT_R32G32B32_TYPELESS";
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return "DXGI_FORMAT_R32G32B32_FLOAT";
        case DXGI_FORMAT_R32G32B32_UINT:
            return "DXGI_FORMAT_R32G32B32_UINT";
        case DXGI_FORMAT_R32G32B32_SINT:
            return "DXGI_FORMAT_R32G32B32_SINT";
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return "DXGI_FORMAT_R16G16B16A16_FLOAT";
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return "DXGI_FORMAT_R16G16B16A16_UNORM";
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return "DXGI_FORMAT_R16G16B16A16_UINT";
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return "DXGI_FORMAT_R16G16B16A16_SNORM";
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return "DXGI_FORMAT_R16G16B16A16_SINT";
        case DXGI_FORMAT_R32G32_TYPELESS:
            return "DXGI_FORMAT_R32G32_TYPELESS";
        case DXGI_FORMAT_R32G32_FLOAT:
            return "DXGI_FORMAT_R32G32_FLOAT";
        case DXGI_FORMAT_R32G32_UINT:
            return "DXGI_FORMAT_R32G32_UINT";
        case DXGI_FORMAT_R32G32_SINT:
            return "DXGI_FORMAT_R32G32_SINT";
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return "DXGI_FORMAT_R32G8X24_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return "DXGI_FORMAT_R10G10B10A2_TYPELESS";
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return "DXGI_FORMAT_R10G10B10A2_UNORM";
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return "DXGI_FORMAT_R10G10B10A2_UINT";
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return "DXGI_FORMAT_R11G11B10_FLOAT";
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return "DXGI_FORMAT_R8G8B8A8_TYPELESS";
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return "DXGI_FORMAT_R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return "DXGI_FORMAT_R8G8B8A8_UINT";
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return "DXGI_FORMAT_R8G8B8A8_SNORM";
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return "DXGI_FORMAT_R8G8B8A8_SINT";
        case DXGI_FORMAT_R16G16_TYPELESS:
            return "DXGI_FORMAT_R16G16_TYPELESS";
        case DXGI_FORMAT_R16G16_FLOAT:
            return "DXGI_FORMAT_R16G16_FLOAT";
        case DXGI_FORMAT_R16G16_UNORM:
            return "DXGI_FORMAT_R16G16_UNORM";
        case DXGI_FORMAT_R16G16_UINT:
            return "DXGI_FORMAT_R16G16_UINT";
        case DXGI_FORMAT_R16G16_SNORM:
            return "DXGI_FORMAT_R16G16_SNORM";
        case DXGI_FORMAT_R16G16_SINT:
            return "DXGI_FORMAT_R16G16_SINT";
        case DXGI_FORMAT_R32_TYPELESS:
            return "DXGI_FORMAT_R32_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT:
            return "DXGI_FORMAT_D32_FLOAT";
        case DXGI_FORMAT_R32_FLOAT:
            return "DXGI_FORMAT_R32_FLOAT";
        case DXGI_FORMAT_R32_UINT:
            return "DXGI_FORMAT_R32_UINT";
        case DXGI_FORMAT_R32_SINT:
            return "DXGI_FORMAT_R32_SINT";
        case DXGI_FORMAT_R24G8_TYPELESS:
            return "DXGI_FORMAT_R24G8_TYPELESS";
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return "DXGI_FORMAT_D24_UNORM_S8_UINT";
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return "DXGI_FORMAT_X24_TYPELESS_G8_UINT";
        case DXGI_FORMAT_R8G8_TYPELESS:
            return "DXGI_FORMAT_R8G8_TYPELESS";
        case DXGI_FORMAT_R8G8_UNORM:
            return "DXGI_FORMAT_R8G8_UNORM";
        case DXGI_FORMAT_R8G8_UINT:
            return "DXGI_FORMAT_R8G8_UINT";
        case DXGI_FORMAT_R8G8_SNORM:
            return "DXGI_FORMAT_R8G8_SNORM";
        case DXGI_FORMAT_R8G8_SINT:
            return "DXGI_FORMAT_R8G8_SINT";
        case DXGI_FORMAT_R16_TYPELESS:
            return "DXGI_FORMAT_R16_TYPELESS";
        case DXGI_FORMAT_R16_FLOAT:
            return "DXGI_FORMAT_R16_FLOAT";
        case DXGI_FORMAT_D16_UNORM:
            return "DXGI_FORMAT_D16_UNORM";
        case DXGI_FORMAT_R16_UNORM:
            return "DXGI_FORMAT_R16_UNORM";
        case DXGI_FORMAT_R16_UINT:
            return "DXGI_FORMAT_R16_UINT";
        case DXGI_FORMAT_R16_SNORM:
            return "DXGI_FORMAT_R16_SNORM";
        case DXGI_FORMAT_R16_SINT:
            return "DXGI_FORMAT_R16_SINT";
        case DXGI_FORMAT_R8_TYPELESS:
            return "DXGI_FORMAT_R8_TYPELESS";
        case DXGI_FORMAT_R8_UNORM:
            return "DXGI_FORMAT_R8_UNORM";
        case DXGI_FORMAT_R8_UINT:
            return "DXGI_FORMAT_R8_UINT";
        case DXGI_FORMAT_R8_SNORM:
            return "DXGI_FORMAT_R8_SNORM";
        case DXGI_FORMAT_R8_SINT:
            return "DXGI_FORMAT_R8_SINT";
        case DXGI_FORMAT_A8_UNORM:
            return "DXGI_FORMAT_A8_UNORM";
        case DXGI_FORMAT_R1_UNORM:
            return "DXGI_FORMAT_R1_UNORM";
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
            return "DXGI_FORMAT_R8G8_B8G8_UNORM";
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            return "DXGI_FORMAT_G8R8_G8B8_UNORM";
        case DXGI_FORMAT_BC1_TYPELESS:
            return "DXGI_FORMAT_BC1_TYPELESS";
        case DXGI_FORMAT_BC1_UNORM:
            return "DXGI_FORMAT_BC1_UNORM";
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return "DXGI_FORMAT_BC1_UNORM_SRGB";
        case DXGI_FORMAT_BC2_TYPELESS:
            return "DXGI_FORMAT_BC2_TYPELESS";
        case DXGI_FORMAT_BC2_UNORM:
            return "DXGI_FORMAT_BC2_UNORM";
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return "DXGI_FORMAT_BC2_UNORM_SRGB";
        case DXGI_FORMAT_BC3_TYPELESS:
            return "DXGI_FORMAT_BC3_TYPELESS";
        case DXGI_FORMAT_BC3_UNORM:
            return "DXGI_FORMAT_BC3_UNORM";
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return "DXGI_FORMAT_BC3_UNORM_SRGB";
        case DXGI_FORMAT_BC4_TYPELESS:
            return "DXGI_FORMAT_BC4_TYPELESS";
        case DXGI_FORMAT_BC4_UNORM:
            return "DXGI_FORMAT_BC4_UNORM";
        case DXGI_FORMAT_BC4_SNORM:
            return "DXGI_FORMAT_BC4_SNORM";
        case DXGI_FORMAT_BC5_TYPELESS:
            return "DXGI_FORMAT_BC5_TYPELESS";
        case DXGI_FORMAT_BC5_UNORM:
            return "DXGI_FORMAT_BC5_UNORM";
        case DXGI_FORMAT_BC5_SNORM:
            return "DXGI_FORMAT_BC5_SNORM";
        case DXGI_FORMAT_B5G6R5_UNORM:
            return "DXGI_FORMAT_B5G6R5_UNORM";
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return "DXGI_FORMAT_B5G5R5A1_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return "DXGI_FORMAT_B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return "DXGI_FORMAT_B8G8R8X8_UNORM";
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return "DXGI_FORMAT_B8G8R8A8_TYPELESS";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return "DXGI_FORMAT_B8G8R8X8_TYPELESS";
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";
        case DXGI_FORMAT_BC6H_TYPELESS:
            return "DXGI_FORMAT_BC6H_TYPELESS";
        case DXGI_FORMAT_BC6H_UF16:
            return "DXGI_FORMAT_BC6H_UF16";
        case DXGI_FORMAT_BC6H_SF16:
            return "DXGI_FORMAT_BC6H_SF16";
        case DXGI_FORMAT_BC7_TYPELESS:
            return "DXGI_FORMAT_BC7_TYPELESS";
        case DXGI_FORMAT_BC7_UNORM:
            return "DXGI_FORMAT_BC7_UNORM";
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return "DXGI_FORMAT_BC7_UNORM_SRGB";
        case DXGI_FORMAT_AYUV:
            return "DXGI_FORMAT_AYUV";
        case DXGI_FORMAT_Y410:
            return "DXGI_FORMAT_Y410";
        case DXGI_FORMAT_Y416:
            return "DXGI_FORMAT_Y416";
        case DXGI_FORMAT_NV12:
            return "DXGI_FORMAT_NV12";
        case DXGI_FORMAT_P010:
            return "DXGI_FORMAT_P010";
        case DXGI_FORMAT_P016:
            return "DXGI_FORMAT_P016";
        case DXGI_FORMAT_420_OPAQUE:
            return "DXGI_FORMAT_420_OPAQUE";
        case DXGI_FORMAT_YUY2:
            return "DXGI_FORMAT_YUY2";
        case DXGI_FORMAT_Y210:
            return "DXGI_FORMAT_Y210";
        case DXGI_FORMAT_Y216:
            return "DXGI_FORMAT_Y216";
        case DXGI_FORMAT_NV11:
            return "DXGI_FORMAT_NV11";
        case DXGI_FORMAT_AI44:
            return "DXGI_FORMAT_AI44";
        case DXGI_FORMAT_IA44:
            return "DXGI_FORMAT_IA44";
        case DXGI_FORMAT_P8:
            return "DXGI_FORMAT_P8";
        case DXGI_FORMAT_A8P8:
            return "DXGI_FORMAT_A8P8";
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return "DXGI_FORMAT_B4G4R4A4_UNORM";
        default:
            return "DXGI_FORMAT_UNKNOWN";
    }
}


Resource::Resource(Device* device, const ResourceSettings& resourceSettings)
        : device(device), resourceSettings(resourceSettings) {

    auto* d3d12Device = device->getD3D12Device2();
    D3D12_CLEAR_VALUE clearValue{};
    const D3D12_CLEAR_VALUE* optimizedClearValue = nullptr;
    if (resourceSettings.optimizedClearValue.has_value()) {
        memcpy(&clearValue, &resourceSettings.optimizedClearValue.value(), sizeof(D3D12_CLEAR_VALUE));
        if (clearValue.Format == DXGI_FORMAT_UNKNOWN) {
            clearValue.Format = resourceSettings.resourceDesc.Format;
        }
        optimizedClearValue = &clearValue;
    }
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &resourceSettings.heapProperties,
            resourceSettings.heapFlags,
            &resourceSettings.resourceDesc,
            resourceSettings.resourceStates,
            optimizedClearValue,
            IID_PPV_ARGS(&resource)));

    uint32_t arraySize;
    if (resourceSettings.resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        arraySize = resourceSettings.resourceDesc.DepthOrArraySize;
    } else {
        arraySize = 1;
    }
    auto formatPlaneCount = uint32_t(D3D12GetFormatPlaneCount(d3d12Device, resourceSettings.resourceDesc.Format));
    numSubresources = uint32_t(resourceSettings.resourceDesc.MipLevels) * arraySize * formatPlaneCount;
}

Resource::~Resource() = default;


void* Resource::map() {
    return map(0, getCopiableSizeInBytes());
}

void* Resource::map(size_t readRangeBegin, size_t readRangeEnd) {
    D3D12_RANGE readRange = { readRangeBegin, readRangeEnd };
    void* dataPtr = nullptr;
    if (FAILED(resource->Map(0, &readRange, &dataPtr))) {
        sgl::Logfile::get()->throwError("Error in Resource::map: CPU mapping of resource failed.");
    }
    return dataPtr;
}

void Resource::unmap() {
    D3D12_RANGE writtenRange = { 0, 0 };
    resource->Unmap(0, &writtenRange);
}

void Resource::unmap(size_t writtenRangeBegin, size_t writtenRangeEnd) {
    D3D12_RANGE writtenRange = { writtenRangeBegin, writtenRangeEnd };
    resource->Unmap(0, &writtenRange);
}

void Resource::uploadDataLinear(size_t sizeInBytesData, const void* dataPtr) {
    size_t intermediateSizeInBytes;
    if (resourceSettings.resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        intermediateSizeInBytes = sizeInBytesData;
        if (sizeInBytesData > getCopiableSizeInBytes()) {
            sgl::Logfile::get()->throwError(
                    "Error in Resource::uploadDataLinear: "
                    "The copy source is larger than the destination buffer.");
        }
    } else {
        intermediateSizeInBytes = getCopiableSizeInBytes();
        if (sizeInBytesData > getRowSizeInBytes() * resourceSettings.resourceDesc.Height * resourceSettings.resourceDesc.DepthOrArraySize) {
            sgl::Logfile::get()->throwError(
                    "Error in Resource::readBackDataInternal: "
                    "The copy source is larger than the destination texture.");
        }
    }

    queryCopiableFootprints();
    auto* d3d12Device = device->getD3D12Device2();
    CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDescUpload = CD3DX12_RESOURCE_DESC::Buffer(intermediateSizeInBytes);
    ComPtr<ID3D12Resource> intermediateResource{};
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
    // "Upload heaps must start out in the state D3D12_RESOURCE_STATE_GENERIC_READ"
    // "Readback heaps must start out in the D3D12_RESOURCE_STATE_COPY_DEST state"
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesUpload,
            D3D12_HEAP_FLAG_NONE,
            &bufferDescUpload,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));

    device->runSingleTimeCommands([&](CommandList* commandList){
        this->transition(D3D12_RESOURCE_STATE_COPY_DEST, commandList);
        uploadDataLinearInternal(sizeInBytesData, dataPtr, intermediateResource.Get(), commandList);
    });
}

void Resource::uploadDataLinear(
        size_t sizeInBytesData, const void* dataPtr,
        const ResourcePtr& intermediateResource, const CommandListPtr& commandList) {
    uploadDataLinearInternal(sizeInBytesData, dataPtr, intermediateResource->getD3D12ResourcePtr(), commandList.get());
}

void Resource::uploadDataLinearInternal(
        size_t sizeInBytesData, const void* dataPtr,
        ID3D12Resource* intermediateResource, CommandList* commandList) {
    auto* d3d12CommandList = commandList->getD3D12GraphicsCommandListPtr();
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = dataPtr;
    if (resourceSettings.resourceDesc.Height <= 1 && resourceSettings.resourceDesc.DepthOrArraySize <= 1) {
        // 1D data (no pitches necessary).
        subresourceData.RowPitch = LONG_PTR(sizeInBytesData);
        subresourceData.SlicePitch = subresourceData.RowPitch;
    } else if (resourceSettings.resourceDesc.DepthOrArraySize <= 1) {
        // 2D data (no slice pitch necessary).
        subresourceData.RowPitch = LONG_PTR(getRowSizeInBytes());
        subresourceData.SlicePitch = subresourceData.RowPitch * LONG_PTR(resourceSettings.resourceDesc.Width);
    } else {
        // 3D Data.
        subresourceData.RowPitch = LONG_PTR(getRowSizeInBytes());
        subresourceData.SlicePitch = subresourceData.RowPitch * LONG_PTR(resourceSettings.resourceDesc.Width * resourceSettings.resourceDesc.Height);
    }

    queryCopiableFootprints();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = subresourceLayoutArray.at(0);
    UINT64 rowSizeInBytes = subresourceRowSizeInBytesArray.at(0);
    UINT numRows = subresourceNumRowsArray.at(0);
    UINT64 totalSize = subresourceTotalBytesArray.at(0);
    UpdateSubresources(
            d3d12CommandList, getD3D12ResourcePtr(), intermediateResource, 0, 1,
            totalSize, &layout, &numRows, &rowSizeInBytes, &subresourceData);
}

void Resource::readBackDataLinear(size_t sizeInBytesData, void* dataPtr) {
    if (numSubresources > 1) {
        sgl::Logfile::get()->throwError(
                "Error in Resource::readBackDataInternal: "
                "The function only supports for resources with one single subresource.");
    }
    if (resourceSettings.resourceDesc.SampleDesc.Count > 1) {
        sgl::Logfile::get()->throwError(
                "Error in Resource::readBackDataInternal: "
                "The function does not support multi-sampled resources.");
    }

    size_t rowSizeInBytes = 0;
    size_t srcRowPitch;
    size_t intermediateSizeInBytes;
    if (resourceSettings.resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        srcRowPitch = sizeInBytesData;
        intermediateSizeInBytes = srcRowPitch;
        if (sizeInBytesData > getCopiableSizeInBytes()) {
            sgl::Logfile::get()->throwError(
                    "Error in Resource::readBackDataInternal: "
                    "The copy destination is larger than the source buffer.");
        }
    } else {
        rowSizeInBytes = getRowSizeInBytes();
        srcRowPitch = getRowPitchInBytes();
        intermediateSizeInBytes = srcRowPitch;
        if (resourceSettings.resourceDesc.Height > 1) {
            intermediateSizeInBytes *= resourceSettings.resourceDesc.Height;
        }
        if (resourceSettings.resourceDesc.DepthOrArraySize > 1) {
            intermediateSizeInBytes *= resourceSettings.resourceDesc.DepthOrArraySize;
        }
        if (sizeInBytesData > rowSizeInBytes * resourceSettings.resourceDesc.Height * resourceSettings.resourceDesc.DepthOrArraySize) {
            sgl::Logfile::get()->throwError(
                    "Error in Resource::readBackDataInternal: "
                    "The copy destination is larger than the source texture.");
        }
    }

    auto* d3d12Device = device->getD3D12Device2();
    CD3DX12_HEAP_PROPERTIES heapPropertiesReadBack(D3D12_HEAP_TYPE_READBACK);
    auto bufferDescReadBack = CD3DX12_RESOURCE_DESC::Buffer(intermediateSizeInBytes);
    ComPtr<ID3D12Resource> intermediateResource{};
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesReadBack,
            D3D12_HEAP_FLAG_NONE,
            &bufferDescReadBack,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));

    device->runSingleTimeCommands([&](CommandList* commandList){
        auto* d3d12CommandList = commandList->getD3D12GraphicsCommandListPtr();
        if (resourceSettings.resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
            d3d12CommandList->CopyBufferRegion(
                    intermediateResource.Get(), 0, resource.Get(), 0, sizeInBytesData);
        } else {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
            bufferFootprint.Footprint.Width = static_cast<UINT>(resourceSettings.resourceDesc.Width);
            bufferFootprint.Footprint.Height = resourceSettings.resourceDesc.Height;
            bufferFootprint.Footprint.Depth = resourceSettings.resourceDesc.DepthOrArraySize;
            bufferFootprint.Footprint.RowPitch = static_cast<UINT>(srcRowPitch);
            bufferFootprint.Footprint.Format = resourceSettings.resourceDesc.Format;

            const CD3DX12_TEXTURE_COPY_LOCATION Dst(intermediateResource.Get(), bufferFootprint);
            const CD3DX12_TEXTURE_COPY_LOCATION Src(resource.Get(), 0);
            this->transition(D3D12_RESOURCE_STATE_COPY_SOURCE, commandList);
            d3d12CommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
        }
    });

    uint8_t* intermediateDataPtr;
    D3D12_RANGE readRange = { 0, sizeInBytesData };
    D3D12_RANGE writtenRange = { 0, 0 };
    if (FAILED(intermediateResource->Map(0, &readRange, reinterpret_cast<void**>(&intermediateDataPtr)))) {
        sgl::Logfile::get()->throwError(
                "Error: Resource::readBackDataInternal: ID3D12Resource::Map failed.");
    }
    D3D12_MEMCPY_DEST memcpyDest{};
    memcpyDest.pData = dataPtr;
    D3D12_SUBRESOURCE_DATA subresourceSrc{};
    subresourceSrc.pData = intermediateDataPtr;
    if (resourceSettings.resourceDesc.Height <= 1 && resourceSettings.resourceDesc.DepthOrArraySize <= 1) {
        // 1D data (no pitches necessary).
        memcpyDest.RowPitch = SIZE_T(sizeInBytesData);
        memcpyDest.SlicePitch = memcpyDest.RowPitch;
        subresourceSrc.RowPitch = LONG_PTR(sizeInBytesData);
        subresourceSrc.SlicePitch = subresourceSrc.RowPitch;
    } else if (resourceSettings.resourceDesc.DepthOrArraySize <= 1) {
        // 2D data (no slice pitch necessary).
        memcpyDest.RowPitch = SIZE_T(rowSizeInBytes);
        memcpyDest.SlicePitch = memcpyDest.RowPitch * SIZE_T(resourceSettings.resourceDesc.Width);
        subresourceSrc.RowPitch = LONG_PTR(srcRowPitch);
        subresourceSrc.SlicePitch = subresourceSrc.RowPitch * LONG_PTR(resourceSettings.resourceDesc.Width);
    } else {
        // 3D Data.
        memcpyDest.RowPitch = SIZE_T(rowSizeInBytes);
        memcpyDest.SlicePitch = memcpyDest.RowPitch * SIZE_T(resourceSettings.resourceDesc.Width * resourceSettings.resourceDesc.Height);
        subresourceSrc.RowPitch = LONG_PTR(srcRowPitch);
        memcpyDest.SlicePitch = subresourceSrc.RowPitch * LONG_PTR(resourceSettings.resourceDesc.Width * resourceSettings.resourceDesc.Height);
    }
    MemcpySubresource(
            &memcpyDest, &subresourceSrc, memcpyDest.RowPitch, resourceSettings.resourceDesc.Height,
            resourceSettings.resourceDesc.DepthOrArraySize);
    intermediateResource->Unmap(0, &writtenRange);
}


void Resource::transition(
        D3D12_RESOURCE_STATES stateAfter, const CommandListPtr& commandList) {
    transition(resourceSettings.resourceStates, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, commandList);
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateAfter, CommandList* commandList) {
    transition(resourceSettings.resourceStates, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, commandList);
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, const CommandListPtr& commandList) {
    transition(stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, commandList);
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, CommandList* commandList) {
    transition(stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, commandList);
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, uint32_t subresourcce,
        const CommandListPtr& commandList) {
    transition(stateBefore, stateAfter, subresourcce, commandList.get());
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, uint32_t subresourcce,
        CommandList* commandList) {
    ID3D12GraphicsCommandList* d3d12GraphicsCommandList = commandList->getD3D12GraphicsCommandListPtr();
    D3D12_RESOURCE_BARRIER resourceBarrier{};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = resource.Get();
    resourceBarrier.Transition.Subresource = subresourcce;
    resourceBarrier.Transition.StateBefore = stateBefore;
    resourceBarrier.Transition.StateAfter = stateAfter;
    d3d12GraphicsCommandList->ResourceBarrier(1, &resourceBarrier);
    resourceSettings.resourceStates = stateAfter;
}

void Resource::barrierUAV(const CommandListPtr& commandList) {
    barrierUAV(commandList.get());
}

void Resource::barrierUAV(CommandList* commandList) {
    ID3D12GraphicsCommandList* d3d12GraphicsCommandList = commandList->getD3D12GraphicsCommandListPtr();
    D3D12_RESOURCE_BARRIER resourceBarrier{};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.UAV.pResource = resource.Get();
    d3d12GraphicsCommandList->ResourceBarrier(1, &resourceBarrier);
}


size_t Resource::getAllocationSizeInBytes() {
    auto* d3d12Device = device->getD3D12Device2();
#if defined(_MSC_VER) || !defined(_WIN32)
    D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = d3d12Device->GetResourceAllocationInfo(
            0, 1, &resourceSettings.resourceDesc);
#else
    D3D12_RESOURCE_ALLOCATION_INFO allocationInfo{};
    d3d12Device->GetResourceAllocationInfo(
            &allocationInfo, 0, 1, &resourceSettings.resourceDesc);
#endif
    return allocationInfo.SizeInBytes;
}

void Resource::queryCopiableFootprints() {
    if (!subresourceLayoutArray.empty()) {
        return;
    }
    auto* d3d12Device = device->getD3D12Device2();
    uint32_t numEntries = std::max(numSubresources, uint32_t(1));
    subresourceLayoutArray.resize(numEntries);
    subresourceNumRowsArray.resize(numEntries);
    subresourceRowSizeInBytesArray.resize(numEntries);
    subresourceTotalBytesArray.resize(numEntries);
    d3d12Device->GetCopyableFootprints(
           &resourceSettings.resourceDesc, 0, numEntries, 0,
           subresourceLayoutArray.data(), subresourceNumRowsArray.data(),
           subresourceRowSizeInBytesArray.data(), subresourceTotalBytesArray.data());
}

size_t Resource::getCopiableSizeInBytes() {
    queryCopiableFootprints();
    return size_t(subresourceTotalBytesArray.at(0));
}

size_t Resource::getNumRows() {
    queryCopiableFootprints();
    return size_t(subresourceNumRowsArray.at(0));
}

size_t Resource::getRowSizeInBytes() {
    queryCopiableFootprints();
    return size_t(subresourceRowSizeInBytesArray.at(0));
}

size_t Resource::getRowPitchInBytes() {
    queryCopiableFootprints();
    size_t rowSizeInBytes = getRowSizeInBytes();
    if (rowSizeInBytes % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT == 0) {
        return rowSizeInBytes;
    }
    return sgl::sizeceil(rowSizeInBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
}

D3D12_GPU_VIRTUAL_ADDRESS Resource::getGPUVirtualAddress() {
    return resource->GetGPUVirtualAddress();
}


HANDLE Resource::getSharedHandle(const std::wstring& handleName) {
    auto* d3d12Device = device->getD3D12Device2();
    HANDLE resourceHandle{};
    ThrowIfFailed(d3d12Device->CreateSharedHandle(
            resource.Get(), nullptr, GENERIC_ALL, handleName.data(), &resourceHandle));
    return resourceHandle;
}

HANDLE Resource::getSharedHandle() {
    uint64_t resourceIdx = 0;
    // TODO: Apparently, the handle name may be "nullptr".
    std::wstring handleName = std::wstring(L"Local\\D3D12ResourceHandle") + std::to_wstring(resourceIdx);
    return getSharedHandle(handleName);
}

}}
