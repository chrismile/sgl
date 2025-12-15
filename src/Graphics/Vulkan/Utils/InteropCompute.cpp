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

#include "InteropCompute.hpp"

#ifdef SUPPORT_CUDA_INTEROP
#include "InteropCuda.hpp"
#include "InteropCompute/ImplCuda.hpp"
#endif

#ifdef SUPPORT_HIP_INTEROP
#include "InteropHIP.hpp"
#if HIP_VERSION_MAJOR < 6
#error Please install HIP SDK >= 6.0 for timeline semaphore support.
#endif
#include "InteropCompute/ImplHip.hpp"
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
#include "InteropLevelZero.hpp"
#include "InteropCompute/ImplLevelZero.hpp"
#endif

#ifdef SUPPORT_SYCL_INTEROP
#include "InteropCompute/ImplSycl.hpp"
#include <sycl/sycl.hpp>
#endif

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace sgl {
#ifdef SUPPORT_SYCL_INTEROP
extern sycl::queue* g_syclQueue;
#endif
}

namespace sgl { namespace vk {

InteropComputeApi decideInteropComputeApi(Device* device) {
    InteropComputeApi api = InteropComputeApi::NONE;
#ifdef SUPPORT_CUDA_INTEROP
    if (device->getDeviceDriverId() == VK_DRIVER_ID_NVIDIA_PROPRIETARY
            && getIsCudaDeviceApiFunctionTableInitialized()) {
        api = InteropComputeApi::CUDA;
    }
#endif
#ifdef SUPPORT_HIP_INTEROP
    if ((device->getDeviceDriverId() == VK_DRIVER_ID_AMD_PROPRIETARY
            || device->getDeviceDriverId() == VK_DRIVER_ID_AMD_OPEN_SOURCE)
            && getIsHipDeviceApiFunctionTableInitialized()) {
        api = InteropComputeApi::HIP;
    }
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if ((device->getDeviceDriverId() == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS
            || device->getDeviceDriverId() == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA)
            && getIsLevelZeroFunctionTableInitialized()) {
        api = InteropComputeApi::LEVEL_ZERO;
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (g_syclQueue != nullptr) {
        api = InteropComputeApi::SYCL;
    }
#endif
    return api;
}

SemaphoreVkComputeApiInteropPtr createSemaphoreVkComputeApiInterop(
        Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
        VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(device);
    SemaphoreVkComputeApiInteropPtr semaphore;
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropComputeApi::CUDA) {
        semaphore = std::make_shared<SemaphoreVkCudaInterop>();
    }
#endif
#ifdef SUPPORT_HIP_INTEROP
    if (interopComputeApi == InteropComputeApi::HIP) {
        semaphore = std::make_shared<SemaphoreVkHipInterop>();
    }
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (interopComputeApi == InteropComputeApi::LEVEL_ZERO) {
        semaphore = std::make_shared<SemaphoreVkLevelZeroInterop>();
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        semaphore = std::make_shared<SemaphoreVkSyclInterop>();
    }
#endif
    if (!semaphore) {
        sgl::Logfile::get()->writeError("Error in createSemaphoreVkComputeApiInterop: Unsupported compute API.");
        return semaphore;
    }

    semaphore->initialize(device, semaphoreCreateFlags, semaphoreType, timelineSemaphoreInitialValue);
    return semaphore;
}

BufferVkComputeApiExternalMemoryPtr createBufferVkComputeApiExternalMemory(vk::BufferPtr& vulkanBuffer) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(vulkanBuffer->getDevice());
    BufferVkComputeApiExternalMemoryPtr bufferExtMem;
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropComputeApi::CUDA) {
        bufferExtMem = std::make_shared<BufferVkCudaInterop>();
    }
#endif
#ifdef SUPPORT_HIP_INTEROP
    if (interopComputeApi == InteropComputeApi::HIP) {
        bufferExtMem = std::make_shared<BufferVkHipInterop>();
    }
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (interopComputeApi == InteropComputeApi::LEVEL_ZERO) {
        bufferExtMem = std::make_shared<BufferVkLevelZeroInterop>();
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        bufferExtMem = std::make_shared<BufferVkSyclInterop>();
    }
#endif
    if (!bufferExtMem) {
        sgl::Logfile::get()->writeError("Error in createBufferVkComputeApiExternalMemory: Unsupported compute API.");
        return bufferExtMem;
    }

    bufferExtMem->initialize(vulkanBuffer);
    return bufferExtMem;
}

