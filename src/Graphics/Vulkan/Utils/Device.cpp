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

#ifdef _WIN32
#define NOMINMAX
#endif

#define VMA_IMPLEMENTATION
#include <Utils/File/Logfile.hpp>
#include <Math/Math.hpp>
#include <Graphics/Window.hpp>
#include "Instance.hpp"
#include "Swapchain.hpp"
#include "Device.hpp"

#ifdef SUPPORT_OPENGL
#include "Interop.hpp"
#endif

#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace sgl { namespace vk {

std::vector<const char*> Device::getCudaInteropDeviceExtensions() {
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);

#ifdef _WIN32
    deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#else
    deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
#endif

    return deviceExtensions;
}

#ifdef _WIN32
std::vector<const char*> Device::getD3D12InteropDeviceExtensions() {
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    return deviceExtensions;
}
#endif

void Device::initializeDeviceExtensionList(VkPhysicalDevice physicalDevice) {
    uint32_t deviceExtensionCount = 0;
    VkResult res = vkEnumerateDeviceExtensionProperties(
            physicalDevice, nullptr, &deviceExtensionCount, nullptr);
    if (deviceExtensionCount > 0) {
        VkExtensionProperties *deviceExtensions = new VkExtensionProperties[deviceExtensionCount];
        res = vkEnumerateDeviceExtensionProperties(
                physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions);
        if (res != VK_SUCCESS) {
            Logfile::get()->throwError(
                    "Error in Device::initializeDeviceExtensionList: vkEnumerateDeviceExtensionProperties failed!");
        }

        for (uint32_t i = 0; i < deviceExtensionCount; i++) {
            availableDeviceExtensionNames.insert(deviceExtensions[i].extensionName);
        }

        delete[] deviceExtensions;
    }
}

void Device::printAvailableDeviceExtensionList() {
    std::string deviceExtensionString;
    size_t i = 0;
    for (const std::string& name : availableDeviceExtensionNames) {
        deviceExtensionString += name;
        if (i != availableDeviceExtensionNames.size() - 1) {
            deviceExtensionString += ", ";
        }
        i++;
    }
    sgl::Logfile::get()->write(
            std::string() + "Available Vulkan device extensions: " + deviceExtensionString, BLUE);
}

bool Device::_isDeviceExtensionAvailable(const std::string &extensionName) {
    return availableDeviceExtensionNames.find(extensionName) != availableDeviceExtensionNames.end();
}


uint32_t Device::findQueueFamilies(VkPhysicalDevice physicalDevice, VkQueueFlagBits queueFlags) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    // Get the best fitting queue family (i.e., no more flags than necessary).
    uint32_t bestFittingQueueIdx = std::numeric_limits<uint32_t>::max();
    VkQueueFlagBits bestFittingQueueFlags;
    for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++) {
        const VkQueueFamilyProperties& queueFamily = queueFamilies.at(i);
        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & queueFlags) == queueFlags) {
            if (bestFittingQueueIdx != std::numeric_limits<uint32_t>::max()) {
                uint32_t bitsSetPrev = getNumberOfBitsSet(uint32_t(bestFittingQueueFlags));
                uint32_t bitsSetNext = getNumberOfBitsSet(uint32_t(queueFamily.queueFlags));
                if (bitsSetNext < bitsSetPrev) {
                    bestFittingQueueIdx = i;
                    bestFittingQueueFlags = VkQueueFlagBits(queueFamily.queueFlags);
                }
            } else {
                bestFittingQueueIdx = i;
                bestFittingQueueFlags = VkQueueFlagBits(queueFamily.queueFlags);
            }
        }
    }
    return bestFittingQueueIdx;
}

