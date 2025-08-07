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

#include "ImplLevelZero.hpp"
#include "../InteropLevelZero.hpp"

#ifdef SUPPORT_SYCL_INTEROP
#include <sycl/sycl.hpp>
#include <sycl/ext/oneapi/backend/level_zero.hpp>
#endif

namespace sgl { namespace vk {

extern bool openMessageBoxOnComputeApiError;

#ifdef SUPPORT_SYCL_INTEROP
extern sycl::queue* g_syclQueue;
#endif

ze_device_handle_t g_zeDevice = {};
ze_context_handle_t g_zeContext = {};
ze_command_queue_handle_t g_zeCommandQueue = {};
ze_event_handle_t g_zeSignalEvent = {};
uint32_t g_numWaitEvents = 0;
ze_event_handle_t* g_zeWaitEvents = {};
bool g_useBindlessImagesInterop = {};

void setLevelZeroGlobalState(ze_device_handle_t zeDevice, ze_context_handle_t zeContext) {
    g_zeDevice = zeDevice;
    g_zeContext = zeContext;
}
void setLevelZeroGlobalCommandQueue(ze_command_queue_handle_t zeCommandQueue) {
    g_zeCommandQueue = zeCommandQueue;
}
void setLevelZeroNextCommandEvents(
        ze_event_handle_t zeSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* zeWaitEvents) {
    g_zeSignalEvent = zeSignalEvent;
    g_numWaitEvents = numWaitEvents;
    g_zeWaitEvents = zeWaitEvents;
}
void setLevelZeroUseBindlessImagesInterop(bool useBindlessImages) {
    g_useBindlessImagesInterop = useBindlessImages;
}
#ifdef SUPPORT_SYCL_INTEROP
void setLevelZeroGlobalStateFromSyclQueue(sycl::queue& syclQueue) {
#ifdef SUPPORT_SYCL_INTEROP
    // Reset, as static variables may persist across GoogleTest unit tests.
    g_syclQueue = {};
#endif
    g_zeDevice = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue.get_device());
    g_zeContext = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue.get_context());
}
#endif

SemaphoreVkLevelZeroInterop::SemaphoreVkLevelZeroInterop() {
    externalSemaphoreExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC;
    if (!g_zeDevice) {
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "Level Zero is initialized, but the global device object is not set.");
    }
}

#ifdef _WIN32
void SemaphoreVkLevelZeroInterop::setExternalSemaphoreWin32Handle(HANDLE handle) {
    externalSemaphoreWin32ExtDesc = {};
    externalSemaphoreWin32ExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC;
    externalSemaphoreExtDesc.pNext = &externalSemaphoreWin32ExtDesc;
    if (isTimelineSemaphore()) {
        externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_VK_TIMELINE_SEMAPHORE_WIN32;
    } else {
        externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_OPAQUE_WIN32;
    }
    externalSemaphoreWin32ExtDesc.handle = handle;
}
#endif

#ifdef __linux__
void SemaphoreVkLevelZeroInterop::setExternalSemaphoreFd(int fileDescriptor) {
    externalSemaphoreFdExtDesc = {};
    externalSemaphoreFdExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_FD_EXT_DESC;
    externalSemaphoreExtDesc.pNext = &externalSemaphoreFdExtDesc;
    if (isTimelineSemaphore()) {
        externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_VK_TIMELINE_SEMAPHORE_FD;
    } else {
        externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_OPAQUE_FD;
    }
    externalSemaphoreFdExtDesc.fd = fileDescriptor;
}
#endif

void SemaphoreVkLevelZeroInterop::importExternalSemaphore() {
    ze_external_semaphore_ext_handle_t zeExternalSemaphore{};
    ze_result_t zeResult = g_levelZeroFunctionTable.zeDeviceImportExternalSemaphoreExt(
            g_zeDevice, &externalSemaphoreExtDesc, &zeExternalSemaphore);
    // The Linux driver seem to return ZE_RESULT_ERROR_UNINITIALIZED when the feature is not supported.
    if (zeResult == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE || zeResult == ZE_RESULT_ERROR_UNINITIALIZED) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: "
                    "Unsupported Level Zero external semaphore type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: "
                    "Unsupported Level Zero external semaphore type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported Level Zero external semaphore type");
    } else {
        checkZeResult(zeResult, "Error in zeDeviceImportExternalSemaphoreExt: ");
    }
    externalSemaphore = reinterpret_cast<void*>(zeExternalSemaphore);
}

SemaphoreVkLevelZeroInterop::~SemaphoreVkLevelZeroInterop() {
    if (externalSemaphore) {
        auto zeExternalSemaphore = reinterpret_cast<ze_external_semaphore_ext_handle_t>(externalSemaphore);
        ze_result_t zeResult = g_levelZeroFunctionTable.zeDeviceReleaseExternalSemaphoreExt(zeExternalSemaphore);
        checkZeResult(zeResult, "Error in zeDeviceReleaseExternalSemaphoreExt: ");
    }
}

