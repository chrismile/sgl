/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#include <cstring>

#include "../Resource.hpp"
#include "ImplLevelZero.hpp"

#include "../../../../../../../../../../msys64/mingw64/include/glm/ext/scalar_uint_sized.hpp"

#ifdef SUPPORT_SYCL_INTEROP
#include <sycl/sycl.hpp>
#include <sycl/ext/oneapi/backend/level_zero.hpp>
#endif

namespace sgl {
extern bool openMessageBoxOnComputeApiError;

extern ze_device_handle_t g_zeDevice;
extern ze_context_handle_t g_zeContext;
extern ze_command_queue_handle_t g_zeCommandQueue;
extern ze_event_handle_t g_zeSignalEvent;
extern uint32_t g_numWaitEvents;
extern ze_event_handle_t* g_zeWaitEvents;
extern bool g_useBindlessImagesInterop;
}

namespace sgl { namespace d3d12 {

void FenceD3D12LevelZeroInterop::importExternalFenceWin32Handle() {
    externalSemaphoreExtDesc = {};
    externalSemaphoreExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC;
    if (!g_zeDevice) {
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkComputeApiInterop::SemaphoreVkComputeApiInterop: "
                "Level Zero is initialized, but the global device object is not set.");
    }
    externalSemaphoreWin32ExtDesc = {};
    externalSemaphoreWin32ExtDesc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC;
    externalSemaphoreExtDesc.pNext = &externalSemaphoreWin32ExtDesc;
    externalSemaphoreExtDesc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE;
    externalSemaphoreWin32ExtDesc.handle = handle;

    ze_external_semaphore_ext_handle_t zeExternalSemaphore{};
    ze_result_t zeResult = g_levelZeroFunctionTable.zeDeviceImportExternalSemaphoreExt(
            g_zeDevice, &externalSemaphoreExtDesc, &zeExternalSemaphore);
    if (zeResult == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE || zeResult == ZE_RESULT_ERROR_UNINITIALIZED) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in FenceD3D12LevelZeroInterop::_initialize: "
                    "Unsupported Level Zero external semaphore type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in FenceD3D12LevelZeroInterop::_initialize: "
                    "Unsupported Level Zero external semaphore type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported Level Zero external semaphore type");
    } else {
        checkZeResult(zeResult, "Error in zeDeviceImportExternalSemaphoreExt: ");
    }
    externalSemaphore = reinterpret_cast<void*>(zeExternalSemaphore);
}

void FenceD3D12LevelZeroInterop::free() {
    freeHandle();
    if (externalSemaphore) {
        auto zeExternalSemaphore = reinterpret_cast<ze_external_semaphore_ext_handle_t>(externalSemaphore);
        ze_result_t zeResult = g_levelZeroFunctionTable.zeDeviceReleaseExternalSemaphoreExt(zeExternalSemaphore);
        checkZeResult(zeResult, "Error in zeDeviceReleaseExternalSemaphoreExt: ");
        externalSemaphore = {};
    }
}

FenceD3D12LevelZeroInterop::~FenceD3D12LevelZeroInterop() {
    FenceD3D12LevelZeroInterop::free();
}

void FenceD3D12LevelZeroInterop::signalFenceComputeApi(
            StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
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
                    "Error in FenceD3D12LevelZeroInterop::signalSemaphoreComputeApi: "
                    "Level Zero requires an immediate command list for this command.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in FenceD3D12LevelZeroInterop::signalSemaphoreComputeApi: "
                    "Level Zero requires an immediate command list for this command.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException(
                "Level Zero requires an immediate command list for this command");
    } else {
        checkZeResult(zeResult, "Error in zeCommandListAppendSignalExternalSemaphoreExt: ");
    }
}

void FenceD3D12LevelZeroInterop::waitFenceComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
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


void BufferD3D12LevelZeroInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getCopiableSizeInBytes();
    deviceMemAllocDesc = {};
    deviceMemAllocDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    if (!g_zeDevice || !g_zeContext) {
        sgl::Logfile::get()->throwError(
                "Error in BufferD3D12LevelZeroInterop::importExternalMemoryWin32Handle: "
                "Level Zero is initialized, but the global device or context object are not set.");
    }
    externalMemoryImportWin32Handle = {};
    externalMemoryImportWin32Handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
    deviceMemAllocDesc.pNext = &externalMemoryImportWin32Handle;
    externalMemoryImportWin32Handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE;
    externalMemoryImportWin32Handle.handle = (void*)handle;

    ze_result_t zeResult = g_levelZeroFunctionTable.zeMemAllocDevice(
            g_zeContext, &deviceMemAllocDesc, sizeInBytes, 0, g_zeDevice, &devicePtr);
    checkZeResult(zeResult, "Error in zeMemAllocDevice: ");
}

