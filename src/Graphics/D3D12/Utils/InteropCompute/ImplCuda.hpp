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

#ifndef SGL_D3D12_IMPLCUDA_HPP
#define SGL_D3D12_IMPLCUDA_HPP

#include "../InteropCompute.hpp"
#include "../InteropCuda.hpp"

namespace sgl { namespace d3d12 {

class FenceD3D12CudaInterop : public FenceD3D12ComputeApiInterop {
public:
    ~FenceD3D12CudaInterop() override;

    /// Signal fence.
    void signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) override;

    /// Wait on fence.
    void waitFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) override;

protected:
    void importExternalFenceWin32Handle() override;
    void free() override;

private:
    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC externalSemaphoreHandleDesc{};
    void* externalSemaphore{};
};


class BufferD3D12CudaInterop : public BufferD3D12ComputeApiExternalMemory {
public:
    ~BufferD3D12CudaInterop() override;

    [[nodiscard]] inline CUdeviceptr getCudaDevicePtr() const { return reinterpret_cast<CUdeviceptr>(devicePtr); }

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
    void importExternalMemoryWin32Handle() override;
    void free() override;

private:
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    void* externalMemoryBuffer{}; // CUexternalMemory
};


class ImageD3D12CudaInterop : public ImageD3D12ComputeApiExternalMemory {
    friend class UnsampledImageD3D12CudaInterop;
    friend class SampledImageD3D12CudaInterop;
public:
    ~ImageD3D12CudaInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return reinterpret_cast<CUmipmappedArray>(mipmappedArray); }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0);

protected:
    void importExternalMemoryWin32Handle() override;
    void free() override;

private:
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    void* externalMemoryBuffer{}; // CUexternalMemory

    // Cache for storing the array for mipmap level 0.
    void* arrayLevel0{}; // CUarray
};


class DLL_OBJECT UnsampledImageD3D12CudaInterop : public UnsampledImageD3D12ComputeApiExternalMemory {
public:
    UnsampledImageD3D12CudaInterop() = default;
    void initialize(const ImageD3D12ComputeApiExternalMemoryPtr& _image) override;
    ~UnsampledImageD3D12CudaInterop() override;

    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return std::static_pointer_cast<ImageD3D12CudaInterop>(image)->getCudaMipmappedArray(); }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0) { return std::static_pointer_cast<ImageD3D12CudaInterop>(image)->getCudaMipmappedArrayLevel(level); }

    CUsurfObject getCudaSurfaceObject() { return cudaSurfaceObject; }

protected:
    CUsurfObject cudaSurfaceObject{};
};


class DLL_OBJECT SampledImageD3D12CudaInterop : public SampledImageD3D12ComputeApiExternalMemory {
public:
    SampledImageD3D12CudaInterop() = default;
    void initialize(
            const ImageD3D12ComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) override;
    ~SampledImageD3D12CudaInterop() override;

    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return std::static_pointer_cast<ImageD3D12CudaInterop>(image)->getCudaMipmappedArray(); }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0) { return std::static_pointer_cast<ImageD3D12CudaInterop>(image)->getCudaMipmappedArrayLevel(level); }

    [[nodiscard]] inline CUtexObject getCudaTextureObject() const { return cudaTextureObject; }

protected:
    CUtexObject cudaTextureObject{};
};

}}

#endif //SGL_D3D12_IMPLCUDA_HPP
