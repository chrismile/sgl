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

#include <memory>
#include <limits>
#include <vulkan/vulkan.h>
#include "../libs/VMA/vk_mem_alloc.h"

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#endif

namespace sgl { namespace vk {

class Device;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class BufferView;
typedef std::shared_ptr<BufferView> BufferViewPtr;

class DLL_OBJECT Buffer {
public:
    /**
     * @param device The device to allocate the buffer for.
     * @param sizeInBytes The size of the buffer in bytes.
     * @param usage A combination of flags how the buffer is used. E.g., VK_BUFFER_USAGE_TRANSFER_SRC_BIT, ...
     * @param memoryUsage VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU,
     * VMA_MEMORY_USAGE_GPU_TO_CPU, VMA_MEMORY_USAGE_CPU_COPY or VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED.
     * @param queueExclusive Is the buffer owned by a specific queue family exclusively or shared?
     */
    Buffer(
            Device* device, size_t sizeInBytes, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
            bool queueExclusive = true, bool exportMemory = false);
    /**
     * @param device The device to allocate the buffer for.
     * @param sizeInBytes The size of the buffer in bytes.
     * @param dataPtr Data that is directly uploaded to the GPU.
     * @param usage A combination of flags how the buffer is used. E.g., VK_BUFFER_USAGE_TRANSFER_SRC_BIT, ...
     * @param memoryUsage VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU,
     * VMA_MEMORY_USAGE_GPU_TO_CPU, VMA_MEMORY_USAGE_CPU_COPY or VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED.
     * @param queueExclusive Is the buffer owned by a specific queue family exclusively or shared?
     * @param exportMemory Should the memory be exported for use, e.g., in OpenGL? This is currently only supported on
     * Linux, Android and Windows systems.
     */
    Buffer(
            Device* device, size_t sizeInBytes, void* dataPtr, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
            bool queueExclusive = true, bool exportMemory = false);

    ~Buffer();

    /**
     * Creates a copy of the buffer.
     * @param copyContent Whether to copy the content, too, or only create a buffer with the same settings.
     * @return The new buffer.
     */
    BufferPtr copy(bool copyContent);

    /**
     * Uploads memory to the GPU. If memoryUsage is not VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU or
     * VMA_MEMORY_USAGE_CPU_COPY, a staging buffer is used for uploading.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     * @param commandBuffer The command buffer to use for the copy operation.
     * If the command buffer is a null pointer, the command will be executed synchronously.
     */
    void uploadData(size_t sizeInBytesData, void* dataPtr, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Maps the memory to a host-accessible address.
     * memoryUsage must be VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_MEMORY_USAGE_GPU_TO_CPU or
     * VMA_MEMORY_USAGE_CPU_COPY.
     * @return A host-accessible memory pointer.
     */
    void* mapMemory();
    void unmapMemory();

    VkDeviceAddress getVkDeviceAddress();

    inline VkBuffer getVkBuffer() { return buffer; }
    inline size_t getSizeInBytes() const { return sizeInBytes; }
    inline VkBufferUsageFlags getVkBufferUsageFlags() const { return bufferUsageFlags; }
    inline VmaMemoryUsage getVmaMemoryUsage() const { return memoryUsage; }

#ifdef SUPPORT_OPENGL
    /**
     * Creates an OpenGL memory object from the external Vulkan memory.
     * NOTE: The buffer must have been created with exportMemory set to true.
     * @param memoryObjectGl The OpenGL memory object.
     * @return Whether the OpenGL memory object could be created successfully.
     */
    bool createGlMemoryObject(GLuint& memoryObjectGl);
#endif

private:
    Device* device = nullptr;
    size_t sizeInBytes = 0;
    VkBuffer buffer = VK_NULL_HANDLE;

    // Memory not exported, used only in Vulkan.
    VmaAllocation bufferAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo bufferAllocationInfo = {};

    // Exported memory for external use.
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;

    VkBufferUsageFlags bufferUsageFlags;
    VmaMemoryUsage memoryUsage;
    bool queueExclusive;
    bool exportMemory;
};

class DLL_OBJECT BufferView {
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
    BufferView(
            BufferPtr& buffer, VkFormat format, VkDeviceSize offset = 0,
            VkDeviceSize range = std::numeric_limits<VkDeviceSize>::max());
    ~BufferView();

    /**
     * Creates a copy of the buffer view.
     * @param copyBuffer Whether to create a deep copy (with the underlying buffer also being copied) or to create a
     * shallow copy that shares the buffer object with the original buffer view.
     * @param copyContent If copyBuffer is true: Whether to also copy the content of the underlying buffer.
     * @return The new buffer view.
     */
    BufferViewPtr copy(bool copyBuffer, bool copyContent);

    inline BufferPtr getBuffer() { return buffer; }
    inline VkBufferView getVkBufferView() { return bufferView; }

private:
    Device* device = nullptr;
    BufferPtr buffer;
    VkBufferView bufferView;
    VkFormat format;
    VkDeviceSize offset;
    VkDeviceSize range;
};

}}

#endif //SGL_BUFFER_HPP
