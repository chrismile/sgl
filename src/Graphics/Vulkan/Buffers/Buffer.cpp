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

#include <memory>
#include <stdexcept>
#include <cstring>

#ifdef __APPLE__
// Needs to be included before volk.h such that the functions are defined.
#include <vulkan/vulkan_metal.h>
#endif

#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "../Utils/Interop.hpp"
#include "../Utils/Memory.hpp"
#include "../Utils/Status.hpp"
#include "Buffer.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace sgl { namespace vk {

Buffer::Buffer(
        Device* device, size_t sizeInBytes, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool queueExclusive,
        bool exportMemory, bool useDedicatedAllocationForExportedMemory) : Buffer(device, BufferSettings{
            sizeInBytes, usage, memoryUsage,
            queueExclusive ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT, 0, nullptr,
            exportMemory, useDedicatedAllocationForExportedMemory}) {}

Buffer::Buffer(Device* device, const BufferSettings& bufferSettings)
        : device(device), sizeInBytes(bufferSettings.sizeInBytes), bufferUsageFlags(bufferSettings.usage),
          memoryUsage(bufferSettings.memoryUsage),
          queueExclusive(bufferSettings.sharingMode == VK_SHARING_MODE_EXCLUSIVE),
          exportMemory(bufferSettings.exportMemory) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeInBytes;
    bufferCreateInfo.usage = bufferUsageFlags;
    bufferCreateInfo.sharingMode = queueExclusive ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    if (bufferSettings.sharingMode == VK_SHARING_MODE_CONCURRENT) {
        uint32_t queueFamilyIndexCount = bufferSettings.queueFamilyIndexCount;
        const uint32_t* pQueueFamilyIndices = bufferSettings.pQueueFamilyIndices;
        bufferCreateInfo.queueFamilyIndexCount = queueFamilyIndexCount;
        bufferCreateInfo.pQueueFamilyIndices = pQueueFamilyIndices;
    }

    VkExternalMemoryBufferCreateInfo externalMemoryBufferCreateInfo{};

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = memoryUsage;

    VkExternalMemoryHandleTypeFlags handleTypes = 0;
    bool useDedicatedAllocationForExportedMemory = bufferSettings.useDedicatedAllocationForExportedMemory;
    bool needsDedicatedAllocation = false;
    if (exportMemory) {
#if defined(_WIN32)
        handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
        handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
        Logfile::get()->throwError(
                "Error in Buffer::Buffer: External memory is only supported on Linux, Android and Windows systems!");
#endif
        externalMemoryBufferCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
        externalMemoryBufferCreateInfo.handleTypes = handleTypes;
        bufferCreateInfo.pNext = &externalMemoryBufferCreateInfo;

        needsDedicatedAllocation = device->getNeedsDedicatedAllocationForExternalMemoryBuffer(
                bufferUsageFlags, 0, VkExternalMemoryHandleTypeFlagBits(handleTypes));
        isDedicatedAllocation = needsDedicatedAllocation;
        if (needsDedicatedAllocation && !useDedicatedAllocationForExportedMemory) {
            sgl::Logfile::get()->writeWarning(
                    "Warning in Buffer::Buffer: External memory allocation without a dedicated allocation was "
                    "requested on a system only supporting external memory with dedicated allocations. Switching to "
                    "dedicated allocation.");
            useDedicatedAllocationForExportedMemory = true;
        }
    }

    if (exportMemory && !useDedicatedAllocationForExportedMemory) {
        uint32_t memoryTypeIndex = 0;
        VkResult res = vmaFindMemoryTypeIndexForBufferInfo(
                device->getAllocator(), &bufferCreateInfo, &allocCreateInfo, &memoryTypeIndex);
        if (res != VK_SUCCESS) {
            Logfile::get()->throwError(
                    "Error in Buffer::Buffer: vmaFindMemoryTypeIndexForBufferInfo failed ("
                    + vulkanResultToString(res) + ")!");
        }

        VmaPool pool = device->getExternalMemoryHandlePool(memoryTypeIndex, true);
        allocCreateInfo.pool = pool;
    }

    if (!exportMemory || !useDedicatedAllocationForExportedMemory) {
        if (vmaCreateBuffer(
                device->getAllocator(), &bufferCreateInfo, &allocCreateInfo,
                &buffer, &bufferAllocation, &bufferAllocationInfo) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Buffer::Buffer: Failed to create a buffer of the specified size!");
        }

        deviceMemory = bufferAllocationInfo.deviceMemory;
        deviceMemoryOffset = bufferAllocationInfo.offset;
        // The allocation info size is just the size of this allocation.
        deviceMemoryAllocationSize = bufferAllocationInfo.size;
        if (exportMemory) {
            deviceMemorySize = device->getVmaDeviceMemoryAllocationSize(deviceMemory);
        } else {
            deviceMemorySize = bufferAllocationInfo.size;
        }
    }

    if (exportMemory && useDedicatedAllocationForExportedMemory) {
        // If the memory should be exported for external use, we need to allocate the memory manually.
        if (vkCreateBuffer(device->getVkDevice(), &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Buffer::Buffer: Failed to create a buffer!");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device->getVkDevice(), buffer, &memoryRequirements);
        deviceMemoryAllocationSize = memoryRequirements.size;
        deviceMemorySize = memoryRequirements.size;

        VkExportMemoryAllocateInfo exportMemoryAllocateInfo{};
        exportMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportMemoryAllocateInfo.handleTypes = handleTypes;

        // Pass the dedicated allocate info to the pNext chain if necessary.
        VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo{};
        if (needsDedicatedAllocation) {
            memoryDedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
            memoryDedicatedAllocateInfo.buffer = buffer;
            exportMemoryAllocateInfo.pNext = &memoryDedicatedAllocateInfo;
        }

        VkMemoryPropertyFlags memoryPropertyFlags = convertVmaMemoryUsageToVkMemoryPropertyFlags(memoryUsage);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = device->findMemoryTypeIndex(
                memoryRequirements.memoryTypeBits, memoryPropertyFlags);

        VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
        if ((bufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0) {
            memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            memoryAllocateFlagsInfo.pNext = &exportMemoryAllocateInfo;
            memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
        } else {
            memoryAllocateInfo.pNext = &exportMemoryAllocateInfo;
        }

        if (memoryAllocateInfo.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
            Logfile::get()->throwError("Error in Buffer::Buffer: No suitable memory type index found!");
        }

        if (vkAllocateMemory(device->getVkDevice(), &memoryAllocateInfo, nullptr, &deviceMemory) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Buffer::Buffer: Could not allocate memory!");
        }

        vkBindBufferMemory(device->getVkDevice(), buffer, deviceMemory, 0);
    }
}

Buffer::Buffer(
        Device* device, size_t sizeInBytes, const void* dataPtr, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
        bool queueExclusive, bool exportMemory, bool useDedicatedAllocationForExportedMemory)
        : Buffer(
                device, sizeInBytes, usage, memoryUsage, queueExclusive, exportMemory,
                useDedicatedAllocationForExportedMemory) {
    // Upload in chunks of max. 2GB to avoid overly large staging buffers.
    uploadDataChunked(sizeInBytes, size_t(1) << size_t(31), dataPtr);
}

Buffer::~Buffer() {
    if (bufferAllocation) {
        vmaDestroyBuffer(device->getAllocator(), buffer, bufferAllocation);
    } else if (deviceMemory) {
        vkDestroyBuffer(device->getVkDevice(), buffer, nullptr);
        vkFreeMemory(device->getVkDevice(), deviceMemory, nullptr);
    }
#ifdef _WIN32
    if (handle != nullptr) {
        CloseHandle(handle);
        handle = nullptr;
    }
#endif
}

#if defined(_WIN32)
void Buffer::createFromD3D12SharedResourceHandle(
        HANDLE resourceHandle, size_t sizeInBytesData, VkBufferUsageFlags usage) {
    this->handle = resourceHandle;
    this->exportMemory = true;
    this->sizeInBytes = sizeInBytesData;
    this->bufferUsageFlags = usage;

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeInBytes;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkExternalMemoryBufferCreateInfo externalMemoryBufferCreateInfo{};
    externalMemoryBufferCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    externalMemoryBufferCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT;
    bufferCreateInfo.pNext = &externalMemoryBufferCreateInfo;

    if (vkCreateBuffer(device->getVkDevice(), &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Buffer::createFromD3D12SharedResourceHandle: Failed to create a buffer!");
    }

    auto _vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)vkGetDeviceProcAddr(
            device->getVkDevice(), "vkGetMemoryWin32HandlePropertiesKHR");
    if (!_vkGetMemoryWin32HandlePropertiesKHR) {
        Logfile::get()->throwError(
                "Error in Buffer::createFromD3D12SharedResourceHandle: "
                "vkGetMemoryWin32HandlePropertiesKHR was not found!");
    }

    VkMemoryWin32HandlePropertiesKHR memoryWin32HandleProperties{};
    memoryWin32HandleProperties.sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR;
    memoryWin32HandleProperties.memoryTypeBits = 0xcdcdcdcd;
    if (_vkGetMemoryWin32HandlePropertiesKHR(
            device->getVkDevice(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, resourceHandle,
            &memoryWin32HandleProperties) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Buffer::createFromD3D12SharedResourceHandle: "
                "Calling vkGetMemoryWin32HandlePropertiesKHR failed!");
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device->getVkDevice(), buffer, &memoryRequirements);

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
    memoryDedicatedAllocateInfo.buffer = buffer;

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
                "Error in Buffer::createFromD3D12SharedResourceHandle: No suitable memory type index found!");
    }

    if (vkAllocateMemory(device->getVkDevice(), &memoryAllocateInfo, nullptr, &deviceMemory) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Buffer::createFromD3D12SharedResourceHandle: Could not allocate memory!");
    }

    vkBindBufferMemory(device->getVkDevice(), buffer, deviceMemory, 0);
}
#endif

