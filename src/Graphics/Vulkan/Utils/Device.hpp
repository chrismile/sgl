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
#include <thread>
#include <Defs.hpp>
#include "../libs/volk/volk.h"
#include "../libs/VMA/vk_mem_alloc.h"

namespace sgl { class Window; }

namespace sgl { namespace vk {
/**
 * Different types of command pools are automatically allocated for some often used types of command buffers.
 */
struct DLL_OBJECT CommandPoolType {
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

struct DLL_OBJECT DeviceFeatures {
    DeviceFeatures() {
        timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        scalarBlockLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
        uniformBufferStandardLayoutFeaturesKhr.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES;
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    }
    VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures{};
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR uniformBufferStandardLayoutFeaturesKhr{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
};

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
            const DeviceFeatures& requestedDeviceFeatures = DeviceFeatures(),
            bool computeOnly = false);
    /// For headless rendering without a window (or when coupled with an OpenGL context in interoperability mode).
    void createDeviceHeadless(
            Instance* instance,
            const std::vector<const char*>& requiredDeviceExtensions = {},
            const std::vector<const char*>& optionalDeviceExtensions = {},
            const DeviceFeatures& requestedDeviceFeatures = DeviceFeatures(),
            bool computeOnly = false);
    ~Device();

    void waitIdle();

    /// Returns whether the device extension is supported.
    bool isDeviceExtensionSupported(const std::string& name);

    inline VkPhysicalDevice getVkPhysicalDevice() { return physicalDevice; }
    inline VkDevice getVkDevice() { return device; }
    inline VmaAllocator getAllocator() { return allocator; }
    inline VkQueue getGraphicsQueue() { return graphicsQueue; }
    inline VkQueue getComputeQueue() { return computeQueue; }
    inline VkQueue getWorkerThreadGraphicsQueue() { return workerThreadGraphicsQueue; } ///< For use in another thread.
    inline uint32_t getGraphicsQueueIndex() const { return graphicsQueueIndex; }
    inline uint32_t getComputeQueueIndex() const { return computeQueueIndex; }

    inline bool getIsMainThread() const { return mainThreadId == std::this_thread::get_id(); }

    // Helpers for querying physical device properties and features.
    inline const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const { return physicalDeviceProperties; }
    inline uint32_t getApiVersion() const { return physicalDeviceProperties.apiVersion; }
    inline uint32_t getDriverVersion() const { return physicalDeviceProperties.driverVersion; }
    inline uint32_t getVendorId() const { return physicalDeviceProperties.vendorID; }
    inline uint32_t getDeviceId() const { return physicalDeviceProperties.deviceID; }
    inline VkPhysicalDeviceType getDeviceType() const { return physicalDeviceProperties.deviceType; }
    inline const char* getDeviceName() const { return physicalDeviceProperties.deviceName; }
    inline const uint8_t* getPipelineCacheUuid() const { return physicalDeviceProperties.pipelineCacheUUID; }
    inline const VkPhysicalDeviceLimits& getLimits() const { return physicalDeviceProperties.limits; }
    inline const VkPhysicalDeviceSparseProperties& getSparseProperties() const { return physicalDeviceProperties.sparseProperties; }
    inline VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() { return physicalDeviceFeatures; }
    inline VkPhysicalDeviceAccelerationStructurePropertiesKHR getPhysicalDeviceAccelerationStructureProperties() const {
        return accelerationStructureProperties;
    }
    inline VkPhysicalDeviceAccelerationStructureFeaturesKHR getPhysicalDeviceAccelerationStructureFeatures() const {
        return accelerationStructureFeatures;
    }
    inline VkPhysicalDeviceRayTracingPipelinePropertiesKHR getPhysicalDeviceRayTracingPipelineProperties() const {
        return rayTracingPipelineProperties;
    }
    inline bool getRayQueriesSupported() const { return deviceRayQueryFeatures.rayQuery == VK_TRUE; }
    inline bool getRayTracingPipelineSupported() const { return rayTracingPipelineFeatures.rayTracingPipeline == VK_TRUE; }
    VkSampleCountFlagBits getMaxUsableSampleCount() const;

