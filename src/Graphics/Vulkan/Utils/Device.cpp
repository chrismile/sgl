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

#define VMA_IMPLEMENTATION
#include <Utils/File/Logfile.hpp>
#include <Graphics/Window.hpp>
#include "Instance.hpp"
#include "Swapchain.hpp"
#include "Device.hpp"

namespace sgl { namespace vk {

void Device::initializeDeviceExtensionList(VkPhysicalDevice physicalDevice) {
    uint32_t deviceExtensionCount = 0;
    VkResult res = vkEnumerateDeviceExtensionProperties(
            physicalDevice, NULL, &deviceExtensionCount, NULL);
    if (deviceExtensionCount > 0) {
        VkExtensionProperties *deviceExtensions = new VkExtensionProperties[deviceExtensionCount];
        res = vkEnumerateDeviceExtensionProperties(
                physicalDevice, NULL, &deviceExtensionCount, deviceExtensions);
        assert(!res);

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

bool Device::isDeviceExtensionAvailable(const std::string &extensionName) {
    return availableDeviceExtensionNames.find(extensionName) != availableDeviceExtensionNames.end();
}


uint32_t findQueueFamilies(VkPhysicalDevice device, VkQueueFlagBits queueFlags) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t queueIndex = -1;
    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & queueFlags) {
            queueIndex = i;
            break;
        }

        i++;
    }
    return queueIndex;
}

bool isDeviceSuitable(
        VkPhysicalDevice device, VkSurfaceKHR surface,
        const std::vector<const char*>& requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions,
        std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions) {
    // TODO: Use compute-only queue.
    int graphicsQueueIndex = findQueueFamilies(
            device, static_cast<VkQueueFlagBits>(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    if (graphicsQueueIndex < 0) {
        return false;
    }

    if (surface) {
        SwapchainSupportInfo swapchainSupportInfo = querySwapchainSupportInfo(device, surface);
        if (swapchainSupportInfo.formats.empty() && !swapchainSupportInfo.presentModes.empty()) {
            return false;
        }
    }

    uint32_t numExtensions;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(numExtensions);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, availableExtensions.data());
    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    // Remove extensions that are available. The device is suitable if it has no missing extensions.
    std::set<std::string> availableDeviceExtensionNames;
    for (const VkExtensionProperties& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
        availableDeviceExtensionNames.insert(extension.extensionName);
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &physicalDeviceFeatures);

    VkBool32 presentSupport = false;
    if (surface) {
        vkGetPhysicalDeviceSurfaceSupportKHR(device, graphicsQueueIndex, surface, &presentSupport);
    } else {
        presentSupport = true;
    }

    bool isSuitable = presentSupport && requiredExtensions.empty() && physicalDeviceFeatures.samplerAnisotropy;

    if (isSuitable && !optionalDeviceExtensions.empty()) {
        for (const char* extensionName : optionalDeviceExtensions) {
            if (availableDeviceExtensionNames.find(extensionName) != availableDeviceExtensionNames.end()) {
                deviceExtensionsSet.insert(extensionName);
                deviceExtensions.push_back(extensionName);
            }
        }
        deviceExtensionsSet.insert(optionalDeviceExtensions.begin(), optionalDeviceExtensions.end());
    }

    return isSuitable;
}

VkPhysicalDevice createPhysicalDeviceBinding(
        VkInstance instance, VkSurfaceKHR surface,
        const std::vector<const char*>& requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions,
        std::set<std::string>& deviceExtensionsSet, std::vector<const char*>& deviceExtensions) {
    uint32_t numPhysicalDevices = 0;
    VkResult res = vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, NULL);
    assert(!res && numPhysicalDevices > 0);

    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
    res = vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDevices.data());
    assert(!res);

