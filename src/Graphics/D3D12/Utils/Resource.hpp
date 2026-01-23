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

struct ClearValue {
    DXGI_FORMAT format;
    union {
        glm::vec4 color;
        D3D12_DEPTH_STENCIL_VALUE depthStencilValue;
    };
};

struct ResourceSettings {
    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_DESC resourceDesc{};
    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    std::optional<ClearValue> optimizedClearValue{};
};

class CommandList;
typedef std::shared_ptr<CommandList> CommandListPtr;
class Resource;
typedef std::shared_ptr<Resource> ResourcePtr;

DLL_OBJECT size_t getDXGIFormatNumChannels(DXGI_FORMAT format);
DLL_OBJECT size_t getDXGIFormatSizeInBytes(DXGI_FORMAT format);
DLL_OBJECT std::string getDXGIFormatHLSLStructuredTypeString(DXGI_FORMAT format);
DLL_OBJECT std::string convertDXGIFormatToString(DXGI_FORMAT format);

class DLL_OBJECT Resource {
public:
    explicit Resource(Device* device, const ResourceSettings& resourceSettings);
    ~Resource();

    /*
     * The functions below upload/read back data for subresource 0.
     */
    void uploadDataLinear(size_t sizeInBytesData, const void* dataPtr);
    void uploadDataLinear(
            size_t sizeInBytesData, const void* dataPtr,
            const ResourcePtr& intermediateResource, const CommandListPtr& commandList);
    void readBackDataLinear(size_t sizeInBytesData, void* dataPtr);

    void transition(D3D12_RESOURCE_STATES stateNew, const CommandListPtr& commandList);
    void transition(D3D12_RESOURCE_STATES stateNew, CommandList* commandList);
    void transition(D3D12_RESOURCE_STATES stateOld, D3D12_RESOURCE_STATES stateNew, const CommandListPtr& commandList);
    void transition(D3D12_RESOURCE_STATES stateOld, D3D12_RESOURCE_STATES stateNew, CommandList* commandList);
    void transition(D3D12_RESOURCE_STATES stateOld, D3D12_RESOURCE_STATES stateNew, uint32_t subresourcce, const CommandListPtr& commandList);
    void transition(D3D12_RESOURCE_STATES stateOld, D3D12_RESOURCE_STATES stateNew, uint32_t subresourcce, CommandList* commandList);
    void barrierUAV(const CommandListPtr& commandList);
    void barrierUAV(CommandList* commandList);

    void* map();
    void* map(size_t readRangeBegin, size_t readRangeEnd);
    void unmap();
    void unmap(size_t writtenRangeBegin, size_t writtenRangeEnd);

    [[nodiscard]] size_t getAllocationSizeInBytes();
    [[nodiscard]] size_t getCopiableSizeInBytes();
    [[nodiscard]] size_t getNumRows();
    [[nodiscard]] size_t getRowSizeInBytes();
    [[nodiscard]] size_t getRowPitchInBytes();

    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress();

    HANDLE getSharedHandle(const std::wstring& handleName);
    /** A not thread-safe version using a static counter for handle name "Local\\D3D12ResourceHandle{ctr}". */
    HANDLE getSharedHandle();

    inline Device* getDevice() { return device; }
    inline ID3D12Resource* getD3D12ResourcePtr() { return resource.Get(); }
    [[nodiscard]] inline const ResourceSettings& getResourceSettings() const { return resourceSettings; }
    [[nodiscard]] inline const D3D12_RESOURCE_DESC& getD3D12ResourceDesc() const { return resourceSettings.resourceDesc; }

private:
    Device* device;

    ResourceSettings resourceSettings;
    uint32_t numSubresources = 0;
    ComPtr<ID3D12Resource> resource{};

    void queryCopiableFootprints();
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> subresourceLayoutArray;
    std::vector<UINT> subresourceNumRowsArray;
    std::vector<UINT64> subresourceRowSizeInBytesArray;
    std::vector<UINT64> subresourceTotalBytesArray;

    void uploadDataLinearInternal(
            size_t sizeInBytesData, const void* dataPtr,
            ID3D12Resource* intermediateResource, CommandList* commandList);
};

}}

#endif //SGL_RESOURCE_HPP
