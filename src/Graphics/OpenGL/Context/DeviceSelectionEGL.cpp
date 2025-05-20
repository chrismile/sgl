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

#include <algorithm>
#include <set>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <Utils/HashCombine.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/Json/SimpleJson.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
#include <ImGui/imgui.h>

#include "DeviceSelectionEGL.hpp"

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define dlsym GetProcAddress
#endif

// https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_persistent_id.txt
#ifndef EGL_EXT_device_persistent_id
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDEVICEBINARYEXTPROC) (EGLDeviceEXT device, EGLint name, EGLint max_size, void *value, EGLint *size);
#define EGL_DEVICE_UUID_EXT 0x335C
#define EGL_DRIVER_UUID_EXT 0x335D
#endif

#ifndef EGL_EXT_device_query_name
#define EGL_RENDERER_EXT 0x335F
#endif

#ifndef EGL_EXT_device_drm
#define EGL_DRM_DEVICE_FILE_EXT 0x3233
#define EGL_DRM_MASTER_FD_EXT 0x333C
#endif

#ifndef EGL_EXT_device_drm_render_node
#define EGL_DRM_RENDER_NODE_FILE_EXT 0x3377
#endif

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

namespace sgl {

struct DeviceSelectionEGLFunctionTable {
    PFNEGLGETPROCADDRESSPROC eglGetProcAddress;
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT;
    PFNEGLQUERYDEVICESTRINGEXTPROC eglQueryDeviceStringEXT;
    PFNEGLQUERYDEVICEBINARYEXTPROC eglQueryDeviceBinaryEXT;
};

DeviceSelectorEGL::DeviceSelectorEGL() {
#if defined(__linux__)
    eglHandle = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (!eglHandle) {
        eglHandle = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!eglHandle) {
            sgl::Logfile::get()->writeError("DeviceSelectorEGL::DeviceSelectorEGL: Could not load libEGL.so.", false);
            return;
        }
    }
#elif defined(_WIN32)
    eglHandle = LoadLibraryA("EGL.dll");
    if (!eglHandle) {
        sgl::Logfile::get()->writeError("DeviceSelectorEGL::DeviceSelectorEGL: Could not load EGL.dll.", false);
        return false;
    }
#endif

    f = new DeviceSelectionEGLFunctionTable;
    f->eglGetProcAddress = PFNEGLGETPROCADDRESSPROC(dlsym(eglHandle, TOSTRING(eglGetProcAddress)));

    if (!f->eglGetProcAddress) {
        sgl::Logfile::get()->writeError(
                "Error in DeviceSelectorEGL::DeviceSelectorEGL: "
                "At least one function pointer could not be loaded.", false);
        return;
    }

