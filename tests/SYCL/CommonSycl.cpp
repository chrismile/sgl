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

#include "CommonSycl.hpp"

void* sycl_malloc_host_typed(sgl::ChannelFormat channelFormat, size_t numEntries, sycl::queue& syclQueue) {
    if (channelFormat == sgl::ChannelFormat::FLOAT32) {
        return sycl::malloc_host<float>(numEntries, syclQueue);
    }
    if (channelFormat == sgl::ChannelFormat::UINT32 || channelFormat == sgl::ChannelFormat::INT32) {
        return sycl::malloc_host<uint32_t>(numEntries, syclQueue);
    }
    if (channelFormat == sgl::ChannelFormat::FLOAT16 || channelFormat == sgl::ChannelFormat::UINT16 || channelFormat == sgl::ChannelFormat::INT16) {
        return sycl::malloc_host<uint16_t>(numEntries, syclQueue);
    }
    throw std::runtime_error("Unsupported channel format.");
}

void* sycl_malloc_device_typed(sgl::ChannelFormat channelFormat, size_t numEntries, sycl::queue& syclQueue) {
    if (channelFormat == sgl::ChannelFormat::FLOAT32) {
        return sycl::malloc_device<float>(numEntries, syclQueue);
    }
    if (channelFormat == sgl::ChannelFormat::UINT32 || channelFormat == sgl::ChannelFormat::INT32) {
        return sycl::malloc_device<uint32_t>(numEntries, syclQueue);
    }
    if (channelFormat == sgl::ChannelFormat::FLOAT16 || channelFormat == sgl::ChannelFormat::UINT16 || channelFormat == sgl::ChannelFormat::INT16) {
        return sycl::malloc_device<uint16_t>(numEntries, syclQueue);
    }
    throw std::runtime_error("Unsupported channel format.");
}
