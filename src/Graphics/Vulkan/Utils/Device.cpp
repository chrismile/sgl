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

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include <Utils/File/Logfile.hpp>
#include <Math/Math.hpp>
#include <Graphics/Window.hpp>
#include "Status.hpp"
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
#ifdef __APPLE__
#include <vulkan/vulkan_beta.h>
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

std::vector<const char*> Device::getVulkanInteropDeviceExtensions() {
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);

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
                    "Error in Device::initializeDeviceExtensionList: vkEnumerateDeviceExtensionProperties failed ("
                    + vulkanResultToString(res) + ")!");
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
        SwapchainSupportInfo swapchainSupportInfo = querySwapchainSupportInfo(
                physicalDevice, surface, nullptr);
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

    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
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

#ifdef VK_VERSION_1_1
    VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features{};
    physicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 1, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 1, 0)) {
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &physicalDeviceVulkan11Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        const size_t numVulkan11Features =
                1 + (&requestedDeviceFeatures.requestedVulkan11Features.shaderDrawParameters)
                 - (&requestedDeviceFeatures.requestedVulkan11Features.storageBuffer16BitAccess);
        auto requestedVulkan11FeaturesArray = reinterpret_cast<const VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan11Features.storageBuffer16BitAccess);
        auto physicalDeviceVulkan11FeaturesArray = reinterpret_cast<VkBool32*>(
                &physicalDeviceVulkan11Features.storageBuffer16BitAccess);
        for (size_t i = 0; i < numVulkan11Features; i++) {
            if (requestedVulkan11FeaturesArray[i] && !physicalDeviceVulkan11FeaturesArray[i]) {
                requestedFeaturesAvailable = false;
                break;
            }
        }
    }
#endif
#ifdef VK_VERSION_1_2
    VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{};
    physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 2, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &physicalDeviceVulkan12Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        const size_t numVulkan12Features =
                1 + (&requestedDeviceFeatures.requestedVulkan12Features.subgroupBroadcastDynamicId)
                 - (&requestedDeviceFeatures.requestedVulkan12Features.samplerMirrorClampToEdge);
        auto requestedVulkan12FeaturesArray = reinterpret_cast<const VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan12Features.samplerMirrorClampToEdge);
        auto physicalDeviceVulkan12FeaturesArray = reinterpret_cast<VkBool32*>(
                &physicalDeviceVulkan12Features.samplerMirrorClampToEdge);
        for (size_t i = 0; i < numVulkan12Features; i++) {
            if (requestedVulkan12FeaturesArray[i] && !physicalDeviceVulkan12FeaturesArray[i]) {
                requestedFeaturesAvailable = false;
                break;
            }
        }
    }
#endif
#ifdef VK_VERSION_1_3
    VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
    physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 3, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0)) {
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &physicalDeviceVulkan13Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        const size_t numVulkan13Features =
                1 + (&requestedDeviceFeatures.requestedVulkan13Features.maintenance4)
                 - (&requestedDeviceFeatures.requestedVulkan13Features.robustImageAccess);
        auto requestedVulkan13FeaturesArray = reinterpret_cast<const VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan13Features.robustImageAccess);
        auto physicalDeviceVulkan13FeaturesArray = reinterpret_cast<VkBool32*>(
                &physicalDeviceVulkan13Features.robustImageAccess);
        for (size_t i = 0; i < numVulkan13Features; i++) {
            if (requestedVulkan13FeaturesArray[i] && !physicalDeviceVulkan13FeaturesArray[i]) {
                requestedFeaturesAvailable = false;
                break;
            }
        }
    }
#endif

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
                "Error in Device::createPhysicalDeviceBinding: vkEnumeratePhysicalDevices failed ("
                + vulkanResultToString(res) + ")!");
    }
    if (numPhysicalDevices == 0) {
        Logfile::get()->throwError(
                "Error in Device::createPhysicalDeviceBinding: vkEnumeratePhysicalDevices returned zero devices!");
    }

    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
    res = vkEnumeratePhysicalDevices(instance->getVkInstance(), &numPhysicalDevices, physicalDevices.data());
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Device::createPhysicalDeviceBinding: vkEnumeratePhysicalDevices failed ("
                + vulkanResultToString(res) + ")!");
    }

    deviceExtensionsSet = {requiredDeviceExtensions.begin(), requiredDeviceExtensions.end()};
    deviceExtensions.insert(
            deviceExtensions.end(), requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

#ifdef __linux__
    /*
     * Give priority to GPUs in this order: Discrete, integrated, virtual, CPU, other.
     * This is necessary on Linux, as VK_LAYER_NV_optimus, which should make sure that on systems with hybrid GPU
     * solutions the selected GPU always comes first, seems to not work correctly at least on Ubuntu 20.04.
     * When selecting the Intel GPU on Ubuntu 20.04, the NVIDIA GPU is not reported. When selecting the NVIDIA GPU,
     * the Intel GPU still comes first, but the system crashes when using the Intel GPU. Thus, the discrete GPU is
     * prioritized when available. On Arch Linux, which per default uses prime-run, it seems like only the correct GPU
     * is reported at all, so this should have no side effect for prime-run based systems.
     */
    std::vector<VkPhysicalDeviceType> deviceTypePriorityList = {
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
            VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
            VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
            VK_PHYSICAL_DEVICE_TYPE_CPU,
            VK_PHYSICAL_DEVICE_TYPE_OTHER
    };
    std::map<VkPhysicalDeviceType, std::vector<VkPhysicalDevice>> physicalDeviceMap;
    for (const VkPhysicalDevice& physicalDeviceIt : physicalDevices) {
        VkPhysicalDeviceProperties physicalDeviceItProperties{};
        vkGetPhysicalDeviceProperties(physicalDeviceIt, &physicalDeviceItProperties);
        physicalDeviceMap[physicalDeviceItProperties.deviceType].push_back(physicalDeviceIt);
    }

    std::vector<VkPhysicalDevice> sortedPhysicalDevices;
    sortedPhysicalDevices.reserve(numPhysicalDevices);
    for (VkPhysicalDeviceType deviceType : deviceTypePriorityList) {
        const std::vector<VkPhysicalDevice>& physicalDeviceList = physicalDeviceMap[deviceType];
        sortedPhysicalDevices.insert(
                sortedPhysicalDevices.end(), physicalDeviceList.begin(), physicalDeviceList.end());
    }

    physicalDevice = VK_NULL_HANDLE;
    for (const VkPhysicalDevice& physicalDeviceIt : sortedPhysicalDevices) {
        if (isDeviceSuitable(
                physicalDeviceIt, surface, requiredDeviceExtensions, optionalDeviceExtensions,
                deviceExtensionsSet, deviceExtensions, requestedDeviceFeatures, computeOnly)) {
            physicalDevice = physicalDeviceIt;
            break;
        }
    }
#else
    /**
     * Select the first device on, e.g., Windows, as the selected GPU on hybrid GPU systems seems to always be first.
     */
    physicalDevice = VK_NULL_HANDLE;
    for (const VkPhysicalDevice& physicalDeviceIt : physicalDevices) {
        if (isDeviceSuitable(
                physicalDeviceIt, surface, requiredDeviceExtensions, optionalDeviceExtensions,
                deviceExtensionsSet, deviceExtensions, requestedDeviceFeatures, computeOnly)) {
            physicalDevice = physicalDeviceIt;
            break;
        }
    }
#endif

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
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

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

    // Enable all available optional features.
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

#ifdef VK_VERSION_1_1
    physicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    physicalDeviceVulkan11Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 1, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 1, 0)) {
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &physicalDeviceVulkan11Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        const size_t numVulkan11Features =
                1 + (&requestedDeviceFeatures.requestedVulkan11Features.shaderDrawParameters)
                 - (&requestedDeviceFeatures.requestedVulkan11Features.storageBuffer16BitAccess);
        auto requestedVulkan11FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan11Features.storageBuffer16BitAccess);
        auto optionalVulkan11FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.optionalVulkan11Features.storageBuffer16BitAccess);
        auto physicalDeviceVulkan11FeaturesArray = reinterpret_cast<VkBool32*>(
                &physicalDeviceVulkan11Features.storageBuffer16BitAccess);
        for (size_t i = 0; i < numVulkan11Features; i++) {
            if (optionalVulkan11FeaturesArray[i] && physicalDeviceVulkan11FeaturesArray[i]) {
                requestedVulkan11FeaturesArray[i] = VK_TRUE;
            }
        }

        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &physicalDeviceVulkan11Properties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
