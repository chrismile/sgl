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

#include "../Utils/Device.hpp"
#include "Renderer.hpp"

//#include <d3dx12.h>

namespace sgl { namespace d3d12 {

Renderer::Renderer(Device* device, uint32_t numDescriptors) : device(device) {
    auto* d3d12Device = device->getD3D12Device2Ptr();

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.NumDescriptors = numDescriptors;
    for (int heapType = 0; heapType < int(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES); heapType++) {
        descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE(heapType);
        ThrowIfFailed(d3d12Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeaps[heapType])));
    }

    ComPtr<ID3D12CommandQueue> commandQueue;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
    commandQueueDesc.NodeMask = 0;
    ThrowIfFailed(d3d12Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));

    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    HANDLE fenceEvent;
    fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
    if (!fenceEvent) {
        sgl::Logfile::get()->throwError("Could not create fence event.");
    }

    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(d3d12Device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    // STDCALL vs. THISCALL on MSVC and other compilers: https://stackoverflow.com/a/34322863
    // More info: https://github.com/dotnet/coreclr/pull/23974
#if defined(_MSC_VER) || !defined(_WIN32)
    auto baseRtv = descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
#else
    D3D12_CPU_DESCRIPTOR_HANDLE baseRtv;
    descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(&baseRtv);
#endif
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(baseRtv);
    //D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    //UINT rtvDescIncrementSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    //rtv.ptr = baseRtv.ptr + 0 * rtvDescIncrementSize;

    commandList->Close();
    ID3D12CommandList* const commandLists[] = {
            commandList.Get()
    };
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    uint64_t newFenceValue = 1;
    commandQueue->Signal(fence.Get(), newFenceValue);

    if (fence->GetCompletedValue() < 1) {
        ThrowIfFailed(fence->SetEventOnCompletion(newFenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, INFINITE);
    }


    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);


    ComPtr<ID3D12Resource> destinationResource{};
    ComPtr<ID3D12Resource> intermediateResource{};

    size_t bufferSize = 1024;
    auto* bufferData = new float[bufferSize];

    /*D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC bufferResourceDesc{};
    bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferResourceDesc.Width = bufferSize;
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferResourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&destinationResource)));
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));*/
    D3D12_RESOURCE_FLAGS flags{};
    CD3DX12_HEAP_PROPERTIES heapPropertiesDefault(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesDefault,
            D3D12_HEAP_FLAG_NONE,
            &bufferResourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&destinationResource)));
    CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDescUpload = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesUpload,
            D3D12_HEAP_FLAG_NONE,
            &bufferDescUpload,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));

    ComPtr<ID3D12Resource> depthImageResource{};
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };
    auto depthTexResource = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, 1024, 1024,
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
            &depthTexResource,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&depthImageResource)
    ));

    ComPtr<ID3D12Resource> colorImageResource{};
    float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
    optimizedClearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    memcpy(optimizedClearValue.Color, clearColor, sizeof(float) * 4);
    auto texResource = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32G32B32A32_FLOAT, 1024, 1024,
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
            &texResource,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&colorImageResource)
    ));

    /*D3D12_RESOURCE_BARRIER resourceBarrier{};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = nullptr;
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;*/
    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            colorImageResource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &resourceBarrier);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    // TODO: https://github.com/microsoft/DirectX-Headers/blob/main/include/directx/d3dx12_resource_helpers.h
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = bufferData;
    subresourceData.RowPitch = LONG_PTR(bufferSize);
    subresourceData.SlicePitch = subresourceData.RowPitch;
    UpdateSubresources(
            commandList.Get(),
            destinationResource.Get(), intermediateResource.Get(),
            0, 0, 1, &subresourceData);
    delete[] bufferData;

    uint64_t resourceIdx = 0;
    std::wstring sharedHandleNameString = std::wstring(L"Local\\D3D12ResourceHandle") + std::to_wstring(resourceIdx);
    HANDLE resourceHandle{};
    ThrowIfFailed(d3d12Device->CreateSharedHandle(
            destinationResource.Get(), nullptr, GENERIC_ALL, sharedHandleNameString.data(), &resourceHandle));
}

Renderer::~Renderer() {
}

}}
