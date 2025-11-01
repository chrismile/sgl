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
#include <memory>
#include <thread>

#include <Defs.hpp>
#include "../libs/volk/volk.h"
#include "../libs/VMA/vk_mem_alloc.h"
#include <vulkan/vk_platform.h>
#include <Utils/HashCombine.hpp>

#include "VulkanCompat.hpp"

namespace sgl {
class Window;
class DeviceSelectorVulkan;
}

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
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

struct DLL_OBJECT DeviceFeatures {
    DeviceFeatures() {
        timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        scalarBlockLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
        uniformBufferStandardLayoutFeaturesKhr.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES;
        shaderFloat16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
        device16BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
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
#ifdef VK_EXT_mutable_descriptor_type
        mutableDescriptorTypeFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
#endif
#ifdef VK_EXT_shader_atomic_float2
        shaderAtomicFloat2Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
#endif
#ifdef VK_KHR_shader_bfloat16
        shaderBfloat16Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_BFLOAT16_FEATURES_KHR;
#endif
        meshShaderFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
#ifdef VK_EXT_mesh_shader
        meshShaderFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
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
#ifdef VK_VERSION_1_4
        requestedVulkan14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
        optionalVulkan14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
#endif
    }
    /// Returns false if a pNext entry is not supported by sgl.
    bool setRequestedFeaturesFromPNextChain(const void* pNext, std::vector<const char*>& requiredDeviceExtensions);
    bool setOptionalFeaturesFromPNextChain(const void* pNext, std::vector<const char*>& optionalDeviceExtensions);
    /// Called by the two functions above.
    bool setExtensionFeaturesFromPNextEntry(const void* pNext, std::vector<const char*>& deviceExtensions);
    VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures{};
    VkPhysicalDeviceFeatures optionalPhysicalDeviceFeatures{};
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR uniformBufferStandardLayoutFeaturesKhr{};
    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features{};
    VkPhysicalDevice16BitStorageFeatures device16BitStorageFeatures{};
    VkPhysicalDevice8BitStorageFeatures device8BitStorageFeatures{};
    VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{};
    VkPhysicalDeviceShaderAtomicInt64FeaturesKHR shaderAtomicInt64Features{};
    VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT shaderImageAtomicInt64Features{};
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures{};
#ifdef VK_EXT_mutable_descriptor_type
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeatures{};
#else
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT_Compat mutableDescriptorTypeFeatures{};
#endif
#ifdef VK_EXT_shader_atomic_float2
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features{};
#else
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT_Compat shaderAtomicFloat2Features{};
#endif
#ifdef VK_KHR_shader_bfloat16
    VkPhysicalDeviceShaderBfloat16FeaturesKHR shaderBfloat16Features{};
#else
    VkPhysicalDeviceShaderBfloat16FeaturesKHR_Compat shaderBfloat16Features{};
#endif
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeaturesNV{};
    VkPhysicalDeviceMeshShaderFeaturesNV meshShaderFeaturesNV{};
#ifdef VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT{};
#else
    VkPhysicalDeviceMeshShaderFeaturesEXT_Compat meshShaderFeaturesEXT{};
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
#ifdef VK_NV_cooperative_matrix2
    VkPhysicalDeviceCooperativeMatrix2FeaturesNV cooperativeMatrix2FeaturesNV{};
#else
    VkPhysicalDeviceCooperativeMatrix2FeaturesNV_Compat cooperativeMatrix2FeaturesNV{};
#endif
#ifdef VK_NV_cooperative_vector
    VkPhysicalDeviceCooperativeVectorFeaturesNV cooperativeVectorFeaturesNV{};
#else
    VkPhysicalDeviceCooperativeVectorFeaturesNV_Compat cooperativeVectorFeaturesNV{};
#endif
#ifdef VK_EXT_shader_64bit_indexing
    VkPhysicalDeviceShader64BitIndexingFeaturesEXT shader64BitIndexingFeaturesEXT{};
#else
    VkPhysicalDeviceShader64BitIndexingFeaturesEXT_Compat shader64BitIndexingFeaturesEXT{};
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
#ifdef VK_VERSION_1_4
    VkPhysicalDeviceVulkan14Features requestedVulkan14Features{};
    VkPhysicalDeviceVulkan14Features optionalVulkan14Features{};
#else
    VkPhysicalDeviceVulkan14Features_Compat requestedVulkan14Features{};
    VkPhysicalDeviceVulkan14Features_Compat optionalVulkan14Features{};
#endif
};