ImageVkComputeApiExternalMemoryPtr createImageVkComputeApiExternalMemory(ImagePtr& vulkanImage) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(vulkanImage->getDevice());
    ImageVkComputeApiExternalMemoryPtr imageExtMem;
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropComputeApi::CUDA) {
        imageExtMem = std::make_shared<ImageVkCudaInterop>();
    }
#endif
#ifdef SUPPORT_HIP_INTEROP
    if (interopComputeApi == InteropComputeApi::HIP) {
        imageExtMem = std::make_shared<ImageVkHipInterop>();
    }
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (interopComputeApi == InteropComputeApi::LEVEL_ZERO) {
        imageExtMem = std::make_shared<ImageVkLevelZeroInterop>();
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        imageExtMem = std::make_shared<ImageVkSyclInterop>();
    }
#endif
    if (!imageExtMem) {
        sgl::Logfile::get()->writeError("Error in createImageVkComputeApiExternalMemory: Unsupported compute API.");
        return imageExtMem;
    }

    imageExtMem->initialize(vulkanImage);
    return imageExtMem;
}

ImageVkComputeApiExternalMemoryPtr createImageVkComputeApiExternalMemory(
        ImagePtr& vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(vulkanImage->getDevice());
    ImageVkComputeApiExternalMemoryPtr imageExtMem;
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropComputeApi::CUDA) {
        imageExtMem = std::make_shared<ImageVkCudaInterop>();
    }
#endif
#ifdef SUPPORT_HIP_INTEROP
    if (interopComputeApi == InteropComputeApi::HIP) {
        imageExtMem = std::make_shared<ImageVkHipInterop>();
    }
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (interopComputeApi == InteropComputeApi::LEVEL_ZERO) {
        imageExtMem = std::make_shared<ImageVkLevelZeroInterop>();
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        imageExtMem = std::make_shared<ImageVkSyclInterop>();
    }
#endif
    if (!imageExtMem) {
        sgl::Logfile::get()->writeError("Error in createImageVkComputeApiExternalMemory: Unsupported compute API.");
        return imageExtMem;
    }

    imageExtMem->initialize(vulkanImage, imageViewType, surfaceLoadStore);
    return imageExtMem;
}


void SemaphoreVkComputeApiInterop::initialize(
        sgl::vk::Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
        VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue) {
    VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo = {};
    exportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#if defined(_WIN32)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
    Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "External semaphores are only supported on Linux, Android and Windows systems!");
#endif
    _initialize(
            device, semaphoreCreateFlags, semaphoreType, timelineSemaphoreInitialValue,
            &exportSemaphoreCreateInfo);

    preCheckExternalSemaphoreImport();

#if defined(_WIN32)
    auto _vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
                device->getVkDevice(), "vkGetSemaphoreWin32HandleKHR");
    if (!_vkGetSemaphoreWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "vkGetSemaphoreWin32HandleKHR was not found!");
    }

    VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfo = {};
    semaphoreGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    semaphoreGetWin32HandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    semaphoreGetWin32HandleInfo.semaphore = semaphoreVk;
    handle = nullptr;
    if (_vkGetSemaphoreWin32HandleKHR(
            device->getVkDevice(), &semaphoreGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "vkGetSemaphoreFdKHR failed!");
    }

    setExternalSemaphoreWin32Handle(handle);

#elif defined(__linux__)

    auto _vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(
            device->getVkDevice(), "vkGetSemaphoreFdKHR");
    if (!_vkGetSemaphoreFdKHR) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "vkGetSemaphoreFdKHR was not found!");
    }

    VkSemaphoreGetFdInfoKHR semaphoreGetFdInfo = {};
    semaphoreGetFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    semaphoreGetFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    semaphoreGetFdInfo.semaphore = semaphoreVk;
    fileDescriptor = 0;
    if (_vkGetSemaphoreFdKHR(device->getVkDevice(), &semaphoreGetFdInfo, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "vkGetSemaphoreFdKHR failed!");
    }

    setExternalSemaphoreFd(fileDescriptor);

#endif // defined(__linux__)

    importExternalSemaphore();

    /*
     * https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__EXTRES__INTEROP.html
     * - CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD and CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_FD:
     * "Ownership of the file descriptor is transferred to the CUDA driver when the handle is imported successfully."
     * - CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32 and CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_WIN32:
     * "Ownership of this handle is not transferred to CUDA after the import operation, so the application must release
     * the handle using the appropriate system call."
     */
