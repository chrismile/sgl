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
#endif

#ifdef SUPPORT_HIP_INTEROP
#include "InteropHIP.hpp"
#if HIP_VERSION_MAJOR < 6
#error Please install HIP SDK >= 6.0 for timeline semaphore support.
#endif
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
#include "InteropLevelZero.hpp"
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

#ifdef SUPPORT_SYCL_INTEROP
#include <sycl/sycl.hpp>
#include <sycl/ext/oneapi/backend/level_zero.hpp>
#endif

namespace sgl { namespace vk {

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
static ze_device_handle_t g_zeDevice = {};
static ze_context_handle_t g_zeContext = {};
static ze_event_handle_t g_zeSignalEvent = {};
static uint32_t g_numWaitEvents = 0;
static ze_event_handle_t* g_zeWaitEvents = {};
void setLevelZeroGlobalState(ze_device_handle_t zeDevice, ze_context_handle_t zeContext) {
    g_zeDevice = zeDevice;
    g_zeContext = zeContext;
}
void setLevelZeroNextCommandEvents(
        ze_event_handle_t zeSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* zeWaitEvents) {
    g_zeSignalEvent = zeSignalEvent;
    g_numWaitEvents = numWaitEvents;
    g_zeWaitEvents = zeWaitEvents;
}
#ifdef SUPPORT_SYCL_INTEROP
void setLevelZeroGlobalStateFromSyclQueue(sycl::queue& syclQueue) {
    g_zeDevice = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue.get_device());
    g_zeContext = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue.get_context());
}
#endif
#endif

SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop(
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


    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC externalSemaphoreHandleDesc{};
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipExternalSemaphoreHandleDesc externalSemaphoreHandleDescHip{};
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_semaphore_ext_desc_t externalSemaphoreExtDesc{};
    externalSemaphoreExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC;
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
    if (useLevelZero) {
        if (!g_zeDevice) {
            sgl::Logfile::get()->throwError(
                    "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                    "Level Zero is initialized, but the global device object is not set.");
        }
    }
#endif
    if (int(useCuda) + int(useHip) + int(useLevelZero) != 1) {
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "Only one out of CUDA, HIP and Level Zero interop can be initialized at a time.");
    }

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

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        if (isTimelineSemaphore()) {
#if CUDA_VERSION >= 11020
            externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_WIN32;
#else
            sgl::Logfile::get()->throwError(
                    "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: Timeline semaphores "
                    "are only supported starting in CUDA version 11.2.");
#endif
        } else {
            externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32;
        }
        externalSemaphoreHandleDesc.handle.win32.handle = handle;
#endif
    }

    if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        if (isTimelineSemaphore()) {
            externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeTimelineSemaphoreWin32;
        } else {
            externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeOpaqueWin32;
        }
        externalSemaphoreHandleDescHip.handle.win32.handle = handle;
#endif
    }

    ze_external_semaphore_win32_ext_desc_t externalSemaphoreWin32ExtDesc{};
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalSemaphoreWin32ExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC;
        externalSemaphoreExtDesc.pNext = &externalSemaphoreWin32ExtDesc;
        if (isTimelineSemaphore()) {
            externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_VK_TIMELINE_SEMAPHORE_WIN32;
        } else {
            externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_OPAQUE_WIN32;
        }
        externalSemaphoreWin32ExtDesc.handle = handle;
#endif
    }

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

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        if (isTimelineSemaphore()) {
#if CUDA_VERSION >= 11020
            externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_FD;
#else
            sgl::Logfile::get()->throwError(
                    "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: Timeline semaphores "
                    "are only supported starting in CUDA version 11.2.");
#endif
        } else {
            externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD;
        }
        externalSemaphoreHandleDesc.handle.fd = fileDescriptor;
#endif
    }

    if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        if (isTimelineSemaphore()) {
            externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeTimelineSemaphoreFd;
        } else {
            externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeOpaqueFd;
        }
        externalSemaphoreHandleDescHip.handle.fd = fileDescriptor;
#endif
    }

    ze_external_semaphore_fd_ext_desc_t externalSemaphoreFdExtDesc{};
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalSemaphoreFdExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_FD_EXT_DESC;
        externalSemaphoreExtDesc.pNext = &externalSemaphoreFdExtDesc;
        if (isTimelineSemaphore()) {
            externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_VK_TIMELINE_SEMAPHORE_FD;
        } else {
            externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_OPAQUE_FD;
        }
        externalSemaphoreFdExtDesc.fd = fileDescriptor;
