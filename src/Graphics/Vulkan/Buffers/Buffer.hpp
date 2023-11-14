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

#include <Defs.hpp>
#include "../libs/volk/volk.h"
#include "../libs/VMA/vk_mem_alloc.h"

#ifdef _WIN32
typedef void* HANDLE;
#endif

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
typedef unsigned int GLuint;
namespace sgl {
union InteropMemoryHandle;
}
#endif

namespace sgl { namespace vk {

class Device;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class BufferView;
typedef std::shared_ptr<BufferView> BufferViewPtr;

struct DLL_OBJECT BufferSettings {
    BufferSettings() = default;
    size_t sizeInBytes = 0;
    VkBufferUsageFlags usage = VkBufferUsageFlagBits(0);
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    uint32_t queueFamilyIndexCount = 0; // Only for sharingMode == VK_SHARING_MODE_CONCURRENT.
    const uint32_t* pQueueFamilyIndices = nullptr;
    bool exportMemory = false; // Whether to export the memory for external use, e.g., in OpenGL.
    /**
     * Whether to use a dedicated allocation instead of using VMA. At the moment, this is only supported for exported
     * memory. When not using dedicated allocations, multiple buffers may share one block of VkDeviceMemory.
     * At the moment, some APIs (like OpenCL) may not support creating buffers with memory offsets when not using
     * sub-buffers.
     */
    bool useDedicatedAllocationForExportedMemory = true;
};

class DLL_OBJECT Buffer {
public:
    Buffer(Device* device, const BufferSettings& bufferSettings);
    /**
     * @param device The device to allocate the buffer for.
     * @param sizeInBytes The size of the buffer in bytes.
     * @param usage A combination of flags how the buffer is used. E.g., VK_BUFFER_USAGE_TRANSFER_SRC_BIT, ...
     * @param memoryUsage VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU,
     * VMA_MEMORY_USAGE_GPU_TO_CPU, VMA_MEMORY_USAGE_CPU_COPY or VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED.
     * @param queueExclusive Is the buffer owned by a specific queue family exclusively or shared?
     * @param useDedicatedAllocationForExportedMemory Whether to use a dedicated allocation instead of using VMA.
     * At the moment, this is only supported for exported memory. When not using dedicated allocations, multiple buffers
     * may share one block of VkDeviceMemory. At the moment, some APIs (like OpenCL) may not support creating buffers
     * with memory offsets when not using sub-buffers.
     */
    Buffer(
            Device* device, size_t sizeInBytes, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
            bool queueExclusive = true, bool exportMemory = false, bool useDedicatedAllocationForExportedMemory = true);
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
            Device* device, size_t sizeInBytes, const void* dataPtr, VkBufferUsageFlags usage,
            VmaMemoryUsage memoryUsage, bool queueExclusive = true,
            bool exportMemory = false, bool useDedicatedAllocationForExportedMemory = true);
    /**
     * Does not allocate any memory and buffer. This constructor is mainly needed when later calling
     * @see createFromD3D12SharedResourceHandle.
     */
    explicit Buffer(Device* device) : device(device) {}

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
     * NOTE: This version of uploadData will wait for the copy operation to finish.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     */
    void uploadData(size_t sizeInBytesData, const void* dataPtr);

    /**
     * Uploads memory to the GPU. If memoryUsage is not VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU or
     * VMA_MEMORY_USAGE_CPU_COPY, a staging buffer is used for uploading.
     * NOTE: This version of uploadData will wait for the copy operation to finish and upload using a staging buffer
     * of the specified chunk size.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     */
    void uploadDataChunked(size_t sizeInBytesData, size_t chunkSize, const void* dataPtr);

    /**
     * Uploads memory to the GPU. If memoryUsage is not VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU or
     * VMA_MEMORY_USAGE_CPU_COPY, a staging buffer is used for uploading.
     * NOTE: The version of uploadData with four parameters needs to be called in order to save the staging buffer when
     * using a custom command buffer in combination with VMA_MEMORY_USAGE_GPU_ONLY buffers.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     * @param commandBuffer The command buffer to use for the copy operation.
     * If the command buffer is a null pointer, the command will be executed synchronously.
     */
    void uploadData(size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer);

