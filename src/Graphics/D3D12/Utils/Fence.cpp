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

#include "Device.hpp"
#include "Fence.hpp"

namespace sgl { namespace d3d12 {

Fence::Fence(Device* device, uint64_t value, D3D12_FENCE_FLAGS flags) : device(device) {
    _initialize(device, value, flags);
}

void Fence::_initialize(Device* _device, uint64_t value, D3D12_FENCE_FLAGS flags) {
    device = _device;
    auto* d3d12Device = device->getD3D12Device2Ptr();
    ThrowIfFailed(d3d12Device->CreateFence(0, flags, IID_PPV_ARGS(&fence)));
}

Fence::~Fence() {
    if (fenceEvent) {
        CloseHandle(fenceEvent);
        fenceEvent = {};
    }
}

void Fence::waitOnCpu(uint64_t value) {
    waitOnCpu(value, INFINITE);
}

bool Fence::waitOnCpu(uint64_t value, DWORD timeoutMs) {
    if (fence->GetCompletedValue() < value) {
        if (!fenceEvent) {
            fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
            if (!fenceEvent) {
                sgl::Logfile::get()->throwError("Error in Fence::waitOnCpu: Could not create fence event.");
            }
        }
        ThrowIfFailed(fence->SetEventOnCompletion(value, fenceEvent));
        DWORD ret = ::WaitForSingleObject(fenceEvent, timeoutMs);
        if (ret != WAIT_OBJECT_0) {
            if (ret != WAIT_TIMEOUT) {
                sgl::Logfile::get()->throwError(
                        "Error in Fence::waitOnCpu: WaitForSingleObject failed with error code "
                        + std::to_string(GetLastError()) + ".");
            }
            return false;
        }
    }
    return true;
}

HANDLE Fence::getSharedHandle(const std::wstring& handleName) {
    auto* d3d12Device = device->getD3D12Device2();
    HANDLE resourceHandle{};
    ThrowIfFailed(d3d12Device->CreateSharedHandle(
            fence.Get(), nullptr, GENERIC_ALL, handleName.data(), &resourceHandle));
    return resourceHandle;
}

HANDLE Fence::getSharedHandle() {
    uint64_t resourceIdx = 0;
    std::wstring handleName = std::wstring(L"Local\\D3D12FenceHandle") + std::to_wstring(resourceIdx);
    return getSharedHandle(handleName);
}

}}
