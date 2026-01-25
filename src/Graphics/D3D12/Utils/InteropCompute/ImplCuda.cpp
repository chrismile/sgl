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
#include "ImplCuda.hpp"
namespace sgl {
extern bool openMessageBoxOnComputeApiError;
}

namespace sgl { namespace d3d12 {

void FenceD3D12CudaInterop::importExternalFenceWin32Handle() {
    externalSemaphoreHandleDesc = {};
    externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE;
    externalSemaphoreHandleDesc.handle.win32.handle = handle;

    CUexternalSemaphore cuExternalSemaphore{};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalSemaphore(
            &cuExternalSemaphore, &externalSemaphoreHandleDesc);
    checkCUresult(cuResult, "Error in cuImportExternalSemaphore: ");
    externalSemaphore = reinterpret_cast<void*>(cuExternalSemaphore);
}

void FenceD3D12CudaInterop::free() {
    freeHandle();
    if (externalSemaphore) {
        auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore(cuExternalSemaphore);
        checkCUresult(cuResult, "Error in cuDestroyExternalSemaphore: ");
        externalSemaphore = {};
    }
}

FenceD3D12CudaInterop::~FenceD3D12CudaInterop() {
    FenceD3D12CudaInterop::free();
}

void FenceD3D12CudaInterop::signalFenceComputeApi(
            StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
    CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS signalParams{};
    signalParams.params.fence.value = timelineValue;
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSignalExternalSemaphoresAsync(
            &cuExternalSemaphore, &signalParams, 1, stream.cuStream);
    checkCUresult(cuResult, "Error in cuSignalExternalSemaphoresAsync: ");
}

void FenceD3D12CudaInterop::waitFenceComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
    CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS waitParams{};
    waitParams.params.fence.value = timelineValue;
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuWaitExternalSemaphoresAsync(
            &cuExternalSemaphore, &waitParams, 1, stream.cuStream);
    checkCUresult(cuResult, "Error in cuWaitExternalSemaphoresAsync: ");
}


void BufferD3D12CudaInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getCopiableSizeInBytes();
    externalMemoryHandleDesc = {};
    externalMemoryHandleDesc.size = sizeInBytes;
    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE;
    externalMemoryHandleDesc.handle.win32.handle = (void*)handle;
    externalMemoryHandleDesc.flags = CUDA_EXTERNAL_MEMORY_DEDICATED;

    CUexternalMemory cudaExternalMemoryBuffer{};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
            &cudaExternalMemoryBuffer, &externalMemoryHandleDesc);
    checkCUresult(cuResult, "Error in cuImportExternalMemory: ");
    externalMemoryBuffer = reinterpret_cast<void*>(cudaExternalMemoryBuffer);

    CUdeviceptr cudaDevicePtr{};
    CUDA_EXTERNAL_MEMORY_BUFFER_DESC externalMemoryBufferDesc{};
    externalMemoryBufferDesc.offset = 0;
    externalMemoryBufferDesc.size = sizeInBytes;
    externalMemoryBufferDesc.flags = 0;
    cuResult = g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer(
            &cudaDevicePtr, cudaExternalMemoryBuffer, &externalMemoryBufferDesc);
    checkCUresult(cuResult, "Error in cudaExternalMemoryGetMappedBuffer: ");
    devicePtr = reinterpret_cast<void*>(cudaDevicePtr);
}

void BufferD3D12CudaInterop::free() {
    freeHandle();
    if (externalMemoryBuffer) {
        CUdeviceptr cudaDevicePtr = getCudaDevicePtr();
        auto cudaExternalMemoryBuffer = reinterpret_cast<CUexternalMemory>(externalMemoryBuffer);
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemFree(cudaDevicePtr);
        checkCUresult(cuResult, "Error in cuMemFree: ");
        cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory(cudaExternalMemoryBuffer);
        checkCUresult(cuResult, "Error in cuDestroyExternalMemory: ");
        externalMemoryBuffer = {};
    }
}