#ifdef __APPLE__
MTLBuffer_id Buffer::getMetalBufferId() {
    VkExportMetalBufferInfoEXT metalBufferInfo{};
    metalBufferInfo.sType = VK_STRUCTURE_TYPE_EXPORT_METAL_BUFFER_INFO_EXT;
    metalBufferInfo.memory = deviceMemory;
    VkExportMetalObjectsInfoEXT metalObjectsInfo{};
    metalObjectsInfo.sType = VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT;
    metalObjectsInfo.pNext = &metalBufferInfo;
    vkExportMetalObjectsEXT(device->getVkDevice(), &metalObjectsInfo);
    return metalBufferInfo.mtlBuffer;
}
#endif

BufferPtr Buffer::copy(bool copyContent) {
    BufferPtr newBuffer(new Buffer(device, sizeInBytes, bufferUsageFlags, memoryUsage, queueExclusive));
    if (copyContent) {
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        VkBufferCopy bufferCopy{};
        bufferCopy.size = sizeInBytes;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;
        vkCmdCopyBuffer(commandBuffer, this->buffer, newBuffer->buffer, 1, &bufferCopy);
        device->endSingleTimeCommands(commandBuffer);
    }
    return newBuffer;
}

void Buffer::uploadData(size_t sizeInBytesData, const void* dataPtr) {
    if (sizeInBytesData > sizeInBytes) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: sizeInBytesData > sizeInBytes");
    }

    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU
        || memoryUsage == VMA_MEMORY_USAGE_CPU_COPY) {
        void* mappedData = mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        unmapMemory();
    } else {
        if ((bufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
            sgl::Logfile::get()->throwError(
                    "Error in Buffer::uploadData: Buffer usage flag VK_BUFFER_USAGE_TRANSFER_DST_BIT not set!");
        }

        BufferPtr stagingBuffer(new Buffer(
                device, sizeInBytesData,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                queueExclusive));
        void* mappedData = stagingBuffer->mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        stagingBuffer->unmapMemory();

        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

        VkBufferCopy bufferCopy{};
        bufferCopy.size = sizeInBytesData;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;
        vkCmdCopyBuffer(
                commandBuffer, stagingBuffer->getVkBuffer(), this->getVkBuffer(), 1, &bufferCopy);

        device->endSingleTimeCommands(commandBuffer);
    }
}

