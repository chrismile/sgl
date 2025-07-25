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

#include <Utils/StringUtils.hpp>

#ifdef SUPPORT_VULKAN
#include <algorithm>
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#endif

#include "Device.hpp"
#include "DXGIFactory.hpp"

namespace sgl { namespace d3d12 {

DXGIFactory::DXGIFactory(bool useDebugInterface) {
    UINT createFactoryFlags = 0;
    if (useDebugInterface) {
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    }
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
}

DXGIFactory::~DXGIFactory() {
}

void DXGIFactory::enumerateDevices() {
    sgl::Logfile::get()->writeInfo("Enumerating D3D12 adapters...");
    sgl::Logfile::get()->writeInfo("");
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;
    for (UINT adapterIdx = 0; dxgiFactory->EnumAdapters1(adapterIdx, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++adapterIdx) {
        DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
        dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
        bool isSoftwareRenderer = (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        bool dxgiAdapter4Compatible = SUCCEEDED(dxgiAdapter1.As(&dxgiAdapter4));
        bool feature11_0 = SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr));
        bool feature11_1 = SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_1, __uuidof(ID3D12Device), nullptr));
        bool feature12_0 = SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr));
        bool feature12_1 = SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr));
        bool feature12_2 = SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_2, __uuidof(ID3D12Device), nullptr));
        /*
         * VendorId (https://gamedev.stackexchange.com/questions/31625/get-video-chipset-manufacturer-in-direct3d):
         * NVIDIA: 0x10DE
         * AMD: 0x1002
         * Intel: 0x8086
         */
        sgl::Logfile::get()->writeInfo("D3D12 Adapter #" + sgl::toString(adapterIdx) + ":");
        sgl::Logfile::get()->writeInfo("- Description: " + sgl::wideStringArrayToStdString(dxgiAdapterDesc1.Description));
        sgl::Logfile::get()->writeInfo("- VendorId: 0x" + sgl::toHexString(dxgiAdapterDesc1.VendorId));
        sgl::Logfile::get()->writeInfo("- DeviceId: 0x" + sgl::toHexString(dxgiAdapterDesc1.DeviceId));
        sgl::Logfile::get()->writeInfo("- SubSysId: 0x" + sgl::toHexString(dxgiAdapterDesc1.SubSysId));
        sgl::Logfile::get()->writeInfo("- Revision: 0x" + sgl::toHexString(dxgiAdapterDesc1.Revision));
        sgl::Logfile::get()->writeInfo("- DedicatedVideoMemory: " + std::to_string(dxgiAdapterDesc1.DedicatedVideoMemory));
        sgl::Logfile::get()->writeInfo("- DedicatedSystemMemory: " + std::to_string(dxgiAdapterDesc1.DedicatedSystemMemory));
        sgl::Logfile::get()->writeInfo("- SharedSystemMemory: " + std::to_string(dxgiAdapterDesc1.SharedSystemMemory));
        sgl::Logfile::get()->writeInfo("- AdapterLuid: 0x" + sgl::toHexString(dxgiAdapterDesc1.AdapterLuid.LowPart) + "-" + sgl::toHexString(dxgiAdapterDesc1.AdapterLuid.HighPart));
        sgl::Logfile::get()->writeInfo("- Is software renderer: " + std::to_string(isSoftwareRenderer));
        sgl::Logfile::get()->writeInfo("- IDXGIAdapter4 compatible: " + std::to_string(dxgiAdapter4Compatible));
        sgl::Logfile::get()->writeInfo("- D3D_FEATURE_LEVEL_11_0: " + std::to_string(feature11_0));
        sgl::Logfile::get()->writeInfo("- D3D_FEATURE_LEVEL_11_1: " + std::to_string(feature11_1));
        sgl::Logfile::get()->writeInfo("- D3D_FEATURE_LEVEL_12_0: " + std::to_string(feature12_0));
        sgl::Logfile::get()->writeInfo("- D3D_FEATURE_LEVEL_12_1: " + std::to_string(feature12_1));
        sgl::Logfile::get()->writeInfo("- D3D_FEATURE_LEVEL_12_2: " + std::to_string(feature12_2));
        sgl::Logfile::get()->writeInfo("");
    }
}

#ifdef SUPPORT_VULKAN
sgl::d3d12::DevicePtr DXGIFactory::createMatchingDevice(sgl::vk::Device* device, D3D_FEATURE_LEVEL minFeatureLevel) {
    std::vector<D3D_FEATURE_LEVEL> featureLevels;
    featureLevels.push_back(minFeatureLevel);
    return createMatchingDevice(device, featureLevels);
}

sgl::d3d12::DevicePtr DXGIFactory::createMatchingDevice(
        sgl::vk::Device* device, std::vector<D3D_FEATURE_LEVEL> featureLevels) {
    const VkPhysicalDeviceIDProperties& deviceIdProperties = device->getDeviceIDProperties();
    //const uint64_t vulkanLuid = *reinterpret_cast<const uint64_t*>(deviceIdProperties.deviceLUID);
    uint64_t vulkanLuid = 0;
    for (uint64_t i = 0; i < uint64_t(VK_LUID_SIZE); i++) {
        vulkanLuid = vulkanLuid | (deviceIdProperties.deviceLUID[i] << (i * 8));
    }

    std::sort(featureLevels.begin(), featureLevels.end(), std::greater<>());
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;
    for (UINT adapterIdx = 0; dxgiFactory->EnumAdapters1(adapterIdx, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++adapterIdx) {
        DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
        dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
        uint64_t d3d12Luid =
                (uint64_t(dxgiAdapterDesc1.AdapterLuid.HighPart) << uint64_t(32))
                | uint64_t(dxgiAdapterDesc1.AdapterLuid.LowPart);
        if (vulkanLuid != d3d12Luid) {
            continue;
        }

        bool dxgiAdapter4Compatible = SUCCEEDED(dxgiAdapter1.As(&dxgiAdapter4));
        if (!dxgiAdapter4Compatible) {
            sgl::Logfile::get()->writeInfo(
                    "DXGIFactory::createMatchingDevice: Adapter not IDXGIAdapter4 compatible.");
            return {};
        }

        for (D3D_FEATURE_LEVEL featureLevel : featureLevels) {
            bool featureLevelSupported = SUCCEEDED(D3D12CreateDevice(
                    dxgiAdapter1.Get(), featureLevel, __uuidof(ID3D12Device), nullptr));
            if (!featureLevelSupported) {
                continue;
            }
            return std::make_shared<sgl::d3d12::Device>(
                    dxgiAdapter1, featureLevel, device->getInstance()->getUseValidationLayer());
        }

        sgl::Logfile::get()->writeInfo(
                "DXGIFactory::createMatchingDevice: Minimum feature level not supported.");
        return {};
    }
    sgl::Logfile::get()->writeInfo(
            "DXGIFactory::createMatchingDevice: Couldn't find suitable Direct3D 12 device for passed Vulkan device.");
    return {};
}
#endif

}}
