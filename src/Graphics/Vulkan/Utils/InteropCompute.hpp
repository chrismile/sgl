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

#ifndef SGL_INTEROPCOMPUTE_HPP
#define SGL_INTEROPCOMPUTE_HPP

#include "../Buffers/Buffer.hpp"
#include "../Image/Image.hpp"
#include "SyncObjects.hpp"
#include "Device.hpp"

// Forward declarations for CUDA and HIP objects.
extern "C" {
#if defined(_WIN64) || defined(__LP64__)
typedef unsigned long long CUdeviceptr_v2;
#else
typedef unsigned int CUdeviceptr_v2;
#endif
typedef CUdeviceptr_v2 CUdeviceptr;
typedef void* hipDeviceptr_t;
typedef struct CUstream_st *CUstream;
typedef struct ihipStream_t* hipStream_t;
}

/*
 * This file provides wrappers over InteropCuda and InteropHIP.
 * Depending on what interop API has been initialized, CUDA or HIP objects, semaphores, memory, etc. are used internally.
 */

namespace sgl { namespace vk {

union DLL_OBJECT StreamWrapper {
    void* stream;
#ifdef SUPPORT_CUDA_INTEROP
    CUstream cuStream;
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipStream_t hipStream;
#endif
};

/**
 * A CUDA driver API CUexternalSemaphore/HIP driver API hipExternalSemaphore_t object created from a Vulkan semaphore.
 * Both binary and timeline semaphores are supported, but timeline semaphores require at least CUDA 11.2.
 */
class DLL_OBJECT SemaphoreVkComputeApiInterop : public vk::Semaphore {
public:
    explicit SemaphoreVkComputeApiInterop(
            Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags = 0,
            VkSemaphoreType semaphoreType = VK_SEMAPHORE_TYPE_BINARY, uint64_t timelineSemaphoreInitialValue = 0);
    ~SemaphoreVkComputeApiInterop() override;

    /// Signal semaphore.
    void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0);

    /// Wait on semaphore.
    void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0);

private:
    void* externalSemaphore = {}; // CUexternalSemaphore or hipExternalSemaphore_t
};

typedef std::shared_ptr<SemaphoreVkComputeApiInterop> SemaphoreVkComputeApiInteropPtr;


/**
 * A CUDA driver API CUdeviceptr/HIP driver API hipDeviceptr_t object created from a Vulkan buffer.
 */
class DLL_OBJECT BufferComputeApiExternalMemoryVk
{
public:
    explicit BufferComputeApiExternalMemoryVk(vk::BufferPtr& vulkanBuffer);
    virtual ~BufferComputeApiExternalMemoryVk();

    inline const sgl::vk::BufferPtr& getVulkanBuffer() { return vulkanBuffer; }

    template<class T>
    [[nodiscard]] inline T* getDevicePtr() const { return reinterpret_cast<T*>(devicePtr); }
#ifdef SUPPORT_CUDA_INTEROP
    [[nodiscard]] inline CUdeviceptr getCudaDevicePtr() const { return reinterpret_cast<CUdeviceptr>(devicePtr); }
#endif
#ifdef SUPPORT_CUDA_INTEROP
    [[nodiscard]] inline hipDeviceptr_t getHipDevicePtr() const { return reinterpret_cast<hipDeviceptr_t>(devicePtr); }
#endif

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream);

protected:
    sgl::vk::BufferPtr vulkanBuffer;
    void* externalMemoryBuffer{}; // CUexternalMemory or hipExternalMemory_t
    void* devicePtr{}; // CUdeviceptr or hipDeviceptr_t

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferComputeApiExternalMemoryVk> BufferComputeApiExternalMemoryVkPtr;

}}

#endif //SGL_INTEROPCOMPUTE_HPP
