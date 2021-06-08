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
#include <functional>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "../libs/VMA/vk_mem_alloc.h"

namespace sgl { class Window; }

namespace sgl { namespace vk {
/**
 * Different types of command pools are automatically allocated for some often used types of command buffers.
 */
struct CommandPoolType {
    VkCommandPoolCreateFlags flags = 0;
    uint32_t queueFamilyIndex = 0xFFFFFFFF; // 0xFFFFFFFF encodes standard (graphics queue).

    bool operator==(const CommandPoolType& other) const {
        return flags == other.flags && queueFamilyIndex == other.queueFamilyIndex;
    }
};
}}

namespace std {
template<> struct hash<sgl::vk::CommandPoolType> {
    std::size_t operator()(sgl::vk::CommandPoolType const& s) const noexcept {
        std::size_t h1 = std::hash<uint32_t>{}(s.flags);
        std::size_t h2 = std::hash<uint32_t>{}(s.queueFamilyIndex);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }
};
}

namespace sgl { namespace vk {

class Instance;

/**
 * An encapsulation of VkDevice and VkPhysicalDevice.
 */
class DLL_OBJECT Device {
public:
    /// For rendering using a window surface and a swapchain. VK_KHR_SWAPCHAIN_EXTENSION_NAME is added automatically.
    void createDeviceSwapchain(
            Instance* instance, Window* window,
            std::vector<const char*> requiredDeviceExtensions = {},
            const std::vector<const char*>& optionalDeviceExtensions = {},
            VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures = {});
    /// For headless rendering without a window.
    void createDeviceHeadless(
            Instance* instance,
            const std::vector<const char*>& requiredDeviceExtensions = {},
            const std::vector<const char*>& optionalDeviceExtensions = {},
            VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures = {});
    ~Device();

    bool isDeviceExtensionSupported(const std::string& name);

    inline VkPhysicalDevice getVkPhysicalDevice() { return physicalDevice; }
    inline VkDevice getVkDevice() { return device; }
    inline VmaAllocator getAllocator() { return allocator; }
    inline VkQueue getGraphicsQueue() { return graphicsQueue; }
    inline VkQueue getComputeQueue() { return computeQueue; }
    inline uint32_t getGraphicsQueueIndex() { return graphicsQueueIndex; }
    inline uint32_t getComputeQueueIndex() { return computeQueueIndex; }

    // Helpers for querying physical device properties and features.
    inline VkPhysicalDeviceProperties getPhysicalDeviceProperties() { return physicalDeviceProperties; }
    inline VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() { return physicalDeviceFeatures; }
    VkSampleCountFlagBits getMaxUsableSampleCount();

    // Create command buffers. Remember to call vkFreeCommandBuffers (specifying the pool is necessary for this).
    VkCommandBuffer allocateCommandBuffer(
            CommandPoolType commandPoolType, VkCommandPool* pool,
            VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    std::vector<VkCommandBuffer> allocateCommandBuffers(
            CommandPoolType commandPoolType, VkCommandPool* pool, uint32_t count,
            VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // Create a transient command buffer ready to execute commands (0xFFFFFFFF encodes graphics queue).
    VkCommandBuffer beginSingleTimeCommands(uint32_t queueIndex = 0xFFFFFFFF);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, uint32_t queueIndex = 0xFFFFFFFF);

    inline Instance* getInstance() { return instance; }

    // Get the standard descriptor pool.
    VkDescriptorPool getDefaultVkDescriptorPool() { return descriptorPool; }

private:
    void initializeDeviceExtensionList(VkPhysicalDevice physicalDevice);
    void printAvailableDeviceExtensionList();
    bool isDeviceExtensionAvailable(const std::string &extensionName);

    // Internal implementations.
    void _allocateCommandBuffers(
            CommandPoolType commandPoolType, VkCommandPool* pool, VkCommandBuffer* commandBuffers, uint32_t count,
            VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

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

    // Set of used command pools.
    std::unordered_map<CommandPoolType, VkCommandPool> commandPools;

    // Default descriptor pool.
    VkDescriptorPool descriptorPool;
};

}}

#endif //SGL_DEVICE_HPP