BufferD3D12CudaInterop::~BufferD3D12CudaInterop() {
    BufferD3D12CudaInterop::free();
}

void BufferD3D12CudaInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyAsync(
            this->getCudaDevicePtr(), reinterpret_cast<CUdeviceptr>(devicePtrSrc),
            resource->getCopiableSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyAsync: ");
}

void BufferD3D12CudaInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyAsync(
            reinterpret_cast<CUdeviceptr>(devicePtrDst), this->getCudaDevicePtr(),
            resource->getCopiableSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyAsync: ");
}

void BufferD3D12CudaInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyHtoDAsync(
            this->getCudaDevicePtr(), hostPtrSrc,
            resource->getCopiableSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyHtoDAsync: ");
}

void BufferD3D12CudaInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyDtoHAsync(
            hostPtrDst, this->getCudaDevicePtr(),
            resource->getCopiableSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyDtoHAsync: ");
}


static CUarray_format getCudaArrayFormatFromD3D12Format(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return CU_AD_FORMAT_UNSIGNED_INT8;
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return CU_AD_FORMAT_UNSIGNED_INT16;
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return CU_AD_FORMAT_UNSIGNED_INT32;
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return CU_AD_FORMAT_SIGNED_INT8;
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return CU_AD_FORMAT_SIGNED_INT16;
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return CU_AD_FORMAT_SIGNED_INT32;

        // UNORM formats use UINT instead of FLOAT.
        case DXGI_FORMAT_R8_UNORM:
            return CU_AD_FORMAT_UNORM_INT8X1;
        case DXGI_FORMAT_R8G8_UNORM:
            return CU_AD_FORMAT_UNORM_INT8X2;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return CU_AD_FORMAT_UNORM_INT8X4;
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_D16_UNORM:
            return CU_AD_FORMAT_UNORM_INT16X1;
        case DXGI_FORMAT_R16G16_UNORM:
            return CU_AD_FORMAT_UNORM_INT16X2;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return CU_AD_FORMAT_UNORM_INT16X4;
        case DXGI_FORMAT_R8_SNORM:
            return CU_AD_FORMAT_SNORM_INT8X1;
        case DXGI_FORMAT_R8G8_SNORM:
            return CU_AD_FORMAT_SNORM_INT8X2;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return CU_AD_FORMAT_SNORM_INT8X4;
        case DXGI_FORMAT_R16_SNORM:
            return CU_AD_FORMAT_SNORM_INT16X1;
        case DXGI_FORMAT_R16G16_SNORM:
            return CU_AD_FORMAT_SNORM_INT16X2;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return CU_AD_FORMAT_SNORM_INT16X4;

        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return CU_AD_FORMAT_HALF;
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return CU_AD_FORMAT_FLOAT;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaArrayFormatFromD3D12Format: Unsupported format.");
            return CU_AD_FORMAT_FLOAT;
    }
}

