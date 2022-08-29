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

#include <map>
#include <cstring>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include "Status.hpp"
#include "Instance.hpp"

namespace sgl { namespace vk {

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData) {
    Instance* instance = static_cast<Instance*>(userData);
    if ((int)messageSeverity >= (int)instance->getDebugMessageSeverityLevel()) {
        sgl::Logfile::get()->writeError(
                std::string() + "Validation layer: " + callbackData->pMessage);
        instance->callDebugCallback();
    }
    return VK_FALSE;
}

Instance::Instance() {
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS) {
        Logfile::get()->throwError("Error in AppSettings::initializeVolk: volkInitialize failed.");
    }
    instanceVulkanVersion = volkGetInstanceVersion();

    initializeInstanceExtensionList();
}

Instance::~Instance() {
    if (useValidationLayer) {
        auto _vkDestroyDebugUtilsMessengerEXT =
                (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (_vkDestroyDebugUtilsMessengerEXT != nullptr) {
            _vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
    }

    vkDestroyInstance(instance, nullptr);
}

void Instance::createInstance(std::vector<const char*> instanceExtensionNames, bool _useValidationLayer) {
    useValidationLayer = _useValidationLayer;
    printAvailableInstanceExtensionList();

    std::string instanceExtensionString;
    for (size_t i = 0; i < instanceExtensionNames.size(); i++) {
        instanceExtensionString += instanceExtensionNames.at(i);
        if (i != instanceExtensionNames.size() - 1) {
            instanceExtensionString += ", ";
        }
    }
    sgl::Logfile::get()->write(
            std::string() + "Used Vulkan instance extensions: " + instanceExtensionString, BLUE);

    appInfo = { };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = FileUtils::get()->getAppName().c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "sgl";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    //appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
    appInfo.apiVersion = std::min(instanceVulkanVersion, VK_MAKE_API_VERSION(0, 1, 3, 216));

    // Add a validation layer if requested.
    instanceLayerNames = {};
    if (useValidationLayer) {
        instanceLayerNames.push_back("VK_LAYER_KHRONOS_validation"); // VK_LAYER_LUNARG_standard_validation
        if (!checkRequestedLayersAvailable(instanceLayerNames)) {
            sgl::Logfile::get()->write(
                    "Instance::createInstance: Disabling validation layer, as VK_LAYER_KHRONOS_validation is not "
                    "available.", BLACK);
            useValidationLayer = false;
            instanceLayerNames.clear();
        } else {
            instanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
    }
    if (instanceVulkanVersion > VK_MAKE_API_VERSION(0, 1, 1, 0)) {
        instanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    VkInstanceCreateInfo instanceInfo = { };
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayerNames.size());
    instanceInfo.ppEnabledLayerNames = instanceLayerNames.data();
    instanceInfo.enabledExtensionCount = (uint32_t)instanceExtensionNames.size();
    instanceInfo.ppEnabledExtensionNames = instanceExtensionNames.data();

    VkResult res = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (res == VK_ERROR_EXTENSION_NOT_PRESENT) {
        std::vector<size_t> availableEnabledExtensionIndices;
        std::string enabledExtensionNames;
        for (size_t i = 0; i < instanceExtensionNames.size(); i++) {
            enabledExtensionNames += instanceExtensionNames.at(i);
            if (isInstanceExtensionAvailable(instanceExtensionNames.at(i))) {
                availableEnabledExtensionIndices.push_back(i);
            }
            if (i != instanceExtensionNames.size() - 1) {
                enabledExtensionNames += ", ";
            }
        }
        std::string availableEnabledExtensionNames;
        if (availableEnabledExtensionIndices.empty()) {
            availableEnabledExtensionNames = "None";
        }
        for (size_t i = 0; i < availableEnabledExtensionIndices.size(); i++) {
            availableEnabledExtensionNames += instanceExtensionNames.at(availableEnabledExtensionIndices.at(i));
            if (i != availableEnabledExtensionIndices.size() - 1) {
                availableEnabledExtensionNames += ", ";
            }
        }
        sgl::Logfile::get()->throwError(
                std::string() + "Error in Instance::createInstance: Cannot find a specified extension. Enabled "
                + "extensions: " + enabledExtensionNames + ". Available enabled extensions: "
                + availableEnabledExtensionNames);
    } else if (res == VK_ERROR_INCOMPATIBLE_DRIVER) {
        sgl::Logfile::get()->throwError(
                std::string() + "Error in Instance::createInstance: Could not find a compatible Vulkan driver.");
    } else if (res == VK_ERROR_LAYER_NOT_PRESENT) {
        std::vector<size_t> availableEnabledLayerIndices;
        std::string enabledLayerNames;
        for (size_t i = 0; i < instanceLayerNames.size(); i++) {
            enabledLayerNames += instanceLayerNames.at(i);
            if (checkRequestedLayersAvailable({ instanceLayerNames.at(i) })) {
                availableEnabledLayerIndices.push_back(i);
            }
            if (i != instanceLayerNames.size() - 1) {
                enabledLayerNames += ", ";
            }
        }
        std::string availableEnabledLayerNames;
        if (availableEnabledLayerIndices.empty()) {
            availableEnabledLayerNames = "None";
        }
        for (size_t i = 0; i < availableEnabledLayerIndices.size(); i++) {
            availableEnabledLayerNames += instanceLayerNames.at(availableEnabledLayerIndices.at(i));
            if (i != availableEnabledLayerIndices.size() - 1) {
                availableEnabledLayerNames += ", ";
            }
        }
        sgl::Logfile::get()->throwError(
                std::string() + "Error in Instance::createInstance: Cannot find a specified layer. Enabled layers: "
                + enabledLayerNames + ". Available enabled layers: " + availableEnabledLayerNames);
    } else if (res != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                std::string() + "Error in Instance::createInstance: Failed to create a Vulkan instance ("
                + vulkanResultToString(res) + ").");
    }
    volkLoadInstance(instance);

    if (useValidationLayer) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = { };
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = vulkanDebugCallback;
        createInfo.pUserData = this;

        auto _vkCreateDebugUtilsMessengerEXT =
                (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                        instance, "vkCreateDebugUtilsMessengerEXT");
        if (_vkCreateDebugUtilsMessengerEXT != nullptr) {
            if (_vkCreateDebugUtilsMessengerEXT(instance, &createInfo, NULL, &debugMessenger) != VK_SUCCESS) {
                sgl::Logfile::get()->writeError(
                        "Error in Instance::createInstance: Failed to create Vulkan debug utils messenger.");
            }
        } else {
            sgl::Logfile::get()->writeError(
                    "Error in Instance::createInstance: Missing vkCreateDebugUtilsMessengerEXT.");
        }
    }
}

bool Instance::checkRequestedLayersAvailable(const std::vector<const char*> &requestedLayers) const {
    uint32_t numLayers;
    VkResult res = vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
    if (res != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                "Error in Instance::checkRequestedLayersAvailable: "
                "vkEnumerateInstanceLayerProperties (1) failed (" + vulkanResultToString(res) + ")!");
    }
    std::vector<VkLayerProperties> availableLayerList(numLayers);
    res = vkEnumerateInstanceLayerProperties(&numLayers, availableLayerList.data());
    if (res != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                "Error in Instance::checkRequestedLayersAvailable: "
                "vkEnumerateInstanceLayerProperties (2) failed (" + vulkanResultToString(res) + ")!");
    }

    std::map<std::string, VkLayerProperties> availableLayers;
    for (const VkLayerProperties& layerProperties : availableLayerList) {
        availableLayers.insert(std::make_pair(layerProperties.layerName, layerProperties));
    }

    for (const char *requestedLayer : requestedLayers) {
        bool isValidationLayer = strcmp(requestedLayer, "VK_LAYER_KHRONOS_validation") == 0;

        if (availableLayers.find(requestedLayer) == availableLayers.end()) {
            if (isValidationLayer) {
                sgl::Logfile::get()->writeWarning(
                        std::string() + "Warning: Invalid Vulkan layer name \"" + requestedLayer + "\".",
                        false);
            } else {
                sgl::Logfile::get()->writeError(
                        std::string() + "Error: Invalid Vulkan layer name \"" + requestedLayer + "\".");
            }
            return false;
        }

        // Disable the validation layer when it is older than the Vulkan version (ignoring patch version).
        if (isValidationLayer) {
            uint32_t validationLayerVersion = availableLayers[requestedLayer].specVersion;
            if ((validationLayerVersion & 0xFFFFF000u) < (getInstanceVulkanVersion() & 0xFFFFF000u)) {
                return false;
            }
        }
    }

    return true;
}

