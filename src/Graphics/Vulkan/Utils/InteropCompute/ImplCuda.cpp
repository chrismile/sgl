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

#include <cstring>
#include "ImplCuda.hpp"
#include "../InteropCuda.hpp"

namespace sgl {
extern bool openMessageBoxOnComputeApiError;
}

namespace sgl { namespace vk {

#ifdef _WIN32
void SemaphoreVkCudaInterop::setExternalSemaphoreWin32Handle(HANDLE handle) {
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
}
#endif

#ifdef __linux__
void SemaphoreVkCudaInterop::setExternalSemaphoreFd(int fileDescriptor) {
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
}
#endif

void SemaphoreVkCudaInterop::importExternalSemaphore() {
    CUexternalSemaphore cuExternalSemaphore{};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalSemaphore(
            &cuExternalSemaphore, &externalSemaphoreHandleDesc);
    checkCUresult(cuResult, "Error in cuImportExternalSemaphore: ");
    externalSemaphore = reinterpret_cast<void*>(cuExternalSemaphore);
}

SemaphoreVkCudaInterop::~SemaphoreVkCudaInterop() {
    if (externalSemaphore) {
        auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore(cuExternalSemaphore);
        checkCUresult(cuResult, "Error in cuDestroyExternalSemaphore: ");
        externalSemaphore = {};
    }
}

void SemaphoreVkCudaInterop::signalSemaphoreComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
    CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS signalParams{};
    if (isTimelineSemaphore()) {
        signalParams.params.fence.value = timelineValue;
    }
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSignalExternalSemaphoresAsync(
            &cuExternalSemaphore, &signalParams, 1, stream.cuStream);
    checkCUresult(cuResult, "Error in cuSignalExternalSemaphoresAsync: ");
}

void SemaphoreVkCudaInterop::waitSemaphoreComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    auto cuExternalSemaphore = reinterpret_cast<CUexternalSemaphore>(externalSemaphore);
    CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS waitParams{};
    if (isTimelineSemaphore()) {
        waitParams.params.fence.value = timelineValue;
    }
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuWaitExternalSemaphoresAsync(
            &cuExternalSemaphore, &waitParams, 1, stream.cuStream);
    checkCUresult(cuResult, "Error in cuWaitExternalSemaphoresAsync: ");
}

void BufferVkCudaInterop::preCheckExternalMemoryImport() {
    externalMemoryHandleDesc = {};
    externalMemoryHandleDesc.size = vulkanBuffer->getDeviceMemorySize(); // memoryRequirements.size
    if (vulkanBuffer->getIsDedicatedAllocation()) {
        externalMemoryHandleDesc.flags = CUDA_EXTERNAL_MEMORY_DEDICATED;
    }
}

#ifdef _WIN32
void BufferVkCudaInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
    externalMemoryHandleDesc.handle.win32.handle = (void*)handle;
}
#endif

#ifdef __linux__
void BufferVkCudaInterop::setExternalMemoryFd(int fileDescriptor) {
    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
    externalMemoryHandleDesc.handle.fd = fileDescriptor;
}
#endif

void BufferVkCudaInterop::importExternalMemory() {
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
}

void BufferVkCudaInterop::free() {
    freeHandlesAndFds();
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

BufferVkCudaInterop::~BufferVkCudaInterop() {
    BufferVkCudaInterop::free();
}

void BufferVkCudaInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyAsync(
            this->getCudaDevicePtr(), reinterpret_cast<CUdeviceptr>(devicePtrSrc),
            vulkanBuffer->getSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyAsync: ");
}

void BufferVkCudaInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyAsync(
            reinterpret_cast<CUdeviceptr>(devicePtrDst), this->getCudaDevicePtr(),
            vulkanBuffer->getSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyAsync: ");
}

void BufferVkCudaInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyHtoDAsync(
            this->getCudaDevicePtr(), hostPtrSrc,
            vulkanBuffer->getSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyHtoDAsync: ");
}

void BufferVkCudaInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpyDtoHAsync(
            hostPtrDst, this->getCudaDevicePtr(),
            vulkanBuffer->getSizeInBytes(), stream.cuStream);
    checkCUresult(cuResult, "Error in cuMemcpyDtoHAsync: ");
}


void ImageVkCudaInterop::preCheckExternalMemoryImport() {
    externalMemoryHandleDesc = {};
    externalMemoryHandleDesc.size = vulkanImage->getDeviceMemorySize(); // memoryRequirements.size
    if (vulkanImage->getIsDedicatedAllocation()) {
        externalMemoryHandleDesc.flags = CUDA_EXTERNAL_MEMORY_DEDICATED;
    }
}