void ImageD3D12CudaInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getCopiableSizeInBytes();
    externalMemoryHandleDesc = {};
    externalMemoryHandleDesc.size = sizeInBytes;
    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE;
    externalMemoryHandleDesc.handle.win32.handle = (void*)handle;
    externalMemoryHandleDesc.flags = CUDA_EXTERNAL_MEMORY_DEDICATED;

    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    CUexternalMemory cudaExternalMemoryBuffer{};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
            &cudaExternalMemoryBuffer, &externalMemoryHandleDesc);
    checkCUresult(cuResult, "Error in cuImportExternalMemory: ");
    externalMemoryBuffer = reinterpret_cast<void*>(cudaExternalMemoryBuffer);

    CUDA_ARRAY3D_DESCRIPTOR arrayDescriptor{};
    arrayDescriptor.Width = static_cast<size_t>(resourceDesc.Width);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
            || resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        arrayDescriptor.Height = static_cast<size_t>(resourceDesc.Height);
    }
    arrayDescriptor.Depth = resourceDesc.DepthOrArraySize;
    arrayDescriptor.Format = getCudaArrayFormatFromD3D12Format(resourceDesc.Format);
    arrayDescriptor.NumChannels = uint32_t(getDXGIFormatNumChannels(resourceDesc.Format));
    if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_COLOR_ATTACHMENT;
    }
    if (imageComputeApiInfo.surfaceLoadStore) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_SURFACE_LDST;
    }
    if (resourceDesc.Format == DXGI_FORMAT_D32_FLOAT || resourceDesc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT
            || resourceDesc.Format == DXGI_FORMAT_D16_UNORM) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_DEPTH_TEXTURE;
    }
    if (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D && resourceDesc.DepthOrArraySize > 1) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_LAYERED;
    }

    CUmipmappedArray cudaMipmappedArray{};
    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC externalMemoryMipmappedArrayDesc{};
    externalMemoryMipmappedArrayDesc.offset = 0;
    // CUDA_ERROR_ALREADY_MAPPED generated if numLevels == 0.
    externalMemoryMipmappedArrayDesc.numLevels = std::max(static_cast<unsigned>(resourceDesc.MipLevels), 1u);
    externalMemoryMipmappedArrayDesc.arrayDesc = arrayDescriptor;
    cuResult = g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedMipmappedArray(
            &cudaMipmappedArray, cudaExternalMemoryBuffer, &externalMemoryMipmappedArrayDesc);
    if (cuResult == CUDA_ERROR_INVALID_VALUE) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: Unsupported CUDA image type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in ImageComputeApiExternalMemoryVk::_initialize: Unsupported CUDA image type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported CUDA image type");
    } else {
        checkCUresult(cuResult, "Error in cuExternalMemoryGetMappedMipmappedArray: ");
    }
    mipmappedArray = reinterpret_cast<void*>(cudaMipmappedArray);
}

void ImageD3D12CudaInterop::free() {
    freeHandle();
    if (mipmappedArray) {
        CUmipmappedArray cudaMipmappedArray = getCudaMipmappedArray();
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMipmappedArrayDestroy(cudaMipmappedArray);
        checkCUresult(cuResult, "Error in cuMipmappedArrayDestroy: ");
        mipmappedArray = {};
    }
    if (externalMemoryBuffer) {
        auto cudaExternalMemoryBuffer = reinterpret_cast<CUexternalMemory>(externalMemoryBuffer);
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory(cudaExternalMemoryBuffer);
        checkCUresult(cuResult, "Error in cuDestroyExternalMemory: ");
        externalMemoryBuffer = {};
    }
}

ImageD3D12CudaInterop::~ImageD3D12CudaInterop() {
    ImageD3D12CudaInterop::free();
}

CUarray ImageD3D12CudaInterop::getCudaMipmappedArrayLevel(uint32_t level) {
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

void ImageD3D12CudaInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    size_t entryByteSize = getDXGIFormatSizeInBytes(resourceDesc.Format);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        CUDA_MEMCPY2D memcpySettings{};
        memcpySettings.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        memcpySettings.srcDevice = reinterpret_cast<CUdeviceptr>(devicePtrSrc);
        memcpySettings.srcPitch = static_cast<size_t>(resourceDesc.Width) * entryByteSize;

        memcpySettings.dstMemoryType = CU_MEMORYTYPE_ARRAY;
        memcpySettings.dstArray = getCudaMipmappedArrayLevel(0);

        memcpySettings.WidthInBytes = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.Height = static_cast<size_t>(resourceDesc.Height);

        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy2DAsync(&memcpySettings, stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpy2DAsync: ");
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        CUDA_MEMCPY3D memcpySettings{};
        memcpySettings.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        memcpySettings.srcDevice = reinterpret_cast<CUdeviceptr>(devicePtrSrc);
        memcpySettings.srcPitch = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.srcHeight = static_cast<size_t>(resourceDesc.Height);

        memcpySettings.dstMemoryType = CU_MEMORYTYPE_ARRAY;
        memcpySettings.dstArray = getCudaMipmappedArrayLevel(0);

        memcpySettings.WidthInBytes = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.Height = static_cast<size_t>(resourceDesc.Height);
        memcpySettings.Depth = static_cast<size_t>(resourceDesc.DepthOrArraySize);

        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy3DAsync(&memcpySettings, stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpy3DAsync: ");
    } else {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::copyFromDevicePtrAsync: "
                "Unsupported image view type.");
    }
}