void Buffer::uploadDataChunked(size_t sizeInBytesData, size_t chunkSize, const void *dataPtr) {
    if (sizeInBytesData > sizeInBytes) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadDataChunked: sizeInBytesData > sizeInBytes");
    }

    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU
            || memoryUsage == VMA_MEMORY_USAGE_CPU_COPY) {
        void* mappedData = mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        unmapMemory();
    } else {
        if ((bufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
            sgl::Logfile::get()->throwError(
                    "Error in Buffer::uploadDataChunked: Buffer usage flag VK_BUFFER_USAGE_TRANSFER_DST_BIT not set!");
        }

        size_t chunkSizeReal = std::min(chunkSize, sizeInBytesData);
        BufferPtr stagingBuffer(new Buffer(
                device, chunkSizeReal, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, queueExclusive));

        size_t sizeLeft = sizeInBytesData;
        size_t writeOffset = 0;
        void* mappedData = stagingBuffer->mapMemory();
        while (sizeLeft > 0) {
            size_t copySize = std::min(chunkSizeReal, sizeLeft);
            memcpy(mappedData, dataPtr, copySize);

            VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
            VkBufferCopy bufferCopy{};
            bufferCopy.size = copySize;
            bufferCopy.srcOffset = 0;
            bufferCopy.dstOffset = writeOffset;
            vkCmdCopyBuffer(
                    commandBuffer, stagingBuffer->getVkBuffer(), this->getVkBuffer(), 1, &bufferCopy);
            device->endSingleTimeCommands(commandBuffer);

            if (sizeLeft <= chunkSizeReal) {
                sizeLeft = 0;
            }  else {
                sizeLeft -= chunkSizeReal;
            }
            writeOffset += chunkSizeReal;
        }
        stagingBuffer->unmapMemory();
    }
}

