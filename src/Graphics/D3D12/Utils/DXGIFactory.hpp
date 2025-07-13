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

#ifndef SGL_D3D12_DXGIFACTORY_HPP
#define SGL_D3D12_DXGIFACTORY_HPP

#include "d3d12.hpp"

#ifdef SUPPORT_VULKAN
namespace sgl { namespace vk {
class Device;
}}
#endif

namespace sgl { namespace d3d12 {

class Device;
typedef std::shared_ptr<Device> DevicePtr;

class DLL_OBJECT DXGIFactory {
public:
    explicit DXGIFactory(bool useDebugInterface);
    ~DXGIFactory();
    void enumerateDevices();

#ifdef SUPPORT_VULKAN
    sgl::d3d12::DevicePtr createMatchingDevice(sgl::vk::Device* device, D3D_FEATURE_LEVEL minFeatureLevel);
    // Selects the highest supported feature level of the provided set.
    sgl::d3d12::DevicePtr createMatchingDevice(sgl::vk::Device* device, std::vector<D3D_FEATURE_LEVEL> featureLevels);
#endif

private:
    ComPtr<IDXGIFactory4> dxgiFactory;
    ComPtr<ID3D12Debug> debugInterface;
};

typedef std::shared_ptr<DXGIFactory> DXGIFactoryPtr;

}}

#endif //SGL_D3D12_DXGIFACTORY_HPP