#endif
#ifdef VK_VERSION_1_2
    physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 2, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &physicalDeviceVulkan12Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        const size_t numVulkan12Features =
                1 + (&requestedDeviceFeatures.requestedVulkan12Features.subgroupBroadcastDynamicId)
                 - (&requestedDeviceFeatures.requestedVulkan12Features.samplerMirrorClampToEdge);
        auto requestedVulkan12FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan12Features.samplerMirrorClampToEdge);
        auto optionalVulkan12FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.optionalVulkan12Features.samplerMirrorClampToEdge);
        auto physicalDeviceVulkan12FeaturesArray = reinterpret_cast<VkBool32*>(
                &physicalDeviceVulkan12Features.samplerMirrorClampToEdge);
        for (size_t i = 0; i < numVulkan12Features; i++) {
            if (optionalVulkan12FeaturesArray[i] && physicalDeviceVulkan12FeaturesArray[i]) {
                requestedVulkan12FeaturesArray[i] = VK_TRUE;
            }
        }
    }
#endif
#ifdef VK_VERSION_1_3
    physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    physicalDeviceVulkan13Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 3, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0)) {
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &physicalDeviceVulkan13Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        const size_t numVulkan13Features =
                1 + (&requestedDeviceFeatures.requestedVulkan13Features.maintenance4)
                 - (&requestedDeviceFeatures.requestedVulkan13Features.robustImageAccess);
        auto requestedVulkan13FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan13Features.robustImageAccess);
        auto optionalVulkan13FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.optionalVulkan13Features.robustImageAccess);
        auto physicalDeviceVulkan13FeaturesArray = reinterpret_cast<VkBool32*>(
                &physicalDeviceVulkan13Features.robustImageAccess);
        for (size_t i = 0; i < numVulkan13Features; i++) {
            if (optionalVulkan13FeaturesArray[i] && physicalDeviceVulkan13FeaturesArray[i]) {
                requestedVulkan13FeaturesArray[i] = VK_TRUE;
            }
        }

        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &physicalDeviceVulkan13Properties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
#endif


    // Check if Vulkan 1.x extensions are used.
#ifdef VK_VERSION_1_1
    bool hasRequestedVulkan11Features = false;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 1, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 1, 0)) {
        const size_t numVulkan11Features =
                1 + (&requestedDeviceFeatures.requestedVulkan11Features.shaderDrawParameters)
                 - (&requestedDeviceFeatures.requestedVulkan11Features.storageBuffer16BitAccess);
        auto requestedVulkan11FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan11Features.storageBuffer16BitAccess);
        for (size_t i = 0; i < numVulkan11Features; i++) {
            if (requestedVulkan11FeaturesArray[i]) {
                hasRequestedVulkan11Features = true;
            }
        }
    }
#endif
#ifdef VK_VERSION_1_2
    bool hasRequestedVulkan12Features = false;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 2, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        const size_t numVulkan12Features =
                1 + (&requestedDeviceFeatures.requestedVulkan12Features.subgroupBroadcastDynamicId)
                 - (&requestedDeviceFeatures.requestedVulkan12Features.samplerMirrorClampToEdge);
        auto requestedVulkan12FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan12Features.samplerMirrorClampToEdge);
        for (size_t i = 0; i < numVulkan12Features; i++) {
            if (requestedVulkan12FeaturesArray[i]) {
                hasRequestedVulkan12Features = true;
            }
        }
    }
#endif
#ifdef VK_VERSION_1_3
    bool hasRequestedVulkan13Features = false;
    if (getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 3, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0)) {
        const size_t numVulkan13Features =
                1 + (&requestedDeviceFeatures.requestedVulkan13Features.maintenance4)
                 - (&requestedDeviceFeatures.requestedVulkan13Features.robustImageAccess);
        auto requestedVulkan13FeaturesArray = reinterpret_cast<VkBool32*>(
                &requestedDeviceFeatures.requestedVulkan13Features.robustImageAccess);
        for (size_t i = 0; i < numVulkan13Features; i++) {
            if (requestedVulkan13FeaturesArray[i]) {
                hasRequestedVulkan13Features = true;
            }
        }

        // SPIR-V 1.6 needs VkPhysicalDeviceVulkan13Features::maintenance4.
        requestedDeviceFeatures.requestedVulkan13Features.maintenance4 = VK_TRUE;
        hasRequestedVulkan13Features = true;
    }
