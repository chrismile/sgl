/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_WEBGPU_DEVICE_HPP
#define SGL_WEBGPU_DEVICE_HPP

#include <vector>
#include <set>
#include <functional>
#include <optional>
#include <webgpu/webgpu.h>

namespace sgl {
class Window;
}

namespace sgl { namespace webgpu {

class Instance;

DLL_OBJECT WGPULimits getDefaultWGPULimits();

class DLL_OBJECT Device {
public:
    /// For rendering using a window surface and a swapchain.
    void createDeviceSwapchain(
            Instance* _instance, Window* window,
            const std::vector<WGPUFeatureName>& requiredFeatures = {},
            const std::vector<WGPUFeatureName>& optionalFeatures = {},
            std::optional<WGPULimits> requiredLimits = {},
            std::optional<WGPULimits> optionalLimits = {},
            WGPUPowerPreference _powerPreference = WGPUPowerPreference_Undefined);
    /// For headless rendering without a window.
    void createDeviceHeadless(
            Instance* _instance,
            const std::vector<WGPUFeatureName>& requiredFeatures = {},
            const std::vector<WGPUFeatureName>& optionalFeatures = {},
            std::optional<WGPULimits> requiredLimits = {},
            std::optional<WGPULimits> optionalLimits = {},
            WGPUPowerPreference _powerPreference = WGPUPowerPreference_Undefined);
    ~Device();

    /**
     * Polls the device events such that asynchronous callbacks are processed.
     * @param yieldExecution Whether to yield execution to the browser if Emscripten is used.
     */
    void pollEvents(bool yieldExecution);

    /// Encodes and executes commands.
    void executeCommands(const std::function<void(WGPUCommandEncoder encoder)>& encodeCommandsCallback);

    [[nodiscard]] inline Instance* getInstance() { return instance; }
    [[nodiscard]] inline WGPUAdapter getWGPUAdapter() { return adapter; }
    [[nodiscard]] inline WGPUDevice getWGPUDevice() { return device; }
    [[nodiscard]] inline WGPUQueue getWGPUQueue() { return queue; }
    [[nodiscard]] inline bool getHasFeature(WGPUFeatureName feature) const {
        return deviceFeaturesSet.find(feature) != deviceFeaturesSet.end();
    }
    [[nodiscard]] inline const WGPULimits& getLimits() const { return deviceLimits; }

    void printAdapterInfo();
    void printDeviceInfo();

private:
    void createInternal(
            Window* window,
            const std::vector<WGPUFeatureName>& requiredFeatures,
            const std::vector<WGPUFeatureName>& optionalFeatures,
            std::optional<WGPULimits> requiredLimits,
            std::optional<WGPULimits> optionalLimits);
    void queryAdapterCapabilities();
    void queryDeviceCapabilities();
    Instance* instance = nullptr;

    // Adapter & info.
    WGPUAdapter adapter{};
    WGPUPowerPreference powerPreference{};
#if defined(__EMSCRIPTEN__) || defined(WEBGPU_BACKEND_DAWN)
    bool adapterInfoValid = false;
    WGPUAdapterInfo adapterInfo{};
#endif
#if defined(__EMSCRIPTEN__) || !defined(WEBGPU_BACKEND_DAWN)
    WGPUAdapterProperties adapterProperties{};
#endif
    std::vector<WGPUFeatureName> adapterSupportedFeatures;
    bool adapterSupportedLimitsValid = false;
    WGPUSupportedLimits adapterSupportedLimits{};

    // Device.
    WGPUDevice device{};
    std::vector<WGPUFeatureName> deviceFeatures{};
    std::set<WGPUFeatureName> deviceFeaturesSet{};
    bool deviceSupportedLimitsValid = false;
    WGPULimits deviceLimits{};

    // Device queue.
    WGPUQueue queue{};
};

}}

#endif //SGL_WEBGPU_DEVICE_HPP