bool Device::isDeviceSuitable(
        VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
        const std::vector<const char*>& requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions,
        std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions,
        const DeviceFeatures& requestedDeviceFeatures, bool computeOnly) {
    int graphicsQueueIndex = findQueueFamilies(
            physicalDevice, static_cast<VkQueueFlagBits>(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    int computeQueueIndex = findQueueFamilies(
            physicalDevice, static_cast<VkQueueFlagBits>(VK_QUEUE_COMPUTE_BIT));
    if ((graphicsQueueIndex < 0 && !computeOnly) || computeQueueIndex < 0) {
        return false;
    }

    if (surface) {
        SwapchainSupportInfo swapchainSupportInfo = querySwapchainSupportInfo(physicalDevice, surface);
        if (swapchainSupportInfo.formats.empty() && !swapchainSupportInfo.presentModes.empty()) {
            return false;
        }
    }

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
    if (openGlInteropEnabled && !isDeviceCompatibleWithOpenGl(physicalDevice)) {
        return false;
    }
#endif

    uint32_t numExtensions;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(numExtensions);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, availableExtensions.data());
    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    // Remove extensions that are available. The device is suitable if it has no missing extensions.
    std::set<std::string> availableDeviceExtensionNames;
    for (const VkExtensionProperties& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
        availableDeviceExtensionNames.insert(extension.extensionName);
    }

    VkBool32 presentSupport = false;
    if (surface) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueIndex, surface, &presentSupport);
    } else {
        presentSupport = true;
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

    // Check if all requested features are available.
    bool requestedFeaturesAvailable = true;
    constexpr size_t numFeatures = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    auto requestedPhysicalDeviceFeaturesArray = reinterpret_cast<const VkBool32*>(
            &requestedDeviceFeatures.requestedPhysicalDeviceFeatures);
    auto physicalDeviceFeaturesArray = reinterpret_cast<VkBool32*>(&physicalDeviceFeatures);
    for (size_t i = 0; i < numFeatures; i++) {
        if (requestedPhysicalDeviceFeaturesArray[i] && !physicalDeviceFeaturesArray[i]) {
            requestedFeaturesAvailable = false;
            break;
        }
    }

    bool isSuitable = presentSupport && requiredExtensions.empty() && requestedFeaturesAvailable;
    if (isSuitable && !optionalDeviceExtensions.empty()) {
        for (const char* extensionName : optionalDeviceExtensions) {
            if (availableDeviceExtensionNames.find(extensionName) != availableDeviceExtensionNames.end()) {
                deviceExtensionsSet.insert(extensionName);
                deviceExtensions.push_back(extensionName);
            }
        }
    }

    return isSuitable;
}

