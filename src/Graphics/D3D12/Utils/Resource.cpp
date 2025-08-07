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

void Resource::uploadData(size_t sizeInBytesData, const void* dataPtr) {
    auto* d3d12Device = device->getD3D12Device2();
    CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDescUpload = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytesData);
    ComPtr<ID3D12Resource> intermediateResource{};
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPropertiesUpload,
            D3D12_HEAP_FLAG_NONE,
            &bufferDescUpload,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));

    device->runSingleTimeCommands([&](CommandList* commandList){
        auto* d3d12CommandList = commandList->getD3D12GraphicsCommandList();
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = dataPtr;
        subresourceData.RowPitch = LONG_PTR(sizeInBytesData);
        subresourceData.SlicePitch = subresourceData.RowPitch;
        UpdateSubresources(d3d12CommandList, getD3D12Resource(), intermediateResource.Get(), 0, 0, 1, &subresourceData);
    });
}

HANDLE Resource::getSharedHandle(const std::wstring& handleName) {
    auto* d3d12Device = device->getD3D12Device2();
    HANDLE resourceHandle{};
    ThrowIfFailed(d3d12Device->CreateSharedHandle(
            this, nullptr, GENERIC_ALL, handleName.data(), &resourceHandle));
    return resourceHandle;
}

HANDLE Resource::getSharedHandle() {
    uint64_t resourceIdx = 0;
    std::wstring handleName = std::wstring(L"Local\\D3D12ResourceHandle") + std::to_wstring(resourceIdx);
    return getSharedHandle(handleName);
}

}}
