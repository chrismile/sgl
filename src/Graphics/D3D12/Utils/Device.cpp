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

#include "../Render/CommandList.hpp"
#include "Fence.hpp"
#include "Device.hpp"

namespace sgl { namespace d3d12 {

void debugMessageCallbackD3D12(
        D3D12_MESSAGE_CATEGORY Category,
        D3D12_MESSAGE_SEVERITY Severity,
        D3D12_MESSAGE_ID ID,
        LPCSTR pDescription,
        void* pContext) {
    auto* device = static_cast<Device*>(pContext);
    device->debugMessageCallback(Category, Severity, ID, pDescription);
}

Device::Device(const ComPtr<IDXGIAdapter1> &dxgiAdapter1, D3D_FEATURE_LEVEL featureLevel, bool useDebugLayer)
        : dxgiAdapter1(dxgiAdapter1), featureLevel(featureLevel), useDebugLayer(useDebugLayer) {
    commandListsSingleTime.resize(int(CommandListType::MAX_VAL), {});
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

        ComPtr<ID3D12InfoQueue1> pInfoQueue1;
        if (SUCCEEDED(pInfoQueue.As(&pInfoQueue1))) {
            ThrowIfFailed(pInfoQueue1->RegisterMessageCallback(
                    debugMessageCallbackD3D12, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &callbackCookie));
        }
    }

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
    commandQueueDesc.NodeMask = 0;
    ThrowIfFailed(d3d12Device2->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueDirect)));

    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    if (!FAILED(d3d12Device2->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueCompute)))) {
        supportsComputeQueue = true;
    }
}

Device::~Device() {
    commandListsSingleTime.clear();

    ComPtr<ID3D12InfoQueue1> pInfoQueue1;
    if (useDebugLayer && SUCCEEDED(d3d12Device2.As(&pInfoQueue1))) {
        ThrowIfFailed(pInfoQueue1->UnregisterMessageCallback(callbackCookie));
    }
}

void Device::debugMessageCallback(
        D3D12_MESSAGE_CATEGORY Category,
        D3D12_MESSAGE_SEVERITY Severity,
        D3D12_MESSAGE_ID ID,
        LPCSTR pDescription) {
    sgl::Logfile::get()->writeError(std::string() + "Debug message: " + pDescription);
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

bool Device::getFormatSupportsTypedLoadStore(DXGI_FORMAT format, bool typedLoad, bool typedStore) const {
    if (format == DXGI_FORMAT_R32_FLOAT || format == DXGI_FORMAT_R32_UINT || format == DXGI_FORMAT_R32_SINT) {
        return true;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions{};
    if (!SUCCEEDED(d3d12Device2->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(featureDataOptions)))) {
        return false;
    }
    if (!featureDataOptions.TypedUAVLoadAdditionalFormats) {
        return false;
    }

    if (format == DXGI_FORMAT_R32G32B32A32_FLOAT
            || format == DXGI_FORMAT_R32G32B32A32_UINT
            || format == DXGI_FORMAT_R32G32B32A32_SINT
            || format == DXGI_FORMAT_R16G16B16A16_FLOAT
            || format == DXGI_FORMAT_R16G16B16A16_UINT
            || format == DXGI_FORMAT_R16G16B16A16_SINT
            || format == DXGI_FORMAT_R8G8B8A8_UNORM
            || format == DXGI_FORMAT_R8G8B8A8_UINT
            || format == DXGI_FORMAT_R8G8B8A8_SINT
            || format == DXGI_FORMAT_R16_FLOAT
            || format == DXGI_FORMAT_R16_UINT
            || format == DXGI_FORMAT_R16_SINT
            || format == DXGI_FORMAT_R8_UNORM
            || format == DXGI_FORMAT_R8_UINT
            || format == DXGI_FORMAT_R8_SINT) {
        return true;
    }

    D3D12_FEATURE_DATA_FORMAT_SUPPORT featureDataFormatSupport = {
            format, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
    };
    if (!SUCCEEDED(d3d12Device2->CheckFeatureSupport(
            D3D12_FEATURE_FORMAT_SUPPORT, &featureDataFormatSupport, sizeof(featureDataFormatSupport)))) {
        return false;
    }

    DWORD mask = 0;
    if (typedLoad) {
        mask |= D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD;
    }
    if (typedStore) {
        mask |= D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
    }
    return (featureDataFormatSupport.Support2 & mask) == mask;
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

void Device::runSingleTimeCommands(
        const std::function<void(CommandList*)>& workFunctor, CommandListType commandListType) {
    CommandListPtr commandList = commandListsSingleTime[int(commandListType)];
    if (!commandList) {
        commandList = std::make_shared<CommandList>(this, commandListType);
        commandListsSingleTime[int(commandListType)] = commandList;
    } else {
        commandList->reset();
    }
    ID3D12CommandList* d3d12CommandList = commandList->getD3D12CommandListPtr();
    ID3D12CommandQueue* d3d12CommandQueue = getD3D12CommandQueue(commandListType);
    workFunctor(commandList.get());

    FencePtr fence = std::make_shared<Fence>(this);
    commandList->close();
    d3d12CommandQueue->ExecuteCommandLists(1, &d3d12CommandList);
    ThrowIfFailed(d3d12CommandQueue->Signal(fence->getD3D12Fence(), 1));
    fence->waitOnCpu(1);
}

}}