#ifdef _WIN32
void ImageVkCudaInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
    externalMemoryHandleDesc.handle.win32.handle = (void*)handle;
}
#endif

#ifdef __linux__
void ImageVkCudaInterop::setExternalMemoryFd(int fileDescriptor) {
    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
    externalMemoryHandleDesc.handle.fd = fileDescriptor;
}
#endif

void ImageVkCudaInterop::importExternalMemory() {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
    CUexternalMemory cudaExternalMemoryBuffer{};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
            &cudaExternalMemoryBuffer, &externalMemoryHandleDesc);
    checkCUresult(cuResult, "Error in cuImportExternalMemory: ");
    externalMemoryBuffer = reinterpret_cast<void*>(cudaExternalMemoryBuffer);

    CUDA_ARRAY3D_DESCRIPTOR arrayDescriptor{};
    arrayDescriptor.Width = imageSettings.width;
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Height = imageSettings.height;
    }
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
        arrayDescriptor.Depth = imageSettings.depth;
    } else if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Depth = imageSettings.arrayLayers;
    }
    arrayDescriptor.Format = getCudaArrayFormatFromVkFormat(imageSettings.format);
    arrayDescriptor.NumChannels = uint32_t(getImageFormatNumChannels(imageSettings.format));
    if (imageSettings.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_COLOR_ATTACHMENT;
    }
    if (imageComputeApiInfo.surfaceLoadStore) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_SURFACE_LDST;
    }
    if (isDepthStencilFormat(imageSettings.format)) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_DEPTH_TEXTURE;
    }
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_CUBEMAP;
    }
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_LAYERED;
    }

    CUmipmappedArray cudaMipmappedArray{};
    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC externalMemoryMipmappedArrayDesc{};
    externalMemoryMipmappedArrayDesc.offset = vulkanImage->getDeviceMemoryOffset();
    externalMemoryMipmappedArrayDesc.numLevels = imageSettings.mipLevels;
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

void ImageVkCudaInterop::free() {
    freeHandlesAndFds();
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

ImageVkCudaInterop::~ImageVkCudaInterop() {
    ImageVkCudaInterop::free();
}

CUarray ImageVkCudaInterop::getCudaMipmappedArrayLevel(uint32_t level) {
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

void ImageVkCudaInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
    size_t entryByteSize = getImageFormatEntryByteSize(imageSettings.format);
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D) {
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
    } else if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
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
}

void ImageVkCudaInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
    size_t entryByteSize = getImageFormatEntryByteSize(imageSettings.format);
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D) {
        CUDA_MEMCPY2D memcpySettings{};
        memcpySettings.srcMemoryType = CU_MEMORYTYPE_ARRAY;
        memcpySettings.srcArray = getCudaMipmappedArrayLevel(0);

        memcpySettings.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        memcpySettings.dstDevice = reinterpret_cast<CUdeviceptr>(devicePtrDst);
        memcpySettings.dstPitch = imageSettings.width * entryByteSize;

        memcpySettings.WidthInBytes = imageSettings.width * entryByteSize;
        memcpySettings.Height = imageSettings.height;

        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy2DAsync(&memcpySettings, stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpy2DAsync: ");
    } else if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
        CUDA_MEMCPY3D memcpySettings{};
        memcpySettings.srcMemoryType = CU_MEMORYTYPE_ARRAY;
        memcpySettings.srcArray = getCudaMipmappedArrayLevel(0);

        memcpySettings.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        memcpySettings.dstDevice = reinterpret_cast<CUdeviceptr>(devicePtrDst);
        memcpySettings.dstPitch = imageSettings.width * entryByteSize;
        memcpySettings.dstHeight = imageSettings.height;

        memcpySettings.WidthInBytes = imageSettings.width * entryByteSize;
        memcpySettings.Height = imageSettings.height;
        memcpySettings.Depth = imageSettings.depth;

        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemcpy3DAsync(&memcpySettings, stream.cuStream);
        checkCUresult(cuResult, "Error in cuMemcpy3DAsync: ");
    } else {
        Logfile::get()->throwError(
                "Error in ImageComputeApiExternalMemoryVk::copyToDevicePtrAsync: "
                "Unsupported image view type.");
    }
}


void UnsampledImageVkCudaInterop::initialize(const ImageVkComputeApiExternalMemoryPtr& _image) {
    image = _image;

    CUDA_RESOURCE_DESC cudaResourceDesc{};
    cudaResourceDesc.resType = CU_RESOURCE_TYPE_ARRAY;
    cudaResourceDesc.res.array.hArray = getCudaMipmappedArrayLevel(0);

    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSurfObjectCreate(&cudaSurfaceObject, &cudaResourceDesc);
    checkCUresult(cuResult, "Error in cuSurfObjectCreate: ");
}