DLL_OBJECT void mergePhysicalDeviceFeatures(
        VkPhysicalDeviceFeatures& featuresDst, const VkPhysicalDeviceFeatures& featuresSrc);
#ifdef VK_VERSION_1_1
DLL_OBJECT void mergePhysicalDeviceFeatures11(
        VkPhysicalDeviceVulkan11Features& featuresDst, const VkPhysicalDeviceVulkan11Features& featuresSrc);
#else
DLL_OBJECT void mergePhysicalDeviceFeatures11(
        VkPhysicalDeviceVulkan11Features_Compat& featuresDst, const VkPhysicalDeviceVulkan11Features_Compat& featuresSrc);
#endif
#ifdef VK_VERSION_1_2
DLL_OBJECT void mergePhysicalDeviceFeatures12(
        VkPhysicalDeviceVulkan12Features& featuresDst, const VkPhysicalDeviceVulkan12Features& featuresSrc);
#else
DLL_OBJECT void mergePhysicalDeviceFeatures12(
        VkPhysicalDeviceVulkan12Features_Compat& featuresDst, const VkPhysicalDeviceVulkan12Features_Compat& featuresSrc);
#endif
#ifdef VK_VERSION_1_3
DLL_OBJECT void mergePhysicalDeviceFeatures13(
        VkPhysicalDeviceVulkan13Features& featuresDst, const VkPhysicalDeviceVulkan13Features& featuresSrc);
#else
DLL_OBJECT void mergePhysicalDeviceFeatures13(
        VkPhysicalDeviceVulkan13Features_Compat& featuresDst, const VkPhysicalDeviceVulkan13Features_Compat& featuresSrc);
#endif
#ifdef VK_VERSION_1_4
DLL_OBJECT void mergePhysicalDeviceFeatures14(
        VkPhysicalDeviceVulkan14Features& featuresDst, const VkPhysicalDeviceVulkan14Features& featuresSrc);
#else
DLL_OBJECT void mergePhysicalDeviceFeatures14(
        VkPhysicalDeviceVulkan14Features_Compat& featuresDst, const VkPhysicalDeviceVulkan14Features_Compat& featuresSrc);
#endif

// Public interface for checking all available devices on the user side.
DLL_OBJECT std::vector<VkPhysicalDevice> enumeratePhysicalDevices(Instance* instance);
DLL_OBJECT bool checkIsPhysicalDeviceSuitable(
        Instance* instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
        const std::vector<const char*>& requiredDeviceExtensions,
        const DeviceFeatures& requestedDeviceFeatures, bool computeOnly = false);
DLL_OBJECT void getPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties& deviceProperties);

// Wrapper for use in PhysicalDeviceCheckCallback.
DLL_OBJECT void getPhysicalDeviceProperties2(
        VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2& deviceProperties2);

typedef std::function<bool(VkPhysicalDevice, VkPhysicalDeviceProperties, std::vector<const char*>&, std::vector<const char*>&, DeviceFeatures&)> PhysicalDeviceCheckCallback;

struct DriverVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t subminor;
    uint32_t patch;
};

/**
 * An encapsulation of VkDevice and VkPhysicalDevice.
 */
class DLL_OBJECT Device {
public:
#ifndef DISABLE_VULKAN_SWAPCHAIN_SUPPORT
    /// For rendering using a window surface and a swapchain. VK_KHR_SWAPCHAIN_EXTENSION_NAME is added automatically.
    void createDeviceSwapchain(
            Instance* instance, Window* window,
            std::vector<const char*> requiredDeviceExtensions = {},
            std::vector<const char*> optionalDeviceExtensions = {},
            const DeviceFeatures& requestedDeviceFeaturesIn = DeviceFeatures(),
            bool computeOnly = false);
#endif
    /// For headless rendering without a window (or when coupled with an OpenGL context in interoperability mode).
    void createDeviceHeadless(
            Instance* instance,
            std::vector<const char*> requiredDeviceExtensions = {},
            std::vector<const char*> optionalDeviceExtensions = {},
            const DeviceFeatures& requestedDeviceFeaturesIn = DeviceFeatures(),
            bool computeOnly = false);
    void createDeviceHeadlessFromPhysicalDevice(
            Instance* instance, VkPhysicalDevice usedPhysicalDevice,
            std::vector<const char*> requiredDeviceExtensions = {},
            std::vector<const char*> optionalDeviceExtensions = {},
            const DeviceFeatures& requestedDeviceFeaturesIn = DeviceFeatures(),
            bool computeOnly = false);
    ~Device();