void ImageD3D12CudaInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    size_t entryByteSize = getDXGIFormatSizeInBytes(resourceDesc.Format);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        CUDA_MEMCPY2D memcpySettings{};
        memcpySettings.srcMemoryType = CU_MEMORYTYPE_ARRAY;
        memcpySettings.srcArray = getCudaMipmappedArrayLevel(0);

        memcpySettings.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        memcpySettings.dstDevice = reinterpret_cast<CUdeviceptr>(devicePtrDst);
        memcpySettings.dstPitch = static_cast<size_t>(resourceDesc.Width) * entryByteSize;

        memcpySettings.WidthInBytes = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.Height = static_cast<size_t>(resourceDesc.Height);

        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy2DAsync(&memcpySettings, stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpy2DAsync: ");
    } else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        CUDA_MEMCPY3D memcpySettings{};
        memcpySettings.srcMemoryType = CU_MEMORYTYPE_ARRAY;
        memcpySettings.srcArray = getCudaMipmappedArrayLevel(0);

        memcpySettings.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        memcpySettings.dstDevice = reinterpret_cast<CUdeviceptr>(devicePtrDst);
        memcpySettings.dstPitch = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.dstHeight = static_cast<size_t>(resourceDesc.Height);

        memcpySettings.WidthInBytes = static_cast<size_t>(resourceDesc.Width) * entryByteSize;
        memcpySettings.Height = static_cast<size_t>(resourceDesc.Height);
        memcpySettings.Depth = static_cast<size_t>(resourceDesc.DepthOrArraySize);

        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy3DAsync(&memcpySettings, stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpy3DAsync: ");
    } else {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::copyToDevicePtrAsync: "
                "Unsupported image view type.");
    }
}


void UnsampledImageD3D12CudaInterop::initialize(const ImageD3D12ComputeApiExternalMemoryPtr& _image) {
    this->image = _image;

    CUDA_RESOURCE_DESC cudaResourceDesc{};
    cudaResourceDesc.resType = CU_RESOURCE_TYPE_ARRAY;
    cudaResourceDesc.res.array.hArray = getCudaMipmappedArrayLevel(0);

    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSurfObjectCreate(&cudaSurfaceObject, &cudaResourceDesc);
    checkCUresult(cuResult, "Error in cuSurfObjectCreate: ");
}

UnsampledImageD3D12CudaInterop::~UnsampledImageD3D12CudaInterop() {
    if (cudaSurfaceObject) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSurfObjectDestroy(cudaSurfaceObject);
        checkCUresult(cuResult, "Error in cuSurfObjectDestroy: ");
        cudaSurfaceObject = {};
    }
}


static CUaddress_mode getCudaSamplerAddressModeD3D12(D3D12_TEXTURE_ADDRESS_MODE samplerAddressModeD3D12) {
    switch (samplerAddressModeD3D12) {
        case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
            return CU_TR_ADDRESS_MODE_WRAP;
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR:
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE:
            return CU_TR_ADDRESS_MODE_MIRROR;
        case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
            return CU_TR_ADDRESS_MODE_CLAMP;
        case D3D12_TEXTURE_ADDRESS_MODE_BORDER:
            return CU_TR_ADDRESS_MODE_BORDER;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaSamplerAddressModeD3D12: Unsupported format.");
            return CU_TR_ADDRESS_MODE_WRAP;
    }
}

