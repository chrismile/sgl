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

#include <set>
#include <string>
#include <cassert>

#include <Utils/HashCombine.hpp>
#include <Utils/Json/SimpleJson.hpp>
#include <ImGui/imgui.h>

#include "DeviceSelectionWGL.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

typedef BOOL ( WINAPI *PFN_EnumDisplayDevicesA )( LPCSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICEA lpDisplayDevice, DWORD dwFlags );

extern "C" {
    // https://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000000;
    // https://gpuopen.com/learn/amdpowerxpressrequesthighperformance/
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000000;
}

namespace sgl {

constexpr uint16_t vendorIdNvidia = 0x10DE;
constexpr uint16_t vendorIdAmd = 0x1002;
constexpr uint16_t vendorIdIntel = 0x8086;
#define VENDOR_STRING_NVIDIA "PCI\\VEN_10DE&"
#define VENDOR_STRING_AMD "PCI\\VEN_1002&"
#define VENDOR_STRING_INTEL "PCI\\VEN_8086&"

DeviceSelectorWGL::DeviceSelectorWGL() {
    // Check if we have a dGPU from NVIDIA or AMD and a separate iGPU.
    HMODULE user32Module = LoadLibrary("user32.dll");
    auto* pEnumDisplayDevicesA = PFN_EnumDisplayDevicesA(GetProcAddress(user32Module, "EnumDisplayDevicesA"));
    assert(pEnumDisplayDevicesA);

    bool hasNvidiaGpu = false;
    bool hasAmdGpu = false;
    bool hasIntelGpu = false;
    DWORD adapterIdx = 0;
    DISPLAY_DEVICEA displayDevice{};
    displayDevice.cb = sizeof(DISPLAY_DEVICEA);
    std::map<uint16_t, std::set<std::string>> vendorDeviceNameMap;
    do {
        bool retVal = pEnumDisplayDevicesA(nullptr, adapterIdx, &displayDevice, 0);
        if (!retVal) {
            adapterIdx++;
            break;
        }
        if (strstr(displayDevice.DeviceID, VENDOR_STRING_NVIDIA)) {
            isHybridNvidia = true;
            vendorDeviceNameMap[vendorIdNvidia].insert(std::string(displayDevice.DeviceString));
        }
        if (strstr(displayDevice.DeviceID, VENDOR_STRING_AMD)) {
            hasAmdGpu = true;
            vendorDeviceNameMap[vendorIdAmd].insert(std::string(displayDevice.DeviceString));
        }
        if (strstr(displayDevice.DeviceID, VENDOR_STRING_INTEL)) {
            hasIntelGpu = true;
            vendorDeviceNameMap[vendorIdIntel].insert(std::string(displayDevice.DeviceString));
        }
        hash_combine(systemConfigurationHash, std::string(displayDevice.DeviceID));
        // displayDevice.DeviceString would be a human-readable string.
        adapterIdx++;
    } while (true);
    if (hasNvidiaGpu && (hasIntelGpu || hasAmdGpu)) {
        isHybridNvidia = true;
    }
    if (hasAmdGpu) {
        if (hasIntelGpu || vendorDeviceNameMap[vendorIdAmd].size() > 1) {
            isHybridAmd = true;
        }
    }

    FreeLibrary(user32Module);
}

void DeviceSelectorWGL::serializeSettings(JsonValue& settings) {
    if (!forceUseNvidiaDiscrete && !forceUseAmdDiscrete) {
        return;
    }
    auto& deviceSelection = settings["deviceSelection"];
    deviceSelection["systemConfigHash"] = uint64_t(systemConfigurationHash);
    deviceSelection["forceUseNvidiaDiscrete"] = forceUseNvidiaDiscrete;
    deviceSelection["forceUseAmdDiscrete"] = forceUseAmdDiscrete;
}

void DeviceSelectorWGL::deserializeSettings(const JsonValue& settings) {
    if (!settings.hasMember("deviceSelection")) {
        return;
    }
    const auto& deviceSelection = settings["deviceSelection"];
    uint64_t deserializedSystemConfigHash = deviceSelection["systemConfigHash"].asUint64();
    if (deserializedSystemConfigHash != uint64_t(systemConfigurationHash)) {
        // The user may have swapped out the GPUs in the system.
        return;
    }
    if (deviceSelection.hasMember("forceUseNvidiaDiscrete")) {
        forceUseNvidiaDiscrete = deviceSelection["forceUseNvidiaDiscrete"].asBool();
    }
    if (deviceSelection.hasMember("forceUseAmdDiscrete")) {
        forceUseAmdDiscrete = deviceSelection["forceUseAmdDiscrete"].asBool();
    }
    if (forceUseNvidiaDiscrete) {
        NvOptimusEnablement = 0x00000001;
    }
    if (forceUseAmdDiscrete) {
        AmdPowerXpressRequestHighPerformance = 0x00000001;
    }
}

void DeviceSelectorWGL::renderGui() {
    if (!isHybridNvidia && !isHybridAmd) {
        return;
    }
    if (ImGui::BeginMenu("Window")) {
        if (isHybridNvidia && ImGui::MenuItem("Tet Mesh Volume Renderer", nullptr, forceUseNvidiaDiscrete)) {
            forceUseNvidiaDiscrete = !forceUseNvidiaDiscrete;
            requestOpenRestartAppDialog();
        } else if (isHybridAmd && ImGui::MenuItem("FPS Overlay", nullptr, forceUseAmdDiscrete)) {
            forceUseAmdDiscrete = !forceUseAmdDiscrete;
            requestOpenRestartAppDialog();
        }
        ImGui::EndMenu();
    }
}

}
