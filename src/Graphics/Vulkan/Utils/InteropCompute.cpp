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

#ifdef SUPPORT_SYCL_INTEROP
static sycl::queue* g_syclQueue = nullptr;
void setGlobalSyclQueue(sycl::queue& syclQueue) {
    g_syclQueue = &syclQueue;
}

struct SyclExternalSemaphoreWrapper {
    sycl::ext::oneapi::experimental::external_semaphore syclExternalSemaphore;
};
struct SyclExternalMemWrapper {
    sycl::ext::oneapi::experimental::external_mem syclExternalMem;
};
struct SyclImageMemHandleWrapper {
    sycl::ext::oneapi::experimental::image_descriptor syclImageDescriptor;
    sycl::ext::oneapi::experimental::image_mem_handle syclImageMemHandle;
};
#endif


#ifdef SUPPORT_CUDA_INTEROP
#define CHECK_USE_CUDA useCuda = getIsCudaDeviceApiFunctionTableInitialized();
#else
#define CHECK_USE_CUDA
#endif
#ifdef SUPPORT_HIP_INTEROP
#define CHECK_USE_HIP useHip = getIsHipDeviceApiFunctionTableInitialized();
#else
#define CHECK_USE_HIP
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
#define CHECK_USE_LEVEL_ZERO useLevelZero = getIsLevelZeroFunctionTableInitialized();
#else
#define CHECK_USE_LEVEL_ZERO
#endif
#ifdef SUPPORT_SYCL_INTEROP
#define CHECK_USE_SYCL useSycl = g_syclQueue != nullptr;
#else
#define CHECK_USE_SYCL
#endif

#define CHECK_COMPUTE_API_SUPPORT \
bool useCuda = false, useHip = false, useLevelZero = false, useSycl = false; \
CHECK_USE_CUDA \
CHECK_USE_HIP \
CHECK_USE_LEVEL_ZERO \
CHECK_USE_SYCL


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


    CHECK_COMPUTE_API_SUPPORT;

#ifdef SUPPORT_CUDA_INTEROP
    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC externalSemaphoreHandleDesc{};
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipExternalSemaphoreHandleDesc externalSemaphoreHandleDescHip{};
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_semaphore_ext_desc_t externalSemaphoreExtDesc{};
    externalSemaphoreExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC;
    if (useLevelZero) {
        if (!g_zeDevice) {
            sgl::Logfile::get()->throwError(
                    "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                    "Level Zero is initialized, but the global device object is not set.");
        }
    }
#endif
    int numComputeApis = int(useCuda) + int(useHip) + int(useLevelZero) + int(useSycl);
    if (numComputeApis > 1) {
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "Only one out of CUDA, HIP, Level Zero and SYCL interop can be initialized at a time.");
    } else if (numComputeApis < 1) {
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "No interop API out of CUDA, HIP, Level Zero or SYCL is initialized.");
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

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_semaphore_win32_ext_desc_t externalSemaphoreWin32ExtDesc{};
#endif
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

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
        sycl::ext::oneapi::experimental::external_semaphore_handle_type semaphoreHandleType;
        if (isTimelineSemaphore()) {
#ifndef SYCL_NO_EXTERNAL_TIMELINE_SEMAPHORE_SUPPORT
            semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::timeline_win32_nt_handle;
#else
            sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "The installed version of SYCL does not support external timeline semaphores.");
#endif
        } else {
            semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::win32_nt_handle;
        }
        sycl::ext::oneapi::experimental::external_semaphore_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalSemaphoreDescriptor{handle, semaphoreHandleType};
        auto* wrapper = new SyclExternalSemaphoreWrapper;
        wrapper->syclExternalSemaphore = sycl::ext::oneapi::experimental::import_external_semaphore(
            syclExternalSemaphoreDescriptor, *g_syclQueue);
        externalSemaphore = reinterpret_cast<void*>(wrapper);
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

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_semaphore_fd_ext_desc_t externalSemaphoreFdExtDesc{};
#endif
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

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
        sycl::ext::oneapi::experimental::external_semaphore_handle_type semaphoreHandleType;
        if (isTimelineSemaphore()) {
#ifndef SYCL_NO_EXTERNAL_TIMELINE_SEMAPHORE_SUPPORT
            semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::timeline_fd;
#else
            sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "The installed version of SYCL does not support external timeline semaphores.");
