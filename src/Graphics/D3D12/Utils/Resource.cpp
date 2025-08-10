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

#include "../Render/CommandList.hpp"
#include "Device.hpp"
#include "Resource.hpp"

namespace sgl { namespace d3d12 {

Resource::Resource(Device* device, const ResourceSettings& resourceSettings)
        : device(device), resourceSettings(resourceSettings) {

    auto* d3d12Device = device->getD3D12Device2();
    const D3D12_CLEAR_VALUE* optimizedClearValue = nullptr;
    if (resourceSettings.optimizedClearValue.has_value()) {
        optimizedClearValue = &resourceSettings.optimizedClearValue.value();
    }
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &resourceSettings.heapProperties,
            resourceSettings.heapFlags,
            &resourceSettings.resourceDesc,
            resourceSettings.resourceStates,
            optimizedClearValue,
            IID_PPV_ARGS(&resource)));
}

Resource::~Resource() = default;

void Resource::uploadData(size_t sizeInBytesData, const void* dataPtr) {
    auto* d3d12Device = device->getD3D12Device2();
    CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDescUpload = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytesData);
    ComPtr<ID3D12Resource> intermediateResource{};
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
    // "Upload heaps must start out in the state D3D12_RESOURCE_STATE_GENERIC_READ"
    // "Readback heaps must start out in the D3D12_RESOURCE_STATE_COPY_DEST state"
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesUpload,
            D3D12_HEAP_FLAG_NONE,
            &bufferDescUpload,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));

    device->runSingleTimeCommands([&](CommandList* commandList){
        uploadDataInternal(sizeInBytesData, dataPtr, intermediateResource.Get(), commandList);
    });
}

void Resource::uploadData(
        size_t sizeInBytesData, const void* dataPtr,
        const ResourcePtr& intermediateResource, const CommandListPtr& commandList) {
    uploadDataInternal(sizeInBytesData, dataPtr, intermediateResource->getD3D12Resource(), commandList.get());
}

void Resource::uploadDataInternal(
        size_t sizeInBytesData, const void* dataPtr,
        ID3D12Resource* intermediateResource, CommandList* commandList) {
    auto* d3d12CommandList = commandList->getD3D12GraphicsCommandListPtr();
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = dataPtr;
    if (resourceSettings.resourceDesc.Height <= 1 && resourceSettings.resourceDesc.DepthOrArraySize <= 1) {
        // 1D data (no pitches necessary).
        subresourceData.RowPitch = LONG_PTR(sizeInBytesData);
        subresourceData.SlicePitch = subresourceData.RowPitch;
    } else if (resourceSettings.resourceDesc.DepthOrArraySize <= 1) {
        // 2D data (no slice pitch necessary).
        subresourceData.RowPitch = LONG_PTR(resourceSettings.resourceDesc.Width);
        subresourceData.SlicePitch = LONG_PTR(sizeInBytesData);
    } else {
        // 3D Data.
        subresourceData.RowPitch = LONG_PTR(resourceSettings.resourceDesc.Width);
        subresourceData.SlicePitch = LONG_PTR(resourceSettings.resourceDesc.Width * resourceSettings.resourceDesc.Height);
    }
    UpdateSubresources(d3d12CommandList, getD3D12Resource(), intermediateResource, 0, 0, 1, &subresourceData);
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateAfter, const CommandListPtr& commandList) {
    transition(resourceSettings.resourceStates, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, commandList);
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, const CommandListPtr& commandList) {
    transition(stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, commandList);
}

void Resource::transition(
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, uint32_t subresourcce,
        const CommandListPtr& commandList) {
    ID3D12GraphicsCommandList* d3d12GraphicsCommandList = commandList->getD3D12GraphicsCommandListPtr();
    D3D12_RESOURCE_BARRIER resourceBarrier{};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = resource.Get();
    resourceBarrier.Transition.Subresource = subresourcce;
    resourceBarrier.Transition.StateBefore = stateBefore;
    resourceBarrier.Transition.StateAfter = stateAfter;
    d3d12GraphicsCommandList->ResourceBarrier(1, &resourceBarrier);
    resourceSettings.resourceStates = stateAfter;
}

void Resource::barrierUAV(const CommandListPtr& commandList) {
    ID3D12GraphicsCommandList* d3d12GraphicsCommandList = commandList->getD3D12GraphicsCommandListPtr();
    D3D12_RESOURCE_BARRIER resourceBarrier{};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.UAV.pResource = resource.Get();
    d3d12GraphicsCommandList->ResourceBarrier(1, &resourceBarrier);
}

size_t Resource::getAllocationSizeInBytes() const {
    auto* d3d12Device = device->getD3D12Device2();
    // TODO: https://asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment
#if defined(_MSC_VER) || !defined(_WIN32)
    D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = d3d12Device->GetResourceAllocationInfo(
            0, 1, &resourceSettings.resourceDesc);
#else
    D3D12_RESOURCE_ALLOCATION_INFO allocationInfo{};
    d3d12Device->GetResourceAllocationInfo(
            &allocationInfo, 0, 1, &resourceSettings.resourceDesc);
#endif
    return allocationInfo.SizeInBytes;
}

size_t Resource::getCopiableSizeInBytes() const {
    auto* d3d12Device = device->getD3D12Device2();
    UINT64 sizeInBytes = 0;
    // TODO: Support more subresources?
    d3d12Device->GetCopyableFootprints(
            &resourceSettings.resourceDesc, 0, 1, 0,
            nullptr, nullptr, nullptr, &sizeInBytes);
    return size_t(sizeInBytes);
}

HANDLE Resource::getSharedHandle(const std::wstring& handleName) {
    auto* d3d12Device = device->getD3D12Device2();
    HANDLE fenceHandle{};
    ThrowIfFailed(d3d12Device->CreateSharedHandle(
            resource.Get(), nullptr, GENERIC_ALL, handleName.data(), &fenceHandle));
    return fenceHandle;
}

HANDLE Resource::getSharedHandle() {
    uint64_t resourceIdx = 0;
    std::wstring handleName = std::wstring(L"Local\\D3D12ResourceHandle") + std::to_wstring(resourceIdx);
    return getSharedHandle(handleName);
}

}}
