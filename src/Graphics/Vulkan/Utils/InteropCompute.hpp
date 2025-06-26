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

#include <stdexcept>
#include <utility>

#include "../Buffers/Buffer.hpp"
#include "../Image/Image.hpp"
#include "SyncObjects.hpp"
#include "Device.hpp"

// Forward declarations for CUDA, HIP and Level Zero objects.

extern "C" {
#ifdef SUPPORT_CUDA_INTEROP
#if defined(_WIN64) || defined(__LP64__)
typedef unsigned long long CUdeviceptr_v2;
#else
typedef unsigned int CUdeviceptr_v2;
#endif
typedef CUdeviceptr_v2 CUdeviceptr;
typedef struct CUmipmappedArray_st *CUmipmappedArray;
typedef struct CUarray_st *CUarray;
typedef struct CUstream_st *CUstream;
#endif

#ifdef SUPPORT_HIP_INTEROP
typedef void* hipDeviceptr_t;
typedef struct hipMipmappedArray* hipMipmappedArray_t;
typedef struct hipArray* hipArray_t;
typedef struct ihipStream_t* hipStream_t;
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
typedef struct _ze_device_handle_t* ze_device_handle_t;
typedef struct _ze_context_handle_t* ze_context_handle_t;
typedef struct _ze_command_queue_handle_t* ze_command_queue_handle_t;
typedef struct _ze_command_list_handle_t* ze_command_list_handle_t;
typedef struct _ze_event_handle_t *ze_event_handle_t;
#endif
}

#ifdef SUPPORT_SYCL_INTEROP
namespace sycl { inline namespace _V1 {
class queue;
}}
#endif

/*
 * This file provides wrappers over InteropCuda, InteropHIP and InteropLevelZero.
 * Depending on what interop API has been initialized, CUDA, HIP, Level Zero or SYCL objects, semaphores, memory, etc.
 * are used internally.
 *
 * Documentation of SYCL side code:
 * https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
 */

namespace sgl { namespace vk {

enum class InteropComputeApi {
    NONE, CUDA, HIP, LEVEL_ZERO, SYCL
};

union DLL_OBJECT StreamWrapper {
    void* stream;
#ifdef SUPPORT_CUDA_INTEROP
    CUstream cuStream;
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipStream_t hipStream;
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_command_list_handle_t zeCommandList;
#endif
#ifdef SUPPORT_SYCL_INTEROP
    sycl::queue* syclQueuePtr;
#endif
};

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
/*
 * Internally, Level Zero interop needs more information (device, context, ...) than CUDA or HIP interop.
 * The functions below can be used for setting the state globally.
 */
DLL_OBJECT void setLevelZeroGlobalState(ze_device_handle_t zeDevice, ze_context_handle_t zeContext);
DLL_OBJECT void setLevelZeroGlobalCommandQueue(ze_command_queue_handle_t zeCommandQueue);
DLL_OBJECT void setLevelZeroNextCommandEvents(
        ze_event_handle_t zeSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* zeWaitEvents);
#ifdef SUPPORT_SYCL_INTEROP
DLL_OBJECT void setLevelZeroGlobalStateFromSyclQueue(sycl::queue& syclQueue);
#endif
#endif

#ifdef SUPPORT_SYCL_INTEROP
// For more information on SYCL interop:
// https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
DLL_OBJECT void setGlobalSyclQueue(sycl::queue& syclQueue);
DLL_OBJECT void setOpenMessageBoxOnSyclError(bool _openMessageBox);
#endif

/*
 * Waits for completion of the stream (CUDA, HIP, Level Zero) or event (SYCL, and optionally Level Zero if not nullptr).
 * If using Level Zero, @see setLevelZeroGlobalCommandQueue must have been called.
 */
DLL_OBJECT void waitForCompletion(StreamWrapper stream, void* event = nullptr);

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
    void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr);

    /// Wait on semaphore.
    void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr);

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
#ifdef SUPPORT_HIP_INTEROP
    [[nodiscard]] inline hipDeviceptr_t getHipDevicePtr() const { return reinterpret_cast<hipDeviceptr_t>(devicePtr); }
#endif

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr);
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr);
    void copyFromHostPtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr);
    void copyToHostPtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr);

protected:
    sgl::vk::BufferPtr vulkanBuffer;
    void* externalMemoryBuffer{}; // CUexternalMemory or hipExternalMemory_t or SyclExternalMemWrapper
    void* devicePtr{}; // CUdeviceptr or hipDeviceptr_t or void* device pointer

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferComputeApiExternalMemoryVk> BufferComputeApiExternalMemoryVkPtr;


class UnsupportedComputeApiImageFormatException : public std::exception {
public:
    explicit UnsupportedComputeApiImageFormatException(std::string msg) : message(std::move(msg)) {}
    [[nodiscard]] char const* what() const noexcept override { return message.c_str(); }

private:
    std::string message;
};

/**
 * A CUDA driver API CUmipmappedArray object created from a Vulkan image.
 */
class DLL_OBJECT ImageComputeApiExternalMemoryVk
{
public:
    explicit ImageComputeApiExternalMemoryVk(vk::ImagePtr& vulkanImage);
    ImageComputeApiExternalMemoryVk(
            vk::ImagePtr& vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore);
    virtual ~ImageComputeApiExternalMemoryVk();

    inline const sgl::vk::ImagePtr& getVulkanImage() { return vulkanImage; }
#ifdef SUPPORT_CUDA_INTEROP
    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return reinterpret_cast<CUmipmappedArray>(mipmappedArray); }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0);
#endif
#ifdef SUPPORT_HIP_INTEROP
    [[nodiscard]] inline hipMipmappedArray_t getHipMipmappedArray() const { return reinterpret_cast<hipMipmappedArray_t>(mipmappedArray); }
    hipArray_t getHipMipmappedArrayLevel(uint32_t level = 0);
#endif

    /*
     * Asynchronous copy from a device pointer to level 0 mipmap level.
     */
    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr);

protected:
    void _initialize(vk::ImagePtr& _vulkanImage, VkImageViewType _imageViewType, bool surfaceLoadStore);

    sgl::vk::ImagePtr vulkanImage;
    VkImageViewType imageViewType;
    void* externalMemoryBuffer{}; // CUexternalMemory or hipExternalMemory_t or SyclExternalMemWrapper (external_mem)
    void* mipmappedArray{}; // CUmipmappedArray or hipMipmappedArray_t or ze_image_handle_t or SyclImageMemHandleWrapper (image_mem_handle)

    // Cache for storing the array for mipmap level 0.
    void* arrayLevel0{}; // CUarray or hipArray_t

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<ImageComputeApiExternalMemoryVk> ImageComputeApiExternalMemoryVkPtr;

}}

#endif //SGL_INTEROPCOMPUTE_HPP