#endif
        } else {
            semaphoreHandleType = sycl::ext::oneapi::experimental::external_semaphore_handle_type::opaque_fd;
        }
        sycl::ext::oneapi::experimental::external_semaphore_descriptor<sycl::ext::oneapi::experimental::resource_fd>
            syclExternalSemaphoreDescriptor{fileDescriptor, semaphoreHandleType};
        auto* wrapper = new SyclExternalSemaphoreWrapper;
        wrapper->syclExternalSemaphore = sycl::ext::oneapi::experimental::import_external_semaphore(
            syclExternalSemaphoreDescriptor, *g_syclQueue);
        externalSemaphore = reinterpret_cast<void*>(wrapper);
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
    CHECK_COMPUTE_API_SUPPORT;

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
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
        sycl::ext::oneapi::experimental::release_external_semaphore(wrapper->syclExternalSemaphore, *g_syclQueue);
        delete wrapper;
#endif
    }
}

void SemaphoreVkComputeApiInterop::signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
    CHECK_COMPUTE_API_SUPPORT;

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
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
        stream.syclQueuePtr->ext_oneapi_signal_external_semaphore(
            wrapper->syclExternalSemaphore, uint64_t(timelineValue));
#endif
    }
}

void SemaphoreVkComputeApiInterop::waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
    CHECK_COMPUTE_API_SUPPORT;

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
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
        stream.syclQueuePtr->ext_oneapi_wait_external_semaphore(
            wrapper->syclExternalSemaphore, uint64_t(timelineValue));
#endif
    }
}


BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk(vk::BufferPtr& vulkanBuffer)
        : vulkanBuffer(vulkanBuffer) {
    VkDevice device = vulkanBuffer->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanBuffer->getVkDeviceMemory();

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device, vulkanBuffer->getVkBuffer(), &memoryRequirements);

    CHECK_COMPUTE_API_SUPPORT;

#ifdef SUPPORT_CUDA_INTEROP
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    externalMemoryHandleDesc.size = vulkanBuffer->getDeviceMemorySize(); // memoryRequirements.size
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipExternalMemoryHandleDesc externalMemoryHandleDescHip{};
    externalMemoryHandleDescHip.size = vulkanBuffer->getDeviceMemorySize(); // memoryRequirements.size
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_device_mem_alloc_desc_t deviceMemAllocDesc{};
    deviceMemAllocDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    //deviceMemAllocDesc.ordinal; // TODO: Necessary?
    if (useLevelZero) {
        if (!g_zeDevice || !g_zeContext) {
            sgl::Logfile::get()->throwError(
                    "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                    "Level Zero is initialized, but the global device or context object are not set.");
        }
    }
#endif
    int numComputeApis = int(useCuda) + int(useHip) + int(useLevelZero) + int(useSycl);
    if (numComputeApis > 1) {
        sgl::Logfile::get()->throwError(
                "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                "Only one out of CUDA, HIP, Level Zero and SYCL interop can be initialized at a time.");
    } else if (numComputeApis < 1) {
        sgl::Logfile::get()->throwError(
                "Error in BufferComputeApiExternalMemoryVk::BufferComputeApiExternalMemoryVk: "
                "No interop API out of CUDA, HIP, Level Zero or SYCL is initialized.");
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

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_memory_import_win32_handle_t externalMemoryImportWin32Handle{};
#endif
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalMemoryImportWin32Handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
        deviceMemAllocDesc.pNext = &externalMemoryImportWin32Handle;
        externalMemoryImportWin32Handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32;
        externalMemoryImportWin32Handle.handle = (void*)handle;
#endif
    }

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
        auto memoryHandleType = sycl::ext::oneapi::experimental::external_mem_handle_type::win32_nt_handle;
        sycl::ext::oneapi::experimental::external_mem_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalMemDescriptor{(void*)handle, memoryHandleType, vulkanBuffer->getDeviceMemorySize()}; // memoryRequirements.size;
        auto* wrapper = new SyclExternalMemWrapper;
        wrapper->syclExternalMem = sycl::ext::oneapi::experimental::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
        externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
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

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_memory_import_fd_t externalMemoryImportFd{};
#endif
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalMemoryImportFd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
        deviceMemAllocDesc.pNext = &externalMemoryImportFd;
        externalMemoryImportFd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD;
        externalMemoryImportFd.fd = fileDescriptor;
#endif
    }

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
        auto memoryHandleType = sycl::ext::oneapi::experimental::external_mem_handle_type::opaque_fd;
        sycl::ext::oneapi::experimental::external_mem_descriptor<sycl::ext::oneapi::experimental::resource_fd>
            syclExternalMemDescriptor{fileDescriptor, memoryHandleType, vulkanBuffer->getDeviceMemorySize()}; // memoryRequirements.size;
        auto* wrapper = new SyclExternalMemWrapper;
        wrapper->syclExternalMem = sycl::ext::oneapi::experimental::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
        externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
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

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapper = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
        devicePtr = sycl::ext::oneapi::experimental::map_external_linear_memory(
            wrapper->syclExternalMem, 0, vulkanBuffer->getDeviceMemorySize(), *g_syclQueue);
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

    CHECK_COMPUTE_API_SUPPORT;

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
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapper = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
        sycl::ext::oneapi::experimental::unmap_external_linear_memory(devicePtr, *g_syclQueue);
        sycl::ext::oneapi::experimental::release_external_memory(wrapper->syclExternalMem, *g_syclQueue);
        delete wrapper;
#endif
    }
}