void BufferD3D12LevelZeroInterop::free() {
    freeHandle();
    if (externalMemoryBuffer) {
        ze_result_t zeResult = g_levelZeroFunctionTable.zeMemFree(g_zeContext, devicePtr);
        checkZeResult(zeResult, "Error in zeMemFree: ");
        externalMemoryBuffer = {};
    }
}

BufferD3D12LevelZeroInterop::~BufferD3D12LevelZeroInterop() {
    BufferD3D12LevelZeroInterop::free();
}

void BufferD3D12LevelZeroInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, devicePtr, devicePtrSrc, resource->getCopiableSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}

void BufferD3D12LevelZeroInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, devicePtrDst, devicePtr, resource->getCopiableSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}

void BufferD3D12LevelZeroInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, devicePtr, hostPtrSrc, resource->getCopiableSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}

void BufferD3D12LevelZeroInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy(
            stream.zeCommandList, hostPtrDst, devicePtr, resource->getCopiableSizeInBytes(),
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendMemoryCopy: ");
}


static void getZeImageFormatFromD3D12Format(DXGI_FORMAT format, ze_image_format_t& zeFormat) {
    switch (format) {
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_UINT;
        break;
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_SINT;
        break;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_UNORM;
        break;
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_SNORM;
        break;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
        zeFormat.type = ZE_IMAGE_FORMAT_TYPE_FLOAT;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported type.");
        return;
    }
    switch (format) {
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_SNORM:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_8;
        break;
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_SNORM:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8;
        break;
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
        break;
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_16;
        break;
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_16_16;
        break;
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16;
        break;
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32;
        break;
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32_FLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32;
        break;
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32_32;
        break;
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        zeFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported layout.");
        return;
    }
    switch (format) {
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
        zeFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
        zeFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
        zeFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        zeFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_B;
        zeFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
        zeFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_R;
        zeFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
        break;
    default:
        sgl::Logfile::get()->throwError("Error in getZeImageFormatFromVkFormat: Unsupported swizzle.");
        return;
    }
    size_t numChannels = getDXGIFormatNumChannels(format);
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

static ze_sampler_address_mode_t getLevelZeroSamplerAddressModeD3D12(
        D3D12_TEXTURE_ADDRESS_MODE samplerAddressModeD3D12) {
    switch (samplerAddressModeD3D12) {
        case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
            return ZE_SAMPLER_ADDRESS_MODE_REPEAT;
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR:
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE:
            return ZE_SAMPLER_ADDRESS_MODE_MIRROR;
        case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
            return ZE_SAMPLER_ADDRESS_MODE_CLAMP;
        case D3D12_TEXTURE_ADDRESS_MODE_BORDER:
            return ZE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaSamplerAddressModeD3D12: Unsupported format.");
            return ZE_SAMPLER_ADDRESS_MODE_NONE;
    }
}

void ImageD3D12LevelZeroInterop::importExternalMemoryWin32Handle() {
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

    externalMemoryImportWin32Handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
    if (g_useBindlessImagesInterop) {
        deviceMemAllocDesc.pNext = &externalMemoryImportWin32Handle;
    } else {
        zeImageDesc.pNext = &externalMemoryImportWin32Handle;
    }
    externalMemoryImportWin32Handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE;
    externalMemoryImportWin32Handle.handle = (void*)handle;

    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    const bool isLayered = resourceDesc.DepthOrArraySize > 1;
    zeImageDesc.width = static_cast<uint32_t>(resourceDesc.Width);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
            || resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        zeImageDesc.height = static_cast<uint32_t>(resourceDesc.Height);
    }
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        zeImageDesc.depth = static_cast<uint32_t>(resourceDesc.DepthOrArraySize);
    } else if (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D && isLayered) {
        zeImageDesc.arraylevels = static_cast<uint32_t>(resourceDesc.DepthOrArraySize);
    }
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D && !isLayered) {
        zeImageDesc.type = ZE_IMAGE_TYPE_1D;
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D && isLayered) {
        zeImageDesc.type = ZE_IMAGE_TYPE_1DARRAY;
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && !isLayered) {
        zeImageDesc.type = ZE_IMAGE_TYPE_2D;
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && isLayered) {
        zeImageDesc.type = ZE_IMAGE_TYPE_2DARRAY;
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        zeImageDesc.type = ZE_IMAGE_TYPE_3D;
    }
    getZeImageFormatFromD3D12Format(resourceDesc.Format, zeImageDesc.format);
    if (imageComputeApiInfo.surfaceLoadStore) {
        zeImageDesc.flags |= ZE_IMAGE_FLAG_KERNEL_WRITE;
    }
    // ZE_IMAGE_FLAG_BIAS_UNCACHED currently unused here.

    ze_result_t zeResult;
    if (g_useBindlessImagesInterop) {
        auto elementSizeInBytes = uint32_t(sgl::d3d12::getDXGIFormatSizeInBytes(resourceDesc.Format));
        size_t rowPitch = 0;
        zeResult = g_levelZeroFunctionTable.zeMemGetPitchFor2dImage(
               g_zeContext, g_zeDevice, static_cast<uint32_t>(resourceDesc.Width), static_cast<uint32_t>(resourceDesc.Height), elementSizeInBytes, &rowPitch);
        checkZeResult(zeResult, "Error in zeMemGetPitchFor2dImage: ");
        size_t memorySize = rowPitch * static_cast<uint32_t>(resourceDesc.Height);
        deviceMemAllocDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
        zeResult = g_levelZeroFunctionTable.zeMemAllocDevice(
                g_zeContext, &deviceMemAllocDesc, memorySize, 0, g_zeDevice, &devicePtr);
        checkZeResult(zeResult, "Error in zeMemAllocDevice: ");

        imagePitchedExpDesc.stype = ZE_STRUCTURE_TYPE_PITCHED_IMAGE_EXP_DESC;
        imagePitchedExpDesc.ptr = devicePtr;
        imageBindlessExpDesc.stype = ZE_STRUCTURE_TYPE_BINDLESS_IMAGE_EXP_DESC;
        imageBindlessExpDesc.flags = ZE_IMAGE_BINDLESS_EXP_FLAG_BINDLESS;
        imageBindlessExpDesc.pNext = &imagePitchedExpDesc;
        if (imageComputeApiInfo.useSampledImage) {
            const auto& samplerDescD3D12 = imageComputeApiInfo.samplerDesc;
            samplerDesc.stype = ZE_STRUCTURE_TYPE_SAMPLER_DESC;
            if (samplerDescD3D12.Filter == D3D12_FILTER_MIN_MAG_MIP_POINT
                    || samplerDescD3D12.Filter == D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR) {
                samplerDesc.filterMode = ZE_SAMPLER_FILTER_MODE_NEAREST;
            } else {
                samplerDesc.filterMode = ZE_SAMPLER_FILTER_MODE_LINEAR;
            }
            samplerDesc.addressMode = getLevelZeroSamplerAddressModeD3D12(samplerDescD3D12.AddressU);
            samplerDesc.isNormalized = imageComputeApiInfo.textureExternalMemorySettings.useNormalizedCoordinates;
            imagePitchedExpDesc.pNext = &samplerDesc;
        }
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

void ImageD3D12LevelZeroInterop::free() {
    freeHandle();
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

ImageD3D12LevelZeroInterop::~ImageD3D12LevelZeroInterop() {
    ImageD3D12LevelZeroInterop::free();
}

void ImageD3D12LevelZeroInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    auto imageHandle = reinterpret_cast<ze_image_handle_t>(mipmappedArray);
    ze_image_region_t dstRegion{};
    dstRegion.originX = 0;
    dstRegion.originY = 0;
    dstRegion.originZ = 0;
    dstRegion.width = static_cast<uint32_t>(resourceDesc.Width);
    dstRegion.height = static_cast<uint32_t>(resourceDesc.Height);
    dstRegion.depth = static_cast<uint32_t>(resourceDesc.DepthOrArraySize);
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendImageCopyFromMemory(
            stream.zeCommandList, imageHandle, devicePtrSrc, &dstRegion,
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendImageCopyFromMemory: ");
}

void ImageD3D12LevelZeroInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    auto imageHandle = reinterpret_cast<ze_image_handle_t>(mipmappedArray);
    ze_image_region_t srcRegion{};
    srcRegion.originX = 0;
    srcRegion.originY = 0;
    srcRegion.originZ = 0;
    srcRegion.width = static_cast<uint32_t>(resourceDesc.Width);
    srcRegion.height = static_cast<uint32_t>(resourceDesc.Height);
    srcRegion.depth = static_cast<uint32_t>(resourceDesc.DepthOrArraySize);
    ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListAppendImageCopyToMemory(
            stream.zeCommandList, devicePtrDst, imageHandle, &srcRegion,
            g_zeSignalEvent, g_numWaitEvents, g_zeWaitEvents);
    checkZeResult(zeResult, "Error in zeCommandListAppendImageCopyFromMemory: ");
}

}}
