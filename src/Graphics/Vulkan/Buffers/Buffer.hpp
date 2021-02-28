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

#ifndef SGL_BUFFER_HPP
#define SGL_BUFFER_HPP

#include <vulkan/vulkan.h>
#include <memory>
#include "../libs/VMA/vk_mem_alloc.h"

namespace sgl { namespace vk {

class Device;

class Buffer {
public:
    /**
     *
     * @param device The device to allocate the buffer for.
     * @param sizeInBytes The size of the buffer in bytes.
     * @param usage A combination of flags how the buffer is used. E.g., VK_BUFFER_USAGE_TRANSFER_SRC_BIT, ...
     * @param memoryUsage VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU,
     * VMA_MEMORY_USAGE_GPU_TO_CPU, VMA_MEMORY_USAGE_CPU_COPY or VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED.
     * @param queueExclusive Is the buffer owned by a specific queue family exclusively or shared?
     */
    Buffer(
            Device* device, size_t sizeInBytes, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
            bool queueExclusive = true);
    ~Buffer();

private:
    Device* device = nullptr;
    VkBuffer buffer;
    VmaAllocation bufferAllocation;
    VmaAllocationInfo bufferAllocationInfo;
};

typedef std::shared_ptr<Buffer> BufferPtr;

}}

#endif //SGL_BUFFER_HPP