void BufferComputeApiExternalMemoryVk::copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream) {
    CHECK_COMPUTE_API_SUPPORT;

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
        checkHipResult(hipResult, "Error in hipMemcpyAsync: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
                stream.zeCommandList, devicePtr, devicePtrSrc, vulkanBuffer->getSizeInBytes(),
                g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
        checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
#endif
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        stream.syclQueuePtr->memcpy(devicePtr, devicePtrSrc, vulkanBuffer->getSizeInBytes());
#endif
    }
}

void BufferComputeApiExternalMemoryVk::copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream) {
    CHECK_COMPUTE_API_SUPPORT;

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
        checkHipResult(hipResult, "Error in hipMemcpyAsync: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
                stream.zeCommandList, devicePtrDst, devicePtr, vulkanBuffer->getSizeInBytes(),
                g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
        checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
#endif
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        stream.syclQueuePtr->memcpy(devicePtrDst, devicePtr, vulkanBuffer->getSizeInBytes());
#endif
    }
}


#ifdef SUPPORT_LEVEL_ZERO_INTEROP
static void getZeImageFormatFromVkFormat(VkFormat vkFormat, ze_image_format_t& zeFormat) {
    switch (vkFormat) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_UINT;
        break;
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_SINT;
        break;
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_UNORM;
        break;
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_SNORM;
        break;
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16B16_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32B32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_FLOAT;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported type.");
        return;
    }
    switch (vkFormat) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_S8_UINT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_8;
        break;
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8;
        break;
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_B8G8R8_SNORM:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8;
        break;
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
        break;
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_D16_UNORM:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_16;
        break;
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16_SFLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_16_16;
        break;
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_SFLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_16_16_16;
        break;
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16;
        break;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32;
        break;
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32;
        break;
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32_32;
        break;
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported layout.");
        return;
    }
    switch (vkFormat) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16B16_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32B32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
        zeFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
        zeFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
        zeFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
        break;
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
        zeFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_B;
        zeFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
        zeFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_R;
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
        break;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        zeFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_A;
        zeFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_B;
        zeFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_G;
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_R;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported swizzle.");
        return;
    }
    size_t numChannels = getImageFormatNumChannels(vkFormat);
    // TODO: Check if this is what we expect.
    if (numChannels == 3) {
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
    } else if (numChannels == 2) {
        zeFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
    } else if (numChannels == 1) {
        zeFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
        zeFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_0;
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
    }
}
#endif