#endif


    if (deviceExtensionsSet.find(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &timelineSemaphoreFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.timelineSemaphoreFeatures.timelineSemaphore == VK_FALSE) {
            requestedDeviceFeatures.timelineSemaphoreFeatures = timelineSemaphoreFeatures;
        }
        if (hasRequestedVulkan12Features) {
            requestedDeviceFeatures.requestedVulkan12Features.timelineSemaphore =
                    requestedDeviceFeatures.timelineSemaphoreFeatures.timelineSemaphore;
            requestedDeviceFeatures.timelineSemaphoreFeatures.timelineSemaphore = VK_FALSE;
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
        if (hasRequestedVulkan12Features) {
            requestedDeviceFeatures.requestedVulkan12Features.bufferDeviceAddress =
                    requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddress;
            requestedDeviceFeatures.requestedVulkan12Features.bufferDeviceAddressCaptureReplay =
                    requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay;
            requestedDeviceFeatures.requestedVulkan12Features.bufferDeviceAddressMultiDevice =
                    requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice;
            requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddress = VK_FALSE;
            requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
            requestedDeviceFeatures.bufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;
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
        if (hasRequestedVulkan12Features) {
            requestedDeviceFeatures.requestedVulkan12Features.scalarBlockLayout =
                    requestedDeviceFeatures.scalarBlockLayoutFeatures.scalarBlockLayout;
            requestedDeviceFeatures.scalarBlockLayoutFeatures.scalarBlockLayout = VK_FALSE;
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
        if (hasRequestedVulkan12Features) {
            requestedDeviceFeatures.requestedVulkan12Features.uniformBufferStandardLayout =
                    requestedDeviceFeatures.uniformBufferStandardLayoutFeaturesKhr.uniformBufferStandardLayout;
            requestedDeviceFeatures.uniformBufferStandardLayoutFeaturesKhr.uniformBufferStandardLayout = VK_FALSE;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        shaderFloat16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &shaderFloat16Int8Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.shaderFloat16Int8Features.shaderFloat16 == VK_FALSE
                && requestedDeviceFeatures.shaderFloat16Int8Features.shaderInt8 == VK_FALSE) {
            requestedDeviceFeatures.shaderFloat16Int8Features = shaderFloat16Int8Features;
        }
        if (hasRequestedVulkan12Features) {
            requestedDeviceFeatures.requestedVulkan12Features.shaderFloat16 =
                    requestedDeviceFeatures.shaderFloat16Int8Features.shaderFloat16;
            requestedDeviceFeatures.requestedVulkan12Features.shaderInt8 =
                    requestedDeviceFeatures.shaderFloat16Int8Features.shaderInt8;
            requestedDeviceFeatures.shaderFloat16Int8Features.shaderFloat16 = VK_FALSE;
            requestedDeviceFeatures.shaderFloat16Int8Features.shaderInt8 = VK_FALSE;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_16BIT_STORAGE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        device16BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &device16BitStorageFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.device16BitStorageFeatures.storageBuffer16BitAccess == VK_FALSE
                && requestedDeviceFeatures.device16BitStorageFeatures.uniformAndStorageBuffer16BitAccess == VK_FALSE
                && requestedDeviceFeatures.device16BitStorageFeatures.storagePushConstant16 == VK_FALSE
                && requestedDeviceFeatures.device16BitStorageFeatures.storageInputOutput16 == VK_FALSE) {
            requestedDeviceFeatures.device16BitStorageFeatures = device16BitStorageFeatures;
        }
        if (hasRequestedVulkan12Features) {
            requestedDeviceFeatures.requestedVulkan11Features.storageBuffer16BitAccess =
                    requestedDeviceFeatures.device16BitStorageFeatures.storageBuffer16BitAccess;
            requestedDeviceFeatures.requestedVulkan11Features.uniformAndStorageBuffer16BitAccess =
                    requestedDeviceFeatures.device16BitStorageFeatures.uniformAndStorageBuffer16BitAccess;
            requestedDeviceFeatures.requestedVulkan11Features.storagePushConstant16 =
                    requestedDeviceFeatures.device16BitStorageFeatures.storagePushConstant16;
            requestedDeviceFeatures.requestedVulkan11Features.storageInputOutput16 =
                    requestedDeviceFeatures.device16BitStorageFeatures.storageInputOutput16;
            requestedDeviceFeatures.device16BitStorageFeatures.storageBuffer16BitAccess = VK_FALSE;
            requestedDeviceFeatures.device16BitStorageFeatures.uniformAndStorageBuffer16BitAccess = VK_FALSE;
            requestedDeviceFeatures.device16BitStorageFeatures.storagePushConstant16 = VK_FALSE;
            requestedDeviceFeatures.device16BitStorageFeatures.storageInputOutput16 = VK_FALSE;
        }
    }
    if (deviceExtensionsSet.find(VK_KHR_8BIT_STORAGE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        device8BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &device8BitStorageFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.device8BitStorageFeatures.storageBuffer8BitAccess == VK_FALSE
                && requestedDeviceFeatures.device8BitStorageFeatures.uniformAndStorageBuffer8BitAccess == VK_FALSE
                && requestedDeviceFeatures.device8BitStorageFeatures.storagePushConstant8 == VK_FALSE) {
            requestedDeviceFeatures.device8BitStorageFeatures = device8BitStorageFeatures;
        }
        if (hasRequestedVulkan12Features) {
            requestedDeviceFeatures.requestedVulkan12Features.storageBuffer8BitAccess =
                    requestedDeviceFeatures.device8BitStorageFeatures.storageBuffer8BitAccess;
            requestedDeviceFeatures.requestedVulkan12Features.uniformAndStorageBuffer8BitAccess =
                    requestedDeviceFeatures.device8BitStorageFeatures.uniformAndStorageBuffer8BitAccess;
            requestedDeviceFeatures.requestedVulkan12Features.storagePushConstant8 =
                    requestedDeviceFeatures.device8BitStorageFeatures.storagePushConstant8;
            requestedDeviceFeatures.device8BitStorageFeatures.storageBuffer8BitAccess = VK_FALSE;
            requestedDeviceFeatures.device8BitStorageFeatures.uniformAndStorageBuffer8BitAccess = VK_FALSE;
            requestedDeviceFeatures.device8BitStorageFeatures.storagePushConstant8 = VK_FALSE;
        }
    }
    if ((requestedDeviceFeatures.optionalEnableShaderDrawParametersFeatures
                || requestedDeviceFeatures.shaderDrawParametersFeatures.shaderDrawParameters)
            && instance->getInstanceVulkanVersion() >= VK_MAKE_API_VERSION(0, 1, 1, 0)) {
        shaderDrawParametersFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &shaderDrawParametersFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.shaderDrawParametersFeatures.shaderDrawParameters == VK_FALSE) {
            requestedDeviceFeatures.shaderDrawParametersFeatures = shaderDrawParametersFeatures;
        }
        if (hasRequestedVulkan11Features) {
            requestedDeviceFeatures.requestedVulkan11Features.shaderDrawParameters =
                    requestedDeviceFeatures.shaderDrawParametersFeatures.shaderDrawParameters;
            requestedDeviceFeatures.shaderDrawParametersFeatures.shaderDrawParameters = VK_FALSE;
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
    if (deviceExtensionsSet.find(VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        shaderAtomicInt64Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &shaderAtomicInt64Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.shaderAtomicInt64Features.shaderBufferInt64Atomics == VK_FALSE
            && requestedDeviceFeatures.shaderAtomicInt64Features.shaderSharedInt64Atomics == VK_FALSE) {
            requestedDeviceFeatures.shaderAtomicInt64Features = shaderAtomicInt64Features;
        }
    }
    if (deviceExtensionsSet.find(VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        shaderImageAtomicInt64Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &shaderImageAtomicInt64Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.shaderImageAtomicInt64Features.shaderImageInt64Atomics == VK_FALSE
                && requestedDeviceFeatures.shaderImageAtomicInt64Features.sparseImageInt64Atomics == VK_FALSE) {
            requestedDeviceFeatures.shaderImageAtomicInt64Features = shaderImageAtomicInt64Features;
        }
    }
    if (deviceExtensionsSet.find(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        shaderAtomicFloatFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &shaderAtomicFloatFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.shaderAtomicFloatFeatures.shaderBufferFloat32Atomics == VK_FALSE) {
            requestedDeviceFeatures.shaderAtomicFloatFeatures = shaderAtomicFloatFeatures;
        }
    }
#ifdef VK_EXT_shader_atomic_float2
    if (deviceExtensionsSet.find(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        shaderAtomicFloat2Features.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &shaderAtomicFloat2Features;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.shaderAtomicFloat2Features.shaderBufferFloat32AtomicMinMax == VK_FALSE) {
            requestedDeviceFeatures.shaderAtomicFloat2Features = shaderAtomicFloat2Features;
        }
    }
#endif
    if (deviceExtensionsSet.find(VK_NV_MESH_SHADER_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        meshShaderFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &meshShaderFeaturesNV;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.meshShaderFeaturesNV.meshShader == VK_FALSE) {
            requestedDeviceFeatures.meshShaderFeaturesNV = meshShaderFeaturesNV;
        }
    }
#ifdef VK_EXT_mesh_shader
    if (deviceExtensionsSet.find(VK_EXT_MESH_SHADER_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        meshShaderFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &meshShaderFeaturesEXT;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.meshShaderFeaturesEXT.meshShader == VK_FALSE) {
            requestedDeviceFeatures.meshShaderFeaturesEXT = meshShaderFeaturesEXT;
            // Avoid enabling features relying on other extensions, cf. https://github.com/chrismile/LineVis/issues/6
            requestedDeviceFeatures.meshShaderFeaturesEXT.multiviewMeshShader = VK_FALSE;
            requestedDeviceFeatures.meshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader = VK_FALSE;
        }
    }
#endif
#ifdef VK_AMDX_shader_enqueue
    if (deviceExtensionsSet.find(VK_AMDX_SHADER_ENQUEUE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        shaderEnqueueFeaturesAMDX.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ENQUEUE_FEATURES_AMDX;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &shaderEnqueueFeaturesAMDX;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.shaderEnqueueFeaturesAMDX.shaderEnqueue == VK_FALSE) {
            requestedDeviceFeatures.shaderEnqueueFeaturesAMDX = shaderEnqueueFeaturesAMDX;
        }
    }
#endif
#ifdef VK_NV_device_generated_commands
    if (deviceExtensionsSet.find(VK_NV_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        deviceGeneratedCommandsFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &deviceGeneratedCommandsFeaturesNV;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.deviceGeneratedCommandsFeaturesNV.deviceGeneratedCommands == VK_FALSE) {
            requestedDeviceFeatures.deviceGeneratedCommandsFeaturesNV = deviceGeneratedCommandsFeaturesNV;
        }
    }
#endif
#ifdef VK_NV_device_generated_commands_compute
    if (deviceExtensionsSet.find(VK_NV_DEVICE_GENERATED_COMMANDS_COMPUTE_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        deviceGeneratedCommandsComputeFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_COMPUTE_FEATURES_NV;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &deviceGeneratedCommandsComputeFeaturesNV;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.deviceGeneratedCommandsComputeFeaturesNV.deviceGeneratedCompute == VK_FALSE) {
            requestedDeviceFeatures.deviceGeneratedCommandsComputeFeaturesNV = deviceGeneratedCommandsComputeFeaturesNV;
        }
    }
#endif
    if (deviceExtensionsSet.find(VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        fragmentShaderBarycentricFeaturesNV.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &fragmentShaderBarycentricFeaturesNV;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.fragmentShaderBarycentricFeaturesNV.fragmentShaderBarycentric == VK_FALSE) {
            requestedDeviceFeatures.fragmentShaderBarycentricFeaturesNV = fragmentShaderBarycentricFeaturesNV;
        }
    }
#ifdef VK_NV_cooperative_matrix
    if (deviceExtensionsSet.find(VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        cooperativeMatrixFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &cooperativeMatrixFeaturesNV;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.cooperativeMatrixFeaturesNV.cooperativeMatrix == VK_FALSE) {
            requestedDeviceFeatures.cooperativeMatrixFeaturesNV = cooperativeMatrixFeaturesNV;
        }
    }
#endif
#ifdef VK_KHR_cooperative_matrix
    if (deviceExtensionsSet.find(VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME) != deviceExtensionsSet.end()) {
        cooperativeMatrixFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &cooperativeMatrixFeaturesKHR;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        if (requestedDeviceFeatures.cooperativeMatrixFeaturesKHR.cooperativeMatrix == VK_FALSE) {
            requestedDeviceFeatures.cooperativeMatrixFeaturesKHR = cooperativeMatrixFeaturesKHR;
        }
    }
#endif


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
    if (requestedDeviceFeatures.shaderFloat16Int8Features.shaderFloat16
            || requestedDeviceFeatures.shaderFloat16Int8Features.shaderInt8) {
        *pNextPtr = &requestedDeviceFeatures.shaderFloat16Int8Features;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.shaderFloat16Int8Features.pNext);
    }
    if (requestedDeviceFeatures.device16BitStorageFeatures.storageBuffer16BitAccess
            || requestedDeviceFeatures.device16BitStorageFeatures.uniformAndStorageBuffer16BitAccess
            || requestedDeviceFeatures.device16BitStorageFeatures.storagePushConstant16
            || requestedDeviceFeatures.device16BitStorageFeatures.storageInputOutput16) {
        *pNextPtr = &requestedDeviceFeatures.device16BitStorageFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.device16BitStorageFeatures.pNext);
    }
    if (requestedDeviceFeatures.device8BitStorageFeatures.storageBuffer8BitAccess
            || requestedDeviceFeatures.device8BitStorageFeatures.uniformAndStorageBuffer8BitAccess
            || requestedDeviceFeatures.device8BitStorageFeatures.storagePushConstant8) {
        *pNextPtr = &requestedDeviceFeatures.device8BitStorageFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.device8BitStorageFeatures.pNext);
    }
    if (requestedDeviceFeatures.shaderDrawParametersFeatures.shaderDrawParameters) {
        *pNextPtr = &requestedDeviceFeatures.shaderDrawParametersFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.shaderDrawParametersFeatures.pNext);
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
    if (requestedDeviceFeatures.shaderAtomicInt64Features.shaderBufferInt64Atomics
            || requestedDeviceFeatures.shaderAtomicInt64Features.shaderSharedInt64Atomics) {
        *pNextPtr = &requestedDeviceFeatures.shaderAtomicInt64Features;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.shaderAtomicInt64Features.pNext);
    }
    if (requestedDeviceFeatures.shaderImageAtomicInt64Features.shaderImageInt64Atomics
            || requestedDeviceFeatures.shaderImageAtomicInt64Features.sparseImageInt64Atomics) {
        *pNextPtr = &requestedDeviceFeatures.shaderImageAtomicInt64Features;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.shaderImageAtomicInt64Features.pNext);
    }
    if (requestedDeviceFeatures.shaderAtomicFloatFeatures.shaderBufferFloat32Atomics) {
        *pNextPtr = &requestedDeviceFeatures.shaderAtomicFloatFeatures;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.shaderAtomicFloatFeatures.pNext);
    }