void Buffer::uploadData(size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer) {
    if (sizeInBytesData > sizeInBytes) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: sizeInBytesData > sizeInBytes");
    }

    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU
            || memoryUsage == VMA_MEMORY_USAGE_CPU_COPY) {
        void* mappedData = mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        unmapMemory();
    } else {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: The version of uploadData with four parameters needs to be called in "
                "order to save the staging buffer when using a custom command buffer in combination with "
                "VMA_MEMORY_USAGE_GPU_ONLY buffers!");
    }
}

void Buffer::uploadData(
        size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer, BufferPtr& stagingBuffer) {
    if (sizeInBytesData > sizeInBytes) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: sizeInBytesData > sizeInBytes");
    }

    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU
            || memoryUsage == VMA_MEMORY_USAGE_CPU_COPY) {
        void* mappedData = mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        unmapMemory();
    } else {
        if ((bufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
            sgl::Logfile::get()->throwError(
                    "Error in Buffer::uploadData: Buffer usage flag VK_BUFFER_USAGE_TRANSFER_DST_BIT not set!");
        }

        stagingBuffer = std::make_shared<Buffer>(
                device, sizeInBytesData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                queueExclusive);
        void* mappedData = stagingBuffer->mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        stagingBuffer->unmapMemory();

        VkBufferCopy bufferCopy{};
        bufferCopy.size = sizeInBytesData;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;
        vkCmdCopyBuffer(
                commandBuffer, stagingBuffer->getVkBuffer(), this->getVkBuffer(), 1, &bufferCopy);
    }
}