#ifdef SUPPORT_HIP_INTEROP
static void getHipFormatDescFromVkFormat(VkFormat vkFormat, hipChannelFormatDesc& hipFormat) {
    switch (vkFormat) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
        hipFormat.f = hipChannelFormatKindUnsigned;
        break;
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
        hipFormat.f = hipChannelFormatKindSigned;
        break;
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
        hipFormat.f = hipChannelFormatKindUnsigned;
        break;
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
        hipFormat.f = hipChannelFormatKindSigned;
        break;
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16B16_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32B32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
        hipFormat.f = hipChannelFormatKindFloat;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported channel format.");
        return;
    }

    hipFormat.x = hipFormat.y = hipFormat.z = hipFormat.w;
    switch (vkFormat) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_S8_UINT:
        hipFormat.x = 8;
        break;
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
        hipFormat.x = 8;
        hipFormat.y = 8;
        break;
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_B8G8R8_SNORM:
        hipFormat.x = 8;
        hipFormat.y = 8;
        hipFormat.z = 8;
        break;
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        hipFormat.x = 8;
        hipFormat.y = 8;
        hipFormat.z = 8;
        hipFormat.w = 8;
        break;
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_D16_UNORM:
        hipFormat.x = 16;
        break;
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16_SFLOAT:
        hipFormat.x = 16;
        hipFormat.y = 16;
        break;
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_SFLOAT:
        hipFormat.x = 16;
        hipFormat.y = 16;
        hipFormat.z = 16;
        break;
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        hipFormat.x = 16;
        hipFormat.y = 16;
        hipFormat.z = 16;
        hipFormat.w = 16;
        break;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
        hipFormat.x = 32;
        break;
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
        hipFormat.x = 32;
        hipFormat.y = 32;
        break;
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
        hipFormat.x = 32;
        hipFormat.y = 32;
        hipFormat.z = 32;
        break;
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        hipFormat.x = 32;
        hipFormat.y = 32;
        hipFormat.z = 32;
        hipFormat.w = 32;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported channels.");
        return;
    }
}
#endif


ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk(vk::ImagePtr& vulkanImage) {
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
    _initialize(vulkanImage, imageViewType, surfaceLoadStore);
}

ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk(
        vk::ImagePtr& vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore) {
    _initialize(vulkanImage, imageViewType, surfaceLoadStore);
}

void ImageComputeApiExternalMemoryVk::_initialize(
        vk::ImagePtr& _vulkanImage, VkImageViewType _imageViewType, bool surfaceLoadStore) {
    vulkanImage = _vulkanImage;
    imageViewType = _imageViewType;
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();

    VkDevice device = vulkanImage->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanImage->getVkDeviceMemory();

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device, vulkanImage->getVkImage(), &memoryRequirements);

    CHECK_COMPUTE_API_SUPPORT;

#ifdef SUPPORT_CUDA_INTEROP
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    externalMemoryHandleDesc.size = vulkanImage->getDeviceMemorySize(); // memoryRequirements.size
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipExternalMemoryHandleDesc externalMemoryHandleDescHip{};
    externalMemoryHandleDescHip.size = vulkanImage->getDeviceMemorySize(); // memoryRequirements.size
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_image_desc_t zeImageDesc{};
    zeImageDesc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    //deviceMemAllocDesc.ordinal; // TODO: Necessary?
    if (useLevelZero) {
        if (!g_zeDevice || !g_zeContext) {
            sgl::Logfile::get()->throwError(
                    "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
                    "Level Zero is initialized, but the global device or context object are not set.");
        }
    }
#endif
    int numComputeApis = int(useCuda) + int(useHip) + int(useLevelZero) + int(useSycl);
    if (numComputeApis > 1) {
        sgl::Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
                "Only one out of CUDA, HIP, Level Zero and SYCL interop can be initialized at a time.");
    } else if (numComputeApis < 1) {
        sgl::Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
                "No interop API out of CUDA, HIP, Level Zero or SYCL is initialized.");
    }


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

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_memory_import_win32_handle_t externalMemoryImportWin32Handle{};
#endif
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalMemoryImportWin32Handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
        zeImageDesc.pNext = &externalMemoryImportWin32Handle;
        externalMemoryImportWin32Handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32;
        externalMemoryImportWin32Handle.handle = (void*)handle;
#endif
    }

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
        auto memoryHandleType = sycl::ext::oneapi::experimental::external_mem_handle_type::win32_nt_handle;
        sycl::ext::oneapi::experimental::external_mem_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalMemDescriptor{(void*)handle, memoryHandleType, vulkanImage->getDeviceMemorySize()}; // memoryRequirements.size;
        auto* wrapper = new SyclExternalMemWrapper;
        wrapper->syclExternalMem = sycl::ext::oneapi::experimental::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
        externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
#endif
    }

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

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_external_memory_import_fd_t externalMemoryImportFd{};
#endif
    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        externalMemoryImportFd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
        zeImageDesc.pNext = &externalMemoryImportFd;
        externalMemoryImportFd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD;
        externalMemoryImportFd.fd = fileDescriptor;