    /**
     * Uploads memory to the GPU. If memoryUsage is not VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU or
     * VMA_MEMORY_USAGE_CPU_COPY, a staging buffer is used for uploading.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     * @param commandBuffer The command buffer to use for the copy operation.
     * @param stagingBuffer A reference to which the used staging buffer should be stored. This object must not be
     * deleted before the device queue has finished the copy operation!
     */
    void uploadData(
            size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer, BufferPtr& stagingBuffer);

    /**
     * Uploads memory to the GPU. If memoryUsage is not VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU or
     * VMA_MEMORY_USAGE_CPU_COPY, a staging buffer is used for uploading.
     * NOTE: This version of uploadData will wait for the copy operation to finish.
     * @param regionOffset The offset of the memory region to upload to in bytes.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     */
    void uploadDataOffset(size_t regionOffset, size_t sizeInBytesData, const void* dataPtr);

    /**
     * Uploads memory to the GPU. If memoryUsage is not VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU or
     * VMA_MEMORY_USAGE_CPU_COPY, a staging buffer is used for uploading.
     * NOTE: The version of uploadData with four parameters needs to be called in order to save the staging buffer when
     * using a custom command buffer in combination with VMA_MEMORY_USAGE_GPU_ONLY buffers.
     * @param regionOffset The offset of the memory region to upload to in bytes.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     * @param commandBuffer The command buffer to use for the copy operation.
     * If the command buffer is a null pointer, the command will be executed synchronously.
     */
    void uploadDataOffset(
            size_t regionOffset, size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer);

    /**
     * Uploads memory to the GPU. If memoryUsage is not VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU or
     * VMA_MEMORY_USAGE_CPU_COPY, a staging buffer is used for uploading.
     * @param regionOffset The offset of the memory region to upload to in bytes.
     * @param sizeInBytesData The size of the data to upload in bytes.
     * @param dataPtr Data that is uploaded to the GPU.
     * @param commandBuffer The command buffer to use for the copy operation.
     * @param stagingBuffer A reference to which the used staging buffer should be stored. This object must not be
     * deleted before the device queue has finished the copy operation!
     */
    void uploadDataOffset(
            size_t regionOffset, size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer,
            BufferPtr& stagingBuffer);

    /**
     * Asynchronously updates the buffer data using vkCmdUpdateBuffer.
     * This operation is allowed only outside of a rendering pass. Furthermore, the user needs to ensure that the
     * correct synchronization primitives are used to avoid race conditions when accessing the updated buffer.
     * Also, VK_BUFFER_USAGE_TRANSFER_DST_BIT must be specified in the VkBufferUsageFlags when creating the buffer.
     */
    void updateData(size_t offset, size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer);
    void updateData(size_t sizeInBytesData, const void* dataPtr, VkCommandBuffer commandBuffer);

    /**
     * Copies the data of this buffer object to the passed data object.
     * @param destinationBuffer The destination buffer for the copy operation.
     * @param commandBuffer The command buffer to use for the copy operation.
     */
    void copyDataTo(const BufferPtr& destinationBuffer, VkCommandBuffer commandBuffer);

    /**
     * Copies the data of this buffer object to the passed data object.
     * @param destinationBuffer The destination buffer for the copy operation.
     * @param commandBuffer The command buffer to use for the copy operation.
     */
    void copyDataTo(
            const BufferPtr& destinationBuffer, VkDeviceSize sourceOffset, VkDeviceSize destOffset,
            VkDeviceSize copySizeInBytes, VkCommandBuffer commandBuffer);

    /**
     * Fills the buffer with a 32-bit data value.
     * NOTE: This operation needs VK_BUFFER_USAGE_TRANSFER_DST_BIT.
     * @param data The 32-bit data to fill the buffer with.
     * @param commandBuffer The command buffer to use for the fill operation.
     */
    void fill(uint32_t data, VkCommandBuffer commandBuffer);