bool Instance::getInstanceExtensionsAvailable(const std::vector<const char*>& instanceExtensionNames) {
    for (const char* extensionName : instanceExtensionNames) {
        if (availableInstanceExtensionNames.find(extensionName) == availableInstanceExtensionNames.end()) {
            return false;
        }
    }
    return true;
}

void Instance::initializeInstanceExtensionList() {
    uint32_t availableInstanceExtensionCount = 0;
    VkResult res = vkEnumerateInstanceExtensionProperties(
            nullptr, &availableInstanceExtensionCount, nullptr);
    if (res != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                "Error in Instance::initializeInstanceExtensionList: "
                "vkEnumerateInstanceExtensionProperties failed (" + vulkanResultToString(res) + ")!");
    }

    if (availableInstanceExtensionCount > 0) {
        VkExtensionProperties* instanceExtensions = new VkExtensionProperties[availableInstanceExtensionCount];
        res = vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, instanceExtensions);
        if (res != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    "Error in Instance::initializeInstanceExtensionList: "
                    "vkEnumerateInstanceExtensionProperties failed (" + vulkanResultToString(res) + ")!");
        }

        for (uint32_t i = 0; i < availableInstanceExtensionCount; i++) {
            availableInstanceExtensionNames.insert(instanceExtensions[i].extensionName);
        }

        delete[] instanceExtensions;
    }
}

void Instance::printAvailableInstanceExtensionList() {
    size_t i = 0;
    size_t n = availableInstanceExtensionNames.size();
    std::string instanceExtensionString;
    for (const std::string &extensionName : availableInstanceExtensionNames) {
        instanceExtensionString += extensionName;
        if (i != n-1) {
            instanceExtensionString += ", ";
        }
        i++;
    }
    sgl::Logfile::get()->write(
            std::string() + "Available Vulkan instance extensions: " + instanceExtensionString, BLUE);
}

bool Instance::isInstanceExtensionAvailable(const std::string &extensionName) {
    return availableInstanceExtensionNames.find(extensionName) != availableInstanceExtensionNames.end();
}

std::string Instance::convertVulkanVersionToString(uint32_t version) {
    std::string versionString = "Vulkan ";
    versionString += std::to_string(VK_API_VERSION_MAJOR(version));
    versionString += ".";
    versionString += std::to_string(VK_API_VERSION_MINOR(version));
    versionString += ".";
    versionString += std::to_string(VK_API_VERSION_PATCH(version));
    return versionString;
}

}}
