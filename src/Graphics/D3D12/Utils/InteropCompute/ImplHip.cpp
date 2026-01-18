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
#include "ImplHip.hpp"
namespace sgl {
extern bool openMessageBoxOnComputeApiError;
}

namespace sgl { namespace d3d12 {

void FenceD3D12HipInterop::importExternalFenceWin32Handle() {
    externalSemaphoreHandleDescHip = {};
    externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeD3D12Fence;
    externalSemaphoreHandleDescHip.handle.win32.handle = handle;

    if (!g_hipDeviceApiFunctionTable.hipImportExternalSemaphore) {
        throw UnsupportedComputeApiFeatureException("HIP does not support external semaphore import");
    }
    hipExternalSemaphore_t hipExternalSemaphore{};
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipImportExternalSemaphore(
            &hipExternalSemaphore, &externalSemaphoreHandleDescHip);
    checkHipResult(hipResult, "Error in hipImportExternalSemaphore: ");
    externalSemaphore = reinterpret_cast<void*>(hipExternalSemaphore);
}

void FenceD3D12HipInterop::free() {
    freeHandle();
    if (externalSemaphore) {
        auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalSemaphore(hipExternalSemaphore);
        checkHipResult(hipResult, "Error in hipDestroyExternalSemaphore: ");
        externalSemaphore = {};
    }
}

FenceD3D12HipInterop::~FenceD3D12HipInterop() {
    FenceD3D12HipInterop::free();
}

void FenceD3D12HipInterop::signalFenceComputeApi(
            StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    if (!g_hipDeviceApiFunctionTable.hipImportExternalSemaphore) {
        throw UnsupportedComputeApiFeatureException("HIP does not support signalling external semaphores");
    }
    auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
    hipExternalSemaphoreSignalParams signalParams{};
    signalParams.params.fence.value = timelineValue;
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipSignalExternalSemaphoresAsync(
            &hipExternalSemaphore, &signalParams, 1, stream.hipStream);
    checkHipResult(hipResult, "Error in hipSignalExternalSemaphoresAsync: ");
}

void FenceD3D12HipInterop::waitFenceComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    if (!g_hipDeviceApiFunctionTable.hipImportExternalSemaphore) {
        throw UnsupportedComputeApiFeatureException("HIP does not support waiting on external semaphores");
    }
    auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
    hipExternalSemaphoreWaitParams waitParams{};
    waitParams.params.fence.value = timelineValue;
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipWaitExternalSemaphoresAsync(
            &hipExternalSemaphore, &waitParams, 1, stream.hipStream);
    checkHipResult(hipResult, "Error in hipWaitExternalSemaphoresAsync: ");
}


void BufferD3D12HipInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getCopiableSizeInBytes();
    externalMemoryHandleDescHip = {};
    externalMemoryHandleDescHip.size = sizeInBytes;
    externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeD3D12Resource;
    externalMemoryHandleDescHip.handle.win32.handle = (void*)handle;
    externalMemoryHandleDescHip.flags = hipExternalMemoryDedicated;

    hipExternalMemory_t hipExternalMemory{};
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipImportExternalMemory(
            &hipExternalMemory, &externalMemoryHandleDescHip);
    checkHipResult(hipResult, "Error in hipImportExternalMemory: ");
    externalMemoryBuffer = reinterpret_cast<void*>(hipExternalMemory);

    hipDeviceptr_t hipDevicePtr{};
    hipExternalMemoryBufferDesc externalMemoryBufferDesc{};
    externalMemoryBufferDesc.offset = 0;
    externalMemoryBufferDesc.size = sizeInBytes;
    externalMemoryBufferDesc.flags = 0;
    hipResult = g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedBuffer(
            &hipDevicePtr, hipExternalMemory, &externalMemoryBufferDesc);
    checkHipResult(hipResult, "Error in hipExternalMemoryGetMappedBuffer: ");
    devicePtr = reinterpret_cast<void*>(hipDevicePtr);
}

void BufferD3D12HipInterop::free() {
    freeHandle();
    if (externalMemoryBuffer) {
        hipDeviceptr_t hipDevicePtr = getHipDevicePtr();
        auto hipExternalMemory = reinterpret_cast<hipExternalMemory_t>(externalMemoryBuffer);
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipFree(hipDevicePtr);
        checkHipResult(hipResult, "Error in hipFree: ");
        hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalMemory(hipExternalMemory);
        checkHipResult(hipResult, "Error in hipDestroyExternalMemory: ");
        externalMemoryBuffer = {};
    }
}

BufferD3D12HipInterop::~BufferD3D12HipInterop() {
    BufferD3D12HipInterop::free();
}

void BufferD3D12HipInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyAsync(
            this->getHipDevicePtr(), reinterpret_cast<hipDeviceptr_t>(devicePtrSrc),
            resource->getCopiableSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyAsync: ");
}

