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

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)

#include <Utils/File/Logfile.hpp>

#ifdef _WIN32
#include <windows.h>
#include "../libs/volk/volk.h"
#include <vulkan/vulkan_win32.h>
#endif

#include <Graphics/Renderer.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
#include <Graphics/OpenGL/GeometryBuffer.hpp>
#include <Graphics/OpenGL/Texture.hpp>

#include "Interop.hpp"

namespace sgl {

SemaphoreVkGlInterop::SemaphoreVkGlInterop(sgl::vk::Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags) {
    VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo = {};
    exportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#if defined(_WIN32)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
    Logfile::get()->throwError(
                "Error in SemaphoreVkGlInterop::SemaphoreVkGlInterop: External semaphores are only supported on "
                "Linux, Android and Windows systems!");
#endif
    _initialize(
            device, semaphoreCreateFlags, VK_SEMAPHORE_TYPE_BINARY, 0,
            &exportSemaphoreCreateInfo);

    glGenSemaphoresEXT(1, &semaphoreGl);
#if defined(_WIN32)
    auto _vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
                device->getVkDevice(), "vkGetSemaphoreWin32HandleKHR");
        if (!_vkGetSemaphoreWin32HandleKHR) {
            Logfile::get()->throwError(
                    "Error in SemaphoreVkGlInterop::SemaphoreVkGlInterop: vkGetSemaphoreWin32HandleKHR was not found!");
        }

        VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfo = {};
        semaphoreGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
        semaphoreGetWin32HandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        semaphoreGetWin32HandleInfo.semaphore = semaphoreVk;
        HANDLE semaphoreHandle = nullptr;
        if (_vkGetSemaphoreWin32HandleKHR(
                device->getVkDevice(), &semaphoreGetWin32HandleInfo, &semaphoreHandle) != VK_SUCCESS) {
            Logfile::get()->throwError(
                    "Error in SemaphoreVkGlInterop::SemaphoreVkGlInterop: vkGetSemaphoreFdKHR failed!");
        }

        glImportSemaphoreWin32HandleEXT(semaphoreGl, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, semaphoreHandle);
#elif defined(__linux__)
    auto _vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(
            device->getVkDevice(), "vkGetSemaphoreFdKHR");
    if (!_vkGetSemaphoreFdKHR) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkGlInterop::SemaphoreVkGlInterop: vkGetSemaphoreFdKHR was not found!");
    }

    VkSemaphoreGetFdInfoKHR semaphoreGetFdInfo = {};
    semaphoreGetFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    semaphoreGetFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    semaphoreGetFdInfo.semaphore = semaphoreVk;
    int fileDescriptor = 0;
    if (_vkGetSemaphoreFdKHR(device->getVkDevice(), &semaphoreGetFdInfo, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkGlInterop::SemaphoreVkGlInterop: vkGetSemaphoreFdKHR failed!");
    }

    glImportSemaphoreFdEXT(semaphoreGl, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fileDescriptor);

#ifndef NDEBUG
    if (!glIsSemaphoreEXT(semaphoreGl)) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkGlInterop::SemaphoreVkGlInterop: glIsSemaphoreEXT failed!");
    }
#endif
#endif
}

SemaphoreVkGlInterop::~SemaphoreVkGlInterop() {
    glDeleteSemaphoresEXT(1, &semaphoreGl);
}

/*
 * Calling glFlush seems to be necessary after glSignalSemaphoreEXT.
 *
 * - https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_external_objects.txt
 * "Calling WaitSemaphore on a semaphore that has not previously had a signal operation flushed to the GL server or
 * submitted by an external semaphore signaler since the semaphore was created or last waited on results in undefined
 * behavior."
 *
 * - https://eleni.mutantstargoat.com/hikiko/vk-gl-interop-sema/
 * "EXT_external_objects extension requires we call glSignalSemaphoreEXT followed by a glFlush."
 *
 * - Anecdotal evidence: glFlush was needed on Intel ANV Linux driver, but not on NVIDIA driver.
 */

void SemaphoreVkGlInterop::signalSemaphoreGl() {
    glSignalSemaphoreEXT(semaphoreGl, 0, nullptr, 0, nullptr, nullptr);
    glFlush();
}

void SemaphoreVkGlInterop::signalSemaphoreGl(const sgl::GeometryBufferPtr& buffer) {
    GLuint bufferGl = static_cast<sgl::GeometryBufferGL*>(buffer.get())->getBuffer();
    glSignalSemaphoreEXT(semaphoreGl, 1, &bufferGl, 0, nullptr, nullptr);
}