#if defined(__linux__)
    fileDescriptor = -1;
#endif
}


void BufferVkComputeApiExternalMemory::initialize(vk::BufferPtr& _vulkanBuffer) {
    if (devicePtr) {
        free();
    }
    vulkanBuffer = _vulkanBuffer;

    VkDevice device = vulkanBuffer->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanBuffer->getVkDeviceMemory();

    vkGetBufferMemoryRequirements(device, vulkanBuffer->getVkBuffer(), &memoryRequirements);

    preCheckExternalMemoryImport();

#if defined(_WIN32)
    auto _vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            device, "vkGetMemoryWin32HandleKHR");
    if (!_vkGetMemoryWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                "vkGetMemoryWin32HandleKHR was not found!");
        return;
    }
    VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = {};
    memoryGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    memoryGetWin32HandleInfo.memory = deviceMemory;
    memoryGetWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE handle = nullptr;
    if (_vkGetMemoryWin32HandleKHR(device, &memoryGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    setExternalMemoryWin32Handle(handle);

    this->handle = handle;

#elif defined(__linux__)

    auto _vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!_vkGetMemoryFdKHR) {
        Logfile::get()->throwError(
                "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                "vkGetMemoryFdKHR was not found!");
        return;
    }

    VkMemoryGetFdInfoKHR memoryGetFdInfoKhr = {};
    memoryGetFdInfoKhr.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    memoryGetFdInfoKhr.memory = deviceMemory;
    memoryGetFdInfoKhr.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    int fileDescriptor = 0;
    if (_vkGetMemoryFdKHR(device, &memoryGetFdInfoKhr, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    setExternalMemoryFd(fileDescriptor);

    this->fileDescriptor = fileDescriptor;

#else // defined(__linux__)

    Logfile::get()->throwError(
            "Error in BufferVkComputeApiExternalMemory::initialize: "
            "External memory is only supported on Linux, Android and Windows systems!");

#endif // defined(__linux__)

    importExternalMemory();

    /*
     * https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__EXTRES__INTEROP.html
     * - CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD: "Ownership of the file descriptor is transferred to the CUDA driver
     * when the handle is imported successfully."
     * - CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32: "Ownership of this handle is not transferred to CUDA after the
     * import operation, so the application must release the handle using the appropriate system call."
     */
#if defined(__linux__)
    this->fileDescriptor = -1;
#endif
}

void BufferVkComputeApiExternalMemory::freeHandlesAndFds() {
#ifdef _WIN32
    if (handle) {
        CloseHandle(handle);
        handle = {};
    }
#elif defined(__linux__)
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif
}


void ImageVkComputeApiExternalMemory::initialize(vk::ImagePtr& vulkanImage) {
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    const auto& imageSettings = vulkanImage->getImageSettings();
    if (imageSettings.imageType == VK_IMAGE_TYPE_1D) {
        imageViewType = VK_IMAGE_VIEW_TYPE_1D;
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_2D) {
        imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_3D) {
        imageViewType = VK_IMAGE_VIEW_TYPE_3D;
    }
    bool surfaceLoadStore = (imageSettings.usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0;
    initialize(vulkanImage, imageViewType, surfaceLoadStore);
}

void ImageVkComputeApiExternalMemory::initialize(
        vk::ImagePtr& _vulkanImage, VkImageViewType _imageViewType, bool _surfaceLoadStore) {
    if (mipmappedArray) {
        free();
    }

    vulkanImage = _vulkanImage;
    imageViewType = _imageViewType;
    surfaceLoadStore = _surfaceLoadStore;

    VkDevice device = vulkanImage->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanImage->getVkDeviceMemory();

    vkGetImageMemoryRequirements(device, vulkanImage->getVkImage(), &memoryRequirements);

    preCheckExternalMemoryImport();

#if defined(_WIN32)
    auto _vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            device, "vkGetMemoryWin32HandleKHR");
    if (!_vkGetMemoryWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
                "vkGetMemoryWin32HandleKHR was not found!");
        return;
    }
    VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = {};
    memoryGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    memoryGetWin32HandleInfo.memory = deviceMemory;
    memoryGetWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE handle = nullptr;
    if (_vkGetMemoryWin32HandleKHR(device, &memoryGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    setExternalMemoryWin32Handle(handle);

    this->handle = handle;

#elif defined(__linux__)

    auto _vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!_vkGetMemoryFdKHR) {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
                "vkGetMemoryFdKHR was not found!");
        return;
    }

    VkMemoryGetFdInfoKHR memoryGetFdInfoKhr = {};
    memoryGetFdInfoKhr.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    memoryGetFdInfoKhr.memory = deviceMemory;
    memoryGetFdInfoKhr.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    int fileDescriptor = 0;
    if (_vkGetMemoryFdKHR(device, &memoryGetFdInfoKhr, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    setExternalMemoryFd(fileDescriptor);

    this->fileDescriptor = fileDescriptor;

#else // defined(__linux__)

    Logfile::get()->throwError(
            "Error in ImageVkComputeApiExternalMemory::initialize: "
            "External memory is only supported on Linux, Android and Windows systems!");

#endif // defined(__linux__)

    importExternalMemory();

    /*
     * https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__EXTRES__INTEROP.html
     * - CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD: "Ownership of the file descriptor is transferred to the CUDA driver
     * when the handle is imported successfully."
     * - CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32: "Ownership of this handle is not transferred to CUDA after the
     * import operation, so the application must release the handle using the appropriate system call."
     */
#if defined(__linux__)
    this->fileDescriptor = -1;
#endif
}

void ImageVkComputeApiExternalMemory::freeHandlesAndFds() {
#ifdef _WIN32
    if (handle) {
        CloseHandle(handle);
        handle = {};
    }
#elif defined(__linux__)
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif
}


UnsampledImageVkComputeApiExternalMemoryPtr createUnsampledImageVkComputeApiExternalMemory(
        vk::ImagePtr& vulkanImage) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(vulkanImage->getDevice());
    UnsampledImageVkComputeApiExternalMemoryPtr unsampledImageExtMem;
    ImageVkComputeApiExternalMemoryPtr imageExtMem = createImageVkComputeApiExternalMemory(vulkanImage);
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropComputeApi::CUDA) {
        unsampledImageExtMem = std::make_shared<UnsampledImageVkCudaInterop>();
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        unsampledImageExtMem = std::make_shared<UnsampledImageVkSyclInterop>();
    }
#endif
    if (!unsampledImageExtMem) {
        sgl::Logfile::get()->writeError(
                "Error in createUnsampledImageVkComputeApiExternalMemory: Unsupported compute API.");
        return unsampledImageExtMem;
    }

    unsampledImageExtMem->initialize(imageExtMem);
    return unsampledImageExtMem;
}

UnsampledImageVkComputeApiExternalMemoryPtr createUnsampledImageVkComputeApiExternalMemory(
        ImagePtr& vulkanImage, VkImageViewType imageViewType) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(vulkanImage->getDevice());
    UnsampledImageVkComputeApiExternalMemoryPtr unsampledImageExtMem;
    ImageVkComputeApiExternalMemoryPtr imageExtMem = createImageVkComputeApiExternalMemory(
            vulkanImage, imageViewType, true);
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropComputeApi::CUDA) {
        unsampledImageExtMem = std::make_shared<UnsampledImageVkCudaInterop>();
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        unsampledImageExtMem = std::make_shared<UnsampledImageVkSyclInterop>();
    }
#endif
    if (!unsampledImageExtMem) {
        sgl::Logfile::get()->writeError(
                "Error in createUnsampledImageVkComputeApiExternalMemory: Unsupported compute API.");
        return unsampledImageExtMem;
    }

    unsampledImageExtMem->initialize(imageExtMem);
    return unsampledImageExtMem;
}

UnsampledImageVkComputeApiExternalMemoryPtr createUnsampledImageVkComputeApiExternalMemory(
        const ImageVkComputeApiExternalMemoryPtr& imageExtMem) {
    InteropComputeApi interopComputeApi = decideInteropComputeApi(imageExtMem->getVulkanImage()->getDevice());
    UnsampledImageVkComputeApiExternalMemoryPtr unsampledImageExtMem;
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropComputeApi::CUDA) {
        unsampledImageExtMem = std::make_shared<UnsampledImageVkCudaInterop>();
    }
#endif
#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropComputeApi::SYCL) {
        unsampledImageExtMem = std::make_shared<UnsampledImageVkSyclInterop>();
    }
#endif
    if (!unsampledImageExtMem) {
        sgl::Logfile::get()->writeError(
                "Error in createUnsampledImageVkComputeApiExternalMemory: Unsupported compute API.");
        return unsampledImageExtMem;
    }

    unsampledImageExtMem->initialize(imageExtMem);
    return unsampledImageExtMem;
}

}}
