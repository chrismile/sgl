/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#include <stdexcept>

#include <Math/half/half.hpp>

#include "Common.hpp"
#include "FormatRange.hpp"

void initializeHostPointerTyped(sgl::ChannelFormat channelFormat, size_t numEntries, int value, void* ptr) {
    if (channelFormat == sgl::ChannelFormat::FLOAT32) {
        auto* hostPtr = static_cast<float*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = static_cast<float>(value);
        }
    } else if (channelFormat == sgl::ChannelFormat::UINT32 || channelFormat == sgl::ChannelFormat::INT32) {
        auto* hostPtr = static_cast<int32_t*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = value;
        }
    } else if (channelFormat == sgl::ChannelFormat::UINT16 || channelFormat == sgl::ChannelFormat::INT16) {
        auto* hostPtr = static_cast<int16_t*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = static_cast<int16_t>(value);
        }
    } else if (channelFormat == sgl::ChannelFormat::FLOAT16) {
        auto* hostPtr = static_cast<HalfFloat*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = HalfFloat(value);
        }
    } else {
        throw std::runtime_error("Unsupported channel format.");
    }
}

void initializeHostPointerLinearTyped(sgl::ChannelFormat channelFormat, size_t numEntries, void* ptr) {
    if (channelFormat == sgl::ChannelFormat::FLOAT32) {
        auto* hostPtr = static_cast<float*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = static_cast<float>(i);
        }
    } else if (channelFormat == sgl::ChannelFormat::UINT32 || channelFormat == sgl::ChannelFormat::INT32) {
        auto* hostPtr = static_cast<int32_t*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = static_cast<int32_t>(i);
        }
    } else if (channelFormat == sgl::ChannelFormat::UINT16) {
        auto* hostPtr = static_cast<uint16_t*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = static_cast<uint16_t>(i % static_cast<size_t>(format_range<uint16_t>::modulo_value));
        }
    } else if (channelFormat == sgl::ChannelFormat::INT16) {
        auto* hostPtr = static_cast<int16_t*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = static_cast<int16_t>(i % static_cast<size_t>(format_range<int16_t>::modulo_value));
        }
    } else if (channelFormat == sgl::ChannelFormat::FLOAT16) {
        auto* hostPtr = static_cast<HalfFloat*>(ptr);
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = HalfFloat(static_cast<int>(i) % format_range<half>::modulo_value);
        }
    } else {
        throw std::runtime_error("Unsupported channel format.");
    }
}


template<typename T>
bool checkIsArrayLinear(
        const sgl::FormatInfo& formatInfo, size_t width, size_t height, void* ptr, std::string& errorMessage) {
    size_t numEntries = width * height * formatInfo.numChannels;
    auto* hostPtr = static_cast<T*>(ptr);
    for (size_t i = 0; i < numEntries; i++) {
        if (hostPtr[i] != T(static_cast<int>(i))) {
            size_t channelIdx = i % formatInfo.numChannels;
            size_t x = (i / formatInfo.numChannels) % width;
            size_t y = (i / formatInfo.numChannels) / width;
            errorMessage =
                    "Image content mismatch at x=" + std::to_string(x) + ", y=" + std::to_string(y)
                    + ", c=" + std::to_string(channelIdx);
            return false;
        }
    }
    return true;
}

template<typename T>
bool checkIsArrayLinearModulo(
        const sgl::FormatInfo& formatInfo, size_t width, size_t height, void* ptr, std::string& errorMessage) {
    size_t numEntries = width * height * formatInfo.numChannels;
    auto* hostPtr = static_cast<T*>(ptr);
    for (size_t i = 0; i < numEntries; i++) {
        size_t idx = i % static_cast<size_t>(format_range<T>::modulo_value);
        if (hostPtr[i] != T(static_cast<int>(idx))) {
            size_t channelIdx = i % formatInfo.numChannels;
            size_t x = (i / formatInfo.numChannels) % width;
            size_t y = (i / formatInfo.numChannels) / width;
            errorMessage =
                    "Image content mismatch at x=" + std::to_string(x) + ", y=" + std::to_string(y)
                    + ", c=" + std::to_string(channelIdx);
            return false;
        }
    }
    return true;
}

bool checkIsArrayLinearTyped(
        const sgl::FormatInfo& formatInfo, size_t width, size_t height, void* ptr, std::string& errorMessage) {
    auto channelFormat = formatInfo.channelFormat;
    if (channelFormat == sgl::ChannelFormat::FLOAT32) {
        return checkIsArrayLinear<float>(formatInfo, width, height, ptr, errorMessage);
    }
    if (channelFormat == sgl::ChannelFormat::UINT32 || channelFormat == sgl::ChannelFormat::INT32) {
        return checkIsArrayLinear<uint32_t>(formatInfo, width, height, ptr, errorMessage);
    }
    if (channelFormat == sgl::ChannelFormat::UINT16) {
        return checkIsArrayLinearModulo<uint16_t>(formatInfo, width, height, ptr, errorMessage);
    }
    if (channelFormat == sgl::ChannelFormat::INT16) {
        return checkIsArrayLinearModulo<int16_t>(formatInfo, width, height, ptr, errorMessage);
    }
    if (channelFormat == sgl::ChannelFormat::FLOAT16) {
        return checkIsArrayLinearModulo<half>(formatInfo, width, height, ptr, errorMessage);
    }
    throw std::runtime_error("Unsupported channel format.");
}