static CUresourceViewFormat getCudaResourceViewFormatD3D12(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UINT:
            return CU_RES_VIEW_FORMAT_UINT_1X8;
        case DXGI_FORMAT_R8G8_UINT:
            return CU_RES_VIEW_FORMAT_UINT_2X8;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return CU_RES_VIEW_FORMAT_UINT_4X8;
        case DXGI_FORMAT_R16_UINT:
            return CU_RES_VIEW_FORMAT_UINT_1X16;
        case DXGI_FORMAT_R32_UINT:
            return CU_RES_VIEW_FORMAT_UINT_1X32;
        case DXGI_FORMAT_R16G16_UINT:
            return CU_RES_VIEW_FORMAT_UINT_2X16;
        case DXGI_FORMAT_R32G32_UINT:
            return CU_RES_VIEW_FORMAT_UINT_2X32;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return CU_RES_VIEW_FORMAT_UINT_4X16;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return CU_RES_VIEW_FORMAT_UINT_4X32;
        case DXGI_FORMAT_R8_SINT:
            return CU_RES_VIEW_FORMAT_SINT_1X8;
        case DXGI_FORMAT_R8G8_SINT:
            return CU_RES_VIEW_FORMAT_SINT_2X8;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return CU_RES_VIEW_FORMAT_SINT_4X8;
        case DXGI_FORMAT_R16_SINT:
            return CU_RES_VIEW_FORMAT_SINT_1X16;
        case DXGI_FORMAT_R32_SINT:
            return CU_RES_VIEW_FORMAT_SINT_1X32;
        case DXGI_FORMAT_R16G16_SINT:
            return CU_RES_VIEW_FORMAT_SINT_2X16;
        case DXGI_FORMAT_R32G32_SINT:
            return CU_RES_VIEW_FORMAT_SINT_2X32;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return CU_RES_VIEW_FORMAT_SINT_4X16;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return CU_RES_VIEW_FORMAT_SINT_4X32;

        // UNORM formats use UINT instead of FLOAT.
        case DXGI_FORMAT_R8_UNORM:
            return CU_RES_VIEW_FORMAT_UINT_1X8;
        case DXGI_FORMAT_R8G8_UNORM:
            return CU_RES_VIEW_FORMAT_UINT_2X8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return CU_RES_VIEW_FORMAT_UINT_4X8;
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_D16_UNORM:
            return CU_RES_VIEW_FORMAT_UINT_1X16;
        case DXGI_FORMAT_R16G16_UNORM:
            return CU_RES_VIEW_FORMAT_UINT_2X16;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return CU_RES_VIEW_FORMAT_UINT_4X16;
        case DXGI_FORMAT_R8_SNORM:
            return CU_RES_VIEW_FORMAT_UINT_1X8;
        case DXGI_FORMAT_R8G8_SNORM:
            return CU_RES_VIEW_FORMAT_UINT_2X8;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return CU_RES_VIEW_FORMAT_UINT_4X8;
        case DXGI_FORMAT_R16_SNORM:
            return CU_RES_VIEW_FORMAT_UINT_1X16;
        case DXGI_FORMAT_R16G16_SNORM:
            return CU_RES_VIEW_FORMAT_UINT_2X16;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return CU_RES_VIEW_FORMAT_UINT_4X16;

        case DXGI_FORMAT_R16_FLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_1X16;
        case DXGI_FORMAT_R16G16_FLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_2X16;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_4X16;
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_1X32;
        case DXGI_FORMAT_R32G32_FLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_2X32;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_4X32;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaResourceViewFormatD3D12: Unsupported format.");
            return CU_RES_VIEW_FORMAT_NONE;
    }
}

