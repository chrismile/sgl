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

#ifndef SGL_FORMATINFO_HPP
#define SGL_FORMATINFO_HPP

#include <cstddef>

namespace sgl {

/// The general channel type category (without size) that would be used in a shader.
enum class ChannelCategory {
    UNDEFINED, FLOAT, UINT, SINT, INT = SINT
};

/// The format (sized) of an image channel.
enum class ChannelFormat {
    UNDEFINED,
    FLOAT16, BFLOAT16, FLOAT32, FLOAT64,
    UNORM8, UNORM16, SNORM8, SNORM16, USCALED8, USCALED16, SSCALED8, SSCALED16,
    UINT8, UINT16, UINT32, UINT64,
    SINT8, SINT16, SINT32, SINT64,
    INT8 = SINT8, INT16 = SINT16, INT32 = SINT32, INT64 = SINT64,
};

/**
 * Format info (independent of used API like Vulkan/D3D12/...).
 * Not every entry may make sense for every format, and multi-planar formats are not yet supported.
 */
struct DLL_OBJECT FormatInfo {
    size_t numChannels;
    size_t channelSizeInBytes;
    size_t formatSizeInBytes;
    ChannelCategory channelCategory;
    ChannelFormat channelFormat;
};

}

#endif //SGL_FORMATINFO_HPP
