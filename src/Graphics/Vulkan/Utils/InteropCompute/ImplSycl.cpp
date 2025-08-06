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

#include "ImplSycl.hpp"

namespace sgl { namespace vk {

static sycl::queue* g_syclQueue = nullptr;

void setGlobalSyclQueue(sycl::queue& syclQueue) {
    g_syclQueue = &syclQueue;
}

struct SyclExternalSemaphoreWrapper {
    sycl::ext::oneapi::experimental::external_semaphore syclExternalSemaphore;
};
struct SyclExternalMemWrapper {
    sycl::ext::oneapi::experimental::external_mem syclExternalMem;
};
struct SyclImageMemHandleWrapper {
    sycl::ext::oneapi::experimental::image_descriptor syclImageDescriptor;
    sycl::ext::oneapi::experimental::image_mem_handle syclImageMemHandle;
};

#ifdef _WIN32
void SemaphoreVkSyclInterop::setExternalSemaphoreWin32Handle(HANDLE handle) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    sycl::ext::oneapi::experimental::external_semaphore_handle_type semaphoreHandleType;
    if (isTimelineSemaphore()) {
#ifndef SYCL_NO_EXTERNAL_TIMELINE_SEMAPHORE_SUPPORT
        semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::timeline_win32_nt_handle;
#else
        sgl::Logfile::get()->throwError(
            "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
            "The installed version of SYCL does not support external timeline semaphores.");
#endif
    } else {
        semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::win32_nt_handle;
    }
    sycl::ext::oneapi::experimental::external_semaphore_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
        syclExternalSemaphoreDescriptor{handle, semaphoreHandleType};
    auto* wrapper = new SyclExternalSemaphoreWrapper;
    wrapper->syclExternalSemaphore = sycl::ext::oneapi::experimental::import_external_semaphore(
        syclExternalSemaphoreDescriptor, *g_syclQueue);
    externalSemaphore = reinterpret_cast<void*>(wrapper);
}
#endif

#ifdef __linux__
void SemaphoreVkSyclInterop::setExternalSemaphoreFd(int fd) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    sycl::ext::oneapi::experimental::external_semaphore_handle_type semaphoreHandleType;
    if (isTimelineSemaphore()) {
#ifndef SYCL_NO_EXTERNAL_TIMELINE_SEMAPHORE_SUPPORT
        semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::timeline_fd;
#else
        sgl::Logfile::get()->throwError(
            "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
            "The installed version of SYCL does not support external timeline semaphores.");
#endif
    } else {
        semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::opaque_fd;
    }
    sycl::ext::oneapi::experimental::external_semaphore_descriptor<sycl::ext::oneapi::experimental::resource_fd>
        syclExternalSemaphoreDescriptor{fileDescriptor, semaphoreHandleType};
    auto* wrapper = new SyclExternalSemaphoreWrapper;
    wrapper->syclExternalSemaphore = sycl::ext::oneapi::experimental::import_external_semaphore(
        syclExternalSemaphoreDescriptor, *g_syclQueue);
    externalSemaphore = reinterpret_cast<void*>(wrapper);
}
#endif

SemaphoreVkSyclInterop::~SemaphoreVkSyclInterop() {
    sycl::ext::oneapi::experimental::release_external_semaphore(syclExternalSemaphore, *g_syclQueue);
}

}}