#endif
    }

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
        auto memoryHandleType = sycl::ext::oneapi::experimental::external_mem_handle_type::opaque_fd;
        sycl::ext::oneapi::experimental::external_mem_descriptor<sycl::ext::oneapi::experimental::resource_fd>
            syclExternalMemDescriptor{fileDescriptor, memoryHandleType, vulkanImage->getDeviceMemorySize()}; // memoryRequirements.size;
        auto* wrapper = new SyclExternalMemWrapper;
        wrapper->syclExternalMem = sycl::ext::oneapi::experimental::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
        externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
#endif
    }

    this->fileDescriptor = fileDescriptor;

#else // defined(__linux__)

    Logfile::get()->throwError(
            "Error in ImageComputeApiExternalMemoryVk::ImageComputeApiExternalMemoryVk: "
            "External memory is only supported on Linux, Android and Windows systems!");

#endif

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        CUexternalMemory cudaExternalMemoryBuffer{};
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
                &cudaExternalMemoryBuffer, &externalMemoryHandleDesc);
        checkCUresult(cuResult, "Error in cuImportExternalMemory: ");
        externalMemoryBuffer = reinterpret_cast<void*>(cudaExternalMemoryBuffer);

        CUDA_ARRAY3D_DESCRIPTOR arrayDescriptor{};
        arrayDescriptor.Width = imageSettings.width;
        if (imageViewType == VK_IMAGE_VIEW_TYPE_2D || imageViewType == VK_IMAGE_VIEW_TYPE_3D
                || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
                || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            arrayDescriptor.Height = imageSettings.height;
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            arrayDescriptor.Depth = imageSettings.depth;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
                || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            arrayDescriptor.Depth = imageSettings.arrayLayers;
        }
        arrayDescriptor.Format = getCudaArrayFormatFromVkFormat(imageSettings.format);
        arrayDescriptor.NumChannels = uint32_t(getImageFormatNumChannels(imageSettings.format));
        if (imageSettings.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            arrayDescriptor.Flags |= CUDA_ARRAY3D_COLOR_ATTACHMENT;
        }
        if (surfaceLoadStore) {
            arrayDescriptor.Flags |= CUDA_ARRAY3D_SURFACE_LDST;
        }
        if (isDepthStencilFormat(imageSettings.format)) {
            arrayDescriptor.Flags |= CUDA_ARRAY3D_DEPTH_TEXTURE;
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            arrayDescriptor.Flags |= CUDA_ARRAY3D_CUBEMAP;
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
                || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            arrayDescriptor.Flags |= CUDA_ARRAY3D_LAYERED;
        }

        CUmipmappedArray cudaMipmappedArray{};
        CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC externalMemoryMipmappedArrayDesc{};
        externalMemoryMipmappedArrayDesc.offset = vulkanImage->getDeviceMemoryOffset();
        externalMemoryMipmappedArrayDesc.numLevels = imageSettings.mipLevels;
        externalMemoryMipmappedArrayDesc.arrayDesc = arrayDescriptor;
        cuResult = g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedMipmappedArray(
                &cudaMipmappedArray, cudaExternalMemoryBuffer, &externalMemoryMipmappedArrayDesc);
        checkCUresult(cuResult, "Error in cuExternalMemoryGetMappedMipmappedArray: ");
        mipmappedArray = reinterpret_cast<void*>(cudaMipmappedArray);
#endif
    }

    if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        hipExternalMemory_t hipExternalMemory{};
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipImportExternalMemory(
                &hipExternalMemory, &externalMemoryHandleDescHip);
        checkHipResult(hipResult, "Error in hipImportExternalMemory: ");
        externalMemoryBuffer = reinterpret_cast<void*>(hipExternalMemory);

        hipExternalMemoryMipmappedArrayDesc externalMemoryMipmappedArrayDesc{};
        externalMemoryMipmappedArrayDesc.extent.width = imageSettings.width;
        if (imageViewType == VK_IMAGE_VIEW_TYPE_2D || imageViewType == VK_IMAGE_VIEW_TYPE_3D
                || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
                || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            externalMemoryMipmappedArrayDesc.extent.height = imageSettings.height;
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            externalMemoryMipmappedArrayDesc.extent.depth = imageSettings.depth;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
                || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            externalMemoryMipmappedArrayDesc.extent.depth = imageSettings.arrayLayers;
        }
        externalMemoryMipmappedArrayDesc.offset = vulkanImage->getDeviceMemoryOffset();
        externalMemoryMipmappedArrayDesc.numLevels = imageSettings.mipLevels;
        getHipFormatDescFromVkFormat(imageSettings.format, externalMemoryMipmappedArrayDesc.formatDesc);
        externalMemoryMipmappedArrayDesc.flags = 0;

        hipMipmappedArray_t hipMipmappedArray{};
        hipResult = g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedMipmappedArray(
                &hipMipmappedArray, hipExternalMemory, &externalMemoryMipmappedArrayDesc);
        checkHipResult(hipResult, "Error in hipImportExternalMemory: ");
        mipmappedArray = reinterpret_cast<void*>(hipMipmappedArray);