VkPhysicalDevice Device::createPhysicalDeviceBinding(
        VkSurfaceKHR surface,
        const std::vector<const char*>& requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions,
        std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions,
        const DeviceFeatures& requestedDeviceFeatures, bool computeOnly) {
    uint32_t numPhysicalDevices = 0;
    VkResult res = vkEnumeratePhysicalDevices(instance->getVkInstance(), &numPhysicalDevices, nullptr);
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Device::createPhysicalDeviceBinding: vkEnumeratePhysicalDevices failed!");
    }
    if (numPhysicalDevices == 0) {
        Logfile::get()->throwError(
                "Error in Device::createPhysicalDeviceBinding: vkEnumeratePhysicalDevices returned zero devices!");
    }

    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
    res = vkEnumeratePhysicalDevices(instance->getVkInstance(), &numPhysicalDevices, physicalDevices.data());
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Device::createPhysicalDeviceBinding: vkEnumeratePhysicalDevices failed!");
    }

    deviceExtensionsSet = {requiredDeviceExtensions.begin(), requiredDeviceExtensions.end()};
    deviceExtensions.insert(
            deviceExtensions.end(), requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    physicalDevice = VK_NULL_HANDLE;
    for (const auto &physicalDeviceIt : physicalDevices) {
        if (isDeviceSuitable(
                physicalDeviceIt, surface, requiredDeviceExtensions, optionalDeviceExtensions,
                deviceExtensionsSet, deviceExtensions, requestedDeviceFeatures, computeOnly)) {
            physicalDevice = physicalDeviceIt;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        std::string errorText =
                "Error in Device::createPhysicalDeviceBinding: No suitable GPU found with all necessary extensions "
                "and a graphics queue!";
        bool isErrorFatal = true;
#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
        if (openGlInteropEnabled) {
            isErrorFatal = false;
        }
#endif
        if (isErrorFatal) {
            sgl::Logfile::get()->throwError(errorText);
        } else {
            sgl::Logfile::get()->writeError(errorText, false);
        }
    }

    return physicalDevice;
}

void Device::createLogicalDeviceAndQueues(
        VkPhysicalDevice physicalDevice, bool useValidationLayer, const std::vector<const char*>& layerNames,
        const std::vector<const char*>& deviceExtensions, const std::set<std::string>& deviceExtensionsSet,
        DeviceFeatures requestedDeviceFeatures, bool computeOnly) {
    if (deviceExtensionsSet.find(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &timelineSemaphoreFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.timelineSemaphoreFeatures.timelineSemaphore == VK_FALSE) {
            requestedDeviceFeatures.timelineSemaphoreFeatures = timelineSemaphoreFeatures;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &bufferDeviceAddressFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddress == VK_FALSE) {
            requestedDeviceFeatures.bufferDeviceAddressFeatures = bufferDeviceAddressFeatures;
        }
    }
    if (deviceExtensionsSet.find(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        scalarBlockLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &scalarBlockLayoutFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.scalarBlockLayoutFeatures.scalarBlockLayout == VK_FALSE) {
            requestedDeviceFeatures.scalarBlockLayoutFeatures = scalarBlockLayoutFeatures;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME) != deviceExtensionsSet.end()
            || instance->getInstanceVulkanVersion() >= VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        uniformBufferStandardLayoutFeaturesKhr.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &uniformBufferStandardLayoutFeaturesKhr;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.uniformBufferStandardLayoutFeaturesKhr.uniformBufferStandardLayout == VK_FALSE) {
            requestedDeviceFeatures.uniformBufferStandardLayoutFeaturesKhr = uniformBufferStandardLayoutFeaturesKhr;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &accelerationStructureFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.accelerationStructureFeatures.accelerationStructure == VK_FALSE) {
            requestedDeviceFeatures.accelerationStructureFeatures = accelerationStructureFeatures;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &rayTracingPipelineFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.rayTracingPipelineFeatures.rayTracingPipeline == VK_FALSE) {
            requestedDeviceFeatures.rayTracingPipelineFeatures = rayTracingPipelineFeatures;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_RAY_QUERY_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &rayQueryFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.rayQueryFeatures.rayQuery == VK_FALSE) {
            requestedDeviceFeatures.rayQueryFeatures = rayQueryFeatures;
        }
    }
    if (deviceExtensionsSet.find(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        fragmentShaderInterlockFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &fragmentShaderInterlockFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.fragmentShaderInterlockFeatures.fragmentShaderPixelInterlock == VK_FALSE) {
            requestedDeviceFeatures.fragmentShaderInterlockFeatures = fragmentShaderInterlockFeatures;
        }
    }

    //
    /*
     * If computeOnly is not set: Allocate two graphics & compute queues, and one compute-only queue. One of the
     * graphics queues can be used as a present queue/primary rendering queue, and the other queue can be used in a
     * worker thread. If the hardware doesn't support compute-only queues, three graphics & compute queues are
     * allocated instead of two.
     */
    graphicsQueueIndex = findQueueFamilies(
            physicalDevice, static_cast<VkQueueFlagBits>(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    computeQueueIndex = findQueueFamilies(
            physicalDevice, static_cast<VkQueueFlagBits>(VK_QUEUE_COMPUTE_BIT));
    workerThreadGraphicsQueueIndex = graphicsQueueIndex;

    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
    queueFamilyProperties.resize(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    float queuePriorities[] = { 1.0, 1.0, 1.0 };

    VkDeviceQueueCreateInfo graphicsQueueInfo = {};
    graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.pNext = nullptr;
    graphicsQueueInfo.queueFamilyIndex = uint32_t(graphicsQueueIndex);
    graphicsQueueInfo.queueCount = 2;
    graphicsQueueInfo.pQueuePriorities = queuePriorities;

    VkDeviceQueueCreateInfo computeQueueInfo = {};
    computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    computeQueueInfo.pNext = nullptr;
    computeQueueInfo.queueFamilyIndex = uint32_t(computeQueueIndex);
    computeQueueInfo.queueCount = 1;
    computeQueueInfo.pQueuePriorities = queuePriorities;

    if (!computeOnly && graphicsQueueIndex == computeQueueIndex) {
        graphicsQueueInfo.queueCount = 3;
    }

    if (!computeOnly && queueFamilyProperties.at(graphicsQueueIndex).queueCount < graphicsQueueInfo.queueCount) {
        sgl::Logfile::get()->writeInfo(
                "Warning: The used Vulkan driver does not support enough queues for multi-threaded rendering.");
        graphicsQueueInfo.queueCount = std::min(
                queueFamilyProperties.at(graphicsQueueIndex).queueCount, graphicsQueueInfo.queueCount);
    }

    VkDeviceQueueCreateInfo queueInfos[] = { graphicsQueueInfo, computeQueueInfo };
    VkDeviceQueueCreateInfo* queueInfosPtr;
    if (computeOnly) {
        // Allocate only one compute-only queue.
        queueInfosPtr = &queueInfos[1];
    } else if (graphicsQueueIndex == computeQueueIndex) {
        // Allocate three compute & graphics queues.
        queueInfosPtr = &queueInfos[0];
    } else {
        // Allocate two compute & graphics queues and one compute-only queue.
        queueInfosPtr = queueInfos;
    }

    // Check if all requested features are available.
    constexpr size_t numFeatures = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    auto requestedPhysicalDeviceFeaturesArray = reinterpret_cast<VkBool32*>(
            &requestedDeviceFeatures.requestedPhysicalDeviceFeatures);
    auto optionalPhysicalDeviceFeaturesArray = reinterpret_cast<const VkBool32*>(
            &requestedDeviceFeatures.optionalPhysicalDeviceFeatures);
    auto physicalDeviceFeaturesArray = reinterpret_cast<VkBool32*>(&physicalDeviceFeatures);
    for (size_t i = 0; i < numFeatures; i++) {
        if (optionalPhysicalDeviceFeaturesArray[i] && physicalDeviceFeaturesArray[i]) {
            requestedPhysicalDeviceFeaturesArray[i] = VK_TRUE;
        }
    }


    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = (computeOnly || graphicsQueueIndex == computeQueueIndex) ? 1 : 2;
    deviceInfo.pQueueCreateInfos = queueInfosPtr;
    deviceInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceInfo.pEnabledFeatures = &requestedDeviceFeatures.requestedPhysicalDeviceFeatures;
    if (useValidationLayer) {
        deviceInfo.enabledLayerCount = uint32_t(layerNames.size());
        deviceInfo.ppEnabledLayerNames = layerNames.data();
    } else {
        deviceInfo.enabledLayerCount = 0;
    }

    const void** pNextPtr = &deviceInfo.pNext;
    if (requestedDeviceFeatures.timelineSemaphoreFeatures.timelineSemaphore) {
        *pNextPtr = &requestedDeviceFeatures.timelineSemaphoreFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.timelineSemaphoreFeatures.pNext);
    }
    if (requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddress) {
        *pNextPtr = &requestedDeviceFeatures.bufferDeviceAddressFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.bufferDeviceAddressFeatures.pNext);
    }
    if (requestedDeviceFeatures.scalarBlockLayoutFeatures.scalarBlockLayout) {
        *pNextPtr = &requestedDeviceFeatures.scalarBlockLayoutFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.scalarBlockLayoutFeatures.pNext);
    }
    if (requestedDeviceFeatures.uniformBufferStandardLayoutFeaturesKhr.uniformBufferStandardLayout) {
        *pNextPtr = &requestedDeviceFeatures.uniformBufferStandardLayoutFeaturesKhr;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.uniformBufferStandardLayoutFeaturesKhr.pNext);
    }
    if (requestedDeviceFeatures.accelerationStructureFeatures.accelerationStructure) {
        *pNextPtr = &requestedDeviceFeatures.accelerationStructureFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.accelerationStructureFeatures.pNext);
    }
    if (requestedDeviceFeatures.rayTracingPipelineFeatures.rayTracingPipeline) {
        *pNextPtr = &requestedDeviceFeatures.rayTracingPipelineFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.rayTracingPipelineFeatures.pNext);
    }
    if (requestedDeviceFeatures.rayQueryFeatures.rayQuery) {
        *pNextPtr = &requestedDeviceFeatures.rayQueryFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.rayQueryFeatures.pNext);
    }
    if (requestedDeviceFeatures.fragmentShaderInterlockFeatures.fragmentShaderPixelInterlock) {
        *pNextPtr = &requestedDeviceFeatures.fragmentShaderInterlockFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.fragmentShaderInterlockFeatures.pNext);
    }

    VkResult res = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError("Error in createLogicalDeviceAndQueues: vkCreateDevice failed!");
    }

    graphicsQueue = VK_NULL_HANDLE;
    workerThreadGraphicsQueue = VK_NULL_HANDLE;
    computeQueue = VK_NULL_HANDLE;
    if (computeOnly) {
        vkGetDeviceQueue(device, computeQueueIndex, 0, &computeQueue);
    } else if (graphicsQueueIndex == computeQueueIndex) {
        vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
        if (queueFamilyProperties.at(graphicsQueueIndex).queueCount > 1) {
            vkGetDeviceQueue(device, graphicsQueueIndex, 1, &workerThreadGraphicsQueue);
        } else {
            workerThreadGraphicsQueue = graphicsQueue;
        }
        if (queueFamilyProperties.at(graphicsQueueIndex).queueCount > 2) {
            vkGetDeviceQueue(device, graphicsQueueIndex, 2, &computeQueue);
        } else {
            computeQueue = graphicsQueue;
        }
    } else {
        vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
        if (queueFamilyProperties.at(graphicsQueueIndex).queueCount > 1) {
            vkGetDeviceQueue(device, graphicsQueueIndex, 1, &workerThreadGraphicsQueue);
        } else {
            workerThreadGraphicsQueue = graphicsQueue;
        }
        vkGetDeviceQueue(device, computeQueueIndex, 0, &computeQueue);
    }

    mainThreadId = std::this_thread::get_id();
}