#ifdef VK_EXT_shader_atomic_float2
    if (requestedDeviceFeatures.shaderAtomicFloat2Features.shaderBufferFloat32AtomicMinMax) {
        *pNextPtr = &requestedDeviceFeatures.shaderAtomicFloat2Features;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.shaderAtomicFloat2Features.pNext);
    }
#endif
    if (requestedDeviceFeatures.meshShaderFeaturesNV.meshShader) {
        *pNextPtr = &requestedDeviceFeatures.meshShaderFeaturesNV;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.meshShaderFeaturesNV.pNext);
    }
#ifdef VK_EXT_mesh_shader
    if (requestedDeviceFeatures.meshShaderFeaturesEXT.meshShader) {
        *pNextPtr = &requestedDeviceFeatures.meshShaderFeaturesEXT;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.meshShaderFeaturesEXT.pNext);
    }
#endif
#ifdef VK_AMDX_shader_enqueue
    if (requestedDeviceFeatures.shaderEnqueueFeaturesAMDX.shaderEnqueue) {
        *pNextPtr = &requestedDeviceFeatures.shaderEnqueueFeaturesAMDX;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.shaderEnqueueFeaturesAMDX.pNext);
    }
#endif
#ifdef VK_NV_device_generated_commands
    if (requestedDeviceFeatures.deviceGeneratedCommandsFeaturesNV.deviceGeneratedCommands) {
        *pNextPtr = &requestedDeviceFeatures.deviceGeneratedCommandsFeaturesNV;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.deviceGeneratedCommandsFeaturesNV.pNext);
    }
