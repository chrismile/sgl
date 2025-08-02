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

#include "Device.hpp"

namespace sgl { namespace d3d12 {

Device::Device(const ComPtr<IDXGIAdapter1> &dxgiAdapter1, D3D_FEATURE_LEVEL featureLevel, bool useDebugLayer)
        : dxgiAdapter1(dxgiAdapter1), featureLevel(featureLevel) {
    ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), featureLevel, IID_PPV_ARGS(&d3d12Device2)));

    DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
    ThrowIfFailed(dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1));
    vendorId = dxgiAdapterDesc1.VendorId;
    adapterLuid =
            (uint64_t(dxgiAdapterDesc1.AdapterLuid.HighPart) << uint64_t(32))
            | uint64_t(dxgiAdapterDesc1.AdapterLuid.LowPart);
    adapterName = wideStringArrayToStdString(dxgiAdapterDesc1.Description);

    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (useDebugLayer && SUCCEEDED(d3d12Device2.As(&pInfoQueue))) {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        D3D12_MESSAGE_SEVERITY Severities[] = {
                D3D12_MESSAGE_SEVERITY_INFO
        };
        D3D12_MESSAGE_ID DenyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
        };
        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;
        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
    commandQueueDesc.NodeMask = 0;
    ThrowIfFailed(d3d12Device2->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueDirect)));

    ThrowIfFailed(d3d12Device2->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocatorDirect)));

    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (!FAILED(d3d12Device2->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueCompute)))) {
        supportsComputeQueue = true;
        ThrowIfFailed(d3d12Device2->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&commandAllocatorCompute)));
    }
}

D3D_FEATURE_LEVEL Device::getFeatureLevel() const {
    return featureLevel;
}

DeviceVendor Device::getVendor() const {
    if (vendorId == 0x10DE) {
        return DeviceVendor::NVIDIA;
    } else if (vendorId == 0x1002) {
        return DeviceVendor::AMD;
    } else if (vendorId == 0x8086) {
        return DeviceVendor::INTEL;
    } else {
        return DeviceVendor::UNKNOWN;
    }
}

bool Device::getSupportsROVs() const {
    if (featureLevel >= D3D_FEATURE_LEVEL_12_1) {
        return true;
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions;
    ThrowIfFailed(d3d12Device2->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS)));
    return featureDataOptions.ROVsSupported;
}

ID3D12CommandQueue* Device::getD3D12CommandQueue(CommandListType commandListType) {
    if (commandListType == CommandListType::DIRECT) {
        return commandQueueDirect.Get();
    } else if (commandListType == CommandListType::COMPUTE) {
        return commandQueueCompute.Get();
    }
    sgl::Logfile::get()->throwError("Error in Device::getD3D12CommandQueue: Using unsupported command list type.");
    return nullptr;
}

ID3D12CommandAllocator* Device::getD3D12CommandAllocator(CommandListType commandListType) {
    if (commandListType == CommandListType::DIRECT) {
        return commandAllocatorDirect.Get();
    } else if (commandListType == CommandListType::COMPUTE) {
        return commandAllocatorCompute.Get();
    }
    sgl::Logfile::get()->throwError("Error in Device::getD3D12CommandAllocator: Using unsupported command list type.");
    return nullptr;
}

}}
