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

#ifndef SGL_IMPLCUDA_HPP
#define SGL_IMPLCUDA_HPP

#include "../InteropCompute.hpp"
#include "../InteropCuda.hpp"

namespace sgl { namespace vk {

class SemaphoreVkCudaInterop : public SemaphoreVkComputeApiInterop {
public:
    ~SemaphoreVkCudaInterop() override;

    /// Signal semaphore.
    void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) override;

    /// Wait on semaphore.
    void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) override;

protected:
#ifdef _WIN32
    void setExternalSemaphoreWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalSemaphoreFd(int fileDescriptor) override;
#endif
    void importExternalSemaphore() override;

private:
    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC externalSemaphoreHandleDesc{};
    void* externalSemaphore{};
};


class BufferVkCudaInterop : public BufferVkComputeApiExternalMemory {
public:
    ~BufferVkCudaInterop() override;

    [[nodiscard]] inline CUdeviceptr getCudaDevicePtr() const { return reinterpret_cast<CUdeviceptr>(devicePtr); }

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
    void preCheckExternalMemoryImport() override;
#ifdef _WIN32
    void setExternalMemoryWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalMemoryFd(int fileDescriptor) override;
#endif
    void importExternalMemory() override;
    void free() override;

private:
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    void* externalMemoryBuffer{}; // CUexternalMemory
};


class ImageVkCudaInterop : public ImageVkComputeApiExternalMemory {
public:
    ~ImageVkCudaInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return reinterpret_cast<CUmipmappedArray>(mipmappedArray); }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0);

protected:
    void preCheckExternalMemoryImport() override;
#ifdef _WIN32
    void setExternalMemoryWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalMemoryFd(int fileDescriptor) override;
#endif
    void importExternalMemory() override;
    void free() override;

private:
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    void* externalMemoryBuffer{}; // CUexternalMemory

    // Cache for storing the array for mipmap level 0.
    void* arrayLevel0{}; // CUarray
};


class DLL_OBJECT UnsampledImageVkCudaInterop : public UnsampledImageVkComputeApiExternalMemory {
public:
    UnsampledImageVkCudaInterop() = default;
    void initialize(const ImageVkComputeApiExternalMemoryPtr& _image) override;
    ~UnsampledImageVkCudaInterop() override;

    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return std::static_pointer_cast<ImageVkCudaInterop>(image)->getCudaMipmappedArray(); }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0) { return std::static_pointer_cast<ImageVkCudaInterop>(image)->getCudaMipmappedArrayLevel(level); }

    CUsurfObject getCudaSurfaceObject() { return cudaSurfaceObject; }

protected:
    CUsurfObject cudaSurfaceObject{};
};

class DLL_OBJECT SampledImageVkCudaInterop : public SampledImageVkComputeApiExternalMemory {
public:
    SampledImageVkCudaInterop() = default;
    void initialize(
            const ImageVkComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) override;
    ~SampledImageVkCudaInterop() override;

    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return std::static_pointer_cast<ImageVkCudaInterop>(image)->getCudaMipmappedArray(); }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0) { return std::static_pointer_cast<ImageVkCudaInterop>(image)->getCudaMipmappedArrayLevel(level); }

    [[nodiscard]] inline CUtexObject getCudaTextureObject() const { return cudaTextureObject; }

protected:
    CUtexObject cudaTextureObject{};
};

}}

#endif //SGL_IMPLCUDA_HPP