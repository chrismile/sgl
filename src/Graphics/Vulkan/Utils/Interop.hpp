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

#ifndef SGL_INTEROP_HPP
#define SGL_INTEROP_HPP

#include <algorithm>
#include <cstring>
#include <cassert>
#include <Defs.hpp>
#include "../libs/volk/volk.h"
#include "../libs/VMA/vk_mem_alloc.h"

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#endif

#include "SyncObjects.hpp"
#include "Device.hpp"

/*
 * Utility functions for Vulkan-OpenGL interoperability.
 */

namespace sgl {

class GeometryBuffer;
typedef std::shared_ptr<GeometryBuffer> GeometryBufferPtr;
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

class DLL_OBJECT SemaphoreVkGlInterop : public vk::Semaphore {
public:
    explicit SemaphoreVkGlInterop(sgl::vk::Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags = 0);
    ~SemaphoreVkGlInterop();

    /*
     * @param srcLayout and dstLayout One value out of:
     *  GL_NONE                                          | VK_IMAGE_LAYOUT_UNDEFINED
     *  GL_LAYOUT_GENERAL_EXT                            | VK_IMAGE_LAYOUT_GENERAL
     *  GL_LAYOUT_COLOR_ATTACHMENT_EXT                   | VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
     *  GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT           | VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT
     *  GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT            | VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
     *  GL_LAYOUT_SHADER_READ_ONLY_EXT                   | VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     *  GL_LAYOUT_TRANSFER_SRC_EXT                       | VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
     *  GL_LAYOUT_TRANSFER_DST_EXT                       | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
     *  GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT | VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR
     *  GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT | VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR
     */

    /// Signal semaphore without any barriers.
    void signalSemaphoreGl();
    /// Signal semaphore with one buffer barrier.
    void signalSemaphoreGl(const sgl::GeometryBufferPtr& buffer);
    /// Signal semaphore with multiple buffer barriers.
    void signalSemaphoreGl(const std::vector<sgl::GeometryBufferPtr>& buffers);
    /// Signal semaphore with one texture barrier.
    void signalSemaphoreGl(const sgl::TexturePtr& texture, GLenum dstLayout);
    /// Signal semaphore with multiple texture barriers.
    void signalSemaphoreGl(const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& dstLayouts);
    /// Signal semaphore with multiple buffer and texture barriers.
    void signalSemaphoreGl(
            const std::vector<sgl::GeometryBufferPtr>& buffers,
            const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& dstLayouts);

    /// Wait on semaphore without any memory barriers.
    void waitSemaphoreGl();
    /// Wait on semaphore with one buffer barrier.
    void waitSemaphoreGl(const sgl::GeometryBufferPtr& buffer);
    /// Wait on semaphore with multiple buffer barriers.
    void waitSemaphoreGl(const std::vector<sgl::GeometryBufferPtr>& buffers);
    /// Wait on semaphore with one texture barrier.
    void waitSemaphoreGl(const sgl::TexturePtr& texture, GLenum srcLayout);
    /// Wait on semaphore with multiple texture barriers.
    void waitSemaphoreGl(const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& srcLayouts);
    /// Wait on semaphore with multiple buffer and texture barriers.
    void waitSemaphoreGl(
            const std::vector<sgl::GeometryBufferPtr>& buffers,
            const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& srcLayouts);

private:
    sgl::vk::Device* device = nullptr;
    GLuint semaphoreGl = 0;
};
typedef std::shared_ptr<SemaphoreVkGlInterop> SemaphoreVkGlInteropPtr;

/**
 * Converts VmaMemoryUsage to VkMemoryPropertyFlags.
 * For now, all CPU-visible modes use VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT.
 * As this code is only used for exporting memory for external use, e.g., in OpenGL, most memory will probably be
 * allocated using VMA_MEMORY_USAGE_GPU_ONLY anyways.
 * @param memoryUsage The VMA memory usage enum.
 * @return A VkMemoryPropertyFlags bit mask.
 */
DLL_OBJECT VkMemoryPropertyFlags convertVmaMemoryUsageToVkMemoryPropertyFlags(VmaMemoryUsage memoryUsage);

#ifdef SUPPORT_OPENGL
/**
 * Returns whether the passed Vulkan device is compatible with the currently used OpenGL server.
 * @param physicalDevice The physical Vulkan device.
 * @return True if the device is compatible with the currently used OpenGL server.
 */
DLL_OBJECT bool isDeviceCompatibleWithOpenGl(VkPhysicalDevice physicalDevice);

/**
 * Creates an OpenGL memory object from the external Vulkan memory.
 * @param memoryObjectGl The OpenGL memory object.
 * @param device The Vulkan device handle.
 * @param deviceMemory The Vulkan device memory to export.
 * @param sizeInBytes The size of the memory in bytes.
 * @return Whether the OpenGL memory object could be created successfully.
 */
DLL_OBJECT bool createGlMemoryObjectFromVkDeviceMemory(
        GLuint& memoryObjectGl, VkDevice device, VkDeviceMemory deviceMemory, size_t sizeInBytes);

}
#endif

#endif //SGL_INTEROP_HPP