#endif
    }

#endif // defined(__linux__)


    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        CUexternalSemaphore cuExternalSemaphore{};
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalSemaphore(
                &cuExternalSemaphore, &externalSemaphoreHandleDesc);
        checkCUresult(cuResult, "Error in cuImportExternalSemaphore: ");
        externalSemaphore = reinterpret_cast<void*>(cuExternalSemaphore);
#endif
    }

    if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        hipExternalSemaphore_t hipExternalSemaphore{};
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipImportExternalSemaphore(
                &hipExternalSemaphore, &externalSemaphoreHandleDescHip);
        checkHipResult(hipResult, "Error in hipImportExternalSemaphore: ");
        externalSemaphore = reinterpret_cast<void*>(hipExternalSemaphore);
#endif
    }

    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        ze_external_semaphore_ext_handle_t zeExternalSemaphore{};
        ze_result_t zeResult = g_levelZeroFunctionTable.zeDeviceImportExternalSemaphoreExt(
                g_zeDevice, &externalSemaphoreExtDesc, &zeExternalSemaphore);
        checkZeResult(zeResult, "Error in zeDeviceImportExternalSemaphoreExt: ");
        externalSemaphore = reinterpret_cast<void*>(zeExternalSemaphore);
#endif
    }


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

SemaphoreVkComputeApiInterop::~SemaphoreVkComputeApiInterop() {
    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore(cuExternalSemaphore);
        checkCUresult(cuResult, "Error in cuDestroyExternalSemaphore: ");
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalSemaphore(hipExternalSemaphore);
        checkHipResult(hipResult, "Error in hipDestroyExternalSemaphore: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        auto zeExternalSemaphore = reinterpret_cast<ze_external_semaphore_ext_handle_t>(externalSemaphore);
        ze_result_t zeResult = g_levelZeroFunctionTable.zeDeviceReleaseExternalSemaphoreExt(zeExternalSemaphore);
        checkZeResult(zeResult, "Error in zeDeviceReleaseExternalSemaphoreExt: ");
#endif
    }
}

void SemaphoreVkComputeApiInterop::signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
        CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS signalParams{};
        if (isTimelineSemaphore()) {
            signalParams.params.fence.value = timelineValue;
        }
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSignalExternalSemaphoresAsync(
                &cuExternalSemaphore, &signalParams, 1, stream.cuStream);
        checkCUresult(cuResult, "Error in cuSignalExternalSemaphoresAsync: ");
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
        hipExternalSemaphoreSignalParams signalParams{};
        if (isTimelineSemaphore()) {
            signalParams.params.fence.value = timelineValue;
        }
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipSignalExternalSemaphoresAsync(
                &hipExternalSemaphore, &signalParams, 1, stream.hipStream);
        checkHipResult(hipResult, "Error in hipSignalExternalSemaphoresAsync: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        auto zeExternalSemaphore = reinterpret_cast<ze_external_semaphore_ext_handle_t>(externalSemaphore);
        ze_external_semaphore_signal_params_ext_t externalSemaphoreSignalParamsExt{};
        externalSemaphoreSignalParamsExt.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS_EXT;
        externalSemaphoreSignalParamsExt.value = uint64_t(timelineValue);
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendSignalExternalSemaphoreExt(
                stream.zeCommandList, 1, &zeExternalSemaphore, &externalSemaphoreSignalParamsExt,
                g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
        checkZeResult(zeResult, "Error in zeCommandListAppendSignalExternalSemaphoreExt: ");
#endif
    }
}

