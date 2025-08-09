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

#include "../Resource.hpp"
#include "ImplSycl.hpp"

#include <sycl/sycl.hpp>

namespace sgl {
extern bool openMessageBoxOnComputeApiError;
extern sycl::queue* g_syclQueue;
}

namespace sgl { namespace d3d12 {

struct SyclExternalSemaphoreWrapper {
    sycl::ext::oneapi::experimental::external_semaphore syclExternalSemaphore;
};
struct SyclExternalMemWrapper {
    sycl::ext::oneapi::experimental::external_mem syclExternalMem;
    // Optional.
    sycl::ext::oneapi::experimental::image_descriptor syclImageDescriptor;
    sycl::ext::oneapi::experimental::image_mem_handle syclImageMemHandle;
};

void FenceD3D12SyclInterop::importExternalFenceWin32Handle() {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    sycl::ext::oneapi::experimental::external_semaphore_handle_type semaphoreHandleType =
            sycl::ext::oneapi::experimental::external_semaphore_handle_type::win32_nt_dx12_fence;
    sycl::ext::oneapi::experimental::external_semaphore_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalSemaphoreDescriptor{handle, semaphoreHandleType};
    auto* wrapper = new SyclExternalSemaphoreWrapper;
    wrapper->syclExternalSemaphore = sycl::ext::oneapi::experimental::import_external_semaphore(
            syclExternalSemaphoreDescriptor, *g_syclQueue);
    externalSemaphore = reinterpret_cast<void*>(wrapper);
}

void FenceD3D12SyclInterop::free() {
    freeHandle();
    if (externalSemaphore) {
        auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
        sycl::ext::oneapi::experimental::release_external_semaphore(wrapper->syclExternalSemaphore, *g_syclQueue);
        delete wrapper;
        externalSemaphore = {};
    }
}

FenceD3D12SyclInterop::~FenceD3D12SyclInterop() {
    FenceD3D12SyclInterop::free();
}

void FenceD3D12SyclInterop::signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
    auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
    auto syclEvent = stream.syclQueuePtr->ext_oneapi_signal_external_semaphore(
            wrapper->syclExternalSemaphore, uint64_t(timelineValue));
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void FenceD3D12SyclInterop::waitFenceComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
    auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
    auto syclEvent = stream.syclQueuePtr->ext_oneapi_wait_external_semaphore(
            wrapper->syclExternalSemaphore, uint64_t(timelineValue));
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}


void ResourceD3D12SyclInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getAllocationSizeInBytes();

    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    auto memoryHandleType = sycl::ext::oneapi::experimental::external_mem_handle_type::win32_nt_handle;
    sycl::ext::oneapi::experimental::external_mem_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalMemDescriptor{(void*)handle, memoryHandleType, sizeInBytes};
    auto* wrapper = new SyclExternalMemWrapper;
    wrapper->syclExternalMem = sycl::ext::oneapi::experimental::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
    externalMemory = reinterpret_cast<void*>(wrapper);

    devicePtr = sycl::ext::oneapi::experimental::map_external_linear_memory(
            wrapper->syclExternalMem, 0, sizeInBytes, *g_syclQueue);
}

void ResourceD3D12SyclInterop::free() {
    freeHandle();
    if (externalMemory) {
        auto* wrapper = reinterpret_cast<SyclExternalMemWrapper*>(externalMemory);
        sycl::ext::oneapi::experimental::unmap_external_linear_memory(devicePtr, *g_syclQueue);
        sycl::ext::oneapi::experimental::release_external_memory(wrapper->syclExternalMem, *g_syclQueue);
        delete wrapper;
        externalMemory = {};
    }
}

ResourceD3D12SyclInterop::~ResourceD3D12SyclInterop() {
    ResourceD3D12SyclInterop::free();
}

void ResourceD3D12SyclInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtr, devicePtrSrc, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void ResourceD3D12SyclInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtrDst, devicePtr, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void ResourceD3D12SyclInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtr, hostPtrSrc, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void ResourceD3D12SyclInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(hostPtrDst, devicePtr, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

}}
