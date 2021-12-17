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

#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "../Utils/Interop.hpp"
#include "Buffer.hpp"

namespace sgl { namespace vk {

Buffer::Buffer(
        Device* device, size_t sizeInBytes, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool queueExclusive,
        bool exportMemory)
        : device(device), sizeInBytes(sizeInBytes), bufferUsageFlags(usage), memoryUsage(memoryUsage),
          queueExclusive(queueExclusive), exportMemory(exportMemory) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeInBytes;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = queueExclusive ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

    if (!exportMemory) {
        // When the memory does not need to be exported, we can use the library VMA for allocations.
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = memoryUsage;

        if (vmaCreateBuffer(
                device->getAllocator(), &bufferCreateInfo, &allocCreateInfo,
                &buffer, &bufferAllocation, &bufferAllocationInfo) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Buffer::Buffer: Failed to create a buffer of the specified size!");
        }
    } else {
#if defined(_WIN32)
        VkExternalMemoryHandleTypeFlags handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
        VkExternalMemoryHandleTypeFlags handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
        Logfile::get()->throwError(
                "Error in Buffer::Buffer: External memory is only supported on Linux, Android and Windows systems!");
#endif

        VkExternalMemoryBufferCreateInfo externalMemoryBufferCreateInfo = {};
        externalMemoryBufferCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
        externalMemoryBufferCreateInfo.handleTypes = handleTypes;
        bufferCreateInfo.pNext = &externalMemoryBufferCreateInfo;

        // If the memory should be exported for external use, we need to allocate the memory manually.
        if (vkCreateBuffer(device->getVkDevice(), &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Buffer::Buffer: Failed to create a buffer!");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device->getVkDevice(), buffer, &memoryRequirements);

        VkExportMemoryAllocateInfo exportMemoryAllocateInfo = {};
        exportMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportMemoryAllocateInfo.handleTypes = handleTypes;

        VkMemoryPropertyFlags memoryPropertyFlags = convertVmaMemoryUsageToVkMemoryPropertyFlags(memoryUsage);

        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = device->findMemoryTypeIndex(
                memoryRequirements.memoryTypeBits, memoryPropertyFlags);
        memoryAllocateInfo.pNext = &exportMemoryAllocateInfo;

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
        Device* device, size_t sizeInBytes, void* dataPtr, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
        bool queueExclusive, bool exportMemory)
        : Buffer(device, sizeInBytes, usage, memoryUsage, queueExclusive, exportMemory) {
    uploadData(sizeInBytes, dataPtr);
}

Buffer::~Buffer() {
    if (bufferAllocation) {
        vmaDestroyBuffer(device->getAllocator(), buffer, bufferAllocation);
    }
    if (deviceMemory) {
        vkDestroyBuffer(device->getVkDevice(), buffer, nullptr);
        vkFreeMemory(device->getVkDevice(), deviceMemory, nullptr);
    }
}

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

void Buffer::uploadData(size_t sizeInBytesData, void* dataPtr) {
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
                device, sizeInBytesData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
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

void Buffer::uploadData(size_t sizeInBytesData, void* dataPtr, VkCommandBuffer commandBuffer) {
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
        size_t sizeInBytesData, void* dataPtr, VkCommandBuffer commandBuffer, BufferPtr& stagingBuffer) {
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

void Buffer::updateData(size_t sizeInBytesData, void* dataPtr, VkCommandBuffer commandBuffer) {
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

void Buffer::updateData(size_t offset, size_t sizeInBytesData, void* dataPtr, VkCommandBuffer commandBuffer) {
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
    vmaMapMemory(device->getAllocator(), bufferAllocation, &dataPointer);
    return dataPointer;
}

void Buffer::unmapMemory() {
    vmaUnmapMemory(device->getAllocator(), bufferAllocation);
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
bool Buffer::createGlMemoryObject(GLuint& memoryObjectGl) {
    if (!exportMemory) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: An external memory object can only be created if the "
                "export memory flag was set on creation!");
        return false;
    }
    return createGlMemoryObjectFromVkDeviceMemory(
            memoryObjectGl, device->getVkDevice(), deviceMemory, sizeInBytes);
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
