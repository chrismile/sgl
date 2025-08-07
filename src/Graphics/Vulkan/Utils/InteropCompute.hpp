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

#ifndef SGL_INTEROP_COMPUTE_COMMON_HPP
#define SGL_INTEROP_COMPUTE_COMMON_HPP

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
DLL_OBJECT void setLevelZeroUseBindlessImagesInterop(bool useBindlessImages);
#ifdef SUPPORT_SYCL_INTEROP
DLL_OBJECT void setLevelZeroGlobalStateFromSyclQueue(sycl::queue& syclQueue);
#endif
#endif

#ifdef SUPPORT_SYCL_INTEROP
// For more information on SYCL interop:
// https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
DLL_OBJECT void setGlobalSyclQueue(sycl::queue& syclQueue);
#endif

/// Whether a message box should be shown when a compute API error is generated that is not fatal.
DLL_OBJECT void setOpenMessageBoxOnComputeApiError(bool _openMessageBox);

/// Decides the compute API usable for the passed device. SYCL has precedence over other APIs if available.
DLL_OBJECT InteropComputeApi decideInteropComputeApi(Device* device);

/// Reset function for unit tests, as static variables may persist across GoogleTest test cases.
DLL_OBJECT void resetComputeApiState();

/**
 * Waits for completion of the stream (CUDA, HIP, Level Zero) or event (SYCL, and optionally Level Zero if not nullptr).
 * If using Level Zero, @see setLevelZeroGlobalCommandQueue must have been called.
 */
DLL_OBJECT void waitForCompletion(InteropComputeApi interopComputeApi, StreamWrapper stream, void* event = nullptr);

/**
 * An exception that can be thrown when the compute API does not support the used feature.
 */
class UnsupportedComputeApiFeatureException : public std::exception {
public:
    explicit UnsupportedComputeApiFeatureException(std::string msg) : message(std::move(msg)) {}
    [[nodiscard]] char const* what() const noexcept override { return message.c_str(); }

private:
    std::string message;
};

/**
 * A CUDA driver API CUexternalSemaphore/HIP driver API hipExternalSemaphore_t object created from a Vulkan semaphore.
 * Both binary and timeline semaphores are supported, but timeline semaphores require at least CUDA 11.2.
 */
class DLL_OBJECT SemaphoreVkComputeApiInterop : public vk::Semaphore {
public:
    SemaphoreVkComputeApiInterop() = default;
    void initialize(
            Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
            VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue);
    ~SemaphoreVkComputeApiInterop() override = default;

    /// Signal semaphore.
    virtual void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) = 0;

    /// Wait on semaphore.
    virtual void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) = 0;

protected:
    virtual void preCheckExternalSemaphoreImport() {}
#ifdef _WIN32
    virtual void setExternalSemaphoreWin32Handle(HANDLE handle) = 0;
#endif
#ifdef __linux__
    virtual void setExternalSemaphoreFd(int fd) = 0;
#endif
    virtual void importExternalSemaphore() {}
};

typedef std::shared_ptr<SemaphoreVkComputeApiInterop> SemaphoreVkComputeApiInteropPtr;

SemaphoreVkComputeApiInteropPtr createSemaphoreVkComputeApiInterop(
        Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags = 0,
        VkSemaphoreType semaphoreType = VK_SEMAPHORE_TYPE_BINARY, uint64_t timelineSemaphoreInitialValue = 0);


/**
 * A CUDA driver API CUexternalSemaphore/HIP driver API hipExternalSemaphore_t object created from a Vulkan semaphore.
 * Both binary and timeline semaphores are supported, but timeline semaphores require at least CUDA 11.2.
 */
class DLL_OBJECT BufferVkComputeApiExternalMemory {
public:
    BufferVkComputeApiExternalMemory() = default;
    void initialize(vk::BufferPtr& vulkanBuffer);
    virtual ~BufferVkComputeApiExternalMemory() = default;

    inline const sgl::vk::BufferPtr& getVulkanBuffer() { return vulkanBuffer; }

    template<class T>
    [[nodiscard]] inline T* getDevicePtr() const { return reinterpret_cast<T*>(devicePtr); }
    template<class T>
    [[nodiscard]] inline T getDevicePtrReinterpreted() const { return reinterpret_cast<T>(devicePtr); }

    virtual void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;

protected:
    virtual void preCheckExternalMemoryImport() {}
#ifdef _WIN32
    virtual void setExternalMemoryWin32Handle(HANDLE handle) = 0;
#endif
#ifdef __linux__
    virtual void setExternalMemoryFd(int fd) = 0;
#endif
    virtual void importExternalMemory() {}
    virtual void free() = 0;
    void freeHandlesAndFds();

    sgl::vk::BufferPtr vulkanBuffer;
    VkMemoryRequirements memoryRequirements{};
    void* devicePtr{}; // CUdeviceptr or hipDeviceptr_t or void* device pointer

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferVkComputeApiExternalMemory> BufferVkComputeApiExternalMemoryPtr;

BufferVkComputeApiExternalMemoryPtr createBufferVkComputeApiExternalMemory(vk::BufferPtr& vulkanBuffer);


/**
 * A CUDA driver API CUexternalSemaphore/HIP driver API hipExternalSemaphore_t object created from a Vulkan semaphore.
 * Both binary and timeline semaphores are supported, but timeline semaphores require at least CUDA 11.2.
 */
class DLL_OBJECT ImageVkComputeApiExternalMemory {
public:
    ImageVkComputeApiExternalMemory() = default;
    void initialize(vk::ImagePtr& vulkanImage);
    void initialize(vk::ImagePtr& vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore);
    virtual ~ImageVkComputeApiExternalMemory() = default;

    inline const sgl::vk::ImagePtr& getVulkanImage() { return vulkanImage; }

    /*
     * Asynchronous copy from a device pointer to level 0 mipmap level.
     */
    virtual void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;

protected:
    virtual void preCheckExternalMemoryImport() {}
#ifdef _WIN32
    virtual void setExternalMemoryWin32Handle(HANDLE handle) = 0;
#endif
#ifdef __linux__
    virtual void setExternalMemoryFd(int fd) = 0;
#endif
    virtual void importExternalMemory() {}
    virtual void free() = 0;
    void freeHandlesAndFds();

    sgl::vk::ImagePtr vulkanImage;
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    bool surfaceLoadStore = false;
    VkMemoryRequirements memoryRequirements{};
    void* mipmappedArray{}; // CUmipmappedArray or hipMipmappedArray_t or ze_image_handle_t or SyclImageMemHandleWrapper (image_mem_handle)

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<ImageVkComputeApiExternalMemory> ImageVkComputeApiExternalMemoryPtr;

ImageVkComputeApiExternalMemoryPtr createImageVkComputeApiExternalMemory(vk::ImagePtr& vulkanImage);
ImageVkComputeApiExternalMemoryPtr createImageVkComputeApiExternalMemory(
        vk::ImagePtr& vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore);

}}

#endif //SGL_INTEROP_COMPUTE_COMMON_HPP