void SemaphoreVkLevelZeroInterop::signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
    auto zeExternalSemaphore = reinterpret_cast<ze_external_semaphore_ext_handle_t>(externalSemaphore);
    ze_external_semaphore_signal_params_ext_t externalSemaphoreSignalParamsExt{};
    externalSemaphoreSignalParamsExt.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS_EXT;
    externalSemaphoreSignalParamsExt.value = uint64_t(timelineValue);
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendSignalExternalSemaphoreExt(
            stream.zeCommandList, 1, &zeExternalSemaphore, &externalSemaphoreSignalParamsExt,
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    if (zeResult == ZE_RESULT_ERROR_INVALID_ARGUMENT && g_zeCommandQueue) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in SemaphoreVkComputeApiInterop::signalSemaphoreComputeApi: "
                    "Level Zero requires an immediate command list for this command.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in SemaphoreVkComputeApiInterop::signalSemaphoreComputeApi: "
                    "Level Zero requires an immediate command list for this command.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException(
                "Level Zero requires an immediate command list for this command");
    } else {
        checkZeResult(zeResult, "Error in zeCommandListAppendSignalExternalSemaphoreExt: ");
    }
}

void SemaphoreVkLevelZeroInterop::waitSemaphoreComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
    auto zeExternalSemaphore = reinterpret_cast<ze_external_semaphore_ext_handle_t>(externalSemaphore);
    ze_external_semaphore_wait_params_ext_t externalSemaphoreWaitParamsExt{};
    externalSemaphoreWaitParamsExt.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WAIT_PARAMS_EXT;
    externalSemaphoreWaitParamsExt.value = uint64_t(timelineValue);
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendWaitExternalSemaphoreExt(
            stream.zeCommandList, 1, &zeExternalSemaphore, &externalSemaphoreWaitParamsExt,
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    if (zeResult == ZE_RESULT_ERROR_INVALID_ARGUMENT && g_zeCommandQueue) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in SemaphoreVkComputeApiInterop::waitSemaphoreComputeApi: "
                    "Level Zero requires an immediate command list for this command.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in SemaphoreVkComputeApiInterop::waitSemaphoreComputeApi: "
                    "Level Zero requires an immediate command list for this command.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException(
                "Level Zero requires an immediate command list for this command");
    } else {
        checkZeResult(zeResult, "Error in zeCommandListAppendWaitExternalSemaphoreExt: ");
    }
}


void BufferVkLevelZeroInterop::preCheckExternalMemoryImport() {
    deviceMemAllocDesc = {};
    deviceMemAllocDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    //deviceMemAllocDesc.ordinal; // TODO: Necessary?
    if (!g_zeDevice || !g_zeContext) {
        sgl::Logfile::get()->throwError(
                "Error in BufferVkLevelZeroInterop::preCheckExternalMemoryImport: "
                "Level Zero is initialized, but the global device or context object are not set.");
    }
}

#ifdef _WIN32
void BufferVkLevelZeroInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    externalMemoryImportWin32Handle = {};
    externalMemoryImportWin32Handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
    deviceMemAllocDesc.pNext = &externalMemoryImportWin32Handle;
    externalMemoryImportWin32Handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32;
    externalMemoryImportWin32Handle.handle = (void*)handle;
}
#endif

#ifdef __linux__
void BufferVkLevelZeroInterop::setExternalMemoryFd(int fileDescriptor) {
    externalMemoryImportFd = {};
    externalMemoryImportFd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
    deviceMemAllocDesc.pNext = &externalMemoryImportFd;
    externalMemoryImportFd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD;
    externalMemoryImportFd.fd = fileDescriptor;
}
#endif

void BufferVkLevelZeroInterop::importExternalMemory() {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeMemAllocDevice(
            g_zeContext, &deviceMemAllocDesc, memoryRequirements.size, 0, g_zeDevice, &devicePtr);
    checkZeResult(zeResult, "Error in zeMemAllocDevice: ");
}

void BufferVkLevelZeroInterop::free() {
    freeHandlesAndFds();
    if (externalMemoryBuffer) {
        ze_result_t zeResult = g_levelZeroFunctionTable.zeMemFree(g_zeContext, devicePtr);
        checkZeResult(zeResult, "Error in zeMemFree: ");
        externalMemoryBuffer = {};
    }
}

BufferVkLevelZeroInterop::~BufferVkLevelZeroInterop() {
    BufferVkLevelZeroInterop::free();
}

void BufferVkLevelZeroInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, devicePtr, devicePtrSrc, vulkanBuffer->getSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}

void BufferVkLevelZeroInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, devicePtrDst, devicePtr, vulkanBuffer->getSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}

void BufferVkLevelZeroInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, devicePtr, hostPtrSrc, vulkanBuffer->getSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}

void BufferVkLevelZeroInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, hostPtrDst, devicePtr, vulkanBuffer->getSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}


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

void ImageVkLevelZeroInterop::preCheckExternalMemoryImport() {
    zeImageDesc = {};
    // Bindless images.
    deviceMemAllocDesc = {};
    imagePitchedExpDesc = {};
    imageBindlessExpDesc = {};
    //deviceMemAllocDesc.ordinal; // TODO: Necessary?
    zeImageDesc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    if (!g_zeDevice || !g_zeContext) {
        sgl::Logfile::get()->throwError(
                "Error in ImageVkLevelZeroInterop::preCheckExternalMemoryImport: "
                "Level Zero is initialized, but the global device or context object are not set.");
    }
}

#ifdef _WIN32
void ImageVkLevelZeroInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    externalMemoryImportWin32Handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
    if (g_useBindlessImagesInterop) {
        deviceMemAllocDesc.pNext = &externalMemoryImportWin32Handle;
    } else {
        zeImageDesc.pNext = &externalMemoryImportWin32Handle;
    }
    externalMemoryImportWin32Handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32;
    externalMemoryImportWin32Handle.handle = (void*)handle;
}
#endif

#ifdef __linux__
void ImageVkLevelZeroInterop::setExternalMemoryFd(int fileDescriptor) {
    externalMemoryImportFd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
    if (g_useBindlessImagesInterop) {
        deviceMemAllocDesc.pNext = &externalMemoryImportFd;
    } else {
        zeImageDesc.pNext = &externalMemoryImportFd;
    }
    externalMemoryImportFd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD;
    externalMemoryImportFd.fd = fileDescriptor;
}
#endif

void ImageVkLevelZeroInterop::importExternalMemory() {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
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

    ze_result_t zeResult;
    if (g_useBindlessImagesInterop) {
        auto elementSizeInBytes = uint32_t(sgl::vk::getImageFormatEntryByteSize(imageSettings.format));
        size_t rowPitch = 0;
        zeResult = g_levelZeroFunctionTable.zeMemGetPitchFor2dImage(
               g_zeContext, g_zeDevice, imageSettings.width, imageSettings.height, elementSizeInBytes, &rowPitch);
        checkZeResult(zeResult, "Error in zeMemGetPitchFor2dImage: ");
        size_t memorySize = rowPitch * imageSettings.height;
        deviceMemAllocDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
        zeResult = g_levelZeroFunctionTable.zeMemAllocDevice(
                g_zeContext, &deviceMemAllocDesc, memorySize, 0, g_zeDevice, &devicePtr);
        checkZeResult(zeResult, "Error in zeMemAllocDevice: ");

        imagePitchedExpDesc.stype = ZE_STRUCTURE_TYPE_PITCHED_IMAGE_EXP_DESC;
        imagePitchedExpDesc.ptr = devicePtr;
        imageBindlessExpDesc.stype = ZE_STRUCTURE_TYPE_BINDLESS_IMAGE_EXP_DESC;
        imageBindlessExpDesc.flags = ZE_IMAGE_BINDLESS_EXP_FLAG_BINDLESS;
        imageBindlessExpDesc.pNext = &imagePitchedExpDesc;
        zeImageDesc.pNext = &imageBindlessExpDesc;
    }

    ze_image_handle_t imageHandle{};
    zeResult = g_levelZeroFunctionTable.zeImageCreate(
            g_zeContext, g_zeDevice, &zeImageDesc, &imageHandle);
    if (zeResult == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: "
                    "Unsupported Level Zero image memory type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: "
                    "Unsupported Level Zero image memory type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported Level Zero image memory type");
    } else {
        checkZeResult(zeResult, "Error in zeImageCreate: ");
    }
    mipmappedArray = reinterpret_cast<void*>(imageHandle);
}

void ImageVkLevelZeroInterop::free() {
    freeHandlesAndFds();
    if (mipmappedArray) {
        auto imageHandle = reinterpret_cast<ze_image_handle_t>(mipmappedArray);
        ze_result_t zeResult = g_levelZeroFunctionTable.zeImageDestroy(imageHandle);
        checkZeResult(zeResult, "Error in zeImageDestroy: ");
        mipmappedArray = {};
    }
    if (devicePtr && g_useBindlessImagesInterop) {
        ze_result_t zeResult = g_levelZeroFunctionTable.zeMemFree(g_zeContext, devicePtr);
        checkZeResult(zeResult, "Error in zeMemFree: ");
    }
}

ImageVkLevelZeroInterop::~ImageVkLevelZeroInterop() {
    ImageVkLevelZeroInterop::free();
}

void ImageVkLevelZeroInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
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
}

}}