    f->eglQueryDevicesEXT = PFNEGLQUERYDEVICESEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDevicesEXT)));
    f->eglQueryDeviceStringEXT = PFNEGLQUERYDEVICESTRINGEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDeviceStringEXT)));
    f->eglQueryDeviceBinaryEXT = PFNEGLQUERYDEVICEBINARYEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDeviceBinaryEXT)));

    if (!f->eglQueryDevicesEXT && !f->eglQueryDeviceStringEXT) {
        sgl::Logfile::get()->writeError(
                "Error in DeviceSelectorEGL::DeviceSelectorEGL: "
                "At least one function pointer could not be loaded.", false);
        return;
    }

    EGLint numEglDevices = 0;
    if (!f->eglQueryDevicesEXT(0, nullptr, &numEglDevices)) {
        sgl::Logfile::get()->writeError(
                "Error in DeviceSelectorEGL::DeviceSelectorEGL: eglQueryDevicesEXT failed.", false);
        return;
    }
    auto* eglDevices = new EGLDeviceEXT[numEglDevices];
    if (!f->eglQueryDevicesEXT(numEglDevices, eglDevices, &numEglDevices)) {
        sgl::Logfile::get()->writeError(
                "Error in DeviceSelectorEGL::DeviceSelectorEGL: eglQueryDevicesEXT failed.", false);
        return;
    }
    if (numEglDevices == 0) {
        sgl::Logfile::get()->writeError(
                "Error in DeviceSelectorEGL::DeviceSelectorEGL: eglQueryDevicesEXT returned no device.", false);
        return;
    }

    DeviceSelectionEntryEGL defaultDeviceEntry{};
    defaultDeviceEntry.name = "Default";
    defaultDeviceEntry.deviceIdx = -1;
    deviceList.emplace_back(defaultDeviceEntry);
    for (int i = 0; i < numEglDevices; i++) {
        DeviceSelectionEntryEGL deviceEntry{};
        deviceEntry.deviceIdx = i;

        const char* deviceExtensions = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_EXTENSIONS);
        if (!deviceExtensions) {
            sgl::Logfile::get()->writeError(
                    "Error in DeviceSelectorEGL::DeviceSelectorEGL: eglQueryDeviceStringEXT failed.", false);
            continue;
        }
        std::string deviceExtensionsString(deviceExtensions);
        sgl::Logfile::get()->write("Device #" + std::to_string(i) + " Extensions: " + deviceExtensions, BLUE);
        std::vector<std::string> deviceExtensionsVector;
        sgl::splitStringWhitespace(deviceExtensionsString, deviceExtensionsVector);
        std::set<std::string> deviceExtensionsSet(deviceExtensionsVector.begin(), deviceExtensionsVector.end());

        if (deviceExtensionsSet.find("EGL_EXT_device_query_name") != deviceExtensionsSet.end()) {
            const char* deviceVendor = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_VENDOR);
            const char* deviceRenderer = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_RENDERER_EXT);
            if (deviceVendor) {
                sgl::Logfile::get()->write("Device #" + std::to_string(i) + " Vendor: " + deviceVendor, BLUE);
            }
            if (deviceRenderer) {
                sgl::Logfile::get()->write("Device #" + std::to_string(i) + " Renderer: " + deviceRenderer, BLUE);
                deviceEntry.name = deviceRenderer;
            }
        }

        if (deviceExtensionsSet.find("EGL_EXT_device_drm") != deviceExtensionsSet.end()) {
            const char* deviceDrmFile = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_DRM_DEVICE_FILE_EXT);
            if (deviceDrmFile) {
                sgl::Logfile::get()->write("Device #" + std::to_string(i) + " DRM File: " + deviceDrmFile, BLUE);
            }
        }

        if (deviceExtensionsSet.find("EGL_EXT_device_drm_render_node") != deviceExtensionsSet.end()) {
            const char* deviceDrmRenderNodeFile = f->eglQueryDeviceStringEXT(
                    eglDevices[i], EGL_DRM_RENDER_NODE_FILE_EXT);
            if (deviceDrmRenderNodeFile) {
                sgl::Logfile::get()->write(
                        "Device #" + std::to_string(i) + " DRM Render Node File: " + deviceDrmRenderNodeFile, BLUE);
                if (deviceEntry.name.empty()) {
                    deviceEntry.name = deviceDrmRenderNodeFile;
                }
            }
        }

        if (deviceEntry.name.empty()) {
            deviceEntry.name = "Device #" + std::to_string(i + 1);
        }

        EGLint driverUuidSize = 0;
        EGLint deviceUuidSize = 0;
        if (f->eglQueryDeviceBinaryEXT
                    && deviceExtensionsSet.find("EGL_EXT_device_persistent_id") != deviceExtensionsSet.end()
                    && f->eglQueryDeviceBinaryEXT(eglDevices[i], EGL_DEVICE_UUID_EXT, 16, deviceEntry.deviceUuid.data(), &driverUuidSize)
                    && f->eglQueryDeviceBinaryEXT(eglDevices[i], EGL_DEVICE_UUID_EXT, 16, deviceEntry.deviceUuid.data(), &deviceUuidSize)) {
            if (driverUuidSize == 16 && deviceUuidSize == 16) {
                deviceEntry.hasUuid = true;
            }
        }

        deviceList.push_back(deviceEntry);
    }

    std::vector<std::string> sortedDeviceNames;
    for (const auto& device : deviceList) {
        sortedDeviceNames.push_back(device.name);
    }
    std::sort(sortedDeviceNames.begin(), sortedDeviceNames.end());
    for (const auto& deviceName : sortedDeviceNames) {
        hash_combine(systemConfigurationHash, deviceName);
    }
}

DeviceSelectorEGL::~DeviceSelectorEGL() {
    if (f) {
        delete f;
        f = nullptr;
    }
    if (eglHandle) {
#if defined(__linux__)
        dlclose(eglHandle);
#elif defined(_WIN32)
        FreeLibrary(eglHandle);
#endif
        eglHandle = {};
    }
}