void BufferD3D12HipInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyAsync(
            reinterpret_cast<hipDeviceptr_t>(devicePtrDst), this->getHipDevicePtr(),
            resource->getCopiableSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyAsync: ");
}

void BufferD3D12HipInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyHtoDAsync(
            this->getHipDevicePtr(), hostPtrSrc,
            resource->getCopiableSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyHtoDAsync: ");
}

void BufferD3D12HipInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyDtoHAsync(
            hostPtrDst, this->getHipDevicePtr(),
            resource->getCopiableSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyDtoHAsync: ");
}


static void getHipFormatDescFromD3D12Format(DXGI_FORMAT format, hipChannelFormatDesc& hipFormat) {
    switch (format) {
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            hipFormat.f = hipChannelFormatKindUnsigned;
            break;
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            hipFormat.f = hipChannelFormatKindSigned;
            break;
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            hipFormat.f = hipChannelFormatKindSigned;
            break;
        default:
            sgl::Logfile::get()->throwError("Error in getHipFormatDescFromD3D12Format: Unsupported format.");
            hipFormat.f = hipChannelFormatKindNone;
    }
    switch (format) {
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_SNORM:
            hipFormat.x = 8;
            break;
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_SNORM:
            hipFormat.x = 8;
            hipFormat.y = 8;
            break;
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            hipFormat.x = 8;
            hipFormat.y = 8;
            hipFormat.z = 8;
            hipFormat.w = 8;
            break;
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
            hipFormat.x = 16;
            break;
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_FLOAT:
            hipFormat.x = 16;
            hipFormat.y = 16;
            break;
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            hipFormat.x = 16;
            hipFormat.y = 16;
            hipFormat.z = 16;
            hipFormat.w = 16;
            break;
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
            hipFormat.x = 32;
            break;
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32_FLOAT:
            hipFormat.x = 32;
            hipFormat.y = 32;
            break;
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
            hipFormat.x = 32;
            hipFormat.y = 32;
            hipFormat.z = 32;
            break;
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
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

void ImageD3D12HipInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getCopiableSizeInBytes();
    externalMemoryHandleDescHip = {};
    externalMemoryHandleDescHip.size = sizeInBytes;
    externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeD3D12Resource;
    externalMemoryHandleDescHip.handle.win32.handle = (void*)handle;
    externalMemoryHandleDescHip.flags = hipExternalMemoryDedicated;

    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    hipExternalMemory_t hipExternalMemory{};
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipImportExternalMemory(
            &hipExternalMemory, &externalMemoryHandleDescHip);
    checkHipResult(hipResult, "Error in hipImportExternalMemory: ");
    externalMemoryBuffer = reinterpret_cast<void*>(hipExternalMemory);

    hipExternalMemoryMipmappedArrayDesc externalMemoryMipmappedArrayDesc{};
    externalMemoryMipmappedArrayDesc.extent.width = static_cast<size_t>(resourceDesc.Width);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
            || resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        externalMemoryMipmappedArrayDesc.extent.height = static_cast<size_t>(resourceDesc.Height);
    }
    externalMemoryMipmappedArrayDesc.extent.depth = resourceDesc.DepthOrArraySize;
    externalMemoryMipmappedArrayDesc.numLevels = uint32_t(getDXGIFormatNumChannels(resourceDesc.Format));
    getHipFormatDescFromD3D12Format(resourceDesc.Format, externalMemoryMipmappedArrayDesc.formatDesc);
    externalMemoryMipmappedArrayDesc.flags = 0;

    hipMipmappedArray_t hipMipmappedArray{};
    hipResult = g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedMipmappedArray(
            &hipMipmappedArray, hipExternalMemory, &externalMemoryMipmappedArrayDesc);
    if (hipResult == hipErrorInvalidValue) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in ImageD3D12HipInterop::importExternalMemory: Unsupported HIP image type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in ImageD3D12HipInterop::importExternalMemory: Unsupported HIP image type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported HIP image type");
    } else {
        checkHipResult(hipResult, "Error in hipImportExternalMemory: ");
    }
    mipmappedArray = reinterpret_cast<void*>(hipMipmappedArray);
}

void ImageD3D12HipInterop::free() {
    freeHandle();
    if (mipmappedArray) {
        hipMipmappedArray_t hipMipmappedArray = getHipMipmappedArray();
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMipmappedArrayDestroy(hipMipmappedArray);
        checkHipResult(hipResult, "Error in hipMipmappedArrayDestroy: ");
        mipmappedArray = {};
    }
    if (externalMemoryBuffer) {
        auto hipExternalMemory = reinterpret_cast<hipExternalMemory_t>(externalMemoryBuffer);
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalMemory(hipExternalMemory);
        checkHipResult(hipResult, "Error in hipDestroyExternalMemory: ");
        externalMemoryBuffer = {};
    }
}

