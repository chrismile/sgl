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
//#include <boost/container_hash/hash_fwd.hpp>

#include <Defs.hpp>
#include "../libs/volk/volk.h"
#include "../libs/VMA/vk_mem_alloc.h"
#		include <vulkan/vk_platform.h>

namespace sgl { class Window; }

namespace sgl { namespace vk {
/**
 * Different types of command pools are automatically allocated for some often used types of command buffers.
 */
struct DLL_OBJECT CommandPoolType {
    VkCommandPoolCreateFlags flags = 0;
    uint32_t queueFamilyIndex = 0xFFFFFFFF; // 0xFFFFFFFF encodes standard (graphics queue).
    uint32_t threadIndex = 0;

    bool operator==(const CommandPoolType& other) const {
        return flags == other.flags && queueFamilyIndex == other.queueFamilyIndex && other.threadIndex == threadIndex;
    }
};
}}

// See: https://stackoverflow.com/questions/4948780/magic-number-in-boosthash-combine
template <class T>
inline void hash_combine(std::size_t & s, const T & v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

namespace std {
template<> struct hash<sgl::vk::CommandPoolType> {
    std::size_t operator()(sgl::vk::CommandPoolType const& s) const noexcept {
        //std::size_t h1 = std::hash<uint32_t>{}(s.flags);
        //std::size_t h2 = std::hash<uint32_t>{}(s.queueFamilyIndex);
        //return h1 ^ (h2 << 1); // or use boost::hash_combine
        std::size_t result = 0;
        hash_combine(result, s.flags);
        hash_combine(result, s.queueFamilyIndex);
        hash_combine(result, s.threadIndex);
        return result;
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
        shaderFloat16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
        device8BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        fragmentShaderInterlockFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
        meshShaderFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
/*#ifdef VK_VERSION_1_1
        vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
#endif
#ifdef VK_VERSION_1_2
        vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#endif*/
#ifdef VK_VERSION_1_3
        vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
#endif
    }
    VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures{};
    VkPhysicalDeviceFeatures optionalPhysicalDeviceFeatures{};
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR uniformBufferStandardLayoutFeaturesKhr{};
    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features{};
    VkPhysicalDevice8BitStorageFeatures device8BitStorageFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{};
    VkPhysicalDeviceMeshShaderFeaturesNV meshShaderFeaturesNV{};
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeaturesNV{};
/*#ifdef VK_VERSION_1_1
    VkPhysicalDeviceVulkan11Features vulkan11Features{};
#endif
#ifdef VK_VERSION_1_2
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
#endif*/
#ifdef VK_VERSION_1_3
    VkPhysicalDeviceVulkan13Features vulkan13Features{};
#endif
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
            std::vector<const char*> optionalDeviceExtensions = {},
            const DeviceFeatures& requestedDeviceFeatures = DeviceFeatures(),
            bool computeOnly = false);
    /// For headless rendering without a window (or when coupled with an OpenGL context in interoperability mode).
    void createDeviceHeadless(
            Instance* instance,
            std::vector<const char*> requiredDeviceExtensions = {},
            std::vector<const char*> optionalDeviceExtensions = {},
            const DeviceFeatures& requestedDeviceFeatures = DeviceFeatures(),
            bool computeOnly = false);
    ~Device();

    /// Waits for the device to become idle.
    void waitIdle();
    /// Waits for the passed queue to become idle.
    void waitQueueIdle(VkQueue queue);
    inline void waitGraphicsQueueIdle() { waitQueueIdle(graphicsQueue); }
    inline void waitComputeQueueIdle() { waitQueueIdle(computeQueue); }
    inline void waitWorkerThreadGraphicsQueueIdle() { waitQueueIdle(workerThreadGraphicsQueue); }

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
    inline uint32_t getWorkerThreadGraphicsQueueIndex() const { return workerThreadGraphicsQueueIndex; }

    inline bool getIsMainThread() const { return mainThreadId == std::this_thread::get_id(); }

    // Helpers for querying physical device properties and features.
    inline const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const { return physicalDeviceProperties; }
    inline uint32_t getApiVersion() const { return physicalDeviceProperties.apiVersion; }
    inline uint32_t getDriverVersion() const { return physicalDeviceProperties.driverVersion; }
    inline uint32_t getVendorId() const { return physicalDeviceProperties.vendorID; }
    inline uint32_t getDeviceId() const { return physicalDeviceProperties.deviceID; }
    inline VkPhysicalDeviceType getDeviceType() const { return physicalDeviceProperties.deviceType; }
    inline const char* getDeviceName() const { return physicalDeviceProperties.deviceName; }
    inline VkDriverId getDeviceDriverId() const { return physicalDeviceDriverProperties.driverID; }
    inline const char* getDeviceDriverName() const { return physicalDeviceDriverProperties.driverName; }
    inline const char* getDeviceDriverInfo() const { return physicalDeviceDriverProperties.driverInfo; }
    inline const VkPhysicalDeviceIDProperties& getDeviceIDProperties() const { return physicalDeviceIDProperties; }
    inline const uint8_t* getPipelineCacheUuid() const { return physicalDeviceProperties.pipelineCacheUUID; }
    inline const VkPhysicalDeviceLimits& getLimits() const { return physicalDeviceProperties.limits; }
    inline const VkPhysicalDeviceSparseProperties& getSparseProperties() const { return physicalDeviceProperties.sparseProperties; }
    inline const VkPhysicalDeviceFeatures& getPhysicalDeviceFeatures() { return physicalDeviceFeatures; }
    inline const VkPhysicalDeviceShaderFloat16Int8Features& getPhysicalDeviceShaderFloat16Int8Features() const {
        return shaderFloat16Int8Features;
    }
    inline const VkPhysicalDevice8BitStorageFeatures& getPhysicalDevice8BitStorageFeatures() const {
        return device8BitStorageFeatures;
    }
    inline const VkPhysicalDeviceAccelerationStructurePropertiesKHR& getPhysicalDeviceAccelerationStructureProperties() const {
        return accelerationStructureProperties;
    }
    inline const VkPhysicalDeviceAccelerationStructureFeaturesKHR& getPhysicalDeviceAccelerationStructureFeatures() const {
        return accelerationStructureFeatures;
    }
    inline const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& getPhysicalDeviceRayTracingPipelineProperties() const {
        return rayTracingPipelineProperties;
    }
    inline bool getRayQueriesSupported() const { return rayQueryFeatures.rayQuery == VK_TRUE; }
    inline bool getRayTracingPipelineSupported() const { return rayTracingPipelineFeatures.rayTracingPipeline == VK_TRUE; }
    inline const VkPhysicalDeviceMeshShaderPropertiesNV& getPhysicalDeviceMeshShaderPropertiesNV() const {
        return meshShaderPropertiesNV;
    }
    inline const VkPhysicalDeviceMeshShaderFeaturesNV& getPhysicalDeviceMeshShaderFeaturesNV() const {
        return meshShaderFeaturesNV;
    }
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
#ifdef _WIN32
    static std::vector<const char*> getD3D12InteropDeviceExtensions();
#endif

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
    VmaAllocator allocator = VK_NULL_HANDLE;

    // Device properties.
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkPhysicalDeviceDriverProperties physicalDeviceDriverProperties{};
    VkPhysicalDeviceIDProperties physicalDeviceIDProperties{};
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
    VkPhysicalDeviceMeshShaderPropertiesNV meshShaderPropertiesNV{};

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR uniformBufferStandardLayoutFeaturesKhr{};
    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features{};
    VkPhysicalDevice8BitStorageFeatures device8BitStorageFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{};
    VkPhysicalDeviceMeshShaderFeaturesNV meshShaderFeaturesNV{};
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeaturesNV{};

    // Queues for the logical device.
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    uint32_t graphicsQueueIndex;
    uint32_t computeQueueIndex;
    uint32_t workerThreadGraphicsQueueIndex;
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
