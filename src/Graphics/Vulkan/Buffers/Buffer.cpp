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

#include <stdexcept>

#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "Buffer.hpp"

namespace sgl { namespace vk {

Buffer::Buffer(
        Device* device, size_t sizeInBytes, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool queueExclusive)
        : device(device), sizeInBytes(sizeInBytes), bufferUsageFlags(usage), memoryUsage(memoryUsage),
        queueExclusive(queueExclusive) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeInBytes;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = queueExclusive ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = memoryUsage;

    if (vmaCreateBuffer(
            device->getAllocator(), &bufferCreateInfo, &allocCreateInfo,
            &buffer, &bufferAllocation, &bufferAllocationInfo) != VK_SUCCESS) {
        Logfile::get()->throwError("Error in Buffer::Buffer: Failed to create a buffer of the specified size!");
    }
}

Buffer::~Buffer() {
    vmaDestroyBuffer(device->getAllocator(), buffer, bufferAllocation);
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

void* Buffer::mapMemory() {
    void* dataPointer = nullptr;
    vmaMapMemory(device->getAllocator(), bufferAllocation, &dataPointer);
    return dataPointer;
}

void Buffer::unmapMemory() {
    vmaUnmapMemory(device->getAllocator(), bufferAllocation);
}


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