ImageD3D12HipInterop::~ImageD3D12HipInterop() {
    ImageD3D12HipInterop::free();
}

hipArray_t ImageD3D12HipInterop::getHipMipmappedArrayLevel(uint32_t level) {
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

void ImageD3D12HipInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    size_t entryByteSize = getDXGIFormatSizeInBytes(resourceDesc.Format);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        if (!g_hipDeviceApiFunctionTable.hipMemcpy2DToArrayAsync) {
            throw UnsupportedComputeApiFeatureException("HIP does not support 2D image copies");
        }
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpy2DToArrayAsync(
                getHipMipmappedArrayLevel(0), 0, 0, devicePtrSrc,
                static_cast<size_t>(resourceDesc.Width) * entryByteSize,
                static_cast<size_t>(resourceDesc.Width), static_cast<size_t>(resourceDesc.Height),
                hipMemcpyDeviceToDevice, stream.hipStream);
        checkHipResult(hipResult, "Error in hipMemcpy2DToArrayAsync: ");
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        HIP_MEMCPY3D memcpySettings{};
        memcpySettings.srcMemoryType = hipMemoryTypeDevice;
        memcpySettings.srcDevice = reinterpret_cast<hipDeviceptr_t>(devicePtrSrc);
        memcpySettings.srcPitch = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.srcHeight = static_cast<size_t>(resourceDesc.Height);

        memcpySettings.dstMemoryType = hipMemoryTypeArray;
        memcpySettings.dstArray = getHipMipmappedArrayLevel(0);

        memcpySettings.WidthInBytes = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.Height = static_cast<size_t>(resourceDesc.Height);
        memcpySettings.Depth = static_cast<size_t>(resourceDesc.DepthOrArraySize);

        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDrvMemcpy3DAsync(&memcpySettings, stream.hipStream);
        checkHipResult(hipResult, "Error in hipDrvMemcpy3DAsync: ");
    } else {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::copyFromDevicePtrAsync: "
                "Unsupported image view type.");
    }
}

void ImageD3D12HipInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    size_t entryByteSize = getDXGIFormatSizeInBytes(resourceDesc.Format);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        if (!g_hipDeviceApiFunctionTable.hipMemcpy2DFromArrayAsync) {
            throw UnsupportedComputeApiFeatureException("HIP does not support 2D image copies");
        }
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpy2DFromArrayAsync(
                devicePtrDst, static_cast<size_t>(resourceDesc.Width) * entryByteSize,
                getHipMipmappedArrayLevel(0), 0, 0, static_cast<size_t>(resourceDesc.Width),
                static_cast<size_t>(resourceDesc.Height),
                hipMemcpyDeviceToDevice, stream.hipStream);
        checkHipResult(hipResult, "Error in hipMemcpy2DFromArrayAsync: ");
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        HIP_MEMCPY3D memcpySettings{};
        memcpySettings.srcMemoryType = hipMemoryTypeArray;
        memcpySettings.srcArray = getHipMipmappedArrayLevel(0);

        memcpySettings.dstMemoryType = hipMemoryTypeDevice;
        memcpySettings.dstDevice = reinterpret_cast<hipDeviceptr_t>(devicePtrDst);
        memcpySettings.dstPitch = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.dstHeight = static_cast<size_t>(resourceDesc.Height);

        memcpySettings.WidthInBytes = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.Height = static_cast<size_t>(resourceDesc.Height);
        memcpySettings.Depth = static_cast<size_t>(resourceDesc.DepthOrArraySize);

        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDrvMemcpy3DAsync(&memcpySettings, stream.hipStream);
        checkHipResult(hipResult, "Error in hipDrvMemcpy3DAsync: ");
    } else {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::copyFromDevicePtrAsync: "
                "Unsupported image view type.");
    }
}


void UnsampledImageD3D12HipInterop::initialize(const ImageD3D12ComputeApiExternalMemoryPtr& _image) {
    this->image = _image;

    hipResourceDesc hipResourceDesc{};
    hipResourceDesc.resType = hipResourceTypeMipmappedArray;
    hipResourceDesc.res.mipmap.mipmap = getHipMipmappedArray();

    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipCreateSurfaceObject(&hipSurfaceObject, &hipResourceDesc);
    checkHipResult(hipResult, "Error in hipSurfObjectDestroy: ");
}

UnsampledImageD3D12HipInterop::~UnsampledImageD3D12HipInterop() {
    if (hipSurfaceObject) {
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDestroySurfaceObject(hipSurfaceObject);
        checkHipResult(hipResult, "Error in hipSurfObjectDestroy: ");
        hipSurfaceObject = {};
    }
}

}}