void SemaphoreVkGlInterop::signalSemaphoreGl(const std::vector<sgl::GeometryBufferPtr>& buffers) {
    std::vector<GLuint> buffersGl;
    buffersGl.reserve(buffers.size());
    for (const sgl::GeometryBufferPtr& buffer : buffers) {
        buffersGl.push_back(static_cast<sgl::GeometryBufferGL*>(buffer.get())->getBuffer());
    }
    glSignalSemaphoreEXT(semaphoreGl, GLuint(buffersGl.size()), buffersGl.data(), 0, nullptr, nullptr);
    glFlush();
}

void SemaphoreVkGlInterop::signalSemaphoreGl(const sgl::TexturePtr& texture, GLenum dstLayout) {
    GLuint textureGl = static_cast<sgl::TextureGL*>(texture.get())->getTexture();
    glSignalSemaphoreEXT(semaphoreGl, 0, nullptr, 1, &textureGl, &dstLayout);
    glFlush();
}

void SemaphoreVkGlInterop::signalSemaphoreGl(
        const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& dstLayouts) {
    assert(textures.size() == dstLayouts.size());
    std::vector<GLuint> texturesGl;
    texturesGl.reserve(textures.size());
    for (const sgl::TexturePtr& texture : textures) {
        texturesGl.push_back(static_cast<sgl::TextureGL*>(texture.get())->getTexture());
    }
    glSignalSemaphoreEXT(semaphoreGl, 0, nullptr, GLuint(texturesGl.size()), texturesGl.data(), dstLayouts.data());
    glFlush();
}

void SemaphoreVkGlInterop::signalSemaphoreGl(
        const std::vector<sgl::GeometryBufferPtr>& buffers,
        const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& dstLayouts) {
    assert(textures.size() == dstLayouts.size());
    std::vector<GLuint> buffersGl;
    buffersGl.reserve(buffers.size());
    for (const sgl::GeometryBufferPtr& buffer : buffers) {
        buffersGl.push_back(static_cast<sgl::GeometryBufferGL*>(buffer.get())->getBuffer());
    }
    std::vector<GLuint> texturesGl;
    texturesGl.reserve(textures.size());
    for (const sgl::TexturePtr& texture : textures) {
        texturesGl.push_back(static_cast<sgl::TextureGL*>(texture.get())->getTexture());
    }
    glSignalSemaphoreEXT(
            semaphoreGl, GLuint(buffersGl.size()), buffersGl.data(),
            GLuint(texturesGl.size()), texturesGl.data(), dstLayouts.data());
    glFlush();
}


void SemaphoreVkGlInterop::waitSemaphoreGl() {
    glWaitSemaphoreEXT(semaphoreGl, 0, nullptr, 0, nullptr, nullptr);
}

void SemaphoreVkGlInterop::waitSemaphoreGl(const sgl::GeometryBufferPtr& buffer) {
    GLuint bufferGl = static_cast<sgl::GeometryBufferGL*>(buffer.get())->getBuffer();
    glWaitSemaphoreEXT(semaphoreGl, 1, &bufferGl, 0, nullptr, nullptr);
}

void SemaphoreVkGlInterop::waitSemaphoreGl(const std::vector<sgl::GeometryBufferPtr>& buffers) {
    std::vector<GLuint> buffersGl;
    buffersGl.reserve(buffers.size());
    for (const sgl::GeometryBufferPtr& buffer : buffers) {
        buffersGl.push_back(static_cast<sgl::GeometryBufferGL*>(buffer.get())->getBuffer());
    }
    glWaitSemaphoreEXT(semaphoreGl, GLuint(buffersGl.size()), buffersGl.data(), 0, nullptr, nullptr);
}

void SemaphoreVkGlInterop::waitSemaphoreGl(const sgl::TexturePtr& texture, GLenum srcLayout) {
    GLuint textureGl = static_cast<sgl::TextureGL*>(texture.get())->getTexture();
    glWaitSemaphoreEXT(semaphoreGl, 0, nullptr, 1, &textureGl, &srcLayout);
}

void SemaphoreVkGlInterop::waitSemaphoreGl(
        const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& srcLayouts) {
    assert(textures.size() == srcLayouts.size());
    std::vector<GLuint> texturesGl;
    texturesGl.reserve(textures.size());
    for (const sgl::TexturePtr& texture : textures) {
        texturesGl.push_back(static_cast<sgl::TextureGL*>(texture.get())->getTexture());
    }
    glWaitSemaphoreEXT(semaphoreGl, 0, nullptr, GLuint(texturesGl.size()), texturesGl.data(), srcLayouts.data());
}

