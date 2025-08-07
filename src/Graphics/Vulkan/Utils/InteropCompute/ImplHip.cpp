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

#include "ImplHip.hpp"

namespace sgl { namespace vk {

#ifdef _WIN32
void SemaphoreVkHipInterop::setExternalSemaphoreWin32Handle(HANDLE handle) {
    if (isTimelineSemaphore()) {
        externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeTimelineSemaphoreWin32;
    } else {
        externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeOpaqueWin32;
    }
    externalSemaphoreHandleDescHip.handle.win32.handle = handle;
}
#endif

#ifdef __linux__
void SemaphoreVkHipInterop::setExternalSemaphoreFd(int fd) {
    if (isTimelineSemaphore()) {
        externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    } else {
        externalSemaphoreHandleDescHip.type = hipExternalSemaphoreHandleTypeOpaqueFd;
    }
    externalSemaphoreHandleDescHip.handle.fd = fileDescriptor;
}
#endif

void SemaphoreVkHipInterop::importExternalSemaphore() {
    hipExternalSemaphore_t hipExternalSemaphore{};
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipImportExternalSemaphore(
            &hipExternalSemaphore, &externalSemaphoreHandleDescHip);
    checkHipResult(hipResult, "Error in hipImportExternalSemaphore: ");
    externalSemaphore = reinterpret_cast<void*>(hipExternalSemaphore);
}

SemaphoreVkHipInterop::~SemaphoreVkHipInterop() {
    auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalSemaphore(hipExternalSemaphore);
    checkHipResult(hipResult, "Error in hipDestroyExternalSemaphore: ");
}

void SemaphoreVkHipInterop::signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
    auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
    hipExternalSemaphoreSignalParams signalParams{};
    if (isTimelineSemaphore()) {
        signalParams.params.fence.value = timelineValue;
    }
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipSignalExternalSemaphoresAsync(
            &hipExternalSemaphore, &signalParams, 1, stream.hipStream);
    checkHipResult(hipResult, "Error in hipSignalExternalSemaphoresAsync: ");
}

void SemaphoreVkHipInterop::waitSemaphoreComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
    auto hipExternalSemaphore = reinterpret_cast<hipExternalSemaphore_t>(externalSemaphore);
    hipExternalSemaphoreWaitParams waitParams{};
    if (isTimelineSemaphore()) {
        waitParams.params.fence.value = timelineValue;
    }
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipWaitExternalSemaphoresAsync(
            &hipExternalSemaphore, &waitParams, 1, stream.hipStream);
    checkHipResult(hipResult, "Error in hipWaitExternalSemaphoresAsync: ");
}


void BufferVkHipInterop::preCheckExternalMemoryImport() {
    externalMemoryHandleDescHip = {};
    externalMemoryHandleDescHip.size = vulkanBuffer->getDeviceMemorySize(); // memoryRequirements.size
}

#ifdef _WIN32
void BufferVkHipInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeOpaqueWin32;
    externalMemoryHandleDescHip.handle.win32.handle = (void*)handle;
}
#endif

#ifdef __linux__
void BufferVkHipInterop::setExternalMemoryFd(int fd) {
    externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeOpaqueFd;
    externalMemoryHandleDescHip.handle.fd = fileDescriptor;
}
#endif

void BufferVkHipInterop::importExternalMemory() {
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
}

void BufferVkHipInterop::free() {
    freeHandlesAndFds();
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

BufferVkHipInterop::~BufferVkHipInterop() {
    BufferVkHipInterop::free();
}

void BufferVkHipInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyAsync(
            this->getHipDevicePtr(), reinterpret_cast<hipDeviceptr_t>(devicePtrSrc),
            vulkanBuffer->getSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyAsync: ");
}

void BufferVkHipInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyAsync(
            reinterpret_cast<hipDeviceptr_t>(devicePtrDst), this->getHipDevicePtr(),
            vulkanBuffer->getSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyAsync: ");
}

void BufferVkHipInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyHtoDAsync(
            this->getHipDevicePtr(), hostPtrSrc,
            vulkanBuffer->getSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyHtoDAsync: ");
}

void BufferVkHipInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpyDtoHAsync(
            hostPtrDst, this->getHipDevicePtr(),
            vulkanBuffer->getSizeInBytes(), stream.hipStream);
    checkHipResult(hipResult, "Error in hipMemcpyDtoHAsync: ");
}


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

void ImageVkHipInterop::preCheckExternalMemoryImport() {
    externalMemoryHandleDescHip = {};
    externalMemoryHandleDescHip.size = vulkanImage->getDeviceMemorySize(); // memoryRequirements.size
}

#ifdef _WIN32
void ImageVkHipInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeOpaqueWin32;
    externalMemoryHandleDescHip.handle.win32.handle = (void*)handle;
}
#endif

#ifdef __linux__
void ImageVkHipInterop::setExternalMemoryFd(int fd) {
    externalMemoryHandleDescHip.type = hipExternalMemoryHandleTypeOpaqueFd;
    externalMemoryHandleDescHip.handle.fd = fileDescriptor;
}
#endif

void ImageVkHipInterop::importExternalMemory() {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
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
}

void ImageVkHipInterop::free() {
    freeHandlesAndFds();
    if (externalMemoryBuffer) {
        hipMipmappedArray_t hipMipmappedArray = getHipMipmappedArray();
        auto hipResult = g_hipDeviceApiFunctionTable.hipMipmappedArrayDestroy(hipMipmappedArray);
        checkHipResult(hipResult, "Error in hipMipmappedArrayDestroy: ");
        auto hipExternalMemory = reinterpret_cast<hipExternalMemory_t>(externalMemoryBuffer);
        hipResult = g_hipDeviceApiFunctionTable.hipDestroyExternalMemory(hipExternalMemory);
        checkHipResult(hipResult, "Error in hipDestroyExternalMemory: ");
        externalMemoryBuffer = {};
    }
}

ImageVkHipInterop::~ImageVkHipInterop() {
    ImageVkHipInterop::free();
}

hipArray_t ImageVkHipInterop::getHipMipmappedArrayLevel(uint32_t level) {
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

void ImageVkHipInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
    size_t entryByteSize = getImageFormatEntryByteSize(imageSettings.format);
    if (imageViewType == VK_IMAGE_VIEW_TYPE_2D) {
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipMemcpy2DToArrayAsync(
                getHipMipmappedArrayLevel(0), 0, 0, devicePtrSrc, imageSettings.width * entryByteSize,
                imageSettings.width, imageSettings.height, hipMemcpyDeviceToDevice, stream.hipStream);
        checkHipResult(hipResult, "Error in hipMemcpy2DToArrayAsync: ");
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
}

}}