    VkFormat getSupportedDepthFormat(
            VkFormat hint = VK_FORMAT_D32_SFLOAT, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    std::vector<VkFormat> getSupportedDepthFormats(VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    VkFormat getSupportedDepthStencilFormat(
            VkFormat hint = VK_FORMAT_D24_UNORM_S8_UINT, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    std::vector<VkFormat> getSupportedDepthStencilFormats(VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);

    /**
     * @param memoryTypeBits
     * @param memoryPropertyFlags
     * @return The memoryTypeIndex
     */
    uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags);

    // Create command buffers. Remember to call vkFreeCommandBuffers (specifying the pool is necessary for this).
    VkCommandBuffer allocateCommandBuffer(
            CommandPoolType commandPoolType, VkCommandPool* pool,
            VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    std::vector<VkCommandBuffer> allocateCommandBuffers(
            CommandPoolType commandPoolType, VkCommandPool* pool, uint32_t count,
            VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void freeCommandBuffer(VkCommandPool commandPool, VkCommandBuffer commandBuffer);
    void freeCommandBuffers(VkCommandPool commandPool, const std::vector<VkCommandBuffer>& commandBuffers);

    // Create a transient command buffer ready to execute commands (0xFFFFFFFF encodes graphics queue).
    VkCommandBuffer beginSingleTimeCommands(uint32_t queueIndex = 0xFFFFFFFF, bool beginCommandBuffer = true);
    void endSingleTimeCommands(
            VkCommandBuffer commandBuffer, uint32_t queueIndex = 0xFFFFFFFF, bool endCommandBuffer = true);

    // Create multiple transient command buffers ready to execute commands (0xFFFFFFFF encodes graphics queue).
    std::vector<VkCommandBuffer> beginSingleTimeMultipleCommands(
            uint32_t numCommandBuffers, uint32_t queueIndex = 0xFFFFFFFF, bool beginCommandBuffer = true);
    void endSingleTimeMultipleCommands(
            const std::vector<VkCommandBuffer>& commandBuffers, uint32_t queueIndex = 0xFFFFFFFF,
            bool endCommandBuffer = true);

    inline Instance* getInstance() { return instance; }

    // Get the standard descriptor pool.
    VkDescriptorPool getDefaultVkDescriptorPool() { return descriptorPool; }

#ifdef SUPPORT_OPENGL
    /// This function must be called before the device is created.
    void setOpenGlInteropEnabled(bool enabled) { openGlInteropEnabled = true; }
#endif

    static std::vector<const char*> getCudaInteropDeviceExtensions();


private:
    void initializeDeviceExtensionList(VkPhysicalDevice physicalDevice);
    void printAvailableDeviceExtensionList();
    /// Returns whether the device extension is available in general (not necessarily enabled).
    bool _isDeviceExtensionAvailable(const std::string &extensionName);
    void writeDeviceInfoToLog(const std::vector<const char*>& deviceExtensions);

    void createVulkanMemoryAllocator();

    void createLogicalDeviceAndQueues(
            VkPhysicalDevice physicalDevice, bool useValidationLayer, const std::vector<const char*>& layerNames,
            const std::vector<const char*>& deviceExtensions, const std::set<std::string>& deviceExtensionsSet,
            DeviceFeatures requestedDeviceFeatures, bool computeOnly);
    uint32_t findQueueFamilies(VkPhysicalDevice physicalDevice, VkQueueFlagBits queueFlags);
    bool isDeviceSuitable(
            VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
            const std::vector<const char*>& requiredDeviceExtensions,
            const std::vector<const char*>& optionalDeviceExtensions,
            std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions,
            const DeviceFeatures& requestedDeviceFeatures, bool computeOnly);
    VkPhysicalDevice createPhysicalDeviceBinding(
            VkSurfaceKHR surface,
            const std::vector<const char*>& requiredDeviceExtensions,
            const std::vector<const char*>& optionalDeviceExtensions,
            std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions,
            const DeviceFeatures& requestedDeviceFeatures, bool computeOnly);

    void _getDeviceInformation();

    VkFormat _findSupportedFormat(
            const std::vector<VkFormat>& candidates, VkImageTiling imageTiling, VkFormatFeatureFlags features);

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

    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VmaAllocator allocator;

    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};

    // Ray tracing information.
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR uniformBufferStandardLayoutFeaturesKhr{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR deviceRayQueryFeatures{};

    // Queues for the logical device.
    uint32_t graphicsQueueIndex;
    uint32_t computeQueueIndex;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue workerThreadGraphicsQueue; ///< For use in another thread.
    std::thread::id mainThreadId;

    // Set of used command pools.
    std::unordered_map<CommandPoolType, VkCommandPool> commandPools;

    // Default descriptor pool.
    VkDescriptorPool descriptorPool;

    // Vulkan-OpenGL interoperability enabled?
    bool openGlInteropEnabled = false;
};

}}

#endif //SGL_DEVICE_HPP
