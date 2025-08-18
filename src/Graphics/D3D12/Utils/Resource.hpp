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

#ifndef SGL_RESOURCE_HPP
#define SGL_RESOURCE_HPP

#include <optional>
#include "../Utils/d3d12.hpp"

namespace sgl { namespace d3d12 {

class Device;

struct ResourceSettings {
    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_DESC resourceDesc{};
    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{};
};

class CommandList;
typedef std::shared_ptr<CommandList> CommandListPtr;
class Resource;
typedef std::shared_ptr<Resource> ResourcePtr;

DLL_OBJECT size_t getDXGIFormatNumChannels(DXGI_FORMAT format);

class DLL_OBJECT Resource {
public:
    explicit Resource(Device* device, const ResourceSettings& resourceSettings);
    ~Resource();

    void uploadData(size_t sizeInBytesData, const void* dataPtr);
    void uploadData(
            size_t sizeInBytesData, const void* dataPtr,
            const ResourcePtr& intermediateResource, const CommandListPtr& commandList);

    void transition(D3D12_RESOURCE_STATES stateNew, const CommandListPtr& commandList);
    void transition(D3D12_RESOURCE_STATES stateOld, D3D12_RESOURCE_STATES stateNew, const CommandListPtr& commandList);
    void transition(D3D12_RESOURCE_STATES stateOld, D3D12_RESOURCE_STATES stateNew, uint32_t subresourcce, const CommandListPtr& commandList);
    void barrierUAV(const CommandListPtr& commandList);

    [[nodiscard]] size_t getAllocationSizeInBytes() const;
    [[nodiscard]] size_t getCopiableSizeInBytes() const;
    [[nodiscard]] size_t getNumRows() const;
    [[nodiscard]] size_t getRowSizeInBytes() const;
    [[nodiscard]] size_t getRowPitchInBytes() const;

    HANDLE getSharedHandle(const std::wstring& handleName);
    /** A not thread-safe version using a static counter for handle name "Local\\D3D12ResourceHandle{ctr}". */
    HANDLE getSharedHandle();

    inline Device* getDevice() { return device; }
    inline ID3D12Resource* getD3D12Resource() { return resource.Get(); }
    [[nodiscard]] inline const ResourceSettings& getResourceSettings() const { return resourceSettings; }
    [[nodiscard]] inline const D3D12_RESOURCE_DESC& getD3D12ResourceDesc() const { return resourceSettings.resourceDesc; }

private:
    Device* device;

    ResourceSettings resourceSettings;
    ComPtr<ID3D12Resource> resource{};

    void uploadDataInternal(
            size_t sizeInBytesData, const void* dataPtr,
            ID3D12Resource* intermediateResource, CommandList* commandList);
};

}}

#endif //SGL_RESOURCE_HPP