void SemaphoreVkGlInterop::waitSemaphoreGl(
        const std::vector<sgl::GeometryBufferPtr>& buffers,
        const std::vector<sgl::TexturePtr>& textures, const std::vector<GLenum>& srcLayouts) {
    assert(textures.size() == srcLayouts.size());
    std::vector<GLuint> buffersGl;
    buffersGl.reserve(buffers.size());
    for (const sgl::GeometryBufferPtr& buffer : buffers) {
        buffersGl.push_back(static_cast<sgl::GeometryBufferGL*>(buffer.get())->getBuffer());
    }
    std::vector<GLuint> texturesGl;
    texturesGl.reserve(textures.size());
    for (const sgl::TexturePtr& texture : textures) {
        texturesGl.push_back(static_cast<sgl::TextureGL*>(texture.get())->getTexture());
    }
    glWaitSemaphoreEXT(
            semaphoreGl, GLuint(buffersGl.size()), buffersGl.data(),
            GLuint(texturesGl.size()), texturesGl.data(), srcLayouts.data());
}



InteropSyncVkGl::InteropSyncVkGl(sgl::vk::Device* device, int numFramesInFlight) {
    frameIdx = 0;
    renderReadySemaphores.resize(numFramesInFlight);
    renderFinishedSemaphores.resize(numFramesInFlight);
    for (int i = 0; i < numFramesInFlight; i++) {
        renderReadySemaphores.at(i) = std::make_shared<sgl::SemaphoreVkGlInterop>(device);
        renderFinishedSemaphores.at(i) = std::make_shared<sgl::SemaphoreVkGlInterop>(device);
    }
    interFrameTimelineSemaphore = std::make_shared<sgl::vk::Semaphore>(
            device, 0, VK_SEMAPHORE_TYPE_TIMELINE,
            timelineValue);
    interFrameTimelineSemaphore->setWaitSemaphoreValue(timelineValue);
    interFrameTimelineSemaphore->setSignalSemaphoreValue(timelineValue + 1);
}

const SemaphoreVkGlInteropPtr& InteropSyncVkGl::getRenderReadySemaphore() {
    return renderReadySemaphores.at(frameIdx);
}

const SemaphoreVkGlInteropPtr& InteropSyncVkGl::getRenderFinishedSemaphore() {
    return renderFinishedSemaphores.at(frameIdx);
}

vk::SemaphorePtr InteropSyncVkGl::getRenderReadySemaphoreVk() {
    return std::static_pointer_cast<sgl::vk::Semaphore, sgl::SemaphoreVkGlInterop>(
            renderReadySemaphores.at(frameIdx));
}

vk::SemaphorePtr InteropSyncVkGl::getRenderFinishedSemaphoreVk() {
    return std::static_pointer_cast<sgl::vk::Semaphore, sgl::SemaphoreVkGlInterop>(
            renderFinishedSemaphores.at(frameIdx));
}

void InteropSyncVkGl::frameFinished() {
    frameIdx = (frameIdx + 1) % int(renderReadySemaphores.size());
    timelineValue++;
    interFrameTimelineSemaphore->setWaitSemaphoreValue(timelineValue);
    interFrameTimelineSemaphore->setSignalSemaphoreValue(timelineValue + 1);
}

void InteropSyncVkGl::setFrameIndex(int frame) {
    frameIdx = frame;
    timelineValue++;
    interFrameTimelineSemaphore->setWaitSemaphoreValue(timelineValue);
    interFrameTimelineSemaphore->setSignalSemaphoreValue(timelineValue + 1);
}



