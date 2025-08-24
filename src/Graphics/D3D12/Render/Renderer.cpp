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
#include "../Utils/Fence.hpp"
#include "Renderer.hpp"

#include "CommandList.hpp"
#include "DescriptorAllocator.hpp"
#include "Data.hpp"

namespace sgl { namespace d3d12 {

Renderer::Renderer(Device* device, uint32_t numDescriptors) : device(device) {
    //auto* d3d12Device = device->getD3D12Device2Ptr();
    for (int heapTypeIdx = 0; heapTypeIdx < int(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES); heapTypeIdx++) {
        auto heapType = D3D12_DESCRIPTOR_HEAP_TYPE(heapTypeIdx);
        D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (heapTypeIdx != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && heapTypeIdx != D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {
            flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }
        descriptorHeaps[heapType] = new DescriptorAllocator(device, heapType, flags, numDescriptors);
    }
}

Renderer::~Renderer() {
    for (DescriptorAllocator* desc : descriptorHeaps) {
        delete desc;
    }
}

void Renderer::setCommandList(const CommandListPtr& commandList) {
    currentCommandList = commandList;
}

void Renderer::submit() {
    ID3D12CommandList* d3d12CommandList = currentCommandList->getD3D12CommandListPtr();
    ID3D12CommandQueue* d3d12CommandQueue = device->getD3D12CommandQueue(currentCommandList->getCommandListType());
    currentCommandList->close();
    d3d12CommandQueue->ExecuteCommandLists(1, &d3d12CommandList);
}

void Renderer::submitAndWait() {
    ID3D12CommandList* d3d12CommandList = currentCommandList->getD3D12CommandListPtr();
    ID3D12CommandQueue* d3d12CommandQueue = device->getD3D12CommandQueue(currentCommandList->getCommandListType());
    FencePtr fence = std::make_shared<Fence>(device);
    currentCommandList->close();
    d3d12CommandQueue->ExecuteCommandLists(1, &d3d12CommandList);
    ThrowIfFailed(d3d12CommandQueue->Signal(fence->getD3D12Fence(), 1));
    fence->waitOnCpu(1);
}

void Renderer::dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX) {
    dispatch(computeData, groupCountX, 1, 1);
}

void Renderer::dispatch(
        const ComputeDataPtr& computeData, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    if (currentCommandList->getIsClosed()) {
        currentCommandList->reset();
    }
    auto* d3d12CommandList = currentCommandList->getD3D12GraphicsCommandListPtr();
    computeData->setRootState(d3d12CommandList);
    d3d12CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

}}
