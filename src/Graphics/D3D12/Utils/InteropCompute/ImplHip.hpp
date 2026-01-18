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

#ifndef SGL_D3D12_IMPLHIP_HPP
#define SGL_D3D12_IMPLHIP_HPP

#include "../InteropCompute.hpp"
#include "../InteropHIP.hpp"

namespace sgl { namespace d3d12 {

class FenceD3D12HipInterop : public FenceD3D12ComputeApiInterop {
public:
    ~FenceD3D12HipInterop() override;

    /// Signal fence.
    void signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) override;

    /// Wait on fence.
    void waitFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) override;

protected:
    void importExternalFenceWin32Handle() override;
    void free() override;

private:
    hipExternalSemaphoreHandleDesc externalSemaphoreHandleDescHip{};
    void* externalSemaphore{};
};


class BufferD3D12HipInterop : public BufferD3D12ComputeApiExternalMemory {
public:
    ~BufferD3D12HipInterop() override;

    [[nodiscard]] inline hipDeviceptr_t getHipDevicePtr() const { return reinterpret_cast<hipDeviceptr_t>(devicePtr); }

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
    void importExternalMemoryWin32Handle() override;
    void free() override;

private:
    hipExternalMemoryHandleDesc externalMemoryHandleDescHip{};
    void* externalMemoryBuffer{}; // hipExternalMemory_t
};


class ImageD3D12HipInterop : public ImageD3D12ComputeApiExternalMemory {
    friend class UnsampledImageD3D12HipInterop;
    friend class SampledImageD3D12HipInterop;
public:
    ~ImageD3D12HipInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

    [[nodiscard]] inline hipMipmappedArray_t getHipMipmappedArray() const { return reinterpret_cast<hipMipmappedArray_t>(mipmappedArray); }
    hipArray_t getHipMipmappedArrayLevel(uint32_t level = 0);

protected:
    void importExternalMemoryWin32Handle() override;
    void free() override;

private:
    hipExternalMemoryHandleDesc externalMemoryHandleDescHip{};
    void* externalMemoryBuffer{}; // hipExternalMemory_t

    // Cache for storing the array for mipmap level 0.
    void* arrayLevel0{}; // hipArray_t
};


class DLL_OBJECT UnsampledImageD3D12HipInterop : public UnsampledImageD3D12ComputeApiExternalMemory {
public:
    UnsampledImageD3D12HipInterop() = default;
    void initialize(const ImageD3D12ComputeApiExternalMemoryPtr& _image) override;
    ~UnsampledImageD3D12HipInterop() override;

    [[nodiscard]] inline hipMipmappedArray_t getHipMipmappedArray() const { return std::static_pointer_cast<ImageD3D12HipInterop>(image)->getHipMipmappedArray(); }
    hipArray_t getHipMipmappedArrayLevel(uint32_t level = 0) { return std::static_pointer_cast<ImageD3D12HipInterop>(image)->getHipMipmappedArrayLevel(level); }

    hipSurfaceObject_t getHipSurfaceObject() { return hipSurfaceObject; }

protected:
    hipSurfaceObject_t hipSurfaceObject{};
};

}}

#endif //SGL_D3D12_IMPLHIP_HPP
