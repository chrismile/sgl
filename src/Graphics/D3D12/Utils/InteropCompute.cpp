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
        sgl::d3d12::ResourcePtr& resource, const ImageD3D12ComputeApiInfo& imageComputeApiInfo) {
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

    resourceExtMem->initialize(resource, imageComputeApiInfo);
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

void ImageD3D12ComputeApiExternalMemory::initialize(
            sgl::d3d12::ResourcePtr& _resource, const ImageD3D12ComputeApiInfo& _imageComputeApiInfo) {
    if (mipmappedArray) {
        free();
    }

    resource = _resource;
    imageComputeApiInfo = _imageComputeApiInfo;
    handle = _resource->getSharedHandle();
    importExternalMemoryWin32Handle();
}

void ImageD3D12ComputeApiExternalMemory::freeHandle() {
    if (handle) {
        CloseHandle(handle);
        handle = {};
    }
}


UnsampledImageD3D12ComputeApiExternalMemoryPtr createUnsampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource) {
    [[maybe_unused]] InteropComputeApi interopComputeApi = decideInteropComputeApi(resource->getDevice());
    UnsampledImageD3D12ComputeApiExternalMemoryPtr unsampledImageExtMem;
    ImageD3D12ComputeApiExternalMemoryPtr imageExtMem = createImageD3D12ComputeApiExternalMemory(resource);
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        unsampledImageExtMem = std::make_shared<UnsampledImageD3D12SyclInterop>();
    }
#endif
    if (!unsampledImageExtMem) {
        sgl::Logfile::get()->writeError(
                "Error in createUnsampledImageD3D12ComputeApiExternalMemory: Unsupported compute API.");
        return unsampledImageExtMem;
    }

    unsampledImageExtMem->initialize(imageExtMem);
    return unsampledImageExtMem;
}

UnsampledImageD3D12ComputeApiExternalMemoryPtr createUnsampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, const ImageD3D12ComputeApiInfo& imageComputeApiInfo) {
    if (imageComputeApiInfo.useSampledImage) {
        Logfile::get()->throwError(
                    "Error in createUnsampledImageD3D12ComputeApiExternalMemory: "
                    "ImageD3D12ComputeApiInfo::useSampledImage may not be set to true.");
    }
    [[maybe_unused]] InteropComputeApi interopComputeApi = decideInteropComputeApi(resource->getDevice());
    UnsampledImageD3D12ComputeApiExternalMemoryPtr unsampledImageExtMem;
    ImageD3D12ComputeApiExternalMemoryPtr imageExtMem = createImageD3D12ComputeApiExternalMemory(
            resource, imageComputeApiInfo);
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        unsampledImageExtMem = std::make_shared<UnsampledImageD3D12SyclInterop>();
    }
#endif
    if (!unsampledImageExtMem) {
        sgl::Logfile::get()->writeError(
                "Error in createUnsampledImageD3D12ComputeApiExternalMemory: Unsupported compute API.");
        return unsampledImageExtMem;
    }

    unsampledImageExtMem->initialize(imageExtMem);
    return unsampledImageExtMem;
}

UnsampledImageD3D12ComputeApiExternalMemoryPtr createUnsampledImageD3D12ComputeApiExternalMemory(
        const ImageD3D12ComputeApiExternalMemoryPtr& imageExtMem) {
    [[maybe_unused]] InteropComputeApi interopComputeApi = decideInteropComputeApi(imageExtMem->getResource()->getDevice());
    UnsampledImageD3D12ComputeApiExternalMemoryPtr unsampledImageExtMem;
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        unsampledImageExtMem = std::make_shared<UnsampledImageD3D12SyclInterop>();
    }
#endif
    if (!unsampledImageExtMem) {
        sgl::Logfile::get()->writeError(
                "Error in createUnsampledImageD3D12ComputeApiExternalMemory: Unsupported compute API.");
        return unsampledImageExtMem;
    }

    unsampledImageExtMem->initialize(imageExtMem);
    return unsampledImageExtMem;
}


SampledImageD3D12ComputeApiExternalMemoryPtr createSampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, const ImageD3D12ComputeApiInfo& imageComputeApiInfo) {
    [[maybe_unused]] InteropComputeApi interopComputeApi = decideInteropComputeApi(resource->getDevice());
    SampledImageD3D12ComputeApiExternalMemoryPtr sampledImageExtMem;
    ImageD3D12ComputeApiExternalMemoryPtr imageExtMem = createImageD3D12ComputeApiExternalMemory(
            resource, imageComputeApiInfo);
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        sampledImageExtMem = std::make_shared<SampledImageD3D12SyclInterop>();
    }
#endif
    if (!sampledImageExtMem) {
        sgl::Logfile::get()->writeError(
                "Error in createSampledImageD3D12ComputeApiExternalMemory: Unsupported compute API.");
        return sampledImageExtMem;
    }

    sampledImageExtMem->initialize(imageExtMem, imageComputeApiInfo.textureExternalMemorySettings);
    return sampledImageExtMem;
}

SampledImageD3D12ComputeApiExternalMemoryPtr createSampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, const D3D12_SAMPLER_DESC& samplerDesc,
        const TextureExternalMemorySettings& textureExternalMemorySettings) {
    ImageD3D12ComputeApiInfo imageComputeApiInfo{};
    imageComputeApiInfo.surfaceLoadStore =
            (resource->getResourceSettings().resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
    imageComputeApiInfo.useSampledImage = true;
    imageComputeApiInfo.samplerDesc = samplerDesc;
    imageComputeApiInfo.textureExternalMemorySettings = textureExternalMemorySettings;
    return createSampledImageD3D12ComputeApiExternalMemory(resource, imageComputeApiInfo);
}

}}
