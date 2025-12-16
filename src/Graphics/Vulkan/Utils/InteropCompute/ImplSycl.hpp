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

#ifndef SGL_IMPLSYCL_HPP
#define SGL_IMPLSYCL_HPP

#include "../InteropCompute.hpp"

namespace sgl { namespace vk {

// For more information on SYCL interop:
// https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
DLL_OBJECT void setGlobalSyclQueue(sycl::queue& syclQueue);

class SemaphoreVkSyclInterop : public SemaphoreVkComputeApiInterop {
public:
    ~SemaphoreVkSyclInterop() override;

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

private:
    void* externalSemaphore{};
};


class BufferVkSyclInterop : public BufferVkComputeApiExternalMemory {
public:
    ~BufferVkSyclInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
#ifdef _WIN32
    void setExternalMemoryWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalMemoryFd(int fileDescriptor) override;
#endif
    void importExternalMemory() override;
    void free() override;

private:
    void* externalMemoryBuffer{}; // SyclExternalMemWrapper
};


class ImageVkSyclInterop : public ImageVkComputeApiExternalMemory {
    friend class UnsampledImageVkSyclInterop;
    friend class SampledImageVkSyclInterop;
public:
    ~ImageVkSyclInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
#ifdef _WIN32
    void setExternalMemoryWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalMemoryFd(int fileDescriptor) override;
#endif
    void importExternalMemory() override;
    void free() override;

private:
    void* externalMemoryBuffer{}; // SyclExternalMemWrapper
};


class DLL_OBJECT UnsampledImageVkSyclInterop : public UnsampledImageVkComputeApiExternalMemory {
public:
    UnsampledImageVkSyclInterop() = default;
    void initialize(const ImageVkComputeApiExternalMemoryPtr& _image) override;
    ~UnsampledImageVkSyclInterop() override;

    uint64_t getRawHandle();

protected:
    uint64_t rawImageHandle = 0;
};


class DLL_OBJECT SampledImageVkSyclInterop : public SampledImageVkComputeApiExternalMemory {
public:
    SampledImageVkSyclInterop() = default;
    void initialize(
            const ImageVkComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) override;
    ~SampledImageVkSyclInterop() override;

    uint64_t getRawHandle();

protected:
    uint64_t rawImageHandle = 0;
};

}}

#endif //SGL_IMPLSYCL_HPP
