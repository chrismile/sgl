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
    virtual void signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) = 0;

    /// Wait on fence.
    virtual void waitFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) = 0;

protected:
    virtual void importExternalFenceWin32Handle() = 0;
    virtual void free() = 0;
    void freeHandle();

    HANDLE handle = nullptr;
};

typedef std::shared_ptr<FenceD3D12ComputeApiInterop> FenceD3D12ComputeApiInteropPtr;

FenceD3D12ComputeApiInteropPtr createFenceD3D12ComputeApiInterop(Device* device, uint64_t value = 0);


class DLL_OBJECT ResourceD3D12ComputeApiExternalMemory {
public:
    ResourceD3D12ComputeApiExternalMemory() = default;
    void initialize(sgl::d3d12::ResourcePtr& _resource);
    virtual ~ResourceD3D12ComputeApiExternalMemory() = default;

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

typedef std::shared_ptr<ResourceD3D12ComputeApiExternalMemory> ResourceD3D12ComputeApiExternalMemoryPtr;

ResourceD3D12ComputeApiExternalMemoryPtr createResourceD3D12ComputeApiExternalMemory(sgl::d3d12::ResourcePtr& resource);

}}

#endif //SGL_D3D12_INTEROPCOMPUTE_HPP