bool isDeviceCompatibleWithOpenGl(VkPhysicalDevice physicalDevice) {
    assert(VK_UUID_SIZE == GL_UUID_SIZE_EXT);
    const size_t UUID_SIZE = std::min(size_t(VK_UUID_SIZE), size_t(GL_UUID_SIZE_EXT));

    VkPhysicalDeviceProperties physicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    const std::string infoMessage =
            std::string() + "Checking Vulkan device \'" + physicalDeviceProperties.deviceName
            + "\' for compatibility with OpenGL...";
    sgl::Logfile::get()->write(infoMessage, BLUE);

    if (physicalDeviceProperties.apiVersion < VK_API_VERSION_1_1) {
        sgl::Logfile::get()->write(
                "Discarding the Vulkan device due to not supporting at least Vulkan 1.1.", BLUE);
        return false;
    }

    // Get the Vulkan UUID data for the driver and device.
    VkPhysicalDeviceIDProperties physicalDeviceIdProperties = {};
    physicalDeviceIdProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {};
    physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    physicalDeviceProperties2.pNext = &physicalDeviceIdProperties;
    vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties2);

    // Get the OpenGL UUID of the driver, and compare all associated device UUIDs with the Vulkan device UUID.
    if (!sgl::SystemGL::get()->isGLExtensionAvailable("GL_EXT_memory_object")) {
        sgl::Logfile::get()->write(
                "Discarding the Vulkan device due to the OpenGL context not supporting the extension "
                "GL_EXT_memory_object.", BLUE);
        return false;
    }
    GLubyte deviceUuid[GL_UUID_SIZE_EXT];
    GLubyte driverUuid[GL_UUID_SIZE_EXT];
    glGetUnsignedBytevEXT(GL_DRIVER_UUID_EXT, driverUuid);

    if (strncmp((const char*)driverUuid, (const char*)physicalDeviceIdProperties.driverUUID, UUID_SIZE) != 0) {
        sgl::Logfile::get()->write(
                "Discarding the Vulkan device due to a mismatch in driver UUIDs with the OpenGL context.",
                BLUE);
        return false;
    }

    GLint numDevices = 0;
    glGetIntegerv(GL_NUM_DEVICE_UUIDS_EXT, &numDevices);
    for (int deviceIdx = 0; deviceIdx < numDevices; deviceIdx++) {
        glGetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT, deviceIdx, deviceUuid);
        if (strncmp((const char*)deviceUuid, (const char*)physicalDeviceIdProperties.deviceUUID, UUID_SIZE) == 0) {
            return true;
        }
    }

    return false;
}

bool createGlMemoryObjectFromVkDeviceMemory(
        GLuint& memoryObjectGl, InteropMemoryHandle& interopMemoryHandle,
        VkDevice device, VkDeviceMemory deviceMemory, size_t sizeInBytes) {
#if defined(_WIN32)
    auto _vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            device, "vkGetMemoryWin32HandleKHR");
    if (!_vkGetMemoryWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: vkGetMemoryWin32HandleKHR was not found!");
        return false;
    }
    VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = {};
    memoryGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    memoryGetWin32HandleInfo.memory = deviceMemory;
    memoryGetWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE handle = nullptr;
    if (_vkGetMemoryWin32HandleKHR(device, &memoryGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in createGlMemoryObjectFromVkDeviceMemory: Could not retrieve the file descriptor from the "
                "Vulkan  device memory!");
        return false;
    }

    glCreateMemoryObjectsEXT(1, &memoryObjectGl);
    glImportMemoryWin32HandleEXT(memoryObjectGl, sizeInBytes, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, handle);
    interopMemoryHandle.handle = handle;
#elif defined(__linux__)
    auto _vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!_vkGetMemoryFdKHR) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: vkGetMemoryFdKHR was not found!");
        return false;
    }

    VkMemoryGetFdInfoKHR memoryGetFdInfoKhr = {};
    memoryGetFdInfoKhr.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    memoryGetFdInfoKhr.memory = deviceMemory;
    memoryGetFdInfoKhr.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    int fileDescriptor = 0;
    if (_vkGetMemoryFdKHR(device, &memoryGetFdInfoKhr, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in createGlMemoryObjectFromVkDeviceMemory: Could not retrieve the file descriptor from the "
                "Vulkan device memory!");
        return false;
    }

    glCreateMemoryObjectsEXT(1, &memoryObjectGl);
    glImportMemoryFdEXT(memoryObjectGl, sizeInBytes, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fileDescriptor);
    interopMemoryHandle.fileDescriptor = fileDescriptor;
#else
    Logfile::get()->throwError(
            "Error in createGlMemoryObjectFromVkDeviceMemory: External memory is only supported on Linux, Android "
            "and Windows systems!");
    return false;
#endif

    if (!glIsMemoryObjectEXT(memoryObjectGl)) {
        Logfile::get()->throwError(
                "Error in createGlMemoryObjectFromVkDeviceMemory: Failed to create an OpenGL memory object!");
        return false;
    }

    sgl::Renderer->errorCheck();
    return glGetError() == GL_NO_ERROR;
}

}

#endif