#endif
    }

    if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        zeImageDesc.width = imageSettings.width;
        if (imageViewType == VK_IMAGE_VIEW_TYPE_2D || imageViewType == VK_IMAGE_VIEW_TYPE_3D
                || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
            zeImageDesc.height = imageSettings.height;
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            zeImageDesc.depth = imageSettings.depth;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
            zeImageDesc.arraylevels = imageSettings.arrayLayers;
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_1D) {
            zeImageDesc.type = ZE_IMAGE_TYPE_1D;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY) {
            zeImageDesc.type = ZE_IMAGE_TYPE_1DARRAY;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_2D) {
            zeImageDesc.type = ZE_IMAGE_TYPE_2D;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
            zeImageDesc.type = ZE_IMAGE_TYPE_2DARRAY;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            zeImageDesc.type = ZE_IMAGE_TYPE_3D;
        }
        getZeImageFormatFromVkFormat(imageSettings.format, zeImageDesc.format);
        if (surfaceLoadStore) {
            zeImageDesc.flags |= ZE_IMAGE_FLAG_KERNEL_WRITE;
        }
        // ZE_IMAGE_FLAG_BIAS_UNCACHED currently unused here.

        ze_image_handle_t imageHandle{};
        ze_result_t zeResult = g_levelZeroFunctionTable.zeImageCreate(
                g_zeContext, g_zeDevice, &zeImageDesc, &imageHandle);
        checkZeResult(zeResult, "Error in zeMemAllocDevice: ");
        mipmappedArray = reinterpret_cast<void*>(imageHandle);
#endif
    }

    if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapperImg = new SyclImageMemHandleWrapper;
        sycl::ext::oneapi::experimental::image_descriptor& syclImageDescriptor = wrapperImg->syclImageDescriptor;
        syclImageDescriptor.width = imageSettings.width;
        if (imageViewType == VK_IMAGE_VIEW_TYPE_2D || imageViewType == VK_IMAGE_VIEW_TYPE_3D
                || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
            syclImageDescriptor.height = imageSettings.height;
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            syclImageDescriptor.depth = imageSettings.depth;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
            syclImageDescriptor.array_size = imageSettings.arrayLayers;
        }
        syclImageDescriptor.num_levels = imageSettings.mipLevels;

        syclImageDescriptor.num_channels = unsigned(getImageFormatNumChannels(imageSettings.format));
        if (imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
            syclImageDescriptor.type = sycl::ext::oneapi::experimental::image_type::array;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE) {
            syclImageDescriptor.type = sycl::ext::oneapi::experimental::image_type::cubemap;
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_1D || imageViewType == VK_IMAGE_VIEW_TYPE_2D
                || imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            if (syclImageDescriptor.num_levels > 1) {
                syclImageDescriptor.type = sycl::ext::oneapi::experimental::image_type::mipmap;
            } else {
                syclImageDescriptor.type = sycl::ext::oneapi::experimental::image_type::standard;
            }
        } else {
            Logfile::get()->throwError(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: "
                    "Unsupported image view type for SYCL.");
        }
        switch (imageSettings.format) {
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_S8_UINT:
            syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int8;
            break;
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16A16_UINT:
            syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int16;
            break;
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32A32_UINT:
            syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int32;
            break;
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int8;
            break;
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16A16_SINT:
            syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int16;
            break;
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32A32_SINT:
            syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int32;
            break;
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            syclImageDescriptor.channel_type = sycl::image_channel_type::unorm_int8;
            break;
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16B16A16_UNORM:
            syclImageDescriptor.channel_type = sycl::image_channel_type::unorm_int16;
            break;
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            syclImageDescriptor.channel_type = sycl::image_channel_type::snorm_int8;
            break;
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
            syclImageDescriptor.channel_type = sycl::image_channel_type::snorm_int16;
            break;
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            syclImageDescriptor.channel_type = sycl::image_channel_type::fp16;
            break;
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT:
            syclImageDescriptor.channel_type = sycl::image_channel_type::fp32;
            break;
        default:
            sgl::Logfile::get()->throwError(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: "
                    "Unsupported channel type for SYCL.");
            return;
        }

        auto* wrapperMem = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
        wrapperImg->syclImageMemHandle = sycl::ext::oneapi::experimental::map_external_image_memory(
            wrapperMem->syclExternalMem, syclImageDescriptor, *g_syclQueue);
        mipmappedArray = reinterpret_cast<void*>(wrapperImg);
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

