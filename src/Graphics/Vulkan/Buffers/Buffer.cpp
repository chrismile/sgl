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
        Device* device, size_t sizeInBytes, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
        bool queueExclusive) : device(device), sizeInBytes(sizeInBytes) {
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

void* Buffer::mapMemory() {
    void* dataPointer = nullptr;
    vmaMapMemory(device->getAllocator(), bufferAllocation, &dataPointer);
    return dataPointer;
}

void Buffer::unmapMemory() {
    vmaUnmapMemory(device->getAllocator(), bufferAllocation);
}

}}