    deviceExtensionsSet = {requiredDeviceExtensions.begin(), requiredDeviceExtensions.begin()};
    deviceExtensions.insert(deviceExtensions.end(), requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const auto &device : physicalDevices) {
        if (isDeviceSuitable(
                device, surface, requiredDeviceExtensions, optionalDeviceExtensions,
                deviceExtensionsSet, deviceExtensions)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        sgl::Logfile::get()->throwError(
                "Error in createPhysicalDeviceBinding: No suitable GPU found with all necessary extensions and a "
                "graphics queue!");
    }

    return physicalDevice;
}

struct LogicalDeviceAndQueues {
    VkDevice device;
    uint32_t graphicsQueueIndex;
    uint32_t computeQueueIndex;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
};
LogicalDeviceAndQueues createLogicalDeviceAndQueues(
        VkPhysicalDevice physicalDevice, bool useValidationLayer, const std::vector<const char*>& layerNames,
        const std::vector<const char*>& deviceExtensions) {
    uint32_t queueIndex = findQueueFamilies(
            physicalDevice, static_cast<VkQueueFlagBits>(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));

    float queuePriorities[2] = { 1.0, 1.0 };
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = nullptr;
    queueInfo.queueFamilyIndex = (uint32_t)queueIndex;
    queueInfo.queueCount = 2;
    queueInfo.pQueuePriorities = queuePriorities;

    // TODO
    VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    physicalDeviceFeatures.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo deviceInfo = { };
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = nullptr;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = deviceExtensions.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceInfo.pEnabledFeatures = &physicalDeviceFeatures;
    if (useValidationLayer) {
        deviceInfo.enabledLayerCount = layerNames.size();
        deviceInfo.ppEnabledLayerNames = layerNames.data();
    } else {
        deviceInfo.enabledLayerCount = 0;
    }

    VkDevice device;
    VkResult res = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
    assert(!res);

    VkQueue graphicsQueue, computeQueue;
    vkGetDeviceQueue(device, queueIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueIndex, 1, &computeQueue);

    LogicalDeviceAndQueues logicalDeviceAndQueues;
    logicalDeviceAndQueues.device = device;
    logicalDeviceAndQueues.graphicsQueueIndex = queueIndex;
    logicalDeviceAndQueues.computeQueueIndex = queueIndex;
    logicalDeviceAndQueues.graphicsQueue = graphicsQueue;
    logicalDeviceAndQueues.computeQueue = computeQueue;
    return logicalDeviceAndQueues;
}

void createVulkanMemoryAllocator(
        VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator) {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    //allocatorInfo.flags;

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

bool Device::isDeviceExtensionSupported(const std::string& name) {
    return deviceExtensionsSet.find(name) != deviceExtensionsSet.end();
}

void Device::createDeviceSwapchain(
        Instance* instance, Window* window,
        const std::vector<const char*>& requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions) {
    this->instance = instance;
    this->window = window;

    VkSurfaceKHR surface = window->getVkSurface();
    std::vector<const char*> deviceExtensions;
    physicalDevice = createPhysicalDeviceBinding(
            instance->getVkInstance(), surface, requiredDeviceExtensions, optionalDeviceExtensions,
            deviceExtensionsSet, deviceExtensions);
    initializeDeviceExtensionList(physicalDevice);
    printAvailableDeviceExtensionList();

    auto deviceQueuePair = createLogicalDeviceAndQueues(
            physicalDevice, instance->getUseValidationLayer(), instance->getInstanceLayerNames(), deviceExtensions);
    device = deviceQueuePair.device;
    graphicsQueueIndex = deviceQueuePair.graphicsQueueIndex;
    computeQueueIndex = deviceQueuePair.computeQueueIndex;
    graphicsQueue = deviceQueuePair.graphicsQueue;
    computeQueue = deviceQueuePair.computeQueue;

    std::string deviceExtensionString;
    for (size_t i = 0; i < deviceExtensions.size(); i++) {
        deviceExtensionString += deviceExtensions.at(i);
        if (i != deviceExtensions.size() - 1) {
            deviceExtensionString += ", ";
        }
    }
    sgl::Logfile::get()->write(
            std::string() + "Used Vulkan device extensions: " + deviceExtensionString, BLUE);

    createVulkanMemoryAllocator(instance->getVkInstance(), physicalDevice, device, allocator);
}

void Device::createDeviceHeadless(
        Instance* instance,
        const std::vector<const char*>& requiredDeviceExtensions,
        const std::vector<const char*>& optionalDeviceExtensions) {
    this->instance = instance;
    this->window = nullptr;

    std::vector<const char*> deviceExtensions;
    physicalDevice = createPhysicalDeviceBinding(
            instance->getVkInstance(), nullptr, requiredDeviceExtensions, optionalDeviceExtensions,
            deviceExtensionsSet, deviceExtensions);
    initializeDeviceExtensionList(physicalDevice);
    printAvailableDeviceExtensionList();

    auto deviceQueuePair = createLogicalDeviceAndQueues(
            physicalDevice, instance->getUseValidationLayer(), instance->getInstanceLayerNames(), deviceExtensions);
    device = deviceQueuePair.device;
    graphicsQueueIndex = deviceQueuePair.graphicsQueueIndex;
    computeQueueIndex = deviceQueuePair.computeQueueIndex;
    graphicsQueue = deviceQueuePair.graphicsQueue;
    computeQueue = deviceQueuePair.computeQueue;

    std::string deviceExtensionString;
    for (size_t i = 0; i < deviceExtensions.size(); i++) {
        deviceExtensionString += deviceExtensions.at(i);
        if (i != deviceExtensions.size() - 1) {
            deviceExtensionString += ", ";
        }
    }
    sgl::Logfile::get()->write(
            std::string() + "Used Vulkan device extensions: " + deviceExtensionString, BLUE);

    createVulkanMemoryAllocator(instance->getVkInstance(), physicalDevice, device, allocator);
}

Device::~Device() {
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
}

VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties) {
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

}}
