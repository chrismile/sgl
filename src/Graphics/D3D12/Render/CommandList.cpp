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
#include "CommandList.hpp"

namespace sgl { namespace d3d12 {

CommandList::CommandList(Device* device, CommandListType commandListType)
        : device(device), commandListType(commandListType), ownsCommandAllocator(true) {
    auto* d3d12Device = device->getD3D12Device2Ptr();
    auto d3d12CommandListType = getD3D12CommandListType(commandListType);
    ThrowIfFailed(d3d12Device->CreateCommandAllocator(d3d12CommandListType, IID_PPV_ARGS(&commandAllocator)));
    ThrowIfFailed(d3d12Device->CreateCommandList(
            0, d3d12CommandListType, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    if (SUCCEEDED(commandList.As(&graphicsCommandList))) {
        hasGraphicsCommandList = true;
    }
}

CommandList::CommandList(
        Device* device, ComPtr<ID3D12CommandAllocator> commandAllocator, CommandListType commandListType)
        : device(device), commandListType(commandListType), ownsCommandAllocator(false), commandAllocator(commandAllocator) {
    auto* d3d12Device = device->getD3D12Device2Ptr();
    auto d3d12CommandListType = getD3D12CommandListType(commandListType);
    ThrowIfFailed(d3d12Device->CreateCommandList(
            0, d3d12CommandListType, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    if (SUCCEEDED(commandList.As(&graphicsCommandList))) {
        hasGraphicsCommandList = true;
    }
}

void CommandList::close() {
    if (hasGraphicsCommandList) {
        graphicsCommandList->Close();
    } else {
        sgl::Logfile::get()->throwError("Error in CommandList::close: Unsupported command list type.");
    }
}

void CommandList::reset() {
    if (hasGraphicsCommandList) {
        if (ownsCommandAllocator) {
            ThrowIfFailed(commandAllocator->Reset());
        }
        ThrowIfFailed(graphicsCommandList->Reset(commandAllocator.Get(), nullptr));
    } else {
        sgl::Logfile::get()->throwError("Error in CommandList::close: Unsupported command list type.");
    }
}

}}
