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

#ifndef SGL_COMMON_HPP
#define SGL_COMMON_HPP

#include <string>
#include <sycl/sycl.hpp>

#include <Graphics/Utils/FormatInfo.hpp>

void* sycl_malloc_host_typed(sgl::ChannelFormat channelFormat, size_t numEntries, sycl::queue& syclQueue);
void* sycl_malloc_device_typed(sgl::ChannelFormat channelFormat, size_t numEntries, sycl::queue& syclQueue);

void initializeHostPointerTyped(sgl::ChannelFormat channelFormat, size_t numEntries, int value, void* ptr);
void initializeHostPointerLinearTyped(sgl::ChannelFormat channelFormat, size_t numEntries, void* ptr);

bool checkIsArrayLinearTyped(
        const sgl::FormatInfo& formatInfo, size_t width, size_t height, void* ptr, std::string& errorMessage);

#endif //SGL_COMMON_HPP