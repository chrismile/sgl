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

#ifndef SGL_IMPLHIP_HPP
#define SGL_IMPLHIP_HPP

#include "../InteropCompute.hpp"
#include "../InteropHIP.hpp"

namespace sgl { namespace vk {

class SemaphoreVkHipInterop : public SemaphoreVkComputeApiInterop {
public:
    ~SemaphoreVkHipInterop() override;

    /// Signal semaphore.
    void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) override;

    /// Wait on semaphore.
    void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) override;

protected:
#ifdef _WIN32
    void setExternalSemaphoreWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalSemaphoreFd(int fd) override;
#endif
    void importExternalSemaphore() override;

private:
    hipExternalSemaphoreHandleDesc externalSemaphoreHandleDescHip{};
    void* externalSemaphore{};
};


class BufferVkHipInterop : public BufferVkComputeApiExternalMemory {
public:
    ~BufferVkHipInterop() override;

#ifdef SUPPORT_HIP_INTEROP
    [[nodiscard]] inline hipDeviceptr_t getHipDevicePtr() const { return reinterpret_cast<hipDeviceptr_t>(devicePtr); }
#endif

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
    void setExternalMemoryFd(int fd) override;
#endif
    void importExternalMemory() override;
    void free() override;

private:
    hipExternalMemoryHandleDesc externalMemoryHandleDescHip{};
    void* externalMemoryBuffer{}; // hipExternalMemory_t
};


class ImageVkHipInterop : public ImageVkComputeApiExternalMemory {
public:
    ~ImageVkHipInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
    void preCheckExternalMemoryImport() override;
#ifdef _WIN32
    void setExternalMemoryWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalMemoryFd(int fd) override;
#endif
    void importExternalMemory() override;
    void free() override;

    [[nodiscard]] inline hipMipmappedArray_t getHipMipmappedArray() const { return reinterpret_cast<hipMipmappedArray_t>(mipmappedArray); }
    hipArray_t getHipMipmappedArrayLevel(uint32_t level = 0);

private:
    hipExternalMemoryHandleDesc externalMemoryHandleDescHip{};
    void* externalMemoryBuffer{}; // hipExternalMemory_t

    // Cache for storing the array for mipmap level 0.
    void* arrayLevel0{}; // hipArray_t
};

}}

#endif //SGL_IMPLHIP_HPP