    /*
     * Callback for adding required and optional physical device-dependent extensions and features.
     * Example:
     * auto physicalDeviceCheckCallback = [](
     *         VkPhysicalDevice physicalDevice,
     *         VkPhysicalDeviceProperties physicalDeviceProperties,
     *         std::vector<const char*>& requiredDeviceExtensions,
     *         std::vector<const char*>& optionalDeviceExtensions,
     *         sgl::vk::DeviceFeatures& requestedDeviceFeatures) {
     *     // ...
     *     return true; // false if physical device is not suitable.
     * }
     */
    inline void setPhysicalDeviceCheckCallback(PhysicalDeviceCheckCallback callback) {
        physicalDeviceCheckCallback = std::move(callback);
    }

    /**
     * Enables the use of DeviceSelectorVulkan, which lets the user decide on the device to use in the UI.
     */
    inline void setUseAppDeviceSelector() {
        useAppDeviceSelector = true;
    }
    [[nodiscard]] inline DeviceSelectorVulkan* getDeviceSelector() { return deviceSelector; }

    /// Waits for the device to become idle.
    void waitIdle();
    /// Waits for the passed queue to become idle.
    void waitQueueIdle(VkQueue queue);
    inline void waitGraphicsQueueIdle() { waitQueueIdle(graphicsQueue); }
    inline void waitComputeQueueIdle() { waitQueueIdle(computeQueue); }
    inline void waitWorkerThreadGraphicsQueueIdle() { waitQueueIdle(workerThreadGraphicsQueue); }
    /*
     * Some vendors (e.g., Intel) provide only one graphics queue, thus does not support multi-threaded rendering
     * without adding a lock around the queue. The app can tell sgl if it wants to use multiple queues.
     * If this is not called, sgl may only allocate a single graphics queue. Using this function does NOT mean that GPUs
     * with a single graphics queue fail during device creation!
     */
    void setAppWantsMultithreadedRendering();

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
    inline const std::string& getDriverVersionString() const { return driverVersionString; }
    bool getIsDriverVersionGreaterOrEqual(const DriverVersion& driverVersionComp) const;
    inline bool getIsDriverVersionLessThan(const DriverVersion& driverVersionComp) const { return !getIsDriverVersionGreaterOrEqual(driverVersionComp); }
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
    inline const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const { return physicalDeviceMemoryProperties; }
    inline const VkPhysicalDeviceFeatures& getPhysicalDeviceFeatures() { return physicalDeviceFeatures; }
#ifdef VK_VERSION_1_1
    const VkPhysicalDeviceVulkan11Features& getPhysicalDeviceVulkan11Features() const { return physicalDeviceVulkan11Features; }
    const VkPhysicalDeviceVulkan11Properties& getPhysicalDeviceVulkan11Properties() const { return physicalDeviceVulkan11Properties; }
#else
    const VkPhysicalDeviceVulkan11Features_Compat& getPhysicalDeviceVulkan11Features() const { return physicalDeviceVulkan11Features; }
    const VkPhysicalDeviceVulkan11Properties_Compat& getPhysicalDeviceVulkan11Properties() const { return physicalDeviceVulkan11Properties; }
#endif
#ifdef VK_VERSION_1_2
    const VkPhysicalDeviceVulkan12Features& getPhysicalDeviceVulkan12Features() const { return physicalDeviceVulkan12Features; }
#else
    const VkPhysicalDeviceVulkan12Features_Compat& getPhysicalDeviceVulkan12Features() const { return physicalDeviceVulkan12Features; }
#endif
#ifdef VK_VERSION_1_3
    const VkPhysicalDeviceVulkan13Features& getPhysicalDeviceVulkan13Features() const { return physicalDeviceVulkan13Features; }
    const VkPhysicalDeviceVulkan13Properties& getPhysicalDeviceVulkan13Properties() const { return physicalDeviceVulkan13Properties; }
#else
    const VkPhysicalDeviceVulkan13Features_Compat& getPhysicalDeviceVulkan13Features() const { return physicalDeviceVulkan13Features; }
    const VkPhysicalDeviceVulkan13Properties_Compat& getPhysicalDeviceVulkan13Properties() const { return physicalDeviceVulkan13Properties; }
#endif
#ifdef VK_VERSION_1_4
    const VkPhysicalDeviceVulkan14Features& getPhysicalDeviceVulkan14Features() const { return physicalDeviceVulkan14Features; }
    const VkPhysicalDeviceVulkan14Properties& getPhysicalDeviceVulkan14Properties() const { return physicalDeviceVulkan14Properties; }
#else
    const VkPhysicalDeviceVulkan14Features_Compat& getPhysicalDeviceVulkan14Features() const { return physicalDeviceVulkan14Features; }
    const VkPhysicalDeviceVulkan14Properties_Compat& getPhysicalDeviceVulkan14Properties() const { return physicalDeviceVulkan14Properties; }
#endif
    inline const VkPhysicalDeviceShaderFloat16Int8Features& getPhysicalDeviceShaderFloat16Int8Features() const {
        return shaderFloat16Int8Features;
    }
    inline const VkPhysicalDevice16BitStorageFeatures& getPhysicalDevice16BitStorageFeatures() const {
        return device16BitStorageFeatures;
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
#ifdef VK_EXT_mutable_descriptor_type
    inline const VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT& getVkPhysicalDeviceMutableDescriptorTypeFeatures() const {
        return mutableDescriptorTypeFeatures;
    }
#endif
#ifdef VK_EXT_shader_atomic_float2
    inline const VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT& getPhysicalDeviceShaderAtomicFloat2Features() const {
        return shaderAtomicFloat2Features;
    }
#endif
#ifdef VK_KHR_shader_bfloat16
    inline const VkPhysicalDeviceShaderBfloat16FeaturesKHR& getPhysicalDeviceShaderBfloat16Features() const {
        return shaderBfloat16Features;
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
#ifdef VK_NV_cooperative_matrix
    inline const VkPhysicalDeviceCooperativeMatrixFeaturesNV& getCooperativeMatrixFeaturesNV() const {
        return cooperativeMatrixFeaturesNV;
    }
    inline const VkPhysicalDeviceCooperativeMatrixPropertiesNV& getCooperativeMatrixPropertiesNV() const {
        return cooperativeMatrixPropertiesNV;
    }
    const std::vector<VkCooperativeMatrixPropertiesNV>& getSupportedCooperativeMatrixPropertiesNV();
#endif
#ifdef VK_KHR_cooperative_matrix
    inline const VkPhysicalDeviceCooperativeMatrixFeaturesKHR& getCooperativeMatrixFeaturesKHR() const {
        return cooperativeMatrixFeaturesKHR;
    }
    inline const VkPhysicalDeviceCooperativeMatrixPropertiesKHR& getCooperativeMatrixPropertiesKHR() const {
        return cooperativeMatrixPropertiesKHR;
    }
    const std::vector<VkCooperativeMatrixPropertiesKHR>& getSupportedCooperativeMatrixPropertiesKHR();
#endif
#ifdef VK_NV_cooperative_matrix2
    inline const VkPhysicalDeviceCooperativeMatrix2FeaturesNV& getCooperativeMatrix2FeaturesNV() const {
        return cooperativeMatrix2FeaturesNV;
    }
    inline const VkPhysicalDeviceCooperativeMatrix2PropertiesNV& getCooperativeMatrix2PropertiesNV() const {
        return cooperativeMatrix2PropertiesNV;
    }
    const std::vector<VkCooperativeMatrixFlexibleDimensionsPropertiesNV>& getSupportedCooperativeMatrixFlexibleDimensionsPropertiesNV();
#endif
#ifdef VK_NV_cooperative_vector
    inline const VkPhysicalDeviceCooperativeVectorFeaturesNV& getCooperativeVectorFeaturesNV() const {
        return cooperativeVectorFeaturesNV;
    }
    inline const VkPhysicalDeviceCooperativeVectorPropertiesNV& getCooperativeVectorPropertiesNV() const {
        return cooperativeVectorPropertiesNV;
    }
    const std::vector<VkCooperativeVectorPropertiesNV>& getSupportedCooperativeVectorPropertiesNV();
    void convertCooperativeVectorMatrixNV(const VkConvertCooperativeVectorMatrixInfoNV& convertCoopVecMatInfo);
#endif
    const VkPhysicalDeviceShaderCorePropertiesAMD& getDeviceShaderCorePropertiesAMD();
    const VkPhysicalDeviceShaderCoreProperties2AMD& getDeviceShaderCoreProperties2AMD();
#ifdef VK_EXT_shader_64bit_indexing
    inline const VkPhysicalDeviceShader64BitIndexingFeaturesEXT& getShader64BitIndexingFeaturesEXT() const {
        return shader64BitIndexingFeaturesEXT;
    }
#endif

    VkSampleCountFlagBits getMaxUsableSampleCount() const;

    VkFormat getSupportedDepthFormat(
            VkFormat hint = VK_FORMAT_D32_SFLOAT, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    std::vector<VkFormat> getSupportedDepthFormats(VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    VkFormat getSupportedDepthStencilFormat(
            VkFormat hint = VK_FORMAT_D24_UNORM_S8_UINT, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    std::vector<VkFormat> getSupportedDepthStencilFormats(VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    bool getSupportsFormat(VkFormat format, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL);
    void getPhysicalDeviceFormatProperties2(VkFormat format, VkFormatProperties2& formatProperties);

    bool checkPhysicalDeviceFeaturesSupported(const VkPhysicalDeviceFeatures& featuresRequired);
#ifdef VK_VERSION_1_1
    bool checkPhysicalDeviceFeatures11Supported(const VkPhysicalDeviceVulkan11Features& featuresRequired);
#else
    bool checkPhysicalDeviceFeatures11Supported(const VkPhysicalDeviceVulkan11Features_Compat& featuresRequired);
#endif
#ifdef VK_VERSION_1_2
    bool checkPhysicalDeviceFeatures12Supported(const VkPhysicalDeviceVulkan12Features& featuresRequired);
#else
    bool checkPhysicalDeviceFeatures12Supported(const VkPhysicalDeviceVulkan12Features_Compat& featuresRequired);
#endif

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
    uint32_t findMemoryHeapIndex(VkMemoryHeapFlagBits heapFlags);

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

    // Synchronization primitives.
    void insertMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            VkCommandBuffer commandBuffer);
    void insertBufferMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
            const BufferPtr& buffer, VkCommandBuffer commandBuffer);

    inline Instance* getInstance() { return instance; }

    // Get the standard descriptor pool.
    VkDescriptorPool getDefaultVkDescriptorPool() { return descriptorPool; }

#ifdef SUPPORT_OPENGL
    /// This function must be called before the device is created.
    void setOpenGlInteropEnabled(bool enabled) { openGlInteropEnabled = true; }
#endif

    static std::vector<const char*> getCudaInteropDeviceExtensions();
    static std::vector<const char*> getVulkanInteropDeviceExtensions();
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
            std::vector<const char*>& requiredDeviceExtensionsIn,
            std::vector<const char*>& optionalDeviceExtensionsIn,
            std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions,
            DeviceFeatures& requestedDeviceFeaturesIn, bool computeOnly, bool testOnly);
    void selectPhysicalDevice(
            VkSurfaceKHR surface,
            std::vector<const char*>& requiredDeviceExtensions,
            std::vector<const char*>& optionalDeviceExtensions,
            std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions,
            DeviceFeatures& requestedDeviceFeatures, bool computeOnly,
            std::vector<VkPhysicalDevice>& physicalDevices);
    VkPhysicalDevice createPhysicalDeviceBinding(
            VkSurfaceKHR surface,
            std::vector<const char*>& requiredDeviceExtensions,
            std::vector<const char*>& optionalDeviceExtensions,
            std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions,
            DeviceFeatures& requestedDeviceFeatures, bool computeOnly);

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

    PhysicalDeviceCheckCallback physicalDeviceCheckCallback;
    bool useAppDeviceSelector = false;
    sgl::DeviceSelectorVulkan* deviceSelector = nullptr;

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

    // Information on supported image formats.
    std::unordered_map<VkFormat, VkFormatProperties> formatPropertiesMap;

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
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceVulkan11Features_Compat physicalDeviceVulkan11Features{};
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceVulkan11Properties_Compat physicalDeviceVulkan11Properties{};
#endif
#ifdef VK_VERSION_1_2
    VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{};
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceVulkan12Features_Compat physicalDeviceVulkan12Features{};
#endif
#ifdef VK_VERSION_1_3
    VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
    VkPhysicalDeviceVulkan13Properties physicalDeviceVulkan13Properties{};
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceVulkan13Features_Compat physicalDeviceVulkan13Features{};
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceVulkan13Properties_Compat physicalDeviceVulkan13Properties{};
#endif
#ifdef VK_VERSION_1_4
    VkPhysicalDeviceVulkan14Features physicalDeviceVulkan14Features{};
    VkPhysicalDeviceVulkan14Properties physicalDeviceVulkan14Properties{};
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceVulkan14Features_Compat physicalDeviceVulkan14Features{};
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceVulkan14Properties_Compat physicalDeviceVulkan14Properties{};
#endif
    VkPhysicalDeviceSubgroupProperties subgroupProperties{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
    VkPhysicalDeviceMeshShaderPropertiesNV meshShaderPropertiesNV{};
#ifdef VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderPropertiesEXT{};
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceMeshShaderPropertiesEXT_Compat meshShaderPropertiesEXT{};
#endif

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR uniformBufferStandardLayoutFeaturesKhr{};
    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features{};
    VkPhysicalDevice16BitStorageFeatures device16BitStorageFeatures{};
    VkPhysicalDevice8BitStorageFeatures device8BitStorageFeatures{};
    VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{};
    VkPhysicalDeviceShaderAtomicInt64FeaturesKHR shaderAtomicInt64Features{};
    VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT shaderImageAtomicInt64Features{};
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures{};
#ifdef VK_EXT_mutable_descriptor_type
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeatures{};
#else
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT_Compat mutableDescriptorTypeFeatures{};
#endif
#ifdef VK_EXT_shader_atomic_float2
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features{};
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT_Compat shaderAtomicFloat2Features{};
#endif
#ifdef VK_KHR_shader_bfloat16
    VkPhysicalDeviceShaderBfloat16FeaturesKHR shaderBfloat16Features{};
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceShaderBfloat16FeaturesKHR_Compat shaderBfloat16Features{};
#endif
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeaturesNV{};
    VkPhysicalDeviceMeshShaderFeaturesNV meshShaderFeaturesNV{};
#ifdef VK_EXT_mesh_shader
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT{};
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceMeshShaderFeaturesEXT_Compat meshShaderFeaturesEXT{};
#endif
#ifdef VK_NV_cooperative_matrix
    VkPhysicalDeviceCooperativeMatrixFeaturesNV cooperativeMatrixFeaturesNV{};
    VkPhysicalDeviceCooperativeMatrixPropertiesNV cooperativeMatrixPropertiesNV{};
    std::vector<VkCooperativeMatrixPropertiesNV> supportedCooperativeMatrixPropertiesNV;
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeMatrixFeaturesNV_Compat cooperativeMatrixFeaturesNV{};
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeMatrixPropertiesNV_Compat cooperativeMatrixPropertiesNV{};
    MAYBE_UNUSED_MEMBER std::vector<VkPhysicalDeviceCooperativeMatrixFeaturesNV_Compat> supportedCooperativeMatrixPropertiesNV;
#endif
#ifdef VK_KHR_cooperative_matrix
    VkPhysicalDeviceCooperativeMatrixFeaturesKHR cooperativeMatrixFeaturesKHR{};
    VkPhysicalDeviceCooperativeMatrixPropertiesKHR cooperativeMatrixPropertiesKHR{};
    std::vector<VkCooperativeMatrixPropertiesKHR> supportedCooperativeMatrixPropertiesKHR;
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeMatrixFeaturesKHR_Compat cooperativeMatrixFeaturesKHR{};
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeMatrixPropertiesKHR_Compat cooperativeMatrixPropertiesKHR{};
    MAYBE_UNUSED_MEMBER std::vector<VkPhysicalDeviceCooperativeMatrixFeaturesKHR_Compat> supportedCooperativeMatrixPropertiesKHR;
#endif
#ifdef VK_NV_cooperative_matrix2
    VkPhysicalDeviceCooperativeMatrix2FeaturesNV cooperativeMatrix2FeaturesNV{};
    VkPhysicalDeviceCooperativeMatrix2PropertiesNV cooperativeMatrix2PropertiesNV{};
    std::vector<VkCooperativeMatrixFlexibleDimensionsPropertiesNV> supportedCooperativeMatrixFlexibleDimensionsPropertiesNV;
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeMatrix2FeaturesNV_Compat cooperativeMatrix2FeaturesNV{};
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeMatrix2PropertiesNV_Compat cooperativeMatrix2PropertiesNV{};
    MAYBE_UNUSED_MEMBER std::vector<VkPhysicalDeviceCooperativeMatrix2PropertiesNV_Compat> supportedCooperativeMatrixFlexibleDimensionsPropertiesNV;
#endif
#ifdef VK_NV_cooperative_vector
    VkPhysicalDeviceCooperativeVectorFeaturesNV cooperativeVectorFeaturesNV{};
    VkPhysicalDeviceCooperativeVectorPropertiesNV cooperativeVectorPropertiesNV{};
    std::vector<VkCooperativeVectorPropertiesNV> supportedCooperativeVectorPropertiesNV;
#else
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeVectorFeaturesNV_Compat cooperativeVectorFeaturesNV{};
    MAYBE_UNUSED_MEMBER VkPhysicalDeviceCooperativeVectorPropertiesNV_Compat cooperativeVectorPropertiesNV{};
    MAYBE_UNUSED_MEMBER std::vector<VkPhysicalDeviceCooperativeVectorPropertiesNV_Compat> supportedCooperativeVectorPropertiesNV;
#endif
    bool isInitializedSupportedCooperativeMatrixPropertiesNV = false;
    bool isInitializedSupportedCooperativeMatrixPropertiesKHR = false;
    bool isInitializedSupportedCooperativeMatrixFlexibleDimensionsPropertiesNV = false;
    bool isInitializedSupportedCooperativeVectorPropertiesNV = false;
#ifdef VK_EXT_shader_64bit_indexing
    VkPhysicalDeviceShader64BitIndexingFeaturesEXT shader64BitIndexingFeaturesEXT{};
#else
    VkPhysicalDeviceShader64BitIndexingFeaturesEXT_Compat shader64BitIndexingFeaturesEXT{};
#endif

    // Driver version string (mapped from VkPhysicalDeviceProperties::driverVersion).
    DriverVersion driverVersion{};
    bool hasDriverVersionMinor = true;
    bool hasDriverVersionSubminor = true;
    bool hasDriverVersionPatch = true;
    std::string driverVersionString;

    // AMD-specific properties.
    VkPhysicalDeviceShaderCorePropertiesAMD deviceShaderCorePropertiesAMD{};
    VkPhysicalDeviceShaderCoreProperties2AMD deviceShaderCoreProperties2AMD{};
    bool isInitializedDeviceShaderCorePropertiesAMD = false;
    bool isInitializedDeviceShaderCoreProperties2AMD = false;

    // Queues for the logical device.
    bool appWantsMultithreadedRendering = false;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    uint32_t graphicsQueueIndex = 0;
    uint32_t computeQueueIndex = 0;
    uint32_t workerThreadGraphicsQueueIndex = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkQueue workerThreadGraphicsQueue = VK_NULL_HANDLE; ///< For use in another thread.
    std::thread::id mainThreadId;

    // Set of used command pools.
    std::unordered_map<CommandPoolType, VkCommandPool> commandPools;

    // Default descriptor pool.
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Vulkan-OpenGL interoperability enabled?
    MAYBE_UNUSED_MEMBER bool openGlInteropEnabled = false;
};

}}

#endif //SGL_DEVICE_HPP