#endif
#ifdef VK_NV_device_generated_commands_compute
    if (requestedDeviceFeatures.deviceGeneratedCommandsComputeFeaturesNV.deviceGeneratedCompute) {
        *pNextPtr = &requestedDeviceFeatures.deviceGeneratedCommandsComputeFeaturesNV;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.deviceGeneratedCommandsComputeFeaturesNV.pNext);
    }
#endif
    if (requestedDeviceFeatures.fragmentShaderBarycentricFeaturesNV.fragmentShaderBarycentric) {
        *pNextPtr = &requestedDeviceFeatures.fragmentShaderBarycentricFeaturesNV;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.fragmentShaderBarycentricFeaturesNV.pNext);
    }
#ifdef VK_NV_cooperative_matrix
    if (requestedDeviceFeatures.cooperativeMatrixFeaturesNV.cooperativeMatrix) {
        *pNextPtr = &requestedDeviceFeatures.cooperativeMatrixFeaturesNV;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.cooperativeMatrixFeaturesNV.pNext);
    }
#endif
#ifdef VK_KHR_cooperative_matrix
    if (requestedDeviceFeatures.cooperativeMatrixFeaturesKHR.cooperativeMatrix) {
        *pNextPtr = &requestedDeviceFeatures.cooperativeMatrixFeaturesKHR;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.cooperativeMatrixFeaturesKHR.pNext);
    }
#endif
#ifdef VK_VERSION_1_1
    if (hasRequestedVulkan11Features && getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 1, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 1, 0)) {
        *pNextPtr = &requestedDeviceFeatures.requestedVulkan11Features;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.requestedVulkan11Features.pNext);
    }
#endif
#ifdef VK_VERSION_1_2
    if (hasRequestedVulkan12Features && getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 2, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        *pNextPtr = &requestedDeviceFeatures.requestedVulkan12Features;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.requestedVulkan12Features.pNext);
    }
#endif
#ifdef VK_VERSION_1_3
    if (hasRequestedVulkan13Features && getApiVersion() >= VK_MAKE_API_VERSION(0, 1, 3, 0)
            && getInstance()->getApplicationInfo().apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0)) {
        *pNextPtr = &requestedDeviceFeatures.requestedVulkan13Features;
        pNextPtr = const_cast<const void**>(&requestedDeviceFeatures.requestedVulkan13Features.pNext);
    }
#endif

    VkResult res = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in createLogicalDeviceAndQueues: vkCreateDevice failed ("
                + vulkanResultToString(res) + ")!");
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

VKAPI_ATTR void VKAPI_CALL allocateDeviceMemoryCallback(
        VmaAllocator VMA_NOT_NULL allocator,
        uint32_t memoryType,
        VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
        VkDeviceSize size,
        void* VMA_NULLABLE userData) {
    auto* device = static_cast<Device*>(userData);
    device->vmaAllocateDeviceMemoryCallback(memoryType, memory, size);
}

VKAPI_ATTR void VKAPI_CALL freeDeviceMemoryCallback(
        VmaAllocator VMA_NOT_NULL allocator,
        uint32_t memoryType,
        VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
        VkDeviceSize size,
        void* VMA_NULLABLE userData) {
    auto* device = static_cast<Device*>(userData);
    device->vmaFreeDeviceMemoryCallback(memoryType, memory, size);
}

void Device::vmaAllocateDeviceMemoryCallback(uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size) {
    deviceMemoryToSizeMap.insert(std::make_pair(memory, size));
}