void SemaphoreVkComputeApiInterop::waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
        CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS waitParams{};
        if (isTimelineSemaphore()) {
            waitParams.params.fence.value = timelineValue;
        }
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuWaitExternalSemaphoresAsync(
                &cuExternalSemaphore, &waitParams, 1, stream.cuStream);
        checkCUresult(cuResult, "Error in cuWaitExternalSemaphoresAsync: ");
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
        hipExternalSemaphoreWaitParams waitParams{};
        if (isTimelineSemaphore()) {
            waitParams.params.fence.value = timelineValue;
        }
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipWaitExternalSemaphoresAsync(
                &hipExternalSemaphore, &waitParams, 1, stream.hipStream);
        checkHipResult(hipResult, "Error in hipWaitExternalSemaphoresAsync: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        auto zeExternalSemaphore = reinterpret_cast<ze_external_semaphore_ext_handle_t>(externalSemaphore);
        ze_external_semaphore_wait_params_ext_t externalSemaphoreWaitParamsExt{};
        externalSemaphoreWaitParamsExt.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WAIT_PARAMS_EXT;
        externalSemaphoreWaitParamsExt.value = uint64_t(timelineValue);
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendWaitExternalSemaphoreExt(
                stream.zeCommandList, 1, &zeExternalSemaphore, &externalSemaphoreWaitParamsExt,
                g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
        checkZeResult(zeResult, "Error in zeCommandListAppendWaitExternalSemaphoreExt: ");
#endif
    }
}


BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk(vk::BufferPtr& vulkanBuffer)
        : vulkanBuffer(vulkanBuffer) {
    VkDevice device = vulkanBuffer->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanBuffer->getVkDeviceMemory();

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device, vulkanBuffer->getVkBuffer(), &memoryRequirements);

    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    externalMemoryHandleDesc.size = vulkanBuffer->getDeviceMemorySize(); // memoryRequirements.size
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipExternalMemoryHandleDesc externalMemoryHandleDescHip{};
    externalMemoryHandleDescHip.size = vulkanBuffer->getDeviceMemorySize(); // memoryRequirements.size
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_device_mem_alloc_desc_t deviceMemAllocDesc{};
    deviceMemAllocDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    //deviceMemAllocDesc.ordinal; // TODO: Necessary?
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
    if (useLevelZero) {
        if (!g_zeDevice || !g_zeContext) {
            sgl::Logfile::get()->throwError(
                    "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                    "Level Zero is initialized, but the global device or context object are not set.");
        }
    }
#endif
    if (int(useCuda) + int(useHip) + int(useLevelZero) != 1) {
        sgl::Logfile::get()->throwError(
                "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                "Only one out of CUDA, HIP and Level Zero interop can be initialized at a time.");
    }


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

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
        externalMemoryHandleDesc.handle.win32.handle = (void*)handle;
#endif
    }

    if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeOpaqueWin32;
        externalMemoryHandleDescHip.handle.win32.handle = (void*)handle;
#endif
    }

    ze_external_memory_import_win32_handle_t externalMemoryImportWin32Handle{};
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalMemoryImportWin32Handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
        deviceMemAllocDesc.pNext = &externalMemoryImportWin32Handle;
        externalMemoryImportWin32Handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32;
        externalMemoryImportWin32Handle.handle = (void*)handle;
#endif
    }

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

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
        externalMemoryHandleDesc.handle.fd = fileDescriptor;
#endif
    }

    if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeOpaqueFd;
        externalMemoryHandleDescHip.handle.fd = fileDescriptor;
#endif
    }

    ze_external_memory_import_fd_t externalMemoryImportFd{};
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalMemoryImportFd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
        deviceMemAllocDesc.pNext = &externalMemoryImportFd;
        externalMemoryImportFd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD;
        externalMemoryImportFd.fd = fileDescriptor;
#endif
    }

    this->fileDescriptor = fileDescriptor;

#else // defined(__linux__)

    Logfile::get()->throwError(
            "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
            "External memory is only supported on Linux, Android and Windows systems!");

#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        CUexternalMemory cudaExternalMemoryBuffer{};
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
                &cudaExternalMemoryBuffer, &externalMemoryHandleDesc);
        checkCUresult(cuResult, "Error in cuImportExternalMemory: ");
        externalMemoryBuffer = reinterpret_cast<void*>(cudaExternalMemoryBuffer);

        CUdeviceptr cudaDevicePtr{};
        CUDA_EXTERNAL_MEMORY_BUFFER_DESC externalMemoryBufferDesc{};
        externalMemoryBufferDesc.offset = vulkanBuffer->getDeviceMemoryOffset();
        externalMemoryBufferDesc.size = memoryRequirements.size;
        externalMemoryBufferDesc.flags = 0;
        cuResult = g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer(
                &cudaDevicePtr, cudaExternalMemoryBuffer, &externalMemoryBufferDesc);
        checkCUresult(cuResult, "Error in cudaExternalMemoryGetMappedBuffer: ");
        devicePtr = reinterpret_cast<void*>(cudaDevicePtr);