void DeviceSelectorEGL::serializeSettings(JsonValue& settings) {
    if (selectedDeviceIndex == 0) {
        // Default device is used.
        return;
    }
    auto& deviceSelection = settings["deviceSelection"];
    deviceSelection["systemConfigHash"] = uint64_t(systemConfigurationHash);
    const auto& device = deviceList.at(selectedDeviceIndex);
    if (device.hasUuid) {
        convertUuidToJsonValue(device.driverUuid.data(), deviceSelection["selectedDriverUUID"]);
        convertUuidToJsonValue(device.deviceUuid.data(), deviceSelection["selectedDeviceUUID"]);
    }
    deviceSelection["deviceName"] = device.name;
}

void DeviceSelectorEGL::deserializeSettings(const JsonValue& settings) {
    if (!settings.hasMember("deviceSelection")) {
        return;
    }
    const auto& deviceSelection = settings["deviceSelection"];
    uint64_t deserializedSystemConfigHash = deviceSelection["systemConfigHash"].asUint64();
    if (deserializedSystemConfigHash != uint64_t(systemConfigurationHash)) {
        // The user may have swapped out the GPUs in the system.
        return;
    }
    if (deviceSelection.hasMember("selectedDriverUUID") && deviceSelection["selectedDeviceUUID"].isString()) {
        auto selectedDriverUuid = convertJsonValueToUuid(deviceSelection["selectedDriverUUID"]);
        auto selectedDeviceUuid = convertJsonValueToUuid(deviceSelection["selectedDeviceUUID"]);
        for (size_t i = 1; i < deviceList.size(); i++) {
            if (strncmp(
                        reinterpret_cast<const char*>(deviceList[i].driverUuid.data()),
                        reinterpret_cast<const char*>(selectedDriverUuid.data()), 16) == 0
                    && strncmp(
                        reinterpret_cast<const char*>(deviceList[i].deviceUuid.data()),
                        reinterpret_cast<const char*>(selectedDeviceUuid.data()), 16) == 0) {
                selectedDeviceIndex = i;
                break;
            }
        }
    } else if (deviceSelection.hasMember("deviceName")) {
        auto deviceName = deviceSelection["deviceName"].asString();
        for (size_t i = 1; i < deviceList.size(); i++) {
            if (deviceList[i].name == deviceName) {
                selectedDeviceIndex = i;
                break;
            }
        }
    }
}

void DeviceSelectorEGL::renderGui() {
    if (deviceList.size() == 2) {
        return;
    }
    if (ImGui::BeginCombo("Device selection", deviceList[selectedDeviceIndex].name.c_str())) {
        ImGui::Selectable(usedDeviceName.c_str(), false, ImGuiSelectableFlags_Disabled);
        for (size_t i = 0; i < deviceList.size(); i++) {
            bool useDevice = selectedDeviceIndex == i;
            if (ImGui::Selectable(deviceList[i].name.c_str(), useDevice)) {
                if (selectedDeviceIndex != i) {
                    selectedDeviceIndex = i;
                    requestOpenRestartAppDialog();
                }
            }
        }
        ImGui::EndCombo();
    }
}

void DeviceSelectorEGL::renderGuiMenu() {
    if (deviceList.size() == 2) {
        return;
    }
    if (ImGui::BeginMenu("Device selection")) {
        ImGui::MenuItem(usedDeviceName.c_str(), nullptr, true, false);
        for (size_t i = 0; i < deviceList.size(); i++) {
            bool useDevice = selectedDeviceIndex == i;
            if (ImGui::MenuItem(deviceList[i].name.c_str(), nullptr, useDevice)) {
                if (selectedDeviceIndex != i) {
                    selectedDeviceIndex = i;
                    requestOpenRestartAppDialog();
                }
            }
        }
        ImGui::EndMenu();
    }
}

int DeviceSelectorEGL::getSelectedEglDeviceIdx() {
    return deviceList.at(selectedDeviceIndex).deviceIdx;
}

void DeviceSelectorEGL::retrieveUsedDevice() {
    usedDeviceName = sgl::SystemGL::get()->getRendererString();
    if (usedDeviceName.empty()) {
        usedDeviceName = "UNKNOWN";
    }
}

}
