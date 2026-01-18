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

#ifndef COMMANDLIST_HPP
#define COMMANDLIST_HPP

#include "../Utils/d3d12.hpp"
#include "../Utils/CommandListType.hpp"

namespace sgl { namespace d3d12 {

class Device;

class DLL_OBJECT CommandList {
public:
    explicit CommandList(Device* device, CommandListType commandListType = CommandListType::DIRECT);
    CommandList(
            Device* device, ComPtr<ID3D12CommandAllocator> commandAllocator,
            CommandListType commandListType = CommandListType::DIRECT);

    [[nodiscard]] inline CommandListType getCommandListType() const { return commandListType; }
    inline ComPtr<ID3D12CommandList>& getD3D12CommandList() { return commandList; }
    inline ID3D12CommandList* getD3D12CommandListPtr() { return commandList.Get(); }
    inline ComPtr<ID3D12GraphicsCommandList>& getD3D12GraphicsCommandList() { return graphicsCommandList; }
    inline ID3D12GraphicsCommandList* getD3D12GraphicsCommandListPtr() {
        return hasGraphicsCommandList ? graphicsCommandList.Get() : nullptr;
    }
    template<class T>
    inline ComPtr<T> getD3D12CommandListAs() { ComPtr<T> cmdList; commandList.As(&cmdList); return cmdList; }
    template<class T>
    inline T* getD3D12CommandListPtrAs() { ComPtr<T> cmdList; commandList.As(&cmdList); return cmdList.Get(); }
    [[nodiscard]] Device* getDevice() const { return device; }

    inline ComPtr<ID3D12CommandAllocator>& getD3D12CommandAllocator() { return commandAllocator; }
    inline ID3D12CommandAllocator* getD3D12CommandAllocatorPtr() { return commandAllocator.Get(); }
    [[nodiscard]] inline bool getIsClosed() const { return isClosed; }

    void close();
    /**
     * Resets the command list. If the command allocator is owned by this command list, it is also reset.
     */
    void reset();

private:
    Device* device;
    CommandListType commandListType;
    bool hasGraphicsCommandList = false;
    bool ownsCommandAllocator = true;
    bool isClosed = false;
    ComPtr<ID3D12CommandList> commandList;
    ComPtr<ID3D12GraphicsCommandList> graphicsCommandList;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
};

typedef std::shared_ptr<CommandList> CommandListPtr;

}}

#endif //COMMANDLIST_HPP
