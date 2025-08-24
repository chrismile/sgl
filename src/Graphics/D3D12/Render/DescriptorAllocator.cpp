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

#include "../Utils/Device.hpp"
#include "DescriptorAllocator.hpp"

#include "../libs/D3D12MemoryAllocator/D3D12MemAlloc.h"

namespace sgl { namespace d3d12 {

DescriptorAllocation::DescriptorAllocation(
        DescriptorAllocator* descriptorAllocator, UINT64 allocationHandle, UINT64 allocationOffset,
        size_t numDescriptors)
        : descriptorAllocator(descriptorAllocator), allocationHandle(allocationHandle),
          allocationOffset(allocationOffset), numDescriptors(numDescriptors) {
}

DescriptorAllocation::~DescriptorAllocation() {
    static_assert(sizeof(D3D12MA::VirtualAllocation) == sizeof(allocationHandle), "D3D12MA virtual allocation size mismatch.");
    D3D12MA::VirtualAllocation alloc{ allocationHandle };
    descriptorAllocator->block->FreeAllocation(alloc);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::getCPUDescriptorHandle(uint32_t offset) {
    if (size_t(offset) >= numDescriptors) {
        sgl::Logfile::get()->throwErrorVar(
                "Error in DescriptorAllocation::getCPUDescriptorHandle: offset ", offset,
                " too large for number of descriptors ", numDescriptors, ".");
    }
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                descriptorAllocator->descriptorHandleHeapStartCpu, int(allocationOffset) + int(offset),
                descriptorAllocator->descriptorHandleIncrementSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocation::getGPUDescriptorHandle(uint32_t offset) {
    if (size_t(offset) >= numDescriptors) {
        sgl::Logfile::get()->throwErrorVar(
                "Error in DescriptorAllocation::getGPUDescriptorHandle: offset ", offset,
                " too large for number of descriptors ", numDescriptors, ".");
    }
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
                descriptorAllocator->descriptorHandleHeapStartGpu, int(allocationOffset) + int(offset),
                descriptorAllocator->descriptorHandleIncrementSize);
}


DescriptorAllocator::DescriptorAllocator(
        Device* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags,
        uint32_t numDescriptors)
        : descriptorHeapType(descriptorHeapType) {
    auto* d3d12Device = device->getD3D12Device2Ptr();
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.NumDescriptors = numDescriptors;
    descriptorHeapDesc.Type = descriptorHeapType;
    descriptorHeapDesc.Flags = flags;
    ThrowIfFailed(d3d12Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

#if defined(_MSC_VER) || !defined(_WIN32)
    descriptorHandleHeapStartCpu = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    if ((flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0) {
        descriptorHandleHeapStartGpu = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    }
#else
    descriptorHeap->GetCPUDescriptorHandleForHeapStart(&descriptorHandleHeapStartCpu);
    descriptorHeap->GetGPUDescriptorHandleForHeapStart(&descriptorHandleHeapStartGpu);
#endif
    descriptorHandleIncrementSize = d3d12Device->GetDescriptorHandleIncrementSize(descriptorHeapType);

    D3D12MA::VIRTUAL_BLOCK_DESC blockDesc = {};
    blockDesc.Size = numDescriptors;
    ThrowIfFailed(CreateVirtualBlock(&blockDesc, &block));
}

DescriptorAllocator::~DescriptorAllocator() {
    block->Release();
}

DescriptorAllocationPtr DescriptorAllocator::allocate(size_t numDescriptors) {
    D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc{};
    allocDesc.Size = numDescriptors;
    allocDesc.Alignment = 1;
    D3D12MA::VirtualAllocation alloc{};
    UINT64 allocOffset;
    HRESULT hr = block->Allocate(&allocDesc, &alloc, &allocOffset);
    if (FAILED(hr)) {
        sgl::Logfile::get()->throwError("Error in DescriptorAllocator::allocate: Allocation failed.");
        return {};
    }
    auto descriptorAllocation = std::make_shared<DescriptorAllocation>(
            this, alloc.AllocHandle, allocOffset, numDescriptors);
    return descriptorAllocation;
}

}}