void SampledImageD3D12CudaInterop::initialize(
            const ImageD3D12ComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) {
    this->image = _image;
    const auto& resourceDesc = image->getResource()->getD3D12ResourceDesc();
    const auto& imageComputeApiInfo = image->getImageComputeApiInfo();
    const auto& samplerDesc = imageComputeApiInfo.samplerDesc;

    auto imageVkSycl = std::static_pointer_cast<ImageD3D12CudaInterop>(image);

    CUDA_RESOURCE_DESC cudaResourceDesc{};
    if (textureExternalMemorySettings.useMipmappedArray) {
        cudaResourceDesc.resType = CU_RESOURCE_TYPE_MIPMAPPED_ARRAY;
        cudaResourceDesc.res.mipmap.hMipmappedArray = getCudaMipmappedArray();
    } else {
        cudaResourceDesc.resType = CU_RESOURCE_TYPE_ARRAY;
        cudaResourceDesc.res.array.hArray = getCudaMipmappedArrayLevel(0);
    }

    CUDA_TEXTURE_DESC cudaTextureDesc{};
    cudaTextureDesc.addressMode[0] = getCudaSamplerAddressModeD3D12(samplerDesc.AddressU);
    cudaTextureDesc.addressMode[1] = getCudaSamplerAddressModeD3D12(samplerDesc.AddressV);
    cudaTextureDesc.addressMode[2] = getCudaSamplerAddressModeD3D12(samplerDesc.AddressW);
    cudaTextureDesc.filterMode =
            samplerDesc.Filter == D3D12_FILTER_MIN_MAG_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR
            ? CU_TR_FILTER_MODE_POINT : CU_TR_FILTER_MODE_LINEAR;
    cudaTextureDesc.mipmapFilterMode =
            samplerDesc.Filter == D3D12_FILTER_MIN_MAG_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT
            ? CU_TR_FILTER_MODE_POINT : CU_TR_FILTER_MODE_LINEAR;
    cudaTextureDesc.mipmapLevelBias = samplerDesc.MipLODBias;
    cudaTextureDesc.maxAnisotropy = samplerDesc.MaxAnisotropy;
    cudaTextureDesc.minMipmapLevelClamp = resourceDesc.MipLevels <= 1 ? 0.0f : samplerDesc.MinLOD;
    cudaTextureDesc.maxMipmapLevelClamp = resourceDesc.MipLevels <= 1 ? 0.0f : samplerDesc.MaxLOD;
    memcpy(cudaTextureDesc.borderColor, samplerDesc.BorderColor, sizeof(float) * 4);
    if (textureExternalMemorySettings.useNormalizedCoordinates || textureExternalMemorySettings.useMipmappedArray) {
        cudaTextureDesc.flags |= CU_TRSF_NORMALIZED_COORDINATES;
    }
    if (!textureExternalMemorySettings.useTrilinearOptimization) {
        cudaTextureDesc.flags |= CU_TRSF_DISABLE_TRILINEAR_OPTIMIZATION;
    }
    if (textureExternalMemorySettings.readAsInteger) {
        cudaTextureDesc.flags |= CU_TRSF_READ_AS_INTEGER;
    }

    CUDA_RESOURCE_VIEW_DESC cudaResourceViewDesc{};
    cudaResourceViewDesc.format = getCudaResourceViewFormatD3D12(resourceDesc.Format);
    cudaResourceViewDesc.width = static_cast<size_t>(resourceDesc.Width);
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
            || resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        cudaResourceViewDesc.height = static_cast<size_t>(resourceDesc.Height);
    }
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        cudaResourceViewDesc.depth = static_cast<size_t>(resourceDesc.DepthOrArraySize);
        cudaResourceViewDesc.lastLayer = 1;
    } else {
        cudaResourceViewDesc.lastLayer = std::max(static_cast<unsigned>(resourceDesc.DepthOrArraySize), 1u);
    }
    cudaResourceViewDesc.firstMipmapLevel = 0;
    cudaResourceViewDesc.lastMipmapLevel = resourceDesc.MipLevels;
    cudaResourceViewDesc.firstLayer = 0;

    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuTexObjectCreate(
            &cudaTextureObject, &cudaResourceDesc, &cudaTextureDesc, &cudaResourceViewDesc);
    checkCUresult(cuResult, "Error in cuTexObjectCreate: ");
}

SampledImageD3D12CudaInterop::~SampledImageD3D12CudaInterop() {
    if (cudaTextureObject) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuTexObjectDestroy(cudaTextureObject);
        checkCUresult(cuResult, "Error in cuTexObjectDestroy: ");
        cudaTextureObject = {};
    }
}

}}
