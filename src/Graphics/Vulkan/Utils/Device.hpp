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
#include <tuple>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <functional>
#include <thread>
//#include <boost/container_hash/hash_fwd.hpp>

#define VK_ENABLE_BETA_EXTENSIONS
// Build with vcpkg headers and beta extensions broken as of 2023-08-28.

#include <Defs.hpp>
#include "../libs/volk/volk.h"
#include "../libs/VMA/vk_mem_alloc.h"
#include <vulkan/vk_platform.h>
#include <Utils/HashCombine.hpp>
#include "VulkanCompat.hpp"

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

struct DLL_OBJECT MemoryPoolType {
    /*
     * According to https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/custom_memory_pools.html,
     * "you shouldn't create images in a pool intended for buffers or the other way around".
     */
    uint32_t memoryTypeIndex = 0;
    bool isBufferPool = true;

    bool operator==(const MemoryPoolType& other) const {
        return memoryTypeIndex == other.memoryTypeIndex && isBufferPool == other.isBufferPool;
    }
};
}}

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
template<> struct hash<sgl::vk::MemoryPoolType> {
    std::size_t operator()(sgl::vk::MemoryPoolType const& s) const noexcept {
        //std::size_t h1 = std::hash<uint32_t>{}(s.flags);
        //std::size_t h2 = std::hash<uint32_t>{}(s.queueFamilyIndex);
        //return h1 ^ (h2 << 1); // or use boost::hash_combine
        std::size_t result = 0;
        hash_combine(result, s.memoryTypeIndex);
        hash_combine(result, s.isBufferPool);
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
        shaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        fragmentShaderInterlockFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
        shaderAtomicInt64Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR;
        shaderImageAtomicInt64Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
        shaderAtomicFloatFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
#ifdef VK_EXT_shader_atomic_float2
        shaderAtomicFloat2Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
#endif
        meshShaderFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
#ifdef VK_EXT_mesh_shader
        meshShaderFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
#endif
#ifdef VK_AMDX_shader_enqueue
        shaderEnqueueFeaturesAMDX.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ENQUEUE_FEATURES_AMDX;
#endif
#ifdef VK_NV_device_generated_commands
        deviceGeneratedCommandsFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV;
#endif
#ifdef VK_NV_device_generated_commands_compute
        deviceGeneratedCommandsComputeFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_COMPUTE_FEATURES_NV;
#endif
#ifdef VK_VERSION_1_1
        requestedVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        optionalVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
#endif
#ifdef VK_VERSION_1_2
        requestedVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        optionalVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#endif
#ifdef VK_VERSION_1_3
        requestedVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        optionalVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
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
    VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{};
    VkPhysicalDeviceShaderAtomicInt64FeaturesKHR shaderAtomicInt64Features{};
    VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT shaderImageAtomicInt64Features{};
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures{};
#ifdef VK_EXT_shader_atomic_float2
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features{};
#else
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT_Compat shaderAtomicFloat2Features{};
#endif
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeaturesNV{};
    VkPhysicalDeviceMeshShaderFeaturesNV meshShaderFeaturesNV{};
#ifdef VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT{};
#else
    VkPhysicalDeviceMeshShaderFeaturesEXT_Compat meshShaderFeaturesEXT{};
#endif
#ifdef VK_AMDX_shader_enqueue
    VkPhysicalDeviceShaderEnqueueFeaturesAMDX shaderEnqueueFeaturesAMDX{};
#else
    VkPhysicalDeviceShaderEnqueueFeaturesAMDX_Compat shaderEnqueueFeaturesAMDX{};
#endif
#ifdef VK_NV_device_generated_commands
    VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV deviceGeneratedCommandsFeaturesNV{};
#else
    VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV_Compat deviceGeneratedCommandsFeaturesNV{};
#endif
#ifdef VK_NV_device_generated_commands_compute
    VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV deviceGeneratedCommandsComputeFeaturesNV{};
#else
    VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV_Compat deviceGeneratedCommandsComputeFeaturesNV{};
#endif
#ifdef VK_NV_cooperative_matrix
    VkPhysicalDeviceCooperativeMatrixFeaturesNV cooperativeMatrixFeaturesNV{};
#else
    VkPhysicalDeviceCooperativeMatrixFeaturesNV_Compat cooperativeMatrixFeaturesNV{};
#endif
#ifdef VK_KHR_cooperative_matrix
    VkPhysicalDeviceCooperativeMatrixFeaturesKHR cooperativeMatrixFeaturesKHR{};
#else
    VkPhysicalDeviceCooperativeMatrixFeaturesKHR_Compat cooperativeMatrixFeaturesKHR{};
#endif
    // The following features have no extensions, thus use
    bool optionalEnableShaderDrawParametersFeatures = false;
    // Vulkan 1.x features are only enabled when at least one value in the struct is set to true.
#ifdef VK_VERSION_1_1
    VkPhysicalDeviceVulkan11Features requestedVulkan11Features{};
    VkPhysicalDeviceVulkan11Features optionalVulkan11Features{};
#else
    VkPhysicalDeviceVulkan11Features_Compat requestedVulkan11Features{};
    VkPhysicalDeviceVulkan11Features_Compat optionalVulkan11Features{};
#endif
#ifdef VK_VERSION_1_2
    VkPhysicalDeviceVulkan12Features requestedVulkan12Features{};
    VkPhysicalDeviceVulkan12Features optionalVulkan12Features{};
#else
    VkPhysicalDeviceVulkan12Features_Compat requestedVulkan12Features{};
    VkPhysicalDeviceVulkan12Features_Compat optionalVulkan12Features{};
#endif
#ifdef VK_VERSION_1_3
    VkPhysicalDeviceVulkan13Features requestedVulkan13Features{};
    VkPhysicalDeviceVulkan13Features optionalVulkan13Features{};
#else
    VkPhysicalDeviceVulkan13Features_Compat requestedVulkan13Features{};
    VkPhysicalDeviceVulkan13Features_Compat optionalVulkan13Features{};
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
    [[nodiscard]] const std::vector<const char*>& getEnabledDeviceExtensionNames() const {
        return enabledDeviceExtensionNames;
    }

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
    inline uint32_t getMaxStorageBufferRange() const { return physicalDeviceProperties.limits.maxStorageBufferRange; }
    inline VkDeviceSize getMaxMemoryAllocationSize() const { return physicalDeviceVulkan11Properties.maxMemoryAllocationSize; }
    inline const VkPhysicalDeviceSparseProperties& getSparseProperties() const { return physicalDeviceProperties.sparseProperties; }
    inline const VkPhysicalDeviceFeatures& getPhysicalDeviceFeatures() { return physicalDeviceFeatures; }
    inline const VkPhysicalDeviceShaderFloat16Int8Features& getPhysicalDeviceShaderFloat16Int8Features() const {
        return shaderFloat16Int8Features;
    }
    inline const VkPhysicalDevice8BitStorageFeatures& getPhysicalDevice8BitStorageFeatures() const {
        return device8BitStorageFeatures;
    }
    inline const VkPhysicalDeviceShaderAtomicInt64FeaturesKHR& getPhysicalDeviceShaderAtomicInt64Features() const {
        return shaderAtomicInt64Features;
    }
    inline const VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT& getPhysicalDeviceShaderImageAtomicInt64Features() const {
        return shaderImageAtomicInt64Features;
    }
    inline const VkPhysicalDeviceShaderAtomicFloatFeaturesEXT& getPhysicalDeviceShaderAtomicFloatFeatures() const {
        return shaderAtomicFloatFeatures;
    }
#ifdef VK_EXT_shader_atomic_float2
    inline const VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT& getPhysicalDeviceShaderAtomicFloat2Features() const {
        return shaderAtomicFloat2Features;
    }
#endif
    inline const VkPhysicalDeviceShaderDrawParametersFeatures& getPhysicalDeviceShaderDrawParametersFeatures() const {
        return shaderDrawParametersFeatures;
    }
    inline const VkPhysicalDeviceSubgroupProperties& getPhysicalDeviceSubgroupProperties() const {
        return subgroupProperties;
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
#ifdef VK_EXT_mesh_shader
    inline const VkPhysicalDeviceMeshShaderPropertiesEXT& getPhysicalDeviceMeshShaderPropertiesEXT() const {
        return meshShaderPropertiesEXT;
    }
    inline const VkPhysicalDeviceMeshShaderFeaturesEXT& getPhysicalDeviceMeshShaderFeaturesEXT() const {
        return meshShaderFeaturesEXT;
    }
#endif
#ifdef VK_AMDX_shader_enqueue
    inline const VkPhysicalDeviceShaderEnqueuePropertiesAMDX& getPhysicalDeviceShaderEnqueuePropertiesAMDX() const {
        return shaderEnqueuePropertiesAMDX;
    }
    inline const VkPhysicalDeviceShaderEnqueueFeaturesAMDX& getPhysicalDeviceShaderEnqueueFeaturesAMDX() const {
        return shaderEnqueueFeaturesAMDX;
    }
    /// Warning: Instable API for testing only, may be removed at any point of time in the future.
    FunctionTableShaderEnqueueAMDX getFunctionTableShaderEnqueueAMDX();
#endif
#ifdef VK_NV_device_generated_commands
    inline const VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV& getPhysicalDeviceDeviceGeneratedCommandsPropertiesNV() const {
        return deviceGeneratedCommandsPropertiesNV;
    }
    inline const VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV& getPhysicalDeviceDeviceGeneratedCommandsFeaturesNV() const {
        return deviceGeneratedCommandsFeaturesNV;
    }
    /// Warning: Instable API for testing only, may be removed at any point of time in the future.
    FunctionTableDeviceGeneratedCommandsNV getFunctionTableDeviceGeneratedCommandsNV();
#endif
#ifdef VK_NV_device_generated_commands_compute
    inline const VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV& getPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV() const {
        return deviceGeneratedCommandsComputeFeaturesNV;
    }
    /// Warning: Instable API for testing only, may be removed at any point of time in the future.
    FunctionTableDeviceGeneratedCommandsComputeNV getFunctionTableDeviceGeneratedCommandsComputeNV();
#endif
#ifdef VK_NV_cooperative_matrix
    inline const VkPhysicalDeviceCooperativeMatrixFeaturesNV& getCooperativeMatrixFeaturesNV() const {
        return cooperativeMatrixFeaturesNV;
    }
    const std::vector<VkCooperativeMatrixPropertiesNV>& getSupportedCooperativeMatrixPropertiesNV();
#endif
#ifdef VK_KHR_cooperative_matrix
    inline const VkPhysicalDeviceCooperativeMatrixFeaturesKHR& getCooperativeMatrixFeaturesKHR() const {
        return cooperativeMatrixFeaturesKHR;
    }
    const std::vector<VkCooperativeMatrixPropertiesKHR>& getSupportedCooperativeMatrixPropertiesKHR();
#endif

    VkSampleCountFlagBits getMaxUsableSampleCount() const;

    VkFormat getSupportedDepthFormat(
            VkFormat hint = VK_FORMAT_D32_SFLOAT, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    std::vector<VkFormat> getSupportedDepthFormats(VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    VkFormat getSupportedDepthStencilFormat(
            VkFormat hint = VK_FORMAT_D24_UNORM_S8_UINT, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    std::vector<VkFormat> getSupportedDepthStencilFormats(VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);

    /**
     * Returns the index of the memory type with the corresponding bits and property flags.
     * @param memoryTypeBits The memory type bits as returned by, e.g., VkMemoryRequirements::memoryTypeBits.
     * @param memoryPropertyFlags The requested memory property flags.
     * @return The index of the first suitable memory type.
     */
    uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags);

    /**
     * Returns the index of the first memory heap with the requested flags.
     * @param heapFlags The heap flags.
     * @return The index of the first memory heap with the requested flags.
     */
    VkDeviceSize findMemoryHeapIndex(VkMemoryHeapFlagBits heapFlags);

    /**
     * Returns the amount of available heap memory.
     * NOTE: VK_EXT_memory_budget needs to be used for this function to work correctly.
     * @param memoryHeapIndex The index of the memory heap. NOTE: This is NOT equal to the memory type index!
     * @return The memory budget for the heap in bytes.
     */
    VkDeviceSize getMemoryHeapBudget(uint32_t memoryHeapIndex);

    /**
     * Returns the amount of available heap memory. Compared to @see getMemoryHeapBudget, VMA has some fallbacks if
     * VK_EXT_memory_budget is not supported by the system.
     * @param memoryHeapIndex The index of the memory heap. NOTE: This is NOT equal to the memory type index!
     * @return The memory budget for the heap in bytes.
     */
    VkDeviceSize getMemoryHeapBudgetVma(uint32_t memoryHeapIndex);

    // Create command buffers. Remember to call vkFreeCommandBuffers (specifying the pool is necessary for this).
    VkCommandBuffer allocateCommandBuffer(
            CommandPoolType commandPoolType, VkCommandPool* pool,
            VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    std::vector<VkCommandBuffer> allocateCommandBuffers(
            CommandPoolType commandPoolType, VkCommandPool* pool, uint32_t count,
            VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void freeCommandBuffer(VkCommandPool commandPool, VkCommandBuffer commandBuffer);
    void freeCommandBuffers(VkCommandPool commandPool, const std::vector<VkCommandBuffer>& commandBuffers);

    // Query memory pools (automatically created).
    VmaPool getExternalMemoryHandlePool(uint32_t memoryTypeIndex, bool isBuffer);

    // Queries device memory size of device memory allocated by VMA.
    VkDeviceSize getVmaDeviceMemoryAllocationSize(VkDeviceMemory deviceMemory);

    // Query information about external memory support of the device driver.
    bool getNeedsDedicatedAllocationForExternalMemoryImage(
            VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
            VkExternalMemoryHandleTypeFlagBits handleType);
    bool getNeedsDedicatedAllocationForExternalMemoryBuffer(
            VkBufferUsageFlags usage, VkBufferCreateFlags flags, VkExternalMemoryHandleTypeFlagBits handleType);

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

    // The functions below should only be called by the VMA callbacks.
    void vmaAllocateDeviceMemoryCallback(uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size);
    void vmaFreeDeviceMemoryCallback(uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size);

    // Access to function pointers from volk.
    static PFN_vkGetDeviceProcAddr getVkDeviceProcAddrFunctionPointer();
    void loadVolkDeviceFunctionTable(VolkDeviceTable* table);

private:
    void initializeDeviceExtensionList(VkPhysicalDevice physicalDevice);
    void printAvailableDeviceExtensionList();
    /// Returns whether the device extension is available in general (not necessarily enabled).
    bool _isDeviceExtensionAvailable(const std::string &extensionName);
    void writeDeviceInfoToLog(const std::vector<const char*>& deviceExtensions);

    void createVulkanMemoryAllocator();
    VmaPool createExternalMemoryHandlePool(uint32_t memoryTypeIndex);

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
    std::vector<const char*> enabledDeviceExtensionNames;

    // Theoretically available (but not necessarily enabled) device extensions.
    std::set<std::string> availableDeviceExtensionNames;

    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    std::unordered_map<MemoryPoolType, VmaPool> externalMemoryHandlePools;
    VkExportMemoryAllocateInfo exportMemoryAllocateInfo{}; ///< Must remain in scope for use in VMA.
    std::unordered_map<VkDeviceMemory, VkDeviceSize> deviceMemoryToSizeMap;

    // Information on external memory properties.
    typedef std::tuple<VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags,
            VkExternalMemoryHandleTypeFlagBits> ExternalMemoryImageConfigTuple;
    std::map<ExternalMemoryImageConfigTuple, bool> needsDedicatedAllocationForExternalMemoryImageMap;
    typedef std::tuple<VkBufferUsageFlags, VkBufferCreateFlags,
            VkExternalMemoryHandleTypeFlagBits> ExternalMemoryBufferConfigTuple;
    std::map<ExternalMemoryBufferConfigTuple, bool> needsDedicatedAllocationForExternalMemoryBufferMap;

    // Device properties.
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkPhysicalDeviceDriverProperties physicalDeviceDriverProperties{};
    VkPhysicalDeviceIDProperties physicalDeviceIDProperties{};
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
#ifdef VK_VERSION_1_1
    VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features{};
    VkPhysicalDeviceVulkan11Properties physicalDeviceVulkan11Properties{};
#else
    VkPhysicalDeviceVulkan11Features_Compat physicalDeviceVulkan11Features{};
    VkPhysicalDeviceVulkan11Properties_Compat physicalDeviceVulkan11Properties{};
#endif
#ifdef VK_VERSION_1_2
    VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{};
#else
    VkPhysicalDeviceVulkan12Features_Compat physicalDeviceVulkan12Features{};
#endif
#ifdef VK_VERSION_1_3
    VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
#else
    VkPhysicalDeviceVulkan13Features_Compat physicalDeviceVulkan13Features{};
#endif
    VkPhysicalDeviceSubgroupProperties subgroupProperties{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
    VkPhysicalDeviceMeshShaderPropertiesNV meshShaderPropertiesNV{};
#ifdef VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderPropertiesEXT{};
#else
    VkPhysicalDeviceMeshShaderPropertiesEXT_Compat meshShaderPropertiesEXT{};
#endif
#ifdef VK_AMDX_shader_enqueue
    VkPhysicalDeviceShaderEnqueuePropertiesAMDX shaderEnqueuePropertiesAMDX{};
#else
    VkPhysicalDeviceShaderEnqueuePropertiesAMDX_Compat shaderEnqueuePropertiesAMDX{};
#endif
#ifdef VK_NV_device_generated_commands
    VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV deviceGeneratedCommandsPropertiesNV{};
#else
    VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV_Compat deviceGeneratedCommandsPropertiesNV{};
#endif

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR uniformBufferStandardLayoutFeaturesKhr{};
    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features{};
    VkPhysicalDevice8BitStorageFeatures device8BitStorageFeatures{};
    VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{};
    VkPhysicalDeviceShaderAtomicInt64FeaturesKHR shaderAtomicInt64Features{};
    VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT shaderImageAtomicInt64Features{};
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures{};
#ifdef VK_EXT_shader_atomic_float2
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features{};
#else
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT_Compat shaderAtomicFloat2Features{};
#endif
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeaturesNV{};
    VkPhysicalDeviceMeshShaderFeaturesNV meshShaderFeaturesNV{};
#ifdef VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT{};
#else
    VkPhysicalDeviceMeshShaderFeaturesEXT_Compat meshShaderFeaturesEXT{};
#endif
#ifdef VK_AMDX_shader_enqueue
    VkPhysicalDeviceShaderEnqueueFeaturesAMDX shaderEnqueueFeaturesAMDX{};
#else
    VkPhysicalDeviceShaderEnqueueFeaturesAMDX_Compat shaderEnqueueFeaturesAMDX{};
#endif
#ifdef VK_NV_device_generated_commands
    VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV deviceGeneratedCommandsFeaturesNV{};
#else
    VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV_Compat deviceGeneratedCommandsFeaturesNV{};
#endif
#ifdef VK_NV_device_generated_commands_compute
    VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV deviceGeneratedCommandsComputeFeaturesNV{};
#else
    VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV_Compat deviceGeneratedCommandsComputeFeaturesNV{};
#endif
#ifdef VK_NV_cooperative_matrix
    VkPhysicalDeviceCooperativeMatrixFeaturesNV cooperativeMatrixFeaturesNV{};
    std::vector<VkCooperativeMatrixPropertiesNV> supportedCooperativeMatrixPropertiesNV;
#else
    VkPhysicalDeviceCooperativeMatrixFeaturesNV_Compat cooperativeMatrixFeaturesNV{};
    std::vector<VkPhysicalDeviceCooperativeMatrixFeaturesNV_Compat> supportedCooperativeMatrixPropertiesNV;
#endif
#ifdef VK_KHR_cooperative_matrix
    VkPhysicalDeviceCooperativeMatrixFeaturesKHR cooperativeMatrixFeaturesKHR{};
    std::vector<VkCooperativeMatrixPropertiesKHR> supportedCooperativeMatrixPropertiesKHR;
#else
    VkPhysicalDeviceCooperativeMatrixFeaturesKHR_Compat cooperativeMatrixFeaturesKHR{};
    std::vector<VkPhysicalDeviceCooperativeMatrixFeaturesKHR_Compat> supportedCooperativeMatrixPropertiesKHR;
#endif
    bool isInitializedSupportedCooperativeMatrixPropertiesNV = false;
    bool isInitializedSupportedCooperativeMatrixPropertiesKHR = false;

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
