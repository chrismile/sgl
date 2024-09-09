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

#include <map>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Window.hpp>
#include "../Shader/ShaderManager.hpp"
#include "Instance.hpp"
#include "Device.hpp"

namespace sgl { namespace webgpu {

// See: https://www.w3.org/TR/webgpu/
WGPULimits getDefaultWGPULimits() {
    WGPULimits limits = {
            .maxTextureDimension1D = 8192,
            .maxTextureDimension2D = 8192,
            .maxTextureDimension3D = 2048,
            .maxTextureArrayLayers = 256,
            .maxBindGroups = 4,
            .maxBindGroupsPlusVertexBuffers = 24,
            .maxBindingsPerBindGroup = 1000,
            .maxDynamicUniformBuffersPerPipelineLayout = 8,
            .maxDynamicStorageBuffersPerPipelineLayout = 4,
            .maxSampledTexturesPerShaderStage = 16,
            .maxSamplersPerShaderStage = 16,
            .maxStorageBuffersPerShaderStage = 8,
            .maxStorageTexturesPerShaderStage = 4,
            .maxUniformBuffersPerShaderStage = 12,
            .maxUniformBufferBindingSize = 65536, // bytes
            .maxStorageBufferBindingSize = 134217728, // bytes, 128 MiB
            .minUniformBufferOffsetAlignment = 256, // bytes
            .minStorageBufferOffsetAlignment = 256, // bytes
            .maxVertexBuffers = 8,
            .maxBufferSize = 268435456, // bytes, 256 MiB
            .maxVertexAttributes = 16,
            .maxVertexBufferArrayStride = 2048, // bytes
            .maxInterStageShaderComponents = 64,
            .maxInterStageShaderVariables = 16,
            .maxColorAttachments = 8,
            .maxColorAttachmentBytesPerSample = 32,
            .maxComputeWorkgroupStorageSize = 16384, // bytes
            .maxComputeInvocationsPerWorkgroup = 256,
            .maxComputeWorkgroupSizeX = 256,
            .maxComputeWorkgroupSizeY = 256,
            .maxComputeWorkgroupSizeZ = 64,
            .maxComputeWorkgroupsPerDimension = 65535,
    };
    return limits;
}

template<typename Ret, typename Status, Status statusSuccess, typename Parent, typename Options, typename Func>
Ret requestSync(const char* funcName, Func func, Parent parent, const Options& options) {
    struct RequestData {
        const char* funcName;
        Ret ret;
        bool requestFinished;
    };
    RequestData requestData{};
    requestData.funcName = funcName;
    auto onRequestFinished = [](Status status, Ret ret, char const* message, void* userdata) {
        RequestData& requestData = *reinterpret_cast<RequestData*>(userdata);
        if (status == statusSuccess) {
            requestData.ret = ret;
        } else {
            if (message) {
                sgl::Logfile::get()->writeError(
                        "Error in requestSync: " + std::string(requestData.funcName)
                        + " failed with status 0x" + sgl::toHexString(status) + "and message: " + std::string(message));
            } else {
                sgl::Logfile::get()->writeError(
                        "Error in requestSync: " + std::string(requestData.funcName)
                        + " failed with status 0x" + sgl::toHexString(status) + ".");
            }
        }
        requestData.requestFinished = true;
    };
    func(parent, &options, onRequestFinished, (void*)&requestData);
#ifdef __EMSCRIPTEN__
    while (!requestData.requestFinished) {
        // Needs "-sASYNCIFY"
        emscripten_sleep(100);
    }
#endif
    return requestData.ret;
}

void Device::createDeviceSwapchain(
        Instance* _instance, Window* window,
        const std::vector<WGPUFeatureName>& requiredFeatures,
        const std::vector<WGPUFeatureName>& optionalFeatures,
        std::optional<WGPULimits> requiredLimits,
        std::optional<WGPULimits> optionalLimits,
        WGPUPowerPreference _powerPreference) {
    instance = _instance;
    powerPreference = _powerPreference;
    createInternal(
            window,
            requiredFeatures, optionalFeatures,
            requiredLimits, optionalLimits);
}

void Device::createDeviceHeadless(
        Instance* _instance,
        const std::vector<WGPUFeatureName>& requiredFeatures,
        const std::vector<WGPUFeatureName>& optionalFeatures,
        std::optional<WGPULimits> requiredLimits,
        std::optional<WGPULimits> optionalLimits,
        WGPUPowerPreference _powerPreference) {
    instance = _instance;
    powerPreference = _powerPreference;
    createInternal(
            nullptr,
            requiredFeatures, optionalFeatures,
            requiredLimits, optionalLimits);
}

void Device::createInternal(
        Window* window,
        const std::vector<WGPUFeatureName>& requiredFeatures,
        const std::vector<WGPUFeatureName>& optionalFeatures,
        std::optional<WGPULimits> requiredLimits,
        std::optional<WGPULimits> optionalLimits) {
    WGPURequestAdapterOptions requestAdapterOptions{};
    if (window) {
        requestAdapterOptions.compatibleSurface = window->getWebGPUSurface();
    }
    requestAdapterOptions.powerPreference = powerPreference;
    //requestAdapterOptions.backendType = xxx; //< Could be used for Vulkan interop possibility...
    adapter = requestSync<WGPUAdapter, WGPURequestAdapterStatus, WGPURequestAdapterStatus_Success>(
            "wgpuInstanceRequestAdapter", wgpuInstanceRequestAdapter,
            instance->getWGPUInstance(), requestAdapterOptions);
    queryAdapterCapabilities();

    // Request all required and supported optional features.
    std::set<WGPUFeatureName> adapterSupportedFeaturesSet(adapterSupportedFeatures.begin(), adapterSupportedFeatures.end());
    std::vector<WGPUFeatureName> requestedFeatures = requiredFeatures;
    for (const WGPUFeatureName& optionalFeature : optionalFeatures) {
        if (adapterSupportedFeaturesSet.find(optionalFeature) != adapterSupportedFeaturesSet.end()) {
            requestedFeatures.push_back(optionalFeature);
        }
    }
    std::sort(requestedFeatures.begin(), requestedFeatures.end());

    // Same as above, but for limits.
    bool isAnyLimitRequired = false;
    WGPURequiredLimits requestedLimits{};
    if (requiredLimits.has_value()) {
        isAnyLimitRequired = true;
        requestedLimits.limits = requiredLimits.value();
    }
    if (optionalLimits.has_value() && adapterSupportedLimitsValid) {
        isAnyLimitRequired = true;
#define SET_LIMIT_MAX(x) requestedLimits.limits.x = std::max(requestedLimits.limits.x, std::min(adapterSupportedLimits.limits.x, optionalLimits.value().x))
        SET_LIMIT_MAX(maxTextureDimension1D);
        SET_LIMIT_MAX(maxTextureDimension2D);
        SET_LIMIT_MAX(maxTextureDimension3D);
        SET_LIMIT_MAX(maxTextureArrayLayers);
        SET_LIMIT_MAX(maxBindGroups);
        SET_LIMIT_MAX(maxBindGroupsPlusVertexBuffers);
        SET_LIMIT_MAX(maxBindingsPerBindGroup);
        SET_LIMIT_MAX(maxDynamicUniformBuffersPerPipelineLayout);
        SET_LIMIT_MAX(maxDynamicStorageBuffersPerPipelineLayout);
        SET_LIMIT_MAX(maxSampledTexturesPerShaderStage);
        SET_LIMIT_MAX(maxSamplersPerShaderStage);
        SET_LIMIT_MAX(maxStorageBuffersPerShaderStage);
        SET_LIMIT_MAX(maxStorageTexturesPerShaderStage);
        SET_LIMIT_MAX(maxUniformBuffersPerShaderStage);
        SET_LIMIT_MAX(maxUniformBufferBindingSize);
        SET_LIMIT_MAX(maxStorageBufferBindingSize);
        SET_LIMIT_MAX(maxVertexBuffers);
        SET_LIMIT_MAX(maxBufferSize);
        SET_LIMIT_MAX(maxVertexAttributes);
        SET_LIMIT_MAX(maxVertexBufferArrayStride);
        SET_LIMIT_MAX(maxInterStageShaderComponents);
        SET_LIMIT_MAX(maxInterStageShaderVariables);
        SET_LIMIT_MAX(maxColorAttachments);
        SET_LIMIT_MAX(maxColorAttachmentBytesPerSample);
        SET_LIMIT_MAX(maxComputeWorkgroupStorageSize);
        SET_LIMIT_MAX(maxComputeInvocationsPerWorkgroup);
        SET_LIMIT_MAX(maxComputeWorkgroupSizeX);
        SET_LIMIT_MAX(maxComputeWorkgroupSizeY);
        SET_LIMIT_MAX(maxComputeWorkgroupSizeZ);
        SET_LIMIT_MAX(maxComputeWorkgroupsPerDimension);
#define SET_LIMIT_MIN(x, defaultVal) requestedLimits.limits.x = std::min(requestedLimits.limits.x, std::max(adapterSupportedLimits.limits.x, optionalLimits.value().x))
        SET_LIMIT_MIN(minUniformBufferOffsetAlignment, 256);
        SET_LIMIT_MIN(minStorageBufferOffsetAlignment, 256);
    }

    WGPUDeviceDescriptor deviceDescriptor{};
    deviceDescriptor.label = "PrimaryDevice";
    deviceDescriptor.requiredFeatureCount = requestedFeatures.size();
    deviceDescriptor.requiredFeatures = requestedFeatures.data();
    if (isAnyLimitRequired) {
        deviceDescriptor.requiredLimits = &requestedLimits;
    }
    deviceDescriptor.defaultQueue.label = "DefaultQueue";
    deviceDescriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* userdata) {
        if (message) {
            sgl::Logfile::get()->writeInfo("Device lost: " + std::string(message));
        } else {
            sgl::Logfile::get()->writeInfo("Device lost");
        }
    };
    device = requestSync<WGPUDevice, WGPURequestDeviceStatus, WGPURequestDeviceStatus_Success>(
            "wgpuAdapterRequestDevice", wgpuAdapterRequestDevice, adapter, deviceDescriptor);
    queryDeviceCapabilities();