void Device::vmaFreeDeviceMemoryCallback(uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size) {
    deviceMemoryToSizeMap.erase(memory);
}

VkDeviceSize Device::getVmaDeviceMemoryAllocationSize(VkDeviceMemory deviceMemory) {
    auto it = deviceMemoryToSizeMap.find(deviceMemory);
    if (it == deviceMemoryToSizeMap.end()) {
        sgl::Logfile::get()->throwError("Error in Device::getVmaDeviceMemoryAllocationSize: Device memory not found.");
    }
    return it->second;
}

bool Device::getNeedsDedicatedAllocationForExternalMemoryImage(
        VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
        VkExternalMemoryHandleTypeFlagBits handleType) {
    ExternalMemoryImageConfigTuple configTuple = { format, type, tiling, usage, flags, handleType };
    auto it = needsDedicatedAllocationForExternalMemoryImageMap.find(configTuple);
    if (it != needsDedicatedAllocationForExternalMemoryImageMap.end()) {
        return it->second;
    }

    // Passing VkMemoryDedicatedAllocateInfo is necessary, e.g., with the AMD Windows driver.
    // Part (1): Query the external image format properties.
    VkPhysicalDeviceExternalImageFormatInfo externalImageFormatInfo{};
    externalImageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
    externalImageFormatInfo.handleType = handleType;
    VkPhysicalDeviceImageFormatInfo2 imageFormatInfo{};
    imageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
    imageFormatInfo.format = format;
    imageFormatInfo.type = type;
    imageFormatInfo.tiling = tiling;
    imageFormatInfo.usage = usage;
    imageFormatInfo.pNext = &externalImageFormatInfo;
    VkExternalImageFormatProperties externalImageFormatProperties{};
    externalImageFormatProperties.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
    VkImageFormatProperties2 imageFormatProperties{};
    imageFormatProperties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
    imageFormatProperties.pNext = &externalImageFormatProperties;
    vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &imageFormatInfo, &imageFormatProperties);

    // Part (2): Detect if a dedicated allocation is necessary.
    bool needsDedicatedAllocation =
            (externalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures
             & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;
    needsDedicatedAllocationForExternalMemoryImageMap.insert(std::make_pair(configTuple, needsDedicatedAllocation));

    return needsDedicatedAllocation;
}

