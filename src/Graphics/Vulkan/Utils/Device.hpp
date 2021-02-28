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

#ifndef SGL_DEVICE_HPP
#define SGL_DEVICE_HPP

#include <string>
#include <vector>
#include <set>
#include <vulkan/vulkan.h>
#include "../libs/VMA/vk_mem_alloc.h"

namespace sgl { class Window; }

namespace sgl { namespace vk {

class Instance;

class Device {
public:
    /// For rendering using a window surface and a swapchain.
    void createDeviceSwapchain(
            Instance* instance, Window* window,
            const std::vector<const char*>& requiredDeviceExtensions,
            const std::vector<const char*>& optionalDeviceExtensions);
    /// For headless rendering without a window.
    void createDeviceHeadless(
            Instance* instance,
            const std::vector<const char*>& requiredDeviceExtensions,
            const std::vector<const char*>& optionalDeviceExtensions);
    ~Device();

    bool isDeviceExtensionSupported(const std::string& name);

    inline VkPhysicalDevice getVkPhysicalDevice() { return physicalDevice; }
    inline VkDevice getVkDevice() { return device; }
    inline VmaAllocator getAllocator() { return allocator; }
    inline VkQueue getGraphicsQueue() { return graphicsQueue; }
    inline VkQueue getComputeQueue() { return computeQueue; }

    // Helpers for querying physical device properties and features.
    inline VkPhysicalDeviceProperties getPhysicalDeviceProperties() { return physicalDeviceProperties; }
    inline VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() { return physicalDeviceFeatures; }
    VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties);

private:
    void initializeDeviceExtensionList(VkPhysicalDevice physicalDevice);
    void printAvailableDeviceExtensionList();
    bool isDeviceExtensionAvailable(const std::string &extensionName);

    Instance* instance = nullptr;
    Window* window = nullptr;

    // Enabled device extensions.
    std::set<std::string> deviceExtensionsSet;

    // Theoretically available (but not necessarily enabled) device extensions.
    std::set<std::string> availableDeviceExtensionNames;

    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    VkDevice device;
    VmaAllocator allocator;

    // Queues for the logical device.
    uint32_t graphicsQueueIndex;
    uint32_t computeQueueIndex;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
};

}}

#endif //SGL_DEVICE_HPP
