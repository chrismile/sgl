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

#ifndef SGL_D3D12_RENDERER_HPP
#define SGL_D3D12_RENDERER_HPP

#include <array>
#include "../Utils/d3d12.hpp"

namespace sgl { namespace d3d12 {

class Device;
class CommandList;
typedef std::shared_ptr<CommandList> CommandListPtr;
class DescriptorAllocator;
class ComputeData;
typedef std::shared_ptr<ComputeData> ComputeDataPtr;
class RasterData;
typedef std::shared_ptr<RasterData> RasterDataPtr;

class DLL_OBJECT Renderer {
public:
    explicit Renderer(Device* device, uint32_t numDescriptors = 1024);
    ~Renderer();

    inline Device* getDevice() { return device; }
    inline DescriptorAllocator* getDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE heapType) { return descriptorHeaps[int(heapType)]; }
    void setCommandList(const CommandListPtr& commandList);
    void submit();
    void submitAndWait();

    void dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX);
    void dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void render(const RasterDataPtr& rasterData);

private:
    Device* device;

    // Global descriptor heaps.
    std::array<DescriptorAllocator*, size_t(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)> descriptorHeaps;

    CommandListPtr currentCommandList;
};

}}

#endif //SGL_D3D12_RENDERER_HPP