    /**
     * Fills the buffer with a 32-bit data value.
     * NOTE: This operation needs VK_BUFFER_USAGE_TRANSFER_DST_BIT.
     * @param offset A byte offset into the buffer at which to start filling (must be a multiple of 4).
     * @param size The number of bytes to fill (must be either a multiple of 4 or VK_WHOLE_SIZE).
     * @param data The 32-bit data to fill the buffer with.
     * @param commandBuffer The command buffer to use for the fill operation.
     */
    void fill(VkDeviceSize offset, VkDeviceSize size, uint32_t data, VkCommandBuffer commandBuffer);

    /**
     * Maps the memory to a host-accessible address.
     * memoryUsage must be VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_MEMORY_USAGE_GPU_TO_CPU or
     * VMA_MEMORY_USAGE_CPU_COPY.
     * @return A host-accessible memory pointer.
     */
    void* mapMemory();
    void unmapMemory();

    VkDeviceAddress getVkDeviceAddress();

    [[nodiscard]] inline VkBuffer getVkBuffer() { return buffer; }
    [[nodiscard]] inline Device* getDevice() { return device; }
    [[nodiscard]] inline size_t getSizeInBytes() const { return sizeInBytes; }
    [[nodiscard]] inline VkBufferUsageFlags getVkBufferUsageFlags() const { return bufferUsageFlags; }
    [[nodiscard]] inline VmaMemoryUsage getVmaMemoryUsage() const { return memoryUsage; }
    [[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() { return deviceMemory; }
    [[nodiscard]] inline VkDeviceSize getDeviceMemoryOffset() const { return deviceMemoryOffset; }
    [[nodiscard]] inline VkDeviceSize getDeviceMemorySize() const { return deviceMemorySize; }
    [[nodiscard]] inline VkDeviceSize getDeviceMemoryAllocationSize() const { return deviceMemoryAllocationSize; }

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
    /**
     * Creates an OpenGL memory object from the external Vulkan memory.
     * NOTE: The buffer must have been created with exportMemory set to true.
     * @param memoryObjectGl The OpenGL memory object.
     * @param interopMemoryHandle The handle (Windows) or file descriptor (Unix) to the Vulkan memory object.
     * @return Whether the OpenGL memory object could be created successfully.
     */
    bool createGlMemoryObject(GLuint& memoryObjectGl, InteropMemoryHandle& interopMemoryHandle);
#endif

#ifdef _WIN32
    /**
     * Imports a Direct3D 12 shared resource handle. The object must have been created using the standard constructor.
     * The handle is expected to point to device-local memory. The object will take ownership of the handle and close
     * it on destruction. Below, an example of how to create the shared handle can be found.
     *
     * HANDLE resourceHandle;
     * std::wstring sharedHandleNameString = std::wstring(L"Local\\D3D12ResourceHandle") + std::to_wstring(resourceIdx);
     * ID3D12Device::CreateSharedHandle(dxObject, nullptr, GENERIC_ALL, sharedHandleNameString.data(), &resourceHandle);
     *
     * If the passed handle name already exists, the function will fail with DXGI_ERROR_NAME_ALREADY_EXISTS.
     */
    void createFromD3D12SharedResourceHandle(HANDLE resourceHandle, size_t sizeInBytesData, VkBufferUsageFlags usage);
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
    VkDeviceSize deviceMemoryOffset = 0;
    VkDeviceSize deviceMemorySize = 0;
    VkDeviceSize deviceMemoryAllocationSize = 0;
    bool isDedicatedAllocation = false;

    VkBufferUsageFlags bufferUsageFlags = 0;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    bool queueExclusive = true;
    bool exportMemory = false;

#ifdef _WIN32
    HANDLE handle = nullptr;
#endif
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
    VkBufferView bufferView = VK_NULL_HANDLE;
    VkFormat format;
    VkDeviceSize offset;
    VkDeviceSize range;
};

}}

#endif //SGL_BUFFER_HPP
