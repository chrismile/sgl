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

#ifndef SGL_D3D12_DESCRIPTORALLOCATOR_HPP
#define SGL_D3D12_DESCRIPTORALLOCATOR_HPP

#include "../Utils/d3d12.hpp"

namespace D3D12MA {
class VirtualBlock;
}

namespace sgl { namespace d3d12 {

class Device;
class DescriptorAllocator;

class DLL_OBJECT DescriptorAllocation {
public:
    DescriptorAllocation(
            DescriptorAllocator* descriptorAllocator, UINT64 allocationHandle, UINT64 allocationOffset,
            size_t numDescriptors);
    ~DescriptorAllocation();
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandle(uint32_t offset = 0);
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorHandle(uint32_t offset = 0);
    [[nodiscard]] inline size_t getNumDescriptors() const { return numDescriptors; }

private:
    DescriptorAllocator* descriptorAllocator;
    UINT64 allocationHandle = 0;
    UINT64 allocationOffset = 0;
    size_t numDescriptors = 0;
};
typedef std::shared_ptr<DescriptorAllocation> DescriptorAllocationPtr;


class DLL_OBJECT DescriptorAllocator {
    friend class DescriptorAllocation;
public:
    DescriptorAllocator(
            Device* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            uint32_t numDescriptors);
    ~DescriptorAllocator();
    DescriptorAllocationPtr allocate(size_t numDescriptors = 1);

    [[nodiscard]] ComPtr<ID3D12DescriptorHeap>& getD3D12DescriptorHeap() { return descriptorHeap; }
    [[nodiscard]] ID3D12DescriptorHeap* getD3D12DescriptorHeapPtr() { return descriptorHeap.Get(); }
    [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE getDescriptorHeapType() { return descriptorHeapType; }

private:
    D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType;
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    D3D12MA::VirtualBlock* block = nullptr;
    UINT descriptorHandleIncrementSize = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleHeapStartCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandleHeapStartGpu = {};
};

}}

#endif //SGL_D3D12_DESCRIPTORALLOCATOR_HPP
