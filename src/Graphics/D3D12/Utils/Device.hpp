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

#ifndef SGL_D3D12_DEVICE_HPP
#define SGL_D3D12_DEVICE_HPP

#include "d3d12.hpp"
#include "CommandListType.hpp"

namespace sgl { namespace d3d12 {

enum class DeviceVendor {
    NVIDIA, AMD, INTEL, UNKNOWN
};

class DLL_OBJECT Device {
public:
    Device(const ComPtr<IDXGIAdapter1> &dxgiAdapter1, D3D_FEATURE_LEVEL featureLevel, bool useDebugLayer);
    [[nodiscard]] D3D_FEATURE_LEVEL getFeatureLevel() const;
    [[nodiscard]] inline const std::string& getAdapterName() const { return adapterName; }
    [[nodiscard]] inline uint64_t getAdapterLuid() const { return adapterLuid; }
    [[nodiscard]] DeviceVendor getVendor() const;
    [[nodiscard]] bool getSupportsROVs() const;

    [[nodiscard]] inline ID3D12Device2* getD3D12Device2() { return d3d12Device2.Get(); }
    [[nodiscard]] inline ID3D12Device2* getD3D12Device2Ptr() { return d3d12Device2.Get(); }
    [[nodiscard]] inline ComPtr<ID3D12Device2>& getD3D12Device2ComPtr() { return d3d12Device2; }
    [[nodiscard]] inline ID3D12CommandQueue* getD3D12CommandQueueDirect() { return commandQueueDirect.Get(); }
    [[nodiscard]] inline ID3D12CommandQueue* getD3D12CommandQueueCompute() { return commandQueueCompute.Get(); }
    [[nodiscard]] ID3D12CommandQueue* getD3D12CommandQueue(CommandListType commandListType);
    [[nodiscard]] inline ID3D12CommandAllocator* getD3D12CommandAllocatorDirect() { return commandAllocatorDirect.Get(); }
    [[nodiscard]] inline ID3D12CommandAllocator* getD3D12CommandAllocatorCompute() { return commandAllocatorCompute.Get(); }
    [[nodiscard]] ID3D12CommandAllocator* getD3D12CommandAllocator(CommandListType commandListType);

private:
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;
    D3D_FEATURE_LEVEL featureLevel;
    ComPtr<ID3D12Device2> d3d12Device2;
    ComPtr<ID3D12CommandQueue> commandQueueDirect;
    ComPtr<ID3D12CommandQueue> commandQueueCompute;
    ComPtr<ID3D12CommandAllocator> commandAllocatorDirect;
    ComPtr<ID3D12CommandAllocator> commandAllocatorCompute;
    bool supportsComputeQueue = false;

    // Device information (retrieved from adapter).
    std::string adapterName;
    uint32_t vendorId;
    uint32_t adapterLuid;
};

}}

#endif //SGL_D3D12_DEVICE_HPP