ImageComputeApiExternalMemoryVk::~ImageComputeApiExternalMemoryVk() {
#ifdef _WIN32
    CloseHandle(handle);
#elif defined(__linux__)
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif

    CHECK_COMPUTE_API_SUPPORT;

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        CUmipmappedArray cudaMipmappedArray = getCudaMipmappedArray();
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMipmappedArrayDestroy(cudaMipmappedArray);
        checkCUresult(cuResult, "Error in cuMipmappedArrayDestroy: ");
        auto cudaExternalMemoryBuffer = reinterpret_cast<CUexternalMemory>(externalMemoryBuffer);
        cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory(cudaExternalMemoryBuffer);
        checkCUresult(cuResult, "Error in cuDestroyExternalMemory: ");
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        hipMipmappedArray_t hipMipmappedArray = getHipMipmappedArray();
        auto hipResult = g_hipDeviceApiFunctionTable.hipMipmappedArrayDestroy(hipMipmappedArray);
        checkHipResult(hipResult, "Error in hipMipmappedArrayDestroy: ");
        auto hipExternalMemory = reinterpret_cast<hipExternalMemory_t>(externalMemoryBuffer);
        hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalMemory(hipExternalMemory);
        checkHipResult(hipResult, "Error in hipDestroyExternalMemory: ");
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        auto imageHandle = reinterpret_cast<ze_image_handle_t>(mipmappedArray);
        ze_result_t zeResult = g_levelZeroFunctionTable.zeImageDestroy(imageHandle);
        checkZeResult(zeResult, "Error in zeImageDestroy: ");
#endif
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapperMem = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
        auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
        sycl::ext::oneapi::experimental::free_image_mem(
                wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor.type, *g_syclQueue);
        sycl::ext::oneapi::experimental::release_external_memory(wrapperMem->syclExternalMem, *g_syclQueue);
        delete wrapperMem;
        delete wrapperImg;
#endif
    }
}

#ifdef SUPPORT_CUDA_INTEROP
CUarray ImageComputeApiExternalMemoryVk::getCudaMipmappedArrayLevel(uint32_t level) {
    if (level == 0 && arrayLevel0) {
        return reinterpret_cast<CUarray>(arrayLevel0);
    }

    CUmipmappedArray cudaMipmappedArray = getCudaMipmappedArray();
    CUarray levelArray;
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMipmappedArrayGetLevel(&levelArray, cudaMipmappedArray, level);
    checkCUresult(cuResult, "Error in cuMipmappedArrayGetLevel: ");

    if (level == 0) {
        arrayLevel0 = reinterpret_cast<void*>(levelArray);
    }

    return levelArray;
}
#endif

#ifdef SUPPORT_HIP_INTEROP
hipArray_t ImageComputeApiExternalMemoryVk::getHipMipmappedArrayLevel(uint32_t level) {
    if (level == 0 && arrayLevel0) {
        return reinterpret_cast<hipArray_t>(arrayLevel0);
    }

    hipMipmappedArray_t hipMipmappedArray = getHipMipmappedArray();
    hipArray_t levelArray;
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMipmappedArrayGetLevel(&levelArray, hipMipmappedArray, level);
    checkHipResult(hipResult, "Error in hipMipmappedArrayGetLevel: ");

    if (level == 0) {
        arrayLevel0 = reinterpret_cast<void*>(levelArray);
    }

    return levelArray;
}
#endif

