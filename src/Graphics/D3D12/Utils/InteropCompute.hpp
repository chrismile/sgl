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

#ifndef SGL_D3D12_INTEROPCOMPUTE_HPP
#define SGL_D3D12_INTEROPCOMPUTE_HPP

#include <Graphics/Utils/InteropCompute.hpp>

#include "d3d12.hpp"
#include "Fence.hpp"

namespace sgl { namespace d3d12 {

class Device;
class Resource;
typedef std::shared_ptr<Resource> ResourcePtr;

/// Decides the compute API usable for the passed device. SYCL has precedence over other APIs if available.
DLL_OBJECT InteropComputeApi decideInteropComputeApi(Device* device);

class DLL_OBJECT FenceD3D12ComputeApiInterop : public d3d12::Fence {
public:
    FenceD3D12ComputeApiInterop() = default;
    void initialize(Device* device, uint64_t value = 0);
    ~FenceD3D12ComputeApiInterop() override = default;

    /// Signal fence.
    void signalFenceComputeApi(StreamWrapper stream) {
        signalFenceComputeApi(stream, 0, nullptr, nullptr);
    }
    void signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
        signalFenceComputeApi(stream, timelineValue, nullptr, nullptr);
    }
    void signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
        signalFenceComputeApi(stream, timelineValue, nullptr, eventOut);
    }
    virtual void signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) = 0;

    /// Wait on fence.
    void waitFenceComputeApi(StreamWrapper stream) {
        waitFenceComputeApi(stream, 0, nullptr, nullptr);
    }
    void waitFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
        waitFenceComputeApi(stream, timelineValue, nullptr, nullptr);
    }
    void waitFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
        waitFenceComputeApi(stream, timelineValue, nullptr, eventOut);
    }
    virtual void waitFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) = 0;

protected:
    virtual void importExternalFenceWin32Handle() = 0;
    virtual void free() = 0;
    void freeHandle();

    HANDLE handle = nullptr;
};

typedef std::shared_ptr<FenceD3D12ComputeApiInterop> FenceD3D12ComputeApiInteropPtr;

DLL_OBJECT FenceD3D12ComputeApiInteropPtr createFenceD3D12ComputeApiInterop(Device* device, uint64_t value = 0);


/**
 * Resource needs to be created with D3D12_HEAP_FLAG_SHARED.
 */
class DLL_OBJECT BufferD3D12ComputeApiExternalMemory {
public:
    BufferD3D12ComputeApiExternalMemory() = default;
    void initialize(sgl::d3d12::ResourcePtr& _resource);
    virtual ~BufferD3D12ComputeApiExternalMemory() = default;

    inline const sgl::d3d12::ResourcePtr& getResource() { return resource; }

    template<class T> [[nodiscard]] inline T* getDevicePtr() const { return reinterpret_cast<T*>(devicePtr); }
    template<class T> [[nodiscard]] inline T getDevicePtrReinterpreted() const { return reinterpret_cast<T>(devicePtr); }

    virtual void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;

protected:
    virtual void importExternalMemoryWin32Handle() = 0;
    virtual void free() = 0;
    void freeHandle();

    sgl::d3d12::ResourcePtr resource;
    void* devicePtr{}; // CUdeviceptr or hipDeviceptr_t or void* device pointer

    HANDLE handle = nullptr;
};

typedef std::shared_ptr<BufferD3D12ComputeApiExternalMemory> BufferD3D12ComputeApiExternalMemoryPtr;

/**
 * Resource needs to be created with D3D12_HEAP_FLAG_SHARED.
 */
DLL_OBJECT BufferD3D12ComputeApiExternalMemoryPtr createBufferD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource);


struct DLL_OBJECT ImageD3D12ComputeApiInfo {
    // Only needed for CUDA.
    bool surfaceLoadStore = false;

    // Only needed for sampled images.
    bool useSampledImage = false;
    D3D12_SAMPLER_DESC samplerDesc{};
    TextureExternalMemorySettings textureExternalMemorySettings{};
};

/**
 * Resource needs to be created with D3D12_HEAP_FLAG_SHARED.
 */
class DLL_OBJECT ImageD3D12ComputeApiExternalMemory {
public:
    ImageD3D12ComputeApiExternalMemory() = default;
    void initialize(sgl::d3d12::ResourcePtr& _resource);
    void initialize(sgl::d3d12::ResourcePtr& _resource, const ImageD3D12ComputeApiInfo& _imageComputeApiInfo);
    virtual ~ImageD3D12ComputeApiExternalMemory() = default;