void Device::createVulkanMemoryAllocator() {
    uint32_t vulkanApiVersion = std::min(instance->getInstanceVulkanVersion(), getApiVersion());

    // The shipped version of VMA only supports up to Vulkan 1.3 at the moment.
    if (vulkanApiVersion >= VK_MAKE_API_VERSION(0, 1, 4, 0)) {
        vulkanApiVersion = VK_MAKE_API_VERSION(0, 1, 3, 204);
    }

    sgl::Logfile::get()->write(
            "VMA Vulkan API version: " + Instance::convertVulkanVersionToString(vulkanApiVersion),
            BLUE);

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = vulkanApiVersion;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance->getVkInstance();
#if VMA_DYNAMIC_VULKAN_FUNCTIONS == 1
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
#if defined(VK_VERSION_1_3)
    if (vulkanApiVersion >= VK_API_VERSION_1_3) {
        vulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
        vulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;
    }
#endif
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
#endif
    if (isDeviceExtensionSupported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
        allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

bool Device::isDeviceExtensionSupported(const std::string& name) {
    return deviceExtensionsSet.find(name) != deviceExtensionsSet.end();
}

void Device::_getDeviceInformation() {
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_1) {
        physicalDeviceDriverProperties = {};
        physicalDeviceDriverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &physicalDeviceDriverProperties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_1
            || isDeviceExtensionSupported(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME)
            || isDeviceExtensionSupported(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME)) {
        physicalDeviceIDProperties = {};
        physicalDeviceIDProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &physicalDeviceIDProperties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }

    if (isDeviceExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) {
        accelerationStructureProperties = {};
        accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &accelerationStructureProperties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
    if (isDeviceExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) {
        rayTracingPipelineProperties = {};
        rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &rayTracingPipelineProperties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
}

void Device::writeDeviceInfoToLog(const std::vector<const char*>& deviceExtensions) {
    sgl::Logfile::get()->write(
            std::string() + "Instance Vulkan version: "
            + Instance::convertVulkanVersionToString(getInstance()->getInstanceVulkanVersion()), BLUE);
    sgl::Logfile::get()->write(
            std::string() + "Device Vulkan version: "
            + Instance::convertVulkanVersionToString(getApiVersion()), BLUE);
    sgl::Logfile::get()->write(std::string() + "Device name: " + getDeviceName(), BLUE);
    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_1) {
        sgl::Logfile::get()->write(std::string() + "Device driver name: " + getDeviceDriverName(), BLUE);
        sgl::Logfile::get()->write(std::string() + "Device driver info: " + getDeviceDriverInfo(), BLUE);
        sgl::Logfile::get()->write(
                std::string() + "Device driver ID: " + sgl::toString(getDeviceDriverId()), BLUE);
    }

    printAvailableDeviceExtensionList();

    std::string deviceExtensionString;
    for (size_t i = 0; i < deviceExtensions.size(); i++) {
        deviceExtensionString += deviceExtensions.at(i);
        if (i != deviceExtensions.size() - 1) {
            deviceExtensionString += ", ";
        }
    }
    sgl::Logfile::get()->write(
            std::string() + "Used Vulkan device extensions: " + deviceExtensionString, BLUE);
}

void Device::createDeviceSwapchain(
        Instance* instance, Window* window,
        std::vector<const char*> requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions,
        const DeviceFeatures& requestedDeviceFeatures, bool computeOnly) {
    this->instance = instance;
    this->window = window;

    requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkSurfaceKHR surface = window->getVkSurface();
    std::vector<const char*> deviceExtensions;
    physicalDevice = createPhysicalDeviceBinding(
            surface, requiredDeviceExtensions, optionalDeviceExtensions,
            deviceExtensionsSet, deviceExtensions, requestedDeviceFeatures, computeOnly);
    if (!physicalDevice) {
        return;
    }
    initializeDeviceExtensionList(physicalDevice);

    _getDeviceInformation();

    createLogicalDeviceAndQueues(
            physicalDevice, instance->getUseValidationLayer(),
            instance->getInstanceLayerNames(), deviceExtensions,
            deviceExtensionsSet, requestedDeviceFeatures, computeOnly);

    writeDeviceInfoToLog(deviceExtensions);

    createVulkanMemoryAllocator();
}

void Device::createDeviceHeadless(
        Instance* instance,
        const std::vector<const char*>& requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions,
        const DeviceFeatures& requestedDeviceFeatures, bool computeOnly) {
    this->instance = instance;
    this->window = nullptr;

    std::vector<const char*> deviceExtensions;
    physicalDevice = createPhysicalDeviceBinding(
            nullptr, requiredDeviceExtensions, optionalDeviceExtensions,
            deviceExtensionsSet, deviceExtensions, requestedDeviceFeatures, computeOnly);
    if (!physicalDevice) {
        return;
    }
    initializeDeviceExtensionList(physicalDevice);

    _getDeviceInformation();

    createLogicalDeviceAndQueues(
            physicalDevice, instance->getUseValidationLayer(),
            instance->getInstanceLayerNames(), deviceExtensions,
            deviceExtensionsSet, requestedDeviceFeatures, computeOnly);

    writeDeviceInfoToLog(deviceExtensions);

    createVulkanMemoryAllocator();
}

Device::~Device() {
    for (auto& it : commandPools) {
        vkDestroyCommandPool(device, it.second, nullptr);
    }
    commandPools.clear();

    if (allocator) {
        vmaDestroyAllocator(allocator);
    }
    if (device) {
        vkDestroyDevice(device, nullptr);
    }
}

void Device::waitIdle() {
    vkDeviceWaitIdle(device);
}

void Device::waitQueueIdle(VkQueue queue) {
    vkQueueWaitIdle(queue);
}

VkSampleCountFlagBits Device::getMaxUsableSampleCount() const {
    VkSampleCountFlags counts =
            physicalDeviceProperties.limits.framebufferColorSampleCounts
            & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

VkFormat Device::_findSupportedFormat(
        const std::vector<VkFormat>& candidates, VkImageTiling imageTiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if ((imageTiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                || (imageTiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
            return format;
        }
    }
    Logfile::get()->throwError("Error in Device::_findSupportedFormat: Failed to find a supported format!");
    return VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat Device::getSupportedDepthFormat(VkFormat hint, VkImageTiling imageTiling) {
    std::vector<VkFormat> formats;
    formats.reserve(7);
    formats.push_back(hint);
    formats.push_back(VK_FORMAT_D32_SFLOAT);
    formats.push_back(VK_FORMAT_X8_D24_UNORM_PACK32);
    formats.push_back(VK_FORMAT_D16_UNORM);
    formats.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
    formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
    formats.push_back(VK_FORMAT_D16_UNORM_S8_UINT);
    return _findSupportedFormat(formats, imageTiling, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

std::vector<VkFormat> Device::getSupportedDepthFormats(VkImageTiling imageTiling) {
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    std::vector<VkFormat> allFormats = {
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT
    };
    std::vector<VkFormat> supportedFormats;
    for (VkFormat format : allFormats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if ((imageTiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                || (imageTiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
            supportedFormats.push_back(format);
        }
    }
    return supportedFormats;
}

VkFormat Device::getSupportedDepthStencilFormat(VkFormat hint, VkImageTiling imageTiling) {
    std::vector<VkFormat> formats;
    formats.reserve(4);
    formats.push_back(hint);
    formats.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
    formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
    formats.push_back(VK_FORMAT_D16_UNORM_S8_UINT);
    return _findSupportedFormat(formats, imageTiling, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

std::vector<VkFormat> Device::getSupportedDepthStencilFormats(VkImageTiling imageTiling) {
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    std::vector<VkFormat> allFormats = {
            VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT
    };
    std::vector<VkFormat> supportedFormats;
    for (VkFormat format : allFormats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if ((imageTiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                || (imageTiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
            supportedFormats.push_back(format);
        }
    }
    return supportedFormats;
}

uint32_t Device::findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags) {
    for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < physicalDeviceMemoryProperties.memoryTypeCount; memoryTypeIndex++) {
        if (((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags
                && memoryTypeBits & (1 << memoryTypeIndex))) {
            return memoryTypeIndex;
        }
    }

    Logfile::get()->throwError("Error in Device::findMemoryTypeIndex: Could find suitable memory!");
    return std::numeric_limits<uint32_t>::max();
}

VkCommandBuffer Device::allocateCommandBuffer(
        CommandPoolType commandPoolType, VkCommandPool* pool, VkCommandBufferLevel commandBufferLevel) {
    VkCommandBuffer commandBuffer = nullptr;
    _allocateCommandBuffers(commandPoolType, pool, &commandBuffer, 1, commandBufferLevel);
    return commandBuffer;
}
std::vector<VkCommandBuffer> Device::allocateCommandBuffers(
        CommandPoolType commandPoolType, VkCommandPool* pool, uint32_t count, VkCommandBufferLevel commandBufferLevel) {
    std::vector<VkCommandBuffer> commandBuffers(count);
    _allocateCommandBuffers(commandPoolType, pool, commandBuffers.data(), count, commandBufferLevel);
    return commandBuffers;
}

void Device::freeCommandBuffer(VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Device::freeCommandBuffers(VkCommandPool commandPool, const std::vector<VkCommandBuffer>& commandBuffers) {
    vkFreeCommandBuffers(device, commandPool, uint32_t(commandBuffers.size()), commandBuffers.data());
}

void Device::_allocateCommandBuffers(
        CommandPoolType commandPoolType, VkCommandPool* pool, VkCommandBuffer* commandBuffers, uint32_t count,
        VkCommandBufferLevel commandBufferLevel) {
    if (commandPoolType.queueFamilyIndex == 0xFFFFFFFF) {
        commandPoolType.queueFamilyIndex = getGraphicsQueueIndex();
    }

    VkCommandPool commandPool;
    auto it = commandPools.find(commandPoolType);
    if (it != commandPools.end()) {
        commandPool = it->second;
    } else {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = commandPoolType.queueFamilyIndex;
        poolInfo.flags = commandPoolType.flags;
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    "Error in Device::allocateCommandBuffer: Could not create a command pool.");
        }
        commandPools.insert(std::make_pair(commandPoolType, commandPool));
    }

    *pool = commandPool;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = commandBufferLevel;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = count;

    vkAllocateCommandBuffers(device, &allocInfo, commandBuffers);
}

VkCommandBuffer Device::beginSingleTimeCommands(uint32_t queueIndex, bool beginCommandBuffer) {
    if (queueIndex == 0xFFFFFFFF) {
        queueIndex = getGraphicsQueueIndex();
    }

    CommandPoolType commandPoolType;
    commandPoolType.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolType.queueFamilyIndex = queueIndex;
    VkCommandPool pool;
    VkCommandBuffer commandBuffer = allocateCommandBuffer(commandPoolType, &pool);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (beginCommandBuffer) {
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
    }

    return commandBuffer;
}

void Device::endSingleTimeCommands(VkCommandBuffer commandBuffer, uint32_t queueIndex, bool endCommandBuffer) {
    if (queueIndex == 0xFFFFFFFF) {
        queueIndex = getGraphicsQueueIndex();
    }

    if (endCommandBuffer) {
        vkEndCommandBuffer(commandBuffer);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Could pass fence instead of VK_NULL_HANDLE and call vkWaitForFences later.
    VkQueue queue;
    if (mainThreadId == std::this_thread::get_id()) {
        if (queueIndex == graphicsQueueIndex) {
            queue = graphicsQueue;
        } else {
            queue = computeQueue;
        }
    } else {
        queue = workerThreadGraphicsQueue;
    }
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    CommandPoolType commandPoolType;
    commandPoolType.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolType.queueFamilyIndex = queueIndex;
    VkCommandPool commandPool = commandPools.find(commandPoolType)->second;
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

std::vector<VkCommandBuffer> Device::beginSingleTimeMultipleCommands(
        uint32_t numCommandBuffers, uint32_t queueIndex, bool beginCommandBuffer) {
    if (queueIndex == 0xFFFFFFFF) {
        queueIndex = getGraphicsQueueIndex();
    }

    CommandPoolType commandPoolType;
    commandPoolType.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolType.queueFamilyIndex = queueIndex;
    VkCommandPool pool;
    std::vector<VkCommandBuffer> commandBuffers = allocateCommandBuffers(
            commandPoolType, &pool, numCommandBuffers);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (beginCommandBuffer) {
        for (auto & commandBuffer : commandBuffers) {
            vkBeginCommandBuffer(commandBuffer, &beginInfo);
        }
    }

    return commandBuffers;
}

void Device::endSingleTimeMultipleCommands(
        const std::vector<VkCommandBuffer>& commandBuffers, uint32_t queueIndex, bool endCommandBuffer) {
    if (queueIndex == 0xFFFFFFFF) {
        queueIndex = getGraphicsQueueIndex();
    }

    if (endCommandBuffer) {
        for (auto commandBuffer : commandBuffers) {
            vkEndCommandBuffer(commandBuffer);
        }
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = uint32_t(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();

    // Could pass fence instead of VK_NULL_HANDLE and call vkWaitForFences later.
    VkQueue queue;
    if (mainThreadId == std::this_thread::get_id()) {
        if (queueIndex == graphicsQueueIndex) {
            queue = graphicsQueue;
        } else {
            queue = computeQueue;
        }
    } else {
        queue = workerThreadGraphicsQueue;
    }
    vkQueueSubmit(queue, uint32_t(commandBuffers.size()), &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    CommandPoolType commandPoolType;
    commandPoolType.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolType.queueFamilyIndex = queueIndex;
    VkCommandPool commandPool = commandPools.find(commandPoolType)->second;
    vkFreeCommandBuffers(device, commandPool, uint32_t(commandBuffers.size()), commandBuffers.data());
}

}}
