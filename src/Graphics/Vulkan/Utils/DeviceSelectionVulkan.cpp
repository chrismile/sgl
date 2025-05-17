/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#include <Utils/HashCombine.hpp>
#include <Utils/Json/SimpleJson.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <ImGui/imgui.h>

#include "DeviceSelectionVulkan.hpp"

namespace sgl {

DeviceSelectorVulkan::DeviceSelectorVulkan(const std::vector<VkPhysicalDevice>& suitablePhysicalDevices) {
    physicalDevices.emplace_back("Default", VK_NULL_HANDLE);
    VkPhysicalDeviceProperties physicalDeviceProperties;
    for (auto& physicalDevice : suitablePhysicalDevices) {
        vk::getPhysicalDeviceProperties(physicalDevice, physicalDeviceProperties);
        physicalDevices.emplace_back(physicalDeviceProperties.deviceName, physicalDevice);
    }

    /*
     * We compute the hash in sorted order, as the order of devices may change in one of these events:
     * - The user selects that this application is run in efficiency or performance mode (Windows).
     * - The user may use environment variables to force the use of another GPU (Linux & vk::Device options).
     * - The user plugs a monitor into another GPU.
     * In all of these cases, the system configuration is still the same and the user's choice should be respected.
     */
    std::vector<std::pair<std::string, VkPhysicalDevice>> sortedDeviceNames;
    std::copy(physicalDevices.begin() + 1, physicalDevices.end(), std::back_inserter(sortedDeviceNames));
    std::sort(sortedDeviceNames.begin(), sortedDeviceNames.end());
    for (const auto& physicalDevice : sortedDeviceNames) {
        hash_combine(systemConfigurationHash, physicalDevice.first);
    }
}

void DeviceSelectorVulkan::serializeSettings(JsonValue& settings) {
    if (selectedDeviceIndex == 0) {
        // Default device is used.
        return;
    }
    auto& deviceSelection = settings["deviceSelection"];
    deviceSelection["systemConfigHash"] = uint64_t(systemConfigurationHash);
    auto physicalDevice = physicalDevices.at(selectedDeviceIndex).second;
    VkPhysicalDeviceIDProperties physicalDeviceIDProperties{};
    physicalDeviceIDProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &physicalDeviceIDProperties;
    vk::getPhysicalDeviceProperties2(physicalDevice, deviceProperties2);
    convertUuidToJsonValue(physicalDeviceIDProperties.driverUUID, deviceSelection["selectedDriverUUID"]);
    convertUuidToJsonValue(physicalDeviceIDProperties.deviceUUID, deviceSelection["selectedDeviceUUID"]);
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vk::getPhysicalDeviceProperties(physicalDevice, physicalDeviceProperties);
    deviceSelection["deviceName"] = physicalDeviceProperties.deviceName;
}

void DeviceSelectorVulkan::deserializeSettings(const JsonValue& settings) {
    if (!settings.hasMember("deviceSelection")) {
        return;
    }
    const auto& deviceSelection = settings["deviceSelection"];
    uint64_t deserializedSystemConfigHash = deviceSelection["systemConfigHash"].asUint64();
    if (deserializedSystemConfigHash != uint64_t(systemConfigurationHash)) {
        // The user may have swapped out the GPUs in the system.
        return;
    }
    auto selectedDriverUuid = convertJsonValueToUuid(deviceSelection["selectedDriverUUID"]);
    auto selectedDeviceUuid = convertJsonValueToUuid(deviceSelection["selectedDeviceUUID"]);
    VkPhysicalDeviceIDProperties physicalDeviceIDProperties{};
    physicalDeviceIDProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &physicalDeviceIDProperties;
    for (size_t i = 1; i < physicalDevices.size(); i++) {
        vk::getPhysicalDeviceProperties2(physicalDevices[i].second, deviceProperties2);
        if (strncmp(
                    reinterpret_cast<const char*>(physicalDeviceIDProperties.driverUUID),
                    reinterpret_cast<const char*>(selectedDriverUuid.data()), VK_UUID_SIZE) == 0
                && strncmp(
                    reinterpret_cast<const char*>(physicalDeviceIDProperties.deviceUUID),
                    reinterpret_cast<const char*>(selectedDeviceUuid.data()), VK_UUID_SIZE) == 0) {
            selectedDeviceIndex = i;
            break;
        }
    }
}

void DeviceSelectorVulkan::renderGui() {
    if (physicalDevices.size() == 2) {
        return;
    }
    if (ImGui::BeginMenu("Device selection")) {
        for (size_t i = 0; i < physicalDevices.size(); i++) {
            bool useDevice = selectedDeviceIndex == i;
            if (ImGui::MenuItem(physicalDevices[i].first.c_str(), nullptr, useDevice)) {
                if (selectedDeviceIndex != i) {
                    selectedDeviceIndex = i;
                    requestOpenRestartAppDialog();
                }
            }
        }
        ImGui::EndMenu();
    }
}

VkPhysicalDevice DeviceSelectorVulkan::getSelectedPhysicalDevice() {
    return physicalDevices.at(selectedDeviceIndex).second;
}

void DeviceSelectorVulkan::setUsedPhysicalDevice(VkPhysicalDevice usedPhysicalDevice) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vk::getPhysicalDeviceProperties(usedPhysicalDevice, physicalDeviceProperties);
    physicalDevices.at(0).first = "Default (" + std::string(physicalDeviceProperties.deviceName) + ")";
    physicalDevices.at(0).second = usedPhysicalDevice;
}

}