    const sgl::d3d12::ResourcePtr& getResource() { return resource; }
    inline const ImageD3D12ComputeApiInfo& getImageComputeApiInfo() { return imageComputeApiInfo; }

    /*
     * Asynchronous copy from/to a device pointer to level 0 mipmap level.
     */
    virtual void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;

protected:
    virtual void importExternalMemoryWin32Handle() = 0;
    virtual void free() = 0;
    void freeHandle();

    sgl::d3d12::ResourcePtr resource;
    ImageD3D12ComputeApiInfo imageComputeApiInfo{};
    void* mipmappedArray{}; // CUmipmappedArray or hipMipmappedArray_t or ze_image_handle_t or SyclImageMemHandleWrapper (image_mem_handle)

    HANDLE handle = nullptr;
};

typedef std::shared_ptr<ImageD3D12ComputeApiExternalMemory> ImageD3D12ComputeApiExternalMemoryPtr;

/**
 * Resource needs to be created with D3D12_HEAP_FLAG_SHARED.
 */
DLL_OBJECT ImageD3D12ComputeApiExternalMemoryPtr createImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource);
DLL_OBJECT ImageD3D12ComputeApiExternalMemoryPtr createImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, const ImageD3D12ComputeApiInfo& imageComputeApiInfo);


/**
 * An unsampled image.
 */
class DLL_OBJECT UnsampledImageD3D12ComputeApiExternalMemory {
public:
    UnsampledImageD3D12ComputeApiExternalMemory() = default;
    virtual void initialize(const ImageD3D12ComputeApiExternalMemoryPtr& _image) = 0;
    virtual ~UnsampledImageD3D12ComputeApiExternalMemory() = default;

    const sgl::d3d12::ResourcePtr& getResource() { return image->getResource(); }

    /*
     * Asynchronous copy from/to a device pointer to level 0 mipmap level.
     */
    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyFromDevicePtrAsync(devicePtrSrc, stream, eventOut);
    }
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyToDevicePtrAsync(devicePtrDst, stream, eventOut);
    }

protected:
    ImageD3D12ComputeApiExternalMemoryPtr image;
};

typedef std::shared_ptr<UnsampledImageD3D12ComputeApiExternalMemory> UnsampledImageD3D12ComputeApiExternalMemoryPtr;

/**
 * Resource needs to be created with D3D12_HEAP_FLAG_SHARED.
 */
DLL_OBJECT UnsampledImageD3D12ComputeApiExternalMemoryPtr createUnsampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource);
DLL_OBJECT UnsampledImageD3D12ComputeApiExternalMemoryPtr createUnsampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, const ImageD3D12ComputeApiInfo& imageComputeApiInfo);
DLL_OBJECT UnsampledImageD3D12ComputeApiExternalMemoryPtr createUnsampledImageD3D12ComputeApiExternalMemory(
        const ImageD3D12ComputeApiExternalMemoryPtr& image);


/**
 * An sampled image.
 */
class DLL_OBJECT SampledImageD3D12ComputeApiExternalMemory {
public:
    SampledImageD3D12ComputeApiExternalMemory() {}
    virtual void initialize(
            const ImageD3D12ComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) = 0;
    virtual ~SampledImageD3D12ComputeApiExternalMemory() = default;

    const sgl::d3d12::ResourcePtr& getResource() { return image->getResource(); }

    /*
     * Asynchronous copy from/to a device pointer to level 0 mipmap level.
     */
    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyFromDevicePtrAsync(devicePtrSrc, stream, eventOut);
    }
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyToDevicePtrAsync(devicePtrDst, stream, eventOut);
    }

protected:
    ImageD3D12ComputeApiExternalMemoryPtr image;
};

typedef std::shared_ptr<SampledImageD3D12ComputeApiExternalMemory> SampledImageD3D12ComputeApiExternalMemoryPtr;

/**
 * Resource needs to be created with D3D12_HEAP_FLAG_SHARED.
 */
DLL_OBJECT SampledImageD3D12ComputeApiExternalMemoryPtr createSampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, const ImageD3D12ComputeApiInfo& imageComputeApiInfo);
DLL_OBJECT SampledImageD3D12ComputeApiExternalMemoryPtr createSampledImageD3D12ComputeApiExternalMemory(
        sgl::d3d12::ResourcePtr& resource, const D3D12_SAMPLER_DESC& samplerDesc,
        const TextureExternalMemorySettings& textureExternalMemorySettings);

}}

#endif //SGL_D3D12_INTEROPCOMPUTE_HPP
