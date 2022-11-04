/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#include <unordered_set>
#include <memory>
#include <cstring>

#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "../Utils/Interop.hpp"
#include "../Utils/Memory.hpp"
#include "../Utils/Status.hpp"
#include "../Buffers/Buffer.hpp"
#include "Image.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace sgl { namespace vk {

size_t getImageFormatEntryByteSize(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
            return 1;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return 4;
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6;
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8;
        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;
        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;
        case VK_FORMAT_D16_UNORM:
            return 2;
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            return 4;
        case VK_FORMAT_S8_UINT:
            return 1;
        case VK_FORMAT_D16_UNORM_S8_UINT:
            return 3;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return 5;
        default:
            return 0;
    }
}

size_t getImageFormatNumChannels(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_S8_UINT:
            return 1;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return 2;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return 3;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;
        default:
            return 0;
    }
}

Image::Image(Device* device, const ImageSettings& imageSettings) : device(device), imageSettings(imageSettings) {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.extent.width = imageSettings.width;
    imageCreateInfo.extent.height = imageSettings.height;
    imageCreateInfo.extent.depth = imageSettings.depth;
    imageCreateInfo.imageType = imageSettings.imageType;
    imageCreateInfo.mipLevels = imageSettings.mipLevels;
    imageCreateInfo.arrayLayers = imageSettings.arrayLayers;
    imageCreateInfo.format = imageSettings.format;
    imageCreateInfo.tiling = imageSettings.tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = imageSettings.usage;
    imageCreateInfo.sharingMode = imageSettings.sharingMode;
    imageCreateInfo.samples = imageSettings.numSamples;
    imageCreateInfo.flags = 0;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = imageSettings.memoryUsage;

    VmaAllocationInfo textureImageAllocationInfo{};
    VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo = {};

    if (imageSettings.exportMemory && !imageSettings.useDedicatedAllocationForExportedMemory) {
        VkExternalMemoryHandleTypeFlags handleTypes = 0;
#if defined(_WIN32)
        handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
        handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
        Logfile::get()->throwError(
                "Error in Image::Image: External memory is only supported on Linux, Android and Windows systems!");
#endif

        externalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        externalMemoryImageCreateInfo.handleTypes = handleTypes;
        imageCreateInfo.pNext = &externalMemoryImageCreateInfo;

        uint32_t memoryTypeIndex = 0;
        VkResult res = vmaFindMemoryTypeIndexForImageInfo(
                device->getAllocator(), &imageCreateInfo, &allocCreateInfo, &memoryTypeIndex);
        if (res != VK_SUCCESS) {
            Logfile::get()->throwError(
                    "Error in Image::Image: vmaFindMemoryTypeIndexForImageInfo failed ("
                    + vulkanResultToString(res) + ")!");
        }

        VmaPool pool = device->getExternalMemoryHandlePool(memoryTypeIndex, false);
        allocCreateInfo.pool = pool;
    }

    if (!imageSettings.exportMemory || !imageSettings.useDedicatedAllocationForExportedMemory) {
        if (vmaCreateImage(
                device->getAllocator(), &imageCreateInfo, &allocCreateInfo, &image,
                &imageAllocation, &textureImageAllocationInfo) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError("Image::Image: vmaCreateImage failed!");
        }

        deviceMemory = textureImageAllocationInfo.deviceMemory;
        deviceMemoryOffset = textureImageAllocationInfo.offset;
        deviceMemorySizeInBytes = textureImageAllocationInfo.size;
    }

    if (imageSettings.exportMemory && imageSettings.useDedicatedAllocationForExportedMemory) {
        VkExternalMemoryHandleTypeFlags handleTypes = 0;
#if defined(_WIN32)
        handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
        handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
        Logfile::get()->throwError(
                "Error in Image::Image: External memory is only supported on Linux, Android and Windows systems!");
#endif

        VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo = {};
        externalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        externalMemoryImageCreateInfo.handleTypes = handleTypes;
        imageCreateInfo.pNext = &externalMemoryImageCreateInfo;

        // If the memory should be exported for external use, we need to allocate the memory manually.
        if (vkCreateImage(device->getVkDevice(), &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Image::Image: Failed to create an image!");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device->getVkDevice(), image, &memoryRequirements);
        deviceMemorySizeInBytes = memoryRequirements.size;

        VkExportMemoryAllocateInfo exportMemoryAllocateInfo = {};
        exportMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportMemoryAllocateInfo.handleTypes = handleTypes;

        VkMemoryPropertyFlags memoryPropertyFlags = convertVmaMemoryUsageToVkMemoryPropertyFlags(
                imageSettings.memoryUsage);

        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = device->findMemoryTypeIndex(
                memoryRequirements.memoryTypeBits, memoryPropertyFlags);
        memoryAllocateInfo.pNext = &exportMemoryAllocateInfo;

        if (memoryAllocateInfo.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
            Logfile::get()->throwError("Error in Image::Image: No suitable memory type index found!");
        }

        if (vkAllocateMemory(device->getVkDevice(), &memoryAllocateInfo, 0, &deviceMemory) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Image::Image: Could not allocate memory!");
        }

        vkBindImageMemory(device->getVkDevice(), image, deviceMemory, 0);
    }
}

Image::Image(Device* device, const ImageSettings& imageSettings, VkImage image, bool takeImageOwnership)
    : device(device), hasImageOwnership(takeImageOwnership), imageSettings(imageSettings), image(image) {
}

Image::Image(
        Device* device, const ImageSettings& imageSettings, VkImage image,
        VmaAllocation imageAllocation, VmaAllocationInfo imageAllocationInfo)
        : device(device), imageSettings(imageSettings), image(image), imageAllocation(imageAllocation),
        imageAllocationInfo(imageAllocationInfo) {
    this->imageSettings = imageSettings;
    this->image = image;
    this->imageAllocation = imageAllocation;
    this->imageAllocationInfo = imageAllocationInfo;
    this->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

Image::~Image() {
    if (hasImageOwnership) {
        if (imageAllocation) {
            vmaDestroyImage(device->getAllocator(), image, imageAllocation);
        } else if (deviceMemory) {
            vkDestroyImage(device->getVkDevice(), image, nullptr);
            vkFreeMemory(device->getVkDevice(), deviceMemory, nullptr);
        }
#ifdef _WIN32
        if (handle != nullptr) {
            CloseHandle(handle);
            handle = nullptr;
        }
#endif
    }
}

#if defined(_WIN32)
void Image::createFromD3D12SharedResourceHandle(HANDLE resourceHandle, const ImageSettings& imageSettings) {
    this->handle = resourceHandle;
    this->imageSettings = imageSettings;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.extent.width = imageSettings.width;
    imageCreateInfo.extent.height = imageSettings.height;
    imageCreateInfo.extent.depth = imageSettings.depth;
    imageCreateInfo.imageType = imageSettings.imageType;
    imageCreateInfo.mipLevels = imageSettings.mipLevels;
    imageCreateInfo.arrayLayers = imageSettings.arrayLayers;
    imageCreateInfo.format = imageSettings.format;
    imageCreateInfo.tiling = imageSettings.tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = imageSettings.usage;
    imageCreateInfo.sharingMode = imageSettings.sharingMode;
    imageCreateInfo.samples = imageSettings.numSamples;
    imageCreateInfo.flags = 0;

    VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo = {};
    externalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    externalMemoryImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT;
    imageCreateInfo.pNext = &externalMemoryImageCreateInfo;

    if (vkCreateImage(device->getVkDevice(), &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Image::createFromD3D12SharedResourceHandle: Failed to create an image!");
    }

    auto _vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)vkGetDeviceProcAddr(
            device->getVkDevice(), "vkGetMemoryWin32HandlePropertiesKHR");
    if (!_vkGetMemoryWin32HandlePropertiesKHR) {
        Logfile::get()->throwError(
                "Error in Image::createFromD3D12SharedResourceHandle: "
                "vkGetMemoryWin32HandlePropertiesKHR was not found!");
    }

    VkMemoryWin32HandlePropertiesKHR memoryWin32HandleProperties{};
    memoryWin32HandleProperties.sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR;
    memoryWin32HandleProperties.memoryTypeBits = 0xcdcdcdcd;
    if (_vkGetMemoryWin32HandlePropertiesKHR(
            device->getVkDevice(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, resourceHandle,
            &memoryWin32HandleProperties) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Image::createFromD3D12SharedResourceHandle: "
                "Calling vkGetMemoryWin32HandlePropertiesKHR failed!");
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device->getVkDevice(), image, &memoryRequirements);
    deviceMemorySizeInBytes = memoryRequirements.size;

    /**
     * According to this code (https://github.com/krOoze/Hello_Triangle/blob/dxgi_interop/src/WSI/DxgiWsi.h), it seems
     * like there is some faulty behavior on AMD drivers, where memoryTypeBits may not be set.
     */
    if (memoryWin32HandleProperties.memoryTypeBits == 0xcdcdcdcd) {
        memoryWin32HandleProperties.memoryTypeBits = memoryRequirements.memoryTypeBits;
    } else {
        memoryWin32HandleProperties.memoryTypeBits &= memoryRequirements.memoryTypeBits;
    }

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device->getVkPhysicalDevice(), &memoryProperties);

    VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo{};
    memoryDedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    memoryDedicatedAllocateInfo.image = image;

    VkImportMemoryWin32HandleInfoKHR importMemoryWin32HandleInfo{};
    importMemoryWin32HandleInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
    importMemoryWin32HandleInfo.pNext = &memoryDedicatedAllocateInfo;
    importMemoryWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT_KHR;
    importMemoryWin32HandleInfo.handle = resourceHandle;
    importMemoryWin32HandleInfo.name = nullptr;

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = device->findMemoryTypeIndex(
            memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    memoryAllocateInfo.pNext = &importMemoryWin32HandleInfo;

    if (memoryAllocateInfo.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
        Logfile::get()->throwError(
                "Error in Image::createFromD3D12SharedResourceHandle: No suitable memory type index found!");
    }

    if (vkAllocateMemory(device->getVkDevice(), &memoryAllocateInfo, nullptr, &deviceMemory) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Image::createFromD3D12SharedResourceHandle: Could not allocate memory!");
    }

    vkBindImageMemory(device->getVkDevice(), image, deviceMemory, 0);
}
#endif

ImagePtr Image::copy(bool copyContent, VkImageAspectFlags aspectFlags) {
    ImagePtr newImage(new Image(device, imageSettings));
    if (copyContent) {
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        VkImageSubresourceLayers subresource{};
        subresource.aspectMask = aspectFlags;
        subresource.mipLevel = 0;
        subresource.baseArrayLayer = 0;
        subresource.layerCount = imageSettings.arrayLayers;

        VkImageCopy imageCopy{};
        imageCopy.srcSubresource = subresource;
        imageCopy.srcOffset = { 0, 0, 0 };
        imageCopy.dstSubresource = subresource;
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.extent = { imageSettings.width, imageSettings.height, imageSettings.depth };
        vkCmdCopyImage(
                commandBuffer, this->image, this->imageLayout, newImage->image, newImage->imageLayout,
                1, &imageCopy);
        device->endSingleTimeCommands(commandBuffer);
    }
    return newImage;
}

void Image::uploadData(VkDeviceSize sizeInBytes, void* data, bool generateMipmaps, VkCommandBuffer commandBuffer) {
    if (imageSettings.mipLevels <= 1) {
        generateMipmaps = false;
    }

    BufferPtr stagingBuffer(new Buffer(
            device, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY));

    void* stagingBufferData = stagingBuffer->mapMemory();
    memcpy(stagingBufferData, data, sizeInBytes);
    stagingBuffer->unmapMemory();

    if (generateMipmaps && (imageSettings.memoryUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0) {
        Logfile::get()->throwError(
                "Error in Image::uploadData: Generating mipmaps is requested, but "
                "VK_IMAGE_USAGE_TRANSFER_SRC_BIT is not set.");
    }

    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer);
    copyFromBuffer(stagingBuffer, commandBuffer);
    if (generateMipmaps) {
        _generateMipmaps(commandBuffer);
    } else {
        transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
    }

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::generateMipmaps(VkCommandBuffer commandBuffer) {
    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    if ((imageSettings.memoryUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0) {
        Logfile::get()->throwError(
                "Error in Image::generateMipmaps: Generating mipmaps is requested, but "
                "VK_IMAGE_USAGE_TRANSFER_SRC_BIT is not set.");
    }

    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer);
    _generateMipmaps(commandBuffer);

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::copyFromBuffer(BufferPtr& buffer, VkCommandBuffer commandBuffer) {
    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = imageSettings.arrayLayers;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = { imageSettings.width, imageSettings.height, imageSettings.depth };

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer->getVkBuffer(),
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::copyToBuffer(BufferPtr& buffer, VkCommandBuffer commandBuffer) {
    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = { imageSettings.width, imageSettings.height, imageSettings.depth };

    vkCmdCopyImageToBuffer(
            commandBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            buffer->getVkBuffer(),
            1,
            &region);

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::copyToImage(ImagePtr& destImage, VkImageAspectFlags aspectFlags, VkCommandBuffer commandBuffer) {
    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkImageSubresourceLayers subresource{};
    subresource.aspectMask = aspectFlags;
    subresource.mipLevel = 0;
    subresource.baseArrayLayer = 0;
    subresource.layerCount = imageSettings.arrayLayers;

    VkImageCopy imageCopy{};
    imageCopy.srcSubresource = subresource;
    imageCopy.srcOffset = { 0, 0, 0 };
    imageCopy.dstSubresource = subresource;
    imageCopy.dstOffset = { 0, 0, 0 };
    imageCopy.extent = { imageSettings.width, imageSettings.height, imageSettings.depth };
    vkCmdCopyImage(
            commandBuffer, this->image, this->imageLayout, destImage->image, destImage->imageLayout,
            1, &imageCopy);

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::copyToImage(
        ImagePtr& destImage, VkImageAspectFlags srcAspectFlags, VkImageAspectFlags destAspectFlags,
        VkCommandBuffer commandBuffer) {
    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkImageSubresourceLayers srcSubresource{};
    srcSubresource.aspectMask = srcAspectFlags;
    srcSubresource.mipLevel = 0;
    srcSubresource.baseArrayLayer = 0;
    srcSubresource.layerCount = imageSettings.arrayLayers;

    VkImageSubresourceLayers dstSubresource{};
    dstSubresource.aspectMask = destAspectFlags;
    dstSubresource.mipLevel = 0;
    dstSubresource.baseArrayLayer = 0;
    dstSubresource.layerCount = imageSettings.arrayLayers;

    VkImageCopy imageCopy{};
    imageCopy.srcSubresource = srcSubresource;
    imageCopy.srcOffset = { 0, 0, 0 };
    imageCopy.dstSubresource = dstSubresource;
    imageCopy.dstOffset = { 0, 0, 0 };
    imageCopy.extent = { imageSettings.width, imageSettings.height, imageSettings.depth };
    vkCmdCopyImage(
            commandBuffer, this->image, this->imageLayout, destImage->image, destImage->imageLayout,
            1, &imageCopy);

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::blit(ImagePtr& destImage, VkCommandBuffer commandBuffer) {
    // Does the device support linear filtering for blit operations?
    if (imageSettings.format != cachedFormat) {
        cachedFormat = imageSettings.format;
        vkGetPhysicalDeviceFormatProperties(
                device->getVkPhysicalDevice(), imageSettings.format, &formatProperties);
    }
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        Logfile::get()->throwError(
                "Error in Image::blit: Texture image format does not support linear blitting!");
    }

    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkImageBlit blit{};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = {
            int32_t(imageSettings.width), int32_t(imageSettings.height), int32_t(imageSettings.depth)
    };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = imageSettings.arrayLayers;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = {
            int32_t(destImage->getImageSettings().width),
            int32_t(destImage->getImageSettings().height),
            int32_t(destImage->getImageSettings().depth)
    };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = destImage->getImageSettings().arrayLayers;

    vkCmdBlitImage(commandBuffer,
                   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   destImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit,
                   VK_FILTER_LINEAR);

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::clearColor(const glm::vec4& clearColor, VkCommandBuffer commandBuffer) {
    Image::clearColor(
            0, imageSettings.mipLevels, 0, imageSettings.arrayLayers,
            clearColor, commandBuffer);
}

void Image::clearColor(
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
        const glm::vec4& clearColor, VkCommandBuffer commandBuffer) {
    bool singleTimeCommand = false;
    if (commandBuffer == VK_NULL_HANDLE) {
        singleTimeCommand = true;
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkClearColorValue clearColorValue = { { clearColor.x, clearColor.y, clearColor.z, clearColor.w } };

    VkImageSubresourceRange imageSubresourceRange;
    imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceRange.baseMipLevel = baseMipLevel;
    imageSubresourceRange.levelCount = levelCount;
    imageSubresourceRange.baseArrayLayer = baseArrayLayer;
    imageSubresourceRange.layerCount = layerCount;

    if (imageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && imageLayout != VK_IMAGE_LAYOUT_GENERAL
            && imageLayout != VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR) {
        sgl::Logfile::get()->writeError("Error in Image::clearColor: Invalid image layout for clearing.");
    }

    vkCmdClearColorImage(commandBuffer, image, imageLayout, &clearColorValue, 1, &imageSubresourceRange);

    if (singleTimeCommand) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::clearDepthStencil(
        VkImageAspectFlags aspectFlags, float clearDepth, uint32_t clearStencil, VkCommandBuffer commandBuffer) {
    Image::clearDepthStencil(
            aspectFlags, 0, imageSettings.mipLevels, 0,
            imageSettings.arrayLayers, clearDepth, clearStencil, commandBuffer);
}

void Image::clearDepthStencil(
        VkImageAspectFlags aspectFlags,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
        float clearDepth, uint32_t clearStencil, VkCommandBuffer commandBuffer) {
    bool singleTimeCommand = false;
    if (commandBuffer == VK_NULL_HANDLE) {
        singleTimeCommand = true;
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkClearDepthStencilValue clearDepthStencilValue = { clearDepth, clearStencil };

    VkImageSubresourceRange imageSubresourceRange;
    imageSubresourceRange.aspectMask = aspectFlags;
    imageSubresourceRange.baseMipLevel = baseMipLevel;
    imageSubresourceRange.levelCount = levelCount;
    imageSubresourceRange.baseArrayLayer = baseArrayLayer;
    imageSubresourceRange.layerCount = layerCount;

    if (imageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && imageLayout != VK_IMAGE_LAYOUT_GENERAL) {
        sgl::Logfile::get()->writeError("Error in Image::clearDepthStencil: Invalid image layout for clearing.");
    }

    vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, &clearDepthStencilValue, 1, &imageSubresourceRange);

    if (singleTimeCommand) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::transitionImageLayout(VkImageLayout newLayout) {
    transitionImageLayout(imageLayout, newLayout);
}

void Image::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
    transitionImageLayout(imageLayout, newLayout, commandBuffer);
    device->endSingleTimeCommands(commandBuffer);
}

void Image::transitionImageLayout(VkImageLayout newLayout, VkCommandBuffer commandBuffer) {
    transitionImageLayout(imageLayout, newLayout, commandBuffer);
}

void Image::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer) {
    Image::transitionImageLayoutSubresource(
            oldLayout, newLayout, commandBuffer,
            0, this->getImageSettings().mipLevels,
            0, this->getImageSettings().arrayLayers);
}

void Image::insertMemoryBarrier(
        VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
    Image::insertMemoryBarrierSubresource(
            commandBuffer, oldLayout, newLayout, srcStage, dstStage, srcAccessMask, dstAccessMask,
            0, this->getImageSettings().mipLevels,
            0, this->getImageSettings().arrayLayers);
}

void Image::transitionImageLayoutSubresource(
        VkImageLayout newLayout,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    Image::transitionImageLayoutSubresource(
            imageLayout, newLayout, baseMipLevel, levelCount, baseArrayLayer, layerCount);
}

void Image::transitionImageLayoutSubresource(
        VkImageLayout oldLayout, VkImageLayout newLayout,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
    Image::transitionImageLayoutSubresource(
            imageLayout, newLayout, commandBuffer, baseMipLevel, levelCount, baseArrayLayer, layerCount);
    device->endSingleTimeCommands(commandBuffer);
}

void Image::transitionImageLayoutSubresource(
        VkImageLayout newLayout, VkCommandBuffer commandBuffer,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    Image::transitionImageLayoutSubresource(
            imageLayout, newLayout, commandBuffer, baseMipLevel, levelCount, baseArrayLayer, layerCount);
}

void Image::transitionImageLayoutSubresource(
        VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = baseMipLevel;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount = layerCount;
    if (isDepthStencilFormat(imageSettings.format)) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(imageSettings.format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported
    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } else {
        Logfile::get()->throwError("Error in Image::transitionImageLayout: Unsupported old layout!");
    }

    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else {
        Logfile::get()->throwError("Error in Image::transitionImageLayout: Unsupported new layout!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0, // 0 or VK_DEPENDENCY_BY_REGION_BIT
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    this->imageLayout = newLayout;
}

void Image::insertMemoryBarrierSubresource(
        VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = this->getVkImage();
    barrier.subresourceRange.baseMipLevel = baseMipLevel;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount = layerCount;
    if (isDepthStencilFormat(this->getImageSettings().format)) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(this->getImageSettings().format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,
            0, // 0 or VK_DEPENDENCY_BY_REGION_BIT
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    this->imageLayout = newLayout;
}

void Image::_generateMipmaps(VkCommandBuffer commandBuffer) {
    // Does the device support linear filtering for blit operations?
    if (imageSettings.format != cachedFormat) {
        cachedFormat = imageSettings.format;
        vkGetPhysicalDeviceFormatProperties(
                device->getVkPhysicalDevice(), imageSettings.format, &formatProperties);
    }
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        Logfile::get()->throwError(
                "Error in Image::_generateMipmaps: Texture image format does not support linear blitting!");
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    auto mipWidth = int32_t(imageSettings.width);
    auto mipHeight = int32_t(imageSettings.height);

    for (uint32_t i = 1; i < imageSettings.mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
                commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = imageSettings.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
}

void* Image::mapMemory() {
    //if (imageSettings.tiling != VK_IMAGE_TILING_LINEAR) {
    //    Logfile::get()->throwError("Error in Image::mapMemory: Invalid tiling format for mapping.");
    //}

    void* dataPointer = nullptr;
    vmaMapMemory(device->getAllocator(), imageAllocation, &dataPointer);
    return dataPointer;
}

void Image::unmapMemory() {
    vmaUnmapMemory(device->getAllocator(), imageAllocation);
}

VkSubresourceLayout Image::getSubresourceLayout(
        VkImageAspectFlags aspectFlags, uint32_t mipLevel, uint32_t arrayLayer) const {
    VkImageSubresource imageSubresource{};
    imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresource.mipLevel = 0;
    imageSubresource.arrayLayer = 0;

    VkSubresourceLayout subresourceLayout{};
    vkGetImageSubresourceLayout(device->getVkDevice(), image, &imageSubresource, &subresourceLayout);
    return subresourceLayout;
}

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
bool Image::createGlMemoryObject(GLuint& memoryObjectGl, InteropMemoryHandle& interopMemoryHandle) {
    if (!imageSettings.exportMemory) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: An external memory object can only be created if the "
                "export memory flag was set on creation!");
        return false;
    }
    return createGlMemoryObjectFromVkDeviceMemory(
            memoryObjectGl, interopMemoryHandle,
            device->getVkDevice(), deviceMemory, deviceMemorySizeInBytes);
}
#endif



ImageView::ImageView(
        const ImagePtr& image, VkImageViewType imageViewType,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
        VkImageAspectFlags aspectFlags)
        : device(image->getDevice()), image(image), imageViewType(imageViewType) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image->getVkImage();
    viewInfo.viewType = imageViewType;
    viewInfo.format = image->getImageSettings().format;
    //viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    //viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    //viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    //viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    subresourceRange.aspectMask = aspectFlags;
    subresourceRange.baseMipLevel = baseMipLevel;
    subresourceRange.levelCount = levelCount;
    subresourceRange.baseArrayLayer = baseArrayLayer;
    subresourceRange.layerCount = layerCount;
    viewInfo.subresourceRange = subresourceRange;

    if (vkCreateImageView(device->getVkDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in ImageView::ImageView: vkCreateImageView failed!");
    }
}

ImageView::ImageView(const ImagePtr& image, VkImageViewType imageViewType, VkImageAspectFlags aspectFlags)
        : ImageView(
                image, imageViewType, 0, image->getImageSettings().mipLevels,
                0, image->getImageSettings().arrayLayers, aspectFlags) {}

ImageView::ImageView(const ImagePtr& image, VkImageAspectFlags aspectFlags)
        : ImageView(image, VkImageViewType(image->getImageSettings().imageType), aspectFlags) {}

ImageView::ImageView(
        const ImagePtr& image, VkImageView imageView, VkImageViewType imageViewType, VkImageAspectFlags aspectFlags)
        : device(image->getDevice()), image(image), imageView(imageView), imageViewType(imageViewType) {
    subresourceRange.aspectMask = aspectFlags;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = image->getImageSettings().mipLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = image->getImageSettings().arrayLayers;
}

ImageView::ImageView(
        const ImagePtr& image, VkImageView imageView, VkImageViewType imageViewType, VkImageSubresourceRange range)
        : device(image->getDevice()), image(image), imageView(imageView), imageViewType(imageViewType),
          subresourceRange(range) {
    this->imageView = imageView;
}

ImageView::~ImageView() {
    vkDestroyImageView(device->getVkDevice(), imageView, nullptr);
}

ImageViewPtr ImageView::copy(bool copyImage, bool copyContent) {
    ImagePtr newImage;
    if (copyImage) {
        newImage = image->copy(copyContent, subresourceRange.aspectMask);
    } else {
        newImage = image;
    }
    ImageViewPtr newImageView(new ImageView(
            newImage, imageViewType,
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount,
            subresourceRange.aspectMask));
    return newImageView;
}

void ImageView::clearColor(const glm::vec4& clearColor, VkCommandBuffer commandBuffer) {
    if (subresourceRange.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
        Logfile::get()->throwError("Error in ImageView::clearColor: Invalid aspect flags!");
    }
    image->clearColor(
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount,
            clearColor, commandBuffer);
}

void ImageView::clearDepthStencil(
        float clearDepth, uint32_t clearStencil, VkCommandBuffer commandBuffer) {
    if (!(subresourceRange.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
            && !(subresourceRange.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
        Logfile::get()->throwError("Error in ImageView::clearDepthStencil: Invalid aspect flags!");
    }
    image->clearDepthStencil(
            subresourceRange.aspectMask,
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount,
            clearDepth, clearStencil, commandBuffer);
}

void ImageView::transitionImageLayout(VkImageLayout newLayout) {
    image->transitionImageLayoutSubresource(
            newLayout,
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount);
}

void ImageView::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
    image->transitionImageLayoutSubresource(
            oldLayout, newLayout,
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount);
}

void ImageView::transitionImageLayout(VkImageLayout newLayout, VkCommandBuffer commandBuffer) {
    image->transitionImageLayoutSubresource(
            newLayout, commandBuffer,
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount);
}

void ImageView::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer) {
    image->transitionImageLayoutSubresource(
            oldLayout, newLayout, commandBuffer,
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount);
}

void ImageView::insertMemoryBarrier(
        VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
    image->insertMemoryBarrierSubresource(
            commandBuffer, oldLayout, newLayout, srcStage, dstStage, srcAccessMask, dstAccessMask,
            subresourceRange.baseMipLevel, subresourceRange.levelCount,
            subresourceRange.baseArrayLayer, subresourceRange.layerCount);
}


static const std::unordered_set<VkFormat> integerFormats = {
        VK_FORMAT_R8_UINT, VK_FORMAT_R8_SINT,
        VK_FORMAT_R8G8_UINT, VK_FORMAT_R8G8_SINT,
        VK_FORMAT_R8G8B8_UINT, VK_FORMAT_R8G8B8_SINT,
        VK_FORMAT_B8G8R8_UINT, VK_FORMAT_B8G8R8_SINT,
        VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_SINT,
        VK_FORMAT_B8G8R8A8_UINT, VK_FORMAT_B8G8R8A8_SINT,
        VK_FORMAT_A8B8G8R8_UINT_PACK32, VK_FORMAT_A8B8G8R8_SINT_PACK32,
        VK_FORMAT_A2R10G10B10_UINT_PACK32, VK_FORMAT_A2R10G10B10_SINT_PACK32,
        VK_FORMAT_A2B10G10R10_UINT_PACK32, VK_FORMAT_A2B10G10R10_SINT_PACK32,
        VK_FORMAT_R16_UINT, VK_FORMAT_R16_SINT,
        VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R16G16B16_SINT,
        VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_SINT,
        VK_FORMAT_R32_UINT, VK_FORMAT_R32_SINT,
        VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32_SINT,
        VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_SINT,
        VK_FORMAT_R64_UINT, VK_FORMAT_R64_SINT,
        VK_FORMAT_R64G64_UINT, VK_FORMAT_R64G64_SINT,
        VK_FORMAT_R64G64B64_UINT, VK_FORMAT_R64G64B64_SINT,
        VK_FORMAT_R64G64B64A64_UINT, VK_FORMAT_R64G64B64A64_SINT,
        VK_FORMAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT,
};

ImageSamplerSettings::ImageSamplerSettings(const ImageSettings& imageSettings) {
    auto it = integerFormats.find(imageSettings.format);
    if (it != integerFormats.end()) {
        magFilter = VK_FILTER_NEAREST;
        minFilter = VK_FILTER_NEAREST;
    }
}

ImageSampler::ImageSampler(Device* device, const ImageSamplerSettings& samplerSettings, float maxLodOverwrite)
        : device(device), imageSamplerSettings(samplerSettings) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = samplerSettings.magFilter;
    samplerInfo.minFilter = samplerSettings.minFilter;

    samplerInfo.addressModeU = samplerSettings.addressModeU;
    samplerInfo.addressModeV = samplerSettings.addressModeV;
    samplerInfo.addressModeW = samplerSettings.addressModeW;

    if (samplerSettings.anisotropyEnable && device->getPhysicalDeviceFeatures().samplerAnisotropy) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        if (samplerInfo.maxAnisotropy < 0.0f) {
            samplerInfo.maxAnisotropy = device->getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
        } else {
            samplerInfo.maxAnisotropy = samplerSettings.maxAnisotropy;
        }
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }
    samplerInfo.borderColor = samplerSettings.borderColor;
    samplerInfo.unnormalizedCoordinates = samplerSettings.unnormalizedCoordinates;

    samplerInfo.compareEnable = samplerSettings.compareEnable;
    samplerInfo.compareOp = samplerSettings.compareOp;

    samplerInfo.mipmapMode = samplerSettings.mipmapMode;
    samplerInfo.mipLodBias = samplerSettings.mipLodBias;
    samplerInfo.minLod = samplerSettings.minLod;
    if (maxLodOverwrite < 0.0f) {
        samplerInfo.maxLod = maxLodOverwrite;
    } else {
        samplerInfo.maxLod = samplerSettings.maxLod;
    }

    if (vkCreateSampler(device->getVkDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in ImageSampler::ImageSampler: vkCreateSampler!");
    }
}

ImageSampler::ImageSampler(Device* device, const ImageSamplerSettings& samplerSettings, ImagePtr image)
        : ImageSampler(
                device, samplerSettings, float(
                image->getImageSettings().mipLevels <= 1 ? 0 : image->getImageSettings().mipLevels)) {
}

ImageSampler::~ImageSampler() {
    vkDestroySampler(device->getVkDevice(), sampler, nullptr);
}

Texture::Texture(const ImageViewPtr& imageView, const ImageSamplerPtr& imageSampler)
        : imageView(imageView), imageSampler(imageSampler) {
}

Texture::Texture(Device* device, const ImageSettings& imageSettings, VkImageAspectFlags aspectFlags) {
    vk::ImagePtr image(new Image(device, imageSettings));
    imageView = std::make_shared<ImageView>(image, aspectFlags);
    imageSampler = std::make_shared<ImageSampler>(device, ImageSamplerSettings(imageSettings), image);
}

Texture::Texture(
        Device* device, const ImageSettings& imageSettings, VkImageViewType imageViewType,
        VkImageAspectFlags aspectFlags) {
    vk::ImagePtr image(new Image(device, imageSettings));
    imageView = std::make_shared<ImageView>(image, imageViewType, aspectFlags);
    imageSampler = std::make_shared<ImageSampler>(device, ImageSamplerSettings(imageSettings), image);
}

Texture::Texture(const ImageViewPtr& imageView) : imageView(imageView) {
    imageSampler = std::make_shared<ImageSampler>(
            imageView->getDevice(), ImageSamplerSettings(imageView->getImage()->getImageSettings()),
            imageView->getImage());
}

Texture::Texture(
        Device* device, const ImageSettings& imageSettings, const ImageSamplerSettings& samplerSettings,
        VkImageAspectFlags aspectFlags) {
    vk::ImagePtr image(new Image(device, imageSettings));
    imageView = std::make_shared<ImageView>(image, aspectFlags);
    imageSampler = std::make_shared<ImageSampler>(device, samplerSettings, image);
}

Texture::Texture(
        Device* device, const ImageSettings& imageSettings, VkImageViewType imageViewType,
        const ImageSamplerSettings& samplerSettings, VkImageAspectFlags aspectFlags) {
    vk::ImagePtr image(new Image(device, imageSettings));
    imageView = std::make_shared<ImageView>(image, imageViewType, aspectFlags);
    imageSampler = std::make_shared<ImageSampler>(device, samplerSettings, image);
}

Texture::Texture(const ImageViewPtr& imageView, const ImageSamplerSettings& samplerSettings) : imageView(imageView) {
    imageSampler = std::make_shared<ImageSampler>(imageView->getDevice(), samplerSettings, imageView->getImage());
}

}}