void Buffer::uploadDataOffset(size_t regionOffset, size_t sizeInBytesData, const void* dataPtr) {
    if (sizeInBytesData > sizeInBytes) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: sizeInBytesData > sizeInBytes");
    }

    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU
            || memoryUsage == VMA_MEMORY_USAGE_CPU_COPY) {
        void* mappedData = mapMemory();
        uint8_t* dstRegion = reinterpret_cast<uint8_t*>(mappedData) + regionOffset;
        memcpy(dstRegion, dataPtr, sizeInBytesData);
        unmapMemory();
    } else {
        if ((bufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
            sgl::Logfile::get()->throwError(
                    "Error in Buffer::uploadData: Buffer usage flag VK_BUFFER_USAGE_TRANSFER_DST_BIT not set!");
        }

        BufferPtr stagingBuffer(new Buffer(
                device, sizeInBytesData,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                queueExclusive));
        void* mappedData = stagingBuffer->mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        stagingBuffer->unmapMemory();

        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

        VkBufferCopy bufferCopy{};
        bufferCopy.size = sizeInBytesData;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = regionOffset;
        vkCmdCopyBuffer(
                commandBuffer, stagingBuffer->getVkBuffer(), this->getVkBuffer(), 1, &bufferCopy);

        device->endSingleTimeCommands(commandBuffer);
    }
}

void Buffer::uploadDataOffset(
        size_t regionOffset, size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer) {
    if (sizeInBytesData > sizeInBytes) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: sizeInBytesData > sizeInBytes");
    }

    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU
            || memoryUsage == VMA_MEMORY_USAGE_CPU_COPY) {
        void* mappedData = mapMemory();
        uint8_t* dstRegion = reinterpret_cast<uint8_t*>(mappedData) + regionOffset;
        memcpy(dstRegion, dataPtr, sizeInBytesData);
        unmapMemory();
    } else {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: The version of uploadData with four parameters needs to be called in "
                "order to save the staging buffer when using a custom command buffer in combination with "
                "VMA_MEMORY_USAGE_GPU_ONLY buffers!");
    }
}

void Buffer::uploadDataOffset(
        size_t regionOffset, size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer,
        BufferPtr& stagingBuffer) {
    if (sizeInBytesData > sizeInBytes) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::uploadData: sizeInBytesData > sizeInBytes");
    }

    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU
            || memoryUsage == VMA_MEMORY_USAGE_CPU_COPY) {
        void* mappedData = mapMemory();
        uint8_t* dstRegion = reinterpret_cast<uint8_t*>(mappedData) + regionOffset;
        memcpy(dstRegion, dataPtr, sizeInBytesData);
        unmapMemory();
    } else {
        if ((bufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
            sgl::Logfile::get()->throwError(
                    "Error in Buffer::uploadData: Buffer usage flag VK_BUFFER_USAGE_TRANSFER_DST_BIT not set!");
        }

        stagingBuffer = std::make_shared<Buffer>(
                device, sizeInBytesData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                queueExclusive);
        void* mappedData = stagingBuffer->mapMemory();
        memcpy(mappedData, dataPtr, sizeInBytesData);
        stagingBuffer->unmapMemory();

        VkBufferCopy bufferCopy{};
        bufferCopy.size = sizeInBytesData;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = regionOffset;
        vkCmdCopyBuffer(
                commandBuffer, stagingBuffer->getVkBuffer(), this->getVkBuffer(), 1, &bufferCopy);
    }
}

void Buffer::updateData(size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer) {
    updateData(0, sizeInBytesData, dataPtr, commandBuffer);
}

void Buffer::copyDataTo(const BufferPtr& destinationBuffer, VkCommandBuffer commandBuffer) {
    copyDataTo(destinationBuffer, 0, 0, getSizeInBytes(), commandBuffer);
}

void Buffer::copyDataTo(
        const BufferPtr& destinationBuffer, VkDeviceSize sourceOffset, VkDeviceSize destOffset,
        VkDeviceSize copySizeInBytes, VkCommandBuffer commandBuffer) {
    if (sourceOffset + copySizeInBytes > destOffset + destinationBuffer->getSizeInBytes()) {
        Logfile::get()->throwError(
                "Error in Buffer::copyDataTo: The destination buffer is not large enough to hold the copied data!");
    }

    VkBufferCopy bufferCopy{};
    bufferCopy.size = copySizeInBytes;
    bufferCopy.srcOffset = sourceOffset;
    bufferCopy.dstOffset = destOffset;
    vkCmdCopyBuffer(
            commandBuffer, this->getVkBuffer(), destinationBuffer->getVkBuffer(), 1, &bufferCopy);
}