bool Device::getNeedsDedicatedAllocationForExternalMemoryBuffer(
        VkBufferUsageFlags usage, VkBufferCreateFlags flags, VkExternalMemoryHandleTypeFlagBits handleType) {
    ExternalMemoryBufferConfigTuple configTuple = { usage, flags, handleType };
    auto it = needsDedicatedAllocationForExternalMemoryBufferMap.find(configTuple);
    if (it != needsDedicatedAllocationForExternalMemoryBufferMap.end()) {
        return it->second;
    }

    // Passing VkMemoryDedicatedAllocateInfo is necessary, e.g., with the AMD Windows driver.
    // Part (1): Query the external buffer properties.
    VkPhysicalDeviceExternalBufferInfo externalBufferInfo{};
    externalBufferInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO;
    externalBufferInfo.usage = usage;
    externalBufferInfo.flags = flags;
    externalBufferInfo.handleType = handleType;
    VkExternalBufferProperties externalBufferProperties{};
    externalBufferProperties.sType = VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES;
    vkGetPhysicalDeviceExternalBufferProperties(
            physicalDevice, &externalBufferInfo, &externalBufferProperties);

    // Part (2): Detect if a dedicated allocation is necessary.
    bool needsDedicatedAllocation =
            (externalBufferProperties.externalMemoryProperties.externalMemoryFeatures
             & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;
    needsDedicatedAllocationForExternalMemoryBufferMap.insert(std::make_pair(configTuple, needsDedicatedAllocation));

    return needsDedicatedAllocation;
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

    VmaDeviceMemoryCallbacks deviceMemoryCallbacks{};
    deviceMemoryCallbacks.pfnAllocate = &allocateDeviceMemoryCallback;
    deviceMemoryCallbacks.pfnFree = &freeDeviceMemoryCallback;
    deviceMemoryCallbacks.pUserData = (void*)this;
    allocatorInfo.pDeviceMemoryCallbacks = &deviceMemoryCallbacks;

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

VmaPool Device::getExternalMemoryHandlePool(uint32_t memoryTypeIndex, bool isBuffer) {
    MemoryPoolType memoryPoolType;
    memoryPoolType.memoryTypeIndex = memoryTypeIndex;
    memoryPoolType.isBufferPool = isBuffer;

    auto it = externalMemoryHandlePools.find(memoryPoolType);
    if (it != externalMemoryHandlePools.end()) {
        return it->second;
    }

    VmaPool pool = createExternalMemoryHandlePool(memoryTypeIndex);
    externalMemoryHandlePools.insert(std::make_pair(memoryPoolType, pool));
    return pool;
}

VmaPool Device::createExternalMemoryHandlePool(uint32_t memoryTypeIndex) {
    /*
     * Previously, sgl used one dedicated allocation per external memory allocation.
     * In this case, it was possible to use, e.g., vkGetBufferMemoryRequirements to get the best memory type index.
     * When creating one memory pool with VMA, we need to hope it is able to store both buffers and images with the
     * memory usage/layout one will normally use for exported memory.
     */

    VkExternalMemoryHandleTypeFlags handleTypes = 0;
#if defined(_WIN32)
    handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
    handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
    Logfile::get()->throwError(
                "Error in Device::createExternalMemoryHandlePool: "
                "External memory is only supported on Linux, Android and Windows systems!");
#endif

    exportMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportMemoryAllocateInfo.handleTypes = handleTypes;

    /*
     * According to https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/custom_memory_pools.html,
     * the exact size does not matter.
     */
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = 0x10000;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R32_SFLOAT;
    imageCreateInfo.extent = { 1024, 1024, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 1;
    imageCreateInfo.pQueueFamilyIndices = &graphicsQueueIndex;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    uint32_t memoryTypeIndexBuffer = 0, memoryTypeIndexImage = 0;
    VkResult res = vmaFindMemoryTypeIndexForBufferInfo(
            allocator, &bufferCreateInfo, &allocationCreateInfo, &memoryTypeIndexBuffer);
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Device::createExternalMemoryHandlePool: vmaFindMemoryTypeIndexForBufferInfo failed ("
                + vulkanResultToString(res) + ")!");
    }

    res = vmaFindMemoryTypeIndexForImageInfo(
            allocator, &imageCreateInfo, &allocationCreateInfo, &memoryTypeIndexImage);
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Device::createExternalMemoryHandlePool: vmaFindMemoryTypeIndexForImageInfo failed ("
                + vulkanResultToString(res) + ")!");
    }

    if (memoryTypeIndexBuffer != memoryTypeIndexImage) {
        sgl::Logfile::get()->writeWarning(
                "Warning in Device::createExternalMemoryHandlePool: Mismatch between buffer memory type index ("
                + std::to_string(memoryTypeIndexBuffer) + ") and image memory type index ("
                + std::to_string(memoryTypeIndexImage) + ").");
    }

    VmaPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.pMemoryAllocateNext = &exportMemoryAllocateInfo;
    poolCreateInfo.memoryTypeIndex = memoryTypeIndexImage;

    VmaPool pool = VK_NULL_HANDLE;
    res = vmaCreatePool(allocator, &poolCreateInfo, &pool);
    if (res != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Device::createExternalMemoryHandlePool: vmaCreatePool failed ("
                + vulkanResultToString(res) + ")!");
    }
    return pool;
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

    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_1) {
        subgroupProperties = {};
        subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &subgroupProperties;
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

    if (isDeviceExtensionSupported(VK_NV_MESH_SHADER_EXTENSION_NAME)) {
        meshShaderPropertiesNV = {};
        meshShaderPropertiesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &meshShaderPropertiesNV;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
#ifdef VK_EXT_mesh_shader
    if (isDeviceExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME)) {
        meshShaderPropertiesEXT = {};
        meshShaderPropertiesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &meshShaderPropertiesEXT;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
#endif
#ifdef VK_AMDX_shader_enqueue
    if (isDeviceExtensionSupported(VK_AMDX_SHADER_ENQUEUE_EXTENSION_NAME)) {
        shaderEnqueuePropertiesAMDX = {};
        shaderEnqueuePropertiesAMDX.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ENQUEUE_PROPERTIES_AMDX;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &shaderEnqueuePropertiesAMDX;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
#endif
#ifdef VK_NV_device_generated_commands
    if (isDeviceExtensionSupported(VK_NV_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME)) {
        deviceGeneratedCommandsPropertiesNV = {};
        deviceGeneratedCommandsPropertiesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &deviceGeneratedCommandsPropertiesNV;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    }
#endif
}

#ifdef VK_AMDX_shader_enqueue
/// Warning: Instable API for testing only, may be removed at any point of time in the future.
FunctionTableShaderEnqueueAMDX Device::getFunctionTableShaderEnqueueAMDX() {
    FunctionTableShaderEnqueueAMDX funcTable{};
    funcTable.vkCmdDispatchGraphAMDX = vkCmdDispatchGraphAMDX;
    funcTable.vkCmdDispatchGraphIndirectAMDX = vkCmdDispatchGraphIndirectAMDX;
    funcTable.vkCmdDispatchGraphIndirectCountAMDX = vkCmdDispatchGraphIndirectCountAMDX;
    funcTable.vkCmdInitializeGraphScratchMemoryAMDX = vkCmdInitializeGraphScratchMemoryAMDX;
    funcTable.vkCreateExecutionGraphPipelinesAMDX = vkCreateExecutionGraphPipelinesAMDX;
    funcTable.vkGetExecutionGraphPipelineNodeIndexAMDX = vkGetExecutionGraphPipelineNodeIndexAMDX;
    funcTable.vkGetExecutionGraphPipelineScratchSizeAMDX = vkGetExecutionGraphPipelineScratchSizeAMDX;
    return funcTable;
}
#endif

#ifdef VK_AMDX_shader_enqueue
/// Warning: Instable API for testing only, may be removed at any point of time in the future.
FunctionTableDeviceGeneratedCommandsNV Device::getFunctionTableDeviceGeneratedCommandsNV() {
    FunctionTableDeviceGeneratedCommandsNV funcTable{};
    funcTable.vkCmdBindPipelineShaderGroupNV = vkCmdBindPipelineShaderGroupNV;
    funcTable.vkCmdExecuteGeneratedCommandsNV = vkCmdExecuteGeneratedCommandsNV;
    funcTable.vkCmdPreprocessGeneratedCommandsNV = vkCmdPreprocessGeneratedCommandsNV;
    funcTable.vkCreateIndirectCommandsLayoutNV = vkCreateIndirectCommandsLayoutNV;
    funcTable.vkDestroyIndirectCommandsLayoutNV = vkDestroyIndirectCommandsLayoutNV;
    funcTable.vkGetGeneratedCommandsMemoryRequirementsNV = vkGetGeneratedCommandsMemoryRequirementsNV;
    return funcTable;
}
#endif

#ifdef VK_AMDX_shader_enqueue
/// Warning: Instable API for testing only, may be removed at any point of time in the future.
FunctionTableDeviceGeneratedCommandsComputeNV Device::getFunctionTableDeviceGeneratedCommandsComputeNV() {
    FunctionTableDeviceGeneratedCommandsComputeNV funcTable{};
    funcTable.vkCmdUpdatePipelineIndirectBufferNV = vkCmdUpdatePipelineIndirectBufferNV;
    funcTable.vkGetPipelineIndirectDeviceAddressNV = vkGetPipelineIndirectDeviceAddressNV;
    funcTable.vkGetPipelineIndirectMemoryRequirementsNV = vkGetPipelineIndirectMemoryRequirementsNV;
    return funcTable;
}
#endif

#ifdef VK_NV_cooperative_matrix
const std::vector<VkCooperativeMatrixPropertiesNV>& Device::getSupportedCooperativeMatrixPropertiesNV() {
    if (!isInitializedSupportedCooperativeMatrixPropertiesNV) {
        uint32_t propertyCount = 0;
        vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(physicalDevice, &propertyCount, nullptr);
        supportedCooperativeMatrixPropertiesNV.resize(propertyCount);
        for (size_t i = 0; i < supportedCooperativeMatrixPropertiesNV.size(); i++) {
            supportedCooperativeMatrixPropertiesNV.at(i).sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_NV;
        }
        vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(
                physicalDevice, &propertyCount, supportedCooperativeMatrixPropertiesNV.data());
        isInitializedSupportedCooperativeMatrixPropertiesNV = true;
    }
    return supportedCooperativeMatrixPropertiesNV;
}
#endif

#ifdef VK_KHR_cooperative_matrix
const std::vector<VkCooperativeMatrixPropertiesKHR>& Device::getSupportedCooperativeMatrixPropertiesKHR() {
    if (!isInitializedSupportedCooperativeMatrixPropertiesKHR) {
        uint32_t propertyCount = 0;
        vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(physicalDevice, &propertyCount, nullptr);
        supportedCooperativeMatrixPropertiesKHR.resize(propertyCount);
        for (size_t i = 0; i < supportedCooperativeMatrixPropertiesKHR.size(); i++) {
            supportedCooperativeMatrixPropertiesKHR.at(i).sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_KHR;
        }
        vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(
                physicalDevice, &propertyCount, supportedCooperativeMatrixPropertiesKHR.data());
        isInitializedSupportedCooperativeMatrixPropertiesKHR = true;
    }
    return supportedCooperativeMatrixPropertiesKHR;
}
#endif

const VkPhysicalDeviceShaderCorePropertiesAMD& Device::getDeviceShaderCorePropertiesAMD() {
    if (!isInitializedDeviceShaderCorePropertiesAMD) {
        deviceShaderCorePropertiesAMD.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &deviceShaderCorePropertiesAMD;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
        isInitializedDeviceShaderCorePropertiesAMD = true;
    }
    return deviceShaderCorePropertiesAMD;
}

const VkPhysicalDeviceShaderCoreProperties2AMD& Device::getDeviceShaderCoreProperties2AMD() {
    if (!isInitializedDeviceShaderCoreProperties2AMD) {
        deviceShaderCoreProperties2AMD.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD;
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &deviceShaderCoreProperties2AMD;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
        isInitializedDeviceShaderCoreProperties2AMD = true;
    }
    return deviceShaderCoreProperties2AMD;
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
        std::vector<const char*> optionalDeviceExtensions,
        const DeviceFeatures& requestedDeviceFeatures, bool computeOnly) {
    this->instance = instance;
    this->window = window;

    if (!window->getUseDownloadSwapchain()) {
        requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

#ifdef __APPLE__
    optionalDeviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif
    // For device thread info.
    optionalDeviceExtensions.push_back(VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME);
    optionalDeviceExtensions.push_back(VK_AMD_SHADER_CORE_PROPERTIES_2_EXTENSION_NAME);

    VkSurfaceKHR surface = window->getVkSurface();
    enabledDeviceExtensionNames = {};
    physicalDevice = createPhysicalDeviceBinding(
            surface, requiredDeviceExtensions, optionalDeviceExtensions,
            deviceExtensionsSet, enabledDeviceExtensionNames, requestedDeviceFeatures, computeOnly);
    if (!physicalDevice) {
        return;
    }
    initializeDeviceExtensionList(physicalDevice);

    _getDeviceInformation();

    createLogicalDeviceAndQueues(
            physicalDevice, instance->getUseValidationLayer(),
            instance->getInstanceLayerNames(), enabledDeviceExtensionNames,
            deviceExtensionsSet, requestedDeviceFeatures, computeOnly);

    writeDeviceInfoToLog(enabledDeviceExtensionNames);

    createVulkanMemoryAllocator();
}

void Device::createDeviceHeadless(
        Instance* instance,
        std::vector<const char*> requiredDeviceExtensions,
        std::vector<const char*> optionalDeviceExtensions,
        const DeviceFeatures& requestedDeviceFeatures, bool computeOnly) {
    this->instance = instance;
    this->window = nullptr;

#ifdef __APPLE__
    optionalDeviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif
    // For device thread info.
    optionalDeviceExtensions.push_back(VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME);
    optionalDeviceExtensions.push_back(VK_AMD_SHADER_CORE_PROPERTIES_2_EXTENSION_NAME);

    enabledDeviceExtensionNames = {};
    physicalDevice = createPhysicalDeviceBinding(
            nullptr, requiredDeviceExtensions, optionalDeviceExtensions,
            deviceExtensionsSet, enabledDeviceExtensionNames, requestedDeviceFeatures, computeOnly);
    if (!physicalDevice) {
        return;
    }
    initializeDeviceExtensionList(physicalDevice);

    _getDeviceInformation();

    createLogicalDeviceAndQueues(
            physicalDevice, instance->getUseValidationLayer(),
            instance->getInstanceLayerNames(), enabledDeviceExtensionNames,
            deviceExtensionsSet, requestedDeviceFeatures, computeOnly);

    writeDeviceInfoToLog(enabledDeviceExtensionNames);

    createVulkanMemoryAllocator();
}

Device::~Device() {
    for (auto& it : commandPools) {
        vkDestroyCommandPool(device, it.second, nullptr);
    }
    commandPools.clear();

    if (allocator) {
        for (auto& it : externalMemoryHandlePools) {
            vmaDestroyPool(allocator, it.second);
        }
        externalMemoryHandlePools.clear();
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

    Logfile::get()->throwError("Error in Device::findMemoryTypeIndex: Could not find suitable memory!");
    return std::numeric_limits<uint32_t>::max();
}

VkDeviceSize Device::findMemoryHeapIndex(VkMemoryHeapFlagBits heapFlags) {
    for (uint32_t heapIdx = 0; heapIdx < physicalDeviceMemoryProperties.memoryHeapCount; heapIdx++) {
        if ((physicalDeviceMemoryProperties.memoryHeaps[heapIdx].flags & heapFlags) == heapFlags) {
            return heapIdx;
        }
    }

    Logfile::get()->writeError("Error in Device::findMemoryHeapIndex: Could not find a suitable memory heap!");
    return 0;
}

VkDeviceSize Device::getMemoryHeapBudget(uint32_t memoryHeapIndex) {
    if (memoryHeapIndex >= physicalDeviceMemoryProperties.memoryHeapCount) {
        sgl::Logfile::get()->throwError("Error in Device::getMemoryHeapBudget: Memory heap index out of bounds.");
    }

    VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetProperties = {};
    memoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    VkPhysicalDeviceMemoryProperties2 memoryProperties2 = {};
    memoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    memoryProperties2.pNext = &memoryBudgetProperties;
    vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memoryProperties2);

    return memoryBudgetProperties.heapBudget[memoryHeapIndex];
}

VkDeviceSize Device::getMemoryHeapBudgetVma(uint32_t memoryHeapIndex) {
    auto* budgets = new VmaBudget[physicalDeviceMemoryProperties.memoryHeapCount];
    vmaGetHeapBudgets(allocator, budgets);
    VkDeviceSize heapBudget = budgets[memoryHeapIndex].budget;
    delete[] budgets;
    return heapBudget;
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
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    CommandPoolType commandPoolType;
    commandPoolType.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolType.queueFamilyIndex = queueIndex;
    VkCommandPool commandPool = commandPools.find(commandPoolType)->second;
    vkFreeCommandBuffers(device, commandPool, uint32_t(commandBuffers.size()), commandBuffers.data());
}

PFN_vkGetDeviceProcAddr Device::getVkDeviceProcAddrFunctionPointer() {
    return vkGetDeviceProcAddr;
}

void Device::loadVolkDeviceFunctionTable(VolkDeviceTable* table) {
    volkLoadDeviceTable(table, device);
}

}}