#endif
    }

    if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        hipExternalMemory_t hipExternalMemory{};
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipImportExternalMemory(
                &hipExternalMemory, &externalMemoryHandleDescHip);
        checkHipResult(hipResult, "Error in hipImportExternalMemory: ");
        externalMemoryBuffer = reinterpret_cast<void*>(hipExternalMemory);

        hipDeviceptr_t hipDevicePtr{};
        hipExternalMemoryBufferDesc externalMemoryBufferDesc{};
        externalMemoryBufferDesc.offset = vulkanBuffer->getDeviceMemoryOffset();
        externalMemoryBufferDesc.size = memoryRequirements.size;
        externalMemoryBufferDesc.flags = 0;
        hipResult = g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedBuffer(
                &hipDevicePtr, hipExternalMemory, &externalMemoryBufferDesc);
        checkHipResult(hipResult, "Error in hipExternalMemoryGetMappedBuffer: ");
        devicePtr = reinterpret_cast<void*>(hipDevicePtr);
#endif
    }

    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        ze_result_t zeResult = g_levelZeroFunctionTable.zeMemAllocDevice(
                g_zeContext, &deviceMemAllocDesc, memoryRequirements.size, 0, g_zeDevice, &devicePtr);
        checkZeResult(zeResult, "Error in zeMemAllocDevice: ");
#endif
    }

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

BufferComputeApiExternalMemoryVk::~BufferComputeApiExternalMemoryVk() {
#ifdef _WIN32
    CloseHandle(handle);
#elif defined(__linux__)
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif

    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        CUdeviceptr cudaDevicePtr = getCudaDevicePtr();
        auto cudaExternalMemoryBuffer = reinterpret_cast<CUexternalMemory>(externalMemoryBuffer);
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemFree(cudaDevicePtr);
        checkCUresult(cuResult, "Error in cuMemFree: ");
        cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory(cudaExternalMemoryBuffer);
        checkCUresult(cuResult, "Error in cuDestroyExternalMemory: ");
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        hipDeviceptr_t hipDevicePtr = getHipDevicePtr();
        auto hipExternalMemory = reinterpret_cast<hipExternalMemory_t>(externalMemoryBuffer);
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipFree(hipDevicePtr);
        checkHipResult(hipResult, "Error in hipFree: ");
        hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalMemory(hipExternalMemory);
        checkHipResult(hipResult, "Error in hipDestroyExternalMemory: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        ze_result_t zeResult = g_levelZeroFunctionTable.zeMemFree(g_zeContext, devicePtr);
        checkZeResult(zeResult, "Error in zeMemFree: ");
#endif
    }
}

void BufferComputeApiExternalMemoryVk::copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream) {
    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyAsync(
                this->getCudaDevicePtr(), reinterpret_cast<CUdeviceptr>(devicePtrSrc),
                vulkanBuffer->getSizeInBytes(), stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpyAsync: ");
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyAsync(
                this->getHipDevicePtr(), reinterpret_cast<hipDeviceptr_t>(devicePtrSrc),
                vulkanBuffer->getSizeInBytes(), stream.hipStream);
        checkHipResult(hipResult, "Error in cuMemcpyAsync: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
                stream.zeCommandList, devicePtr, devicePtrSrc, vulkanBuffer->getSizeInBytes(),
                g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
        checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
#endif
    }
}

void BufferComputeApiExternalMemoryVk::copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream) {
    bool useCuda = false, useHip = false, useLevelZero = false;
#ifdef SUPPORT_CUDA_INTEROP
    useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_HIP_INTEROP
    useHip = getIsHipDeviceApiFunctionTableInitialized();
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    useLevelZero = getIsLevelZeroFunctionTableInitialized();
#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyAsync(
                reinterpret_cast<CUdeviceptr>(devicePtrDst), this->getCudaDevicePtr(),
                vulkanBuffer->getSizeInBytes(), stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpyAsync: ");
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyAsync(
                reinterpret_cast<hipDeviceptr_t>(devicePtrDst), this->getHipDevicePtr(),
                vulkanBuffer->getSizeInBytes(), stream.hipStream);
        checkHipResult(hipResult, "Error in cuMemcpyAsync: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
                stream.zeCommandList, devicePtrDst, devicePtr, vulkanBuffer->getSizeInBytes(),
                g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
        checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
#endif
    }
}

}}