UnsampledImageVkCudaInterop::~UnsampledImageVkCudaInterop() {
    if (cudaSurfaceObject) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSurfObjectDestroy(cudaSurfaceObject);
        checkCUresult(cuResult, "Error in cuSurfObjectDestroy: ");
        cudaSurfaceObject = {};
    }
}


void SampledImageVkCudaInterop::initialize(
        const ImageVkComputeApiExternalMemoryPtr& _image,
        const TextureExternalMemorySettings& textureExternalMemorySettings) {
    image = _image;
    const auto& imageComputeApiInfo = image->getImageComputeApiInfo();
    const auto& samplerSettings = imageComputeApiInfo.imageSamplerSettings;
    const auto& vulkanImage = image->getVulkanImage();
    const auto& imageSettings = vulkanImage->getImageSettings();

    CUDA_RESOURCE_DESC cudaResourceDesc{};
    if (textureExternalMemorySettings.useMipmappedArray) {
        cudaResourceDesc.resType = CU_RESOURCE_TYPE_MIPMAPPED_ARRAY;
        cudaResourceDesc.res.mipmap.hMipmappedArray = getCudaMipmappedArray();
    } else {
        cudaResourceDesc.resType = CU_RESOURCE_TYPE_ARRAY;
        cudaResourceDesc.res.array.hArray = getCudaMipmappedArrayLevel(0);
    }

    CUDA_TEXTURE_DESC cudaTextureDesc{};
    cudaTextureDesc.addressMode[0] = getCudaSamplerAddressModeVk(samplerSettings.addressModeU);
    cudaTextureDesc.addressMode[1] = getCudaSamplerAddressModeVk(samplerSettings.addressModeV);
    cudaTextureDesc.addressMode[2] = getCudaSamplerAddressModeVk(samplerSettings.addressModeW);
    cudaTextureDesc.filterMode = getCudaFilterFormatVk(samplerSettings.minFilter);
    cudaTextureDesc.mipmapFilterMode = getCudaMipmapFilterFormatVk(samplerSettings.mipmapMode);
    cudaTextureDesc.mipmapLevelBias = samplerSettings.mipLodBias;
    uint32_t maxAnisotropy = 0;
    if (samplerSettings.anisotropyEnable) {
        if (samplerSettings.maxAnisotropy < 0.0f) {
            auto* device = vulkanImage->getDevice();
            maxAnisotropy = uint32_t(device->getPhysicalDeviceProperties().limits.maxSamplerAnisotropy);
        } else {
            maxAnisotropy = uint32_t(samplerSettings.maxAnisotropy);
        }
    }
    cudaTextureDesc.maxAnisotropy = maxAnisotropy;
    cudaTextureDesc.minMipmapLevelClamp = imageSettings.mipLevels <= 1 ? 0.0f : samplerSettings.minLod;
    cudaTextureDesc.maxMipmapLevelClamp = imageSettings.mipLevels <= 1 ? 0.0f : samplerSettings.maxLod;
    std::array<float, 4> borderColor = getCudaBorderColorVk(samplerSettings.borderColor);
    memcpy(cudaTextureDesc.borderColor, borderColor.data(), sizeof(float) * 4);
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
    cudaResourceViewDesc.format = getCudaResourceViewFormatVk(imageSettings.format);
    cudaResourceViewDesc.width = imageSettings.width;
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        cudaResourceViewDesc.height = imageSettings.height;
    }
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
        cudaResourceViewDesc.depth = imageSettings.depth;
    } else if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        cudaResourceViewDesc.depth = imageSettings.arrayLayers;
    }
    cudaResourceViewDesc.firstMipmapLevel = imageComputeApiInfo.imageSubresourceRange.baseMipLevel;
    cudaResourceViewDesc.lastMipmapLevel =
            imageSettings.mipLevels <= 1 ? 0 : imageComputeApiInfo.imageSubresourceRange.levelCount;
    cudaResourceViewDesc.firstLayer = imageComputeApiInfo.imageSubresourceRange.baseArrayLayer;
    cudaResourceViewDesc.lastLayer =
            imageSettings.arrayLayers <= 1 ? 0 : imageComputeApiInfo.imageSubresourceRange.layerCount;

    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuTexObjectCreate(
            &cudaTextureObject, &cudaResourceDesc, &cudaTextureDesc, &cudaResourceViewDesc);
    checkCUresult(cuResult, "Error in cuTexObjectCreate: ");
}

SampledImageVkCudaInterop::~SampledImageVkCudaInterop() {
    if (cudaTextureObject) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuTexObjectDestroy(cudaTextureObject);
        checkCUresult(cuResult, "Error in cuTexObjectDestroy: ");
        cudaTextureObject = {};
    }
}

}}