void Buffer::updateData(size_t offset, size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer) {
    if (sizeInBytesData > 65536) {
        Logfile::get()->throwError(
                "Error in Buffer::uploadDataAsync: vkCmdUpdateBuffer only supports transferring up to 65536 "
                "bytes of data.");
    }
    vkCmdUpdateBuffer(commandBuffer, buffer, offset, sizeInBytesData, dataPtr);
}

void Buffer::fill(uint32_t data, VkCommandBuffer commandBuffer) {
    fill(0, VK_WHOLE_SIZE, data, commandBuffer);
}

void Buffer::fill(VkDeviceSize offset, VkDeviceSize size, uint32_t data, VkCommandBuffer commandBuffer) {
    vkCmdFillBuffer(commandBuffer, buffer, offset, size, data);
}

void* Buffer::mapMemory() {
    if (memoryUsage != VMA_MEMORY_USAGE_CPU_ONLY && memoryUsage != VMA_MEMORY_USAGE_CPU_TO_GPU
            && memoryUsage != VMA_MEMORY_USAGE_GPU_TO_CPU && memoryUsage != VMA_MEMORY_USAGE_CPU_COPY) {
        sgl::Logfile::get()->throwError(
                "Error in Buffer::mapMemory: The memory is not mappable to a host-accessible address!");
        return nullptr;
    }

    void* dataPointer = nullptr;
    if (bufferAllocation) {
        vmaMapMemory(device->getAllocator(), bufferAllocation, &dataPointer);
    } else if (deviceMemory) {
        VkResult result = vkMapMemory(
                device->getVkDevice(), deviceMemory, deviceMemoryOffset, sizeInBytes, 0, &dataPointer);
        if (result != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    "Error in Buffer::mapMemory: vkMapMemory failed.");
        }
    }
    return dataPointer;
}

void Buffer::unmapMemory() {
    if (bufferAllocation) {
        vmaUnmapMemory(device->getAllocator(), bufferAllocation);
    } else if (deviceMemory) {
        vkUnmapMemory(device->getVkDevice(), deviceMemory);
    }
}

VkDeviceAddress Buffer::getVkDeviceAddress() {
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = buffer;
    VkDeviceAddress bufferDeviceAddress = vkGetBufferDeviceAddress(
            device->getVkDevice(), &bufferDeviceAddressInfo);
    return bufferDeviceAddress;
}

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
bool Buffer::createGlMemoryObject(GLuint& memoryObjectGl, InteropMemoryHandle& interopMemoryHandle) {
    if (!exportMemory) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: An external memory object can only be created if the "
                "export memory flag was set on creation!");
        return false;
    }
    return createGlMemoryObjectFromVkDeviceMemory(
            memoryObjectGl, interopMemoryHandle,
            device->getVkDevice(), deviceMemory, sizeInBytes, isDedicatedAllocation);
}
#endif


BufferView::BufferView(BufferPtr& buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize range)
        : format(format), offset(offset), range(range) {
    VkBufferViewCreateInfo bufferViewCreateInfo{};
    bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bufferViewCreateInfo.buffer = buffer->getVkBuffer();
    bufferViewCreateInfo.format = format;
    bufferViewCreateInfo.offset = offset;
    if (range == std::numeric_limits<VkDeviceSize>::max()) {
        bufferViewCreateInfo.range = buffer->getSizeInBytes();
    } else {
        bufferViewCreateInfo.range = range;
    }

    if (vkCreateBufferView(
            device->getVkDevice(), &bufferViewCreateInfo, nullptr, &bufferView) != VK_SUCCESS) {
        Logfile::get()->throwError("Error in BufferView::BufferView: Failed to create a buffer view!");
    }
}

BufferView::~BufferView() {
    vkDestroyBufferView(device->getVkDevice(), bufferView, nullptr);
}

BufferViewPtr BufferView::copy(bool copyBuffer, bool copyContent) {
    BufferPtr newBuffer;
    if (copyBuffer) {
        newBuffer = buffer->copy(copyContent);
    } else {
        newBuffer = buffer;
    }
    BufferViewPtr newBufferView(new BufferView(newBuffer, format, offset, range));
    return newBufferView;
}

}}
