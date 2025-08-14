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

#include "Device.hpp"
#include "Resource.hpp"
#include "InteropCompute.hpp"

#ifdef SUPPORT_SYCL_INTEROP
#include "InteropCompute/ImplSycl.hpp"
#include <sycl/sycl.hpp>
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#ifdef SUPPORT_SYCL_INTEROP
namespace sgl {
extern sycl::queue* g_syclQueue;
}
#endif

namespace sgl { namespace d3d12 {

InteropComputeApi decideInteropComputeApi(Device* device) {
    InteropComputeApi api = InteropComputeApi::NONE;
#ifdef SUPPORT_SYCL_INTEROP
    if (g_syclQueue != nullptr) {
        api = InteropComputeApi::SYCL;
    }
#endif
    return api;
}

FenceD3D12ComputeApiInteropPtr createFenceD3D12ComputeApiInterop(Device* device, uint64_t value) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(device);
    (void)interopComputeApi;
    FenceD3D12ComputeApiInteropPtr fence;
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        fence = std::make_shared<FenceD3D12SyclInterop>();
    }
#endif
    if (!fence) {
        sgl::Logfile::get()->writeError("Error in createFenceD3D12ComputeApiInterop: Unsupported compute API.");
        return fence;
    }

    fence->initialize(device, value);
    return fence;
}

BufferD3D12ComputeApiExternalMemoryPtr createBufferD3D12ComputeApiExternalMemory(sgl::d3d12::ResourcePtr& resource) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(resource->getDevice());
    (void)interopComputeApi;
    BufferD3D12ComputeApiExternalMemoryPtr resourceExtMem;
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        resourceExtMem = std::make_shared<BufferD3D12SyclInterop>();
    }
#endif
    if (!resourceExtMem) {
        sgl::Logfile::get()->writeError("Error in createBufferD3D12ComputeApiExternalMemory: Unsupported compute API.");
        return resourceExtMem;
    }

    resourceExtMem->initialize(resource);
    return resourceExtMem;
}

ImageD3D12ComputeApiExternalMemoryPtr createImageD3D12ComputeApiExternalMemory(sgl::d3d12::ResourcePtr& resource) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(resource->getDevice());
    (void)interopComputeApi;
    ImageD3D12ComputeApiExternalMemoryPtr resourceExtMem;
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        resourceExtMem = std::make_shared<ImageD3D12SyclInterop>();
    }
#endif
    if (!resourceExtMem) {
        sgl::Logfile::get()->writeError("Error in createImageD3D12ComputeApiExternalMemory: Unsupported compute API.");
        return resourceExtMem;
    }

    resourceExtMem->initialize(resource);
    return resourceExtMem;
}

ImageD3D12ComputeApiExternalMemoryPtr createImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, bool surfaceLoadStore) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(resource->getDevice());
    (void)interopComputeApi;
    ImageD3D12ComputeApiExternalMemoryPtr resourceExtMem;
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        resourceExtMem = std::make_shared<ImageD3D12SyclInterop>();
    }
#endif
    if (!resourceExtMem) {
        sgl::Logfile::get()->writeError("Error in createImageD3D12ComputeApiExternalMemory: Unsupported compute API.");
        return resourceExtMem;
    }

    resourceExtMem->initialize(resource, surfaceLoadStore);
    return resourceExtMem;
}


void FenceD3D12ComputeApiInterop::initialize(Device* device, uint64_t value) {
    _initialize(device, value, D3D12_FENCE_FLAG_SHARED);
    handle = getSharedHandle();
    importExternalFenceWin32Handle();
}

void FenceD3D12ComputeApiInterop::freeHandle() {
    if (handle) {
        CloseHandle(handle);
        handle = {};
    }
}


void BufferD3D12ComputeApiExternalMemory::initialize(sgl::d3d12::ResourcePtr& _resource) {
    if (devicePtr) {
        free();
    }

    resource = _resource;
    handle = _resource->getSharedHandle();
    importExternalMemoryWin32Handle();
}

void BufferD3D12ComputeApiExternalMemory::freeHandle() {
    if (handle) {
        CloseHandle(handle);
        handle = {};
    }
}

void ImageD3D12ComputeApiExternalMemory::initialize(sgl::d3d12::ResourcePtr& _resource) {
    if (mipmappedArray) {
        free();
    }

    resource = _resource;
    handle = _resource->getSharedHandle();
    importExternalMemoryWin32Handle();
}

void ImageD3D12ComputeApiExternalMemory::initialize(sgl::d3d12::ResourcePtr& _resource, bool _surfaceLoadStore) {
    if (mipmappedArray) {
        free();
    }

    resource = _resource;
    surfaceLoadStore = _surfaceLoadStore;
    handle = _resource->getSharedHandle();
    importExternalMemoryWin32Handle();
}

void ImageD3D12ComputeApiExternalMemory::freeHandle() {
    if (handle) {
        CloseHandle(handle);
        handle = {};
    }
}

}}