void ImageComputeApiExternalMemoryVk::copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream) {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();

    CHECK_COMPUTE_API_SUPPORT;

    if (useCuda) {
#ifdef SUPPORT_CUDA_INTEROP
        size_t entryByteSize = getImageFormatEntryByteSize(imageSettings.format);
        if (imageViewType == VK_IMAGE_VIEW_TYPE_2D) {
            CUDA_MEMCPY2D memcpySettings{};
            memcpySettings.srcMemoryType = CU_MEMORYTYPE_DEVICE;
            memcpySettings.srcDevice = reinterpret_cast<CUdeviceptr>(devicePtrSrc);
            memcpySettings.srcPitch = imageSettings.width * entryByteSize;

            memcpySettings.dstMemoryType = CU_MEMORYTYPE_ARRAY;
            memcpySettings.dstArray = getCudaMipmappedArrayLevel(0);

            memcpySettings.WidthInBytes = imageSettings.width * entryByteSize;
            memcpySettings.Height = imageSettings.height;

            CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy2DAsync(&memcpySettings, stream.cuStream);
            checkCUresult(cuResult, "Error in cuMemcpy2DAsync: ");
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            CUDA_MEMCPY3D memcpySettings{};
            memcpySettings.srcMemoryType = CU_MEMORYTYPE_DEVICE;
            memcpySettings.srcDevice = reinterpret_cast<CUdeviceptr>(devicePtrSrc);
            memcpySettings.srcPitch = imageSettings.width * entryByteSize;
            memcpySettings.srcHeight = imageSettings.height;

            memcpySettings.dstMemoryType = CU_MEMORYTYPE_ARRAY;
            memcpySettings.dstArray = getCudaMipmappedArrayLevel(0);

            memcpySettings.WidthInBytes = imageSettings.width * entryByteSize;
            memcpySettings.Height = imageSettings.height;
            memcpySettings.Depth = imageSettings.depth;

            CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy3DAsync(&memcpySettings, stream.cuStream);
            checkCUresult(cuResult, "Error in cuMemcpy3DAsync: ");
        } else {
            Logfile::get()->throwError(
                    "Error in ImageComputeApiExternalMemoryVk::copyFromDevicePtrAsync: "
                    "Unsupported image view type.");
        }
#endif
    } else if (useHip) {
#ifdef SUPPORT_HIP_INTEROP
        size_t entryByteSize = getImageFormatEntryByteSize(imageSettings.format);
        if (imageViewType == VK_IMAGE_VIEW_TYPE_2D) {
            // TODO: AMD seems to have forgotten hipDrvMemcpy2DAsync in the headers...
            hipError_t hipResult = hipErrorNotSupported;
            checkHipResult(hipResult, "Error in hipDrvMemcpy2DAsync: ");
        } else if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
            HIP_MEMCPY3D memcpySettings{};
            memcpySettings.srcMemoryType = hipMemoryTypeDevice;
            memcpySettings.srcDevice = reinterpret_cast<hipDeviceptr_t>(devicePtrSrc);
            memcpySettings.srcPitch = imageSettings.width * entryByteSize;
            memcpySettings.srcHeight = imageSettings.height;

            memcpySettings.dstMemoryType = hipMemoryTypeArray;
            memcpySettings.dstArray = getHipMipmappedArrayLevel(0);

            memcpySettings.WidthInBytes = imageSettings.width * entryByteSize;
            memcpySettings.Height = imageSettings.height;
            memcpySettings.Depth = imageSettings.depth;

            hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDrvMemcpy3DAsync(&memcpySettings, stream.hipStream);
            checkHipResult(hipResult, "Error in hipDrvMemcpy3DAsync: ");
        } else {
            Logfile::get()->throwError(
                    "Error in ImageComputeApiExternalMemoryVk::copyFromDevicePtrAsync: "
                    "Unsupported image view type.");
        }
#endif
    } else if (useLevelZero) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        auto imageHandle = reinterpret_cast<ze_image_handle_t>(mipmappedArray);
        ze_image_region_t dstRegion{};
        dstRegion.originX = 0;
        dstRegion.originY = 0;
        dstRegion.originZ = 0;
        dstRegion.width = imageSettings.width;
        dstRegion.height = imageSettings.height;
        dstRegion.depth = imageSettings.depth;
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendImageCopyFromMemory(
                stream.zeCommandList, imageHandle, devicePtrSrc, &dstRegion,
                g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
        checkZeResult(zeResult, "Error in zeCommandListAppendImageCopyFromMemory: ");
#endif
    } else if (useSycl) {
#ifdef SUPPORT_SYCL_INTEROP
        auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
        stream.syclQueuePtr->ext_oneapi_copy(
            devicePtrSrc, wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor);
#endif
    }
}

}}