    auto uncapturedErrorCallback = [](WGPUErrorType type, char const* message, void* userdata) {
        if (message && sgl::stringContains(message, "wgpuDeviceCreateShaderModule")) {
            ShaderManager->onCompilationFailed(message);
        } else if (message) {
            sgl::Logfile::get()->writeInfo("Uncaptured device error: " + std::string(message));
        } else {
            sgl::Logfile::get()->writeInfo("Uncaptured device error");
        }
    };
    wgpuDeviceSetUncapturedErrorCallback(device, uncapturedErrorCallback, nullptr);

    queue = wgpuDeviceGetQueue(device);
}

Device::~Device() {
    if (queue) {
        wgpuQueueRelease(queue);
        queue = {};
    }

    if (device) {
        wgpuDeviceRelease(device);
        device = {};
    }

    // We release the adapter in the destructor, as otherwise adapterProperties can become invalid.
    if (adapter) {
        wgpuAdapterRelease(adapter);
        adapter = {};
    }
}

static std::map<WGPUFeatureName, std::string> featureNames = {
        { WGPUFeatureName_Undefined, "Undefined" },
        { WGPUFeatureName_DepthClipControl, "DepthClipControl" },
        { WGPUFeatureName_Depth32FloatStencil8, "Depth32FloatStencil8" },
        { WGPUFeatureName_TimestampQuery, "TimestampQuery" },
        { WGPUFeatureName_TextureCompressionBC, "TextureCompressionBC" },
        { WGPUFeatureName_TextureCompressionETC2, "TextureCompressionETC2" },
        { WGPUFeatureName_TextureCompressionASTC, "TextureCompressionASTC" },
        { WGPUFeatureName_IndirectFirstInstance, "IndirectFirstInstance" },
        { WGPUFeatureName_ShaderF16, "ShaderF16" },
        { WGPUFeatureName_RG11B10UfloatRenderable, "RG11B10UfloatRenderable" },
        { WGPUFeatureName_BGRA8UnormStorage, "BGRA8UnormStorage" },
        { WGPUFeatureName_Float32Filterable, "Float32Filterable" },
        { WGPUFeatureName_Force32, "Force32" },
};
#ifdef WEBGPU_BACKEND_WGPU
static std::map<WGPUNativeFeature, std::string> wgpuNativeFeatureNames = {
        { WGPUNativeFeature_PushConstants, "Force32" },
        { WGPUNativeFeature_TextureAdapterSpecificFormatFeatures, "Force32" },
        { WGPUNativeFeature_MultiDrawIndirect, "MultiDrawIndirect" },
        { WGPUNativeFeature_MultiDrawIndirectCount, "MultiDrawIndirectCount" },
        { WGPUNativeFeature_VertexWritableStorage, "VertexWritableStorage" },
        { WGPUNativeFeature_TextureBindingArray, "TextureBindingArray" },
        { WGPUNativeFeature_SampledTextureAndStorageBufferArrayNonUniformIndexing, "SampledTextureAndStorageBufferArrayNonUniformIndexing" },
        { WGPUNativeFeature_PipelineStatisticsQuery, "PipelineStatisticsQuery" },
        { WGPUNativeFeature_StorageResourceBindingArray, "StorageResourceBindingArray" },
        { WGPUNativeFeature_PartiallyBoundBindingArray, "PartiallyBoundBindingArray" },
        { WGPUNativeFeature_Force32, "Force32" },
};
#endif

void Device::queryAdapterCapabilities() {
#if defined(__EMSCRIPTEN__)
    bool isFirefox = EM_ASM_INT({ return navigator.userAgent.toLowerCase().includes('firefox'); });
    if (isFirefox) {
        wgpuAdapterGetProperties(adapter, &adapterProperties);
        adapterInfoValid = false;
        // Firefox seems to not support this yet.
        //wgpuAdapterGetInfo(adapter, &adapterInfo);
        //adapterInfoValid = true;
    } else {
        wgpuAdapterGetInfo(adapter, &adapterInfo);
        adapterInfoValid = true;
    }
#elif defined(WEBGPU_BACKEND_DAWN)
    adapterInfoValid = wgpuAdapterGetInfo(adapter, &adapterInfo) == WGPUStatus_Success;
#else
    wgpuAdapterGetProperties(adapter, &adapterProperties);
#endif

#ifdef WEBGPU_BACKEND_DAWN
    adapterSupportedLimitsValid = wgpuAdapterGetLimits(adapter, &adapterSupportedLimits) == WGPUStatus_Success;
#else
    adapterSupportedLimitsValid = wgpuAdapterGetLimits(adapter, &adapterSupportedLimits);
#endif

    auto numFeatures = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    adapterSupportedFeatures.resize(numFeatures);
    wgpuAdapterEnumerateFeatures(adapter, adapterSupportedFeatures.data());

}

void Device::printAdapterInfo() {
#if defined(__EMSCRIPTEN__) || defined(WEBGPU_BACKEND_DAWN)
    if (adapterInfoValid) {
        sgl::Logfile::get()->writeInfo("Adapter info:");
        if (adapterInfo.vendor) sgl::Logfile::get()->writeInfo("- vendorName: " + std::string(adapterInfo.vendor));
        if (adapterInfo.architecture) sgl::Logfile::get()->writeInfo("- architecture: " + std::string(adapterInfo.architecture));
        if (adapterInfo.device) sgl::Logfile::get()->writeInfo("- device: " + std::string(adapterInfo.device));
        if (adapterInfo.description) sgl::Logfile::get()->writeInfo("- description: " + std::string(adapterInfo.description));
        sgl::Logfile::get()->writeInfo("- backendType: 0x" + sgl::toHexString(adapterInfo.backendType));
        sgl::Logfile::get()->writeInfo("- adapterType: 0x" + sgl::toHexString(adapterInfo.adapterType));
        sgl::Logfile::get()->writeInfo("- vendorID: 0x" + sgl::toHexString(adapterInfo.vendorID));
        sgl::Logfile::get()->writeInfo("- deviceID: 0x" + sgl::toHexString(adapterInfo.deviceID));
        sgl::Logfile::get()->writeInfo("");
    }
#endif
#if defined(__EMSCRIPTEN__)
    if (!adapterInfoValid) {
#endif
#if defined(__EMSCRIPTEN__) || !defined(WEBGPU_BACKEND_DAWN)
    sgl::Logfile::get()->writeInfo("Adapter properties:");
    sgl::Logfile::get()->writeInfo("- vendorID: 0x" + sgl::toHexString(adapterProperties.vendorID));
    if (adapterProperties.vendorName) sgl::Logfile::get()->writeInfo("- vendorName: " + std::string(adapterProperties.vendorName));
    if (adapterProperties.architecture) sgl::Logfile::get()->writeInfo("- architecture: " + std::string(adapterProperties.architecture));
    sgl::Logfile::get()->writeInfo("- deviceID: 0x" + sgl::toHexString(adapterProperties.deviceID));
    if (adapterProperties.name) sgl::Logfile::get()->writeInfo("- name: " + std::string(adapterProperties.name));
    if (adapterProperties.driverDescription) sgl::Logfile::get()->writeInfo("- driverDescription: " + std::string(adapterProperties.driverDescription));
    sgl::Logfile::get()->writeInfo("- adapterType: 0x" + sgl::toHexString(adapterProperties.adapterType));
    sgl::Logfile::get()->writeInfo("- backendType: 0x" + sgl::toHexString(adapterProperties.backendType));
    sgl::Logfile::get()->writeInfo("");
#endif
#if defined(__EMSCRIPTEN__)
    }
#endif

    sgl::Logfile::get()->writeInfo("Adapter features:");
    for (const auto& feature : adapterSupportedFeatures) {
        auto it = featureNames.find(feature);
#ifdef WEBGPU_BACKEND_WGPU
        auto itWgpu = wgpuNativeFeatureNames.find(WGPUNativeFeature(feature));
        if (itWgpu != wgpuNativeFeatureNames.end()) {
            sgl::Logfile::get()->writeInfo("- " + itWgpu->second + " (wgpu)");
        } else
#endif
        if (it != featureNames.end()) {
            sgl::Logfile::get()->writeInfo("- " + it->second);
        } else {
            sgl::Logfile::get()->writeInfo("- 0x" + sgl::toHexString(int(feature)));
        }
    }
    if (!adapterSupportedFeatures.empty()) {
        sgl::Logfile::get()->writeInfo("");
    }

    if (adapterSupportedLimitsValid) {
#define PRINT_LIMIT_ADAPTER(x) sgl::Logfile::get()->writeInfo("- "#x": " + sgl::toString(adapterSupportedLimits.limits.x))
        sgl::Logfile::get()->writeInfo("Adapter limits:");
        PRINT_LIMIT_ADAPTER(maxTextureDimension1D);
        PRINT_LIMIT_ADAPTER(maxTextureDimension2D);
        PRINT_LIMIT_ADAPTER(maxTextureDimension3D);
        PRINT_LIMIT_ADAPTER(maxTextureArrayLayers);
        PRINT_LIMIT_ADAPTER(maxBindGroups);
        PRINT_LIMIT_ADAPTER(maxBindGroupsPlusVertexBuffers);
        PRINT_LIMIT_ADAPTER(maxBindingsPerBindGroup);
        PRINT_LIMIT_ADAPTER(maxDynamicUniformBuffersPerPipelineLayout);
        PRINT_LIMIT_ADAPTER(maxDynamicStorageBuffersPerPipelineLayout);
        PRINT_LIMIT_ADAPTER(maxSampledTexturesPerShaderStage);
        PRINT_LIMIT_ADAPTER(maxSamplersPerShaderStage);
        PRINT_LIMIT_ADAPTER(maxStorageBuffersPerShaderStage);
        PRINT_LIMIT_ADAPTER(maxStorageTexturesPerShaderStage);
        PRINT_LIMIT_ADAPTER(maxUniformBuffersPerShaderStage);
        PRINT_LIMIT_ADAPTER(maxUniformBufferBindingSize);
        PRINT_LIMIT_ADAPTER(maxStorageBufferBindingSize);
        PRINT_LIMIT_ADAPTER(minUniformBufferOffsetAlignment);
        PRINT_LIMIT_ADAPTER(minStorageBufferOffsetAlignment);
        PRINT_LIMIT_ADAPTER(maxVertexBuffers);
        PRINT_LIMIT_ADAPTER(maxBufferSize);
        PRINT_LIMIT_ADAPTER(maxVertexAttributes);
        PRINT_LIMIT_ADAPTER(maxVertexBufferArrayStride);
        PRINT_LIMIT_ADAPTER(maxInterStageShaderComponents);
        PRINT_LIMIT_ADAPTER(maxInterStageShaderVariables);
        PRINT_LIMIT_ADAPTER(maxColorAttachments);
        PRINT_LIMIT_ADAPTER(maxColorAttachmentBytesPerSample);
        PRINT_LIMIT_ADAPTER(maxComputeWorkgroupStorageSize);
        PRINT_LIMIT_ADAPTER(maxComputeInvocationsPerWorkgroup);
        PRINT_LIMIT_ADAPTER(maxComputeWorkgroupSizeX);
        PRINT_LIMIT_ADAPTER(maxComputeWorkgroupSizeY);
        PRINT_LIMIT_ADAPTER(maxComputeWorkgroupSizeZ);
        PRINT_LIMIT_ADAPTER(maxComputeWorkgroupsPerDimension);
        sgl::Logfile::get()->writeInfo("");
    }
}

void Device::queryDeviceCapabilities() {
    auto numDeviceFeatures = wgpuDeviceEnumerateFeatures(device, nullptr);
    deviceFeatures.resize(numDeviceFeatures);
    wgpuDeviceEnumerateFeatures(device, deviceFeatures.data());
    deviceFeaturesSet = std::set<WGPUFeatureName>(deviceFeatures.begin(), deviceFeatures.end());

    WGPUSupportedLimits supportedLimits{};
#ifdef WEBGPU_BACKEND_DAWN
    deviceSupportedLimitsValid = wgpuDeviceGetLimits(device, &supportedLimits) == WGPUStatus_Success;
#else
    deviceSupportedLimitsValid = wgpuDeviceGetLimits(device, &supportedLimits);
#endif
    if (deviceSupportedLimitsValid) {
        deviceLimits = supportedLimits.limits;
    } else {
        deviceLimits = getDefaultWGPULimits();
    }
}

void Device::printDeviceInfo() {
    sgl::Logfile::get()->writeInfo("Device features:");
    for (const auto& feature : deviceFeatures) {
        auto it = featureNames.find(feature);
#ifdef WEBGPU_BACKEND_WGPU
        auto itWgpu = wgpuNativeFeatureNames.find(WGPUNativeFeature(feature));
        if (itWgpu != wgpuNativeFeatureNames.end()) {
            sgl::Logfile::get()->writeInfo("- " + itWgpu->second + " (wgpu)");
        } else
#endif
        if (it != featureNames.end()) {
            sgl::Logfile::get()->writeInfo("- " + it->second);
        } else {
            sgl::Logfile::get()->writeInfo("- 0x" + sgl::toHexString(int(feature)));
        }
    }
    if (!adapterSupportedFeatures.empty()) {
        sgl::Logfile::get()->writeInfo("");
    }

    if (deviceSupportedLimitsValid) {
#define PRINT_LIMIT_DEVICE(x) sgl::Logfile::get()->writeInfo("- "#x": " + sgl::toString(deviceLimits.x))
        sgl::Logfile::get()->writeInfo("Device limits:");
        PRINT_LIMIT_DEVICE(maxTextureDimension1D);
        PRINT_LIMIT_DEVICE(maxTextureDimension2D);
        PRINT_LIMIT_DEVICE(maxTextureDimension3D);
        PRINT_LIMIT_DEVICE(maxTextureArrayLayers);
        PRINT_LIMIT_DEVICE(maxBindGroups);
        PRINT_LIMIT_DEVICE(maxBindGroupsPlusVertexBuffers);
        PRINT_LIMIT_DEVICE(maxBindingsPerBindGroup);
        PRINT_LIMIT_DEVICE(maxDynamicUniformBuffersPerPipelineLayout);
        PRINT_LIMIT_DEVICE(maxDynamicStorageBuffersPerPipelineLayout);
        PRINT_LIMIT_DEVICE(maxSampledTexturesPerShaderStage);
        PRINT_LIMIT_DEVICE(maxSamplersPerShaderStage);
        PRINT_LIMIT_DEVICE(maxStorageBuffersPerShaderStage);
        PRINT_LIMIT_DEVICE(maxStorageTexturesPerShaderStage);
        PRINT_LIMIT_DEVICE(maxUniformBuffersPerShaderStage);
        PRINT_LIMIT_DEVICE(maxUniformBufferBindingSize);
        PRINT_LIMIT_DEVICE(maxStorageBufferBindingSize);
        PRINT_LIMIT_DEVICE(minUniformBufferOffsetAlignment);
        PRINT_LIMIT_DEVICE(minStorageBufferOffsetAlignment);
        PRINT_LIMIT_DEVICE(maxVertexBuffers);
        PRINT_LIMIT_DEVICE(maxBufferSize);
        PRINT_LIMIT_DEVICE(maxVertexAttributes);
        PRINT_LIMIT_DEVICE(maxVertexBufferArrayStride);
        PRINT_LIMIT_DEVICE(maxInterStageShaderComponents);
        PRINT_LIMIT_DEVICE(maxInterStageShaderVariables);
        PRINT_LIMIT_DEVICE(maxColorAttachments);
        PRINT_LIMIT_DEVICE(maxColorAttachmentBytesPerSample);
        PRINT_LIMIT_DEVICE(maxComputeWorkgroupStorageSize);
        PRINT_LIMIT_DEVICE(maxComputeInvocationsPerWorkgroup);
        PRINT_LIMIT_DEVICE(maxComputeWorkgroupSizeX);
        PRINT_LIMIT_DEVICE(maxComputeWorkgroupSizeY);
        PRINT_LIMIT_DEVICE(maxComputeWorkgroupSizeZ);
        PRINT_LIMIT_DEVICE(maxComputeWorkgroupsPerDimension);
        sgl::Logfile::get()->writeInfo("");
    }
}

void Device::pollEvents(bool yieldExecution) {
#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldExecution) {
        emscripten_sleep(100);
    }
#endif
}

void Device::executeCommands(const std::function<void(WGPUCommandEncoder encoder)>& encodeCommandsCallback) {
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    encodeCommandsCallback(encoder);
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command);
}

}}
