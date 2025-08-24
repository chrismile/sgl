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

#ifndef SGL_D3D12_DATA_HPP
#define SGL_D3D12_DATA_HPP

#include <cstring>
#include "../Utils/d3d12.hpp"

namespace sgl { namespace d3d12 {

class Resource;
class DescriptorAllocation;
class ShaderModule;
typedef std::shared_ptr<ShaderModule> ShaderModulePtr;
class Renderer;

enum class RootParameterType {
        UNDEFINED = 0, CONSTANTS_PTR, CONSTANTS_VALUE, CONSTANTS_COPY, CBV, SRV, UAV, DESCRIPTOR_TABLE
};

class DLL_OBJECT RootParameters {
public:
    RootParameters();
    // Passing the shader module allows for doing shader reflection.
    explicit RootParameters(ShaderModulePtr shaderModule);

    UINT pushConstants(
            UINT num32BitValues, UINT shaderRegister, UINT registerSpace = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushConstants(
            const std::string& bindingName,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushConstantBufferView(
            UINT shaderRegister, UINT registerSpace = 0,
            D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushConstantBufferView(
            const std::string& bindingName,
            D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushShaderResourceView(
            UINT shaderRegister, UINT registerSpace = 0,
            D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushShaderResourceView(
            const std::string& bindingName,
            D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushUnorderedAccessView(
            UINT shaderRegister, UINT registerSpace = 0,
            D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushUnorderedAccessView(
            const std::string& bindingName,
            D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    UINT pushDescriptorTable(
            UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE1* descriptorRanges,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    void pushStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc);

    void build(Device* device);

    [[nodiscard]] const ShaderModulePtr& getShaderModule() const { return shaderModule; }
    [[nodiscard]] const std::vector<CD3DX12_ROOT_PARAMETER1>& getRootParameters() const { return rootParameters; }
    [[nodiscard]] const std::vector<D3D12_STATIC_SAMPLER_DESC>& getStaticSamplers() const { return staticSamplers; }
    ID3D12PipelineState* getD3D12PipelineStatePtr() { return pipelineState.Get(); }
    ID3D12RootSignature* getD3D12RootSignaturePtr() { return rootSignature.Get(); }

protected:
    ShaderModulePtr shaderModule;
    void checkPush();
    void checkShaderModule();
    std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12RootSignature> rootSignature;
};

typedef std::shared_ptr<RootParameters> RootParametersPtr;

struct DLL_OBJECT RootParameterValue {
    RootParameterType type;
    UINT rpIdx; ///< Root parameter index.
    uint32_t num32BitValues;
    uint32_t offsetIn32BitValues;
    union {
        struct { // CONSTANTS_PTR, CONSTANTS_COPY
            const void* dataPointer;
        };
        struct { // CONSTANTS_VALUE
            uint32_t value;
        };
        Resource* resource; // CBV, SRV, UAV
        DescriptorAllocation* descriptorAllocation; // DESCRIPTOR_TABLE
    };
};

class DLL_OBJECT ComputeData {
public:
    ComputeData(Renderer* renderer, const RootParametersPtr& rootParameters);
    ComputeData(Renderer* renderer, const ShaderModulePtr& shaderModule, const RootParametersPtr& rootParameters);
    ~ComputeData();

    // Copies a single 32-bit entry.
    void setRootConstantValue(UINT rpIdx, uint32_t value, UINT offsetIn32BitValues = 0);
    void setRootConstantValue(UINT rpIdx, const std::string& varName, uint32_t value);
    template<class T>
    void setRootConstantValue(UINT rpIdx, T value, UINT offsetIn32BitValues = 0) {
        static_assert(sizeof(T) == sizeof(uint32_t));
        uint32_t valueUint32;
        memcpy(&valueUint32, &value, sizeof(uint32_t));
        setRootConstantValue(rpIdx, valueUint32, offsetIn32BitValues);
    }
    template<class T>
    void setRootConstantValue(UINT rpIdx, const std::string& varName, T value) {
        static_assert(sizeof(T) == sizeof(uint32_t));
        uint32_t valueUint32;
        memcpy(&valueUint32, &value, sizeof(uint32_t));
        setRootConstantValue(rpIdx, varName, valueUint32);
    }

    // The passed pointer must stay in scope until at least @see setRootState is called.
    void setRootConstants(UINT rpIdx, const uint32_t* values, UINT num32BitValues, UINT offsetIn32BitValues = 0);
    void setRootConstants(UINT rpIdx, const std::string& varName, const uint32_t* values, UINT num32BitValues);
    template<class T>
    void setRootConstants(UINT rpIdx, const T* value, UINT offsetIn32BitValues = 0) {
        static_assert(sizeof(T) >= sizeof(uint32_t));
        setRootConstants(rpIdx, reinterpret_cast<const uint32_t*>(&value), sizeof(T) / sizeof(uint32_t), offsetIn32BitValues);
    }
    template<class T>
    void setRootConstants(UINT rpIdx, const std::string& varName, const T* value) {
        static_assert(sizeof(T) >= sizeof(uint32_t));
        setRootConstants(rpIdx, reinterpret_cast<const uint32_t*>(&value), sizeof(T) / sizeof(uint32_t));
    }

    // Creates a copy of the data on the heap.
    void setRootConstantsCopy(UINT rpIdx, const uint32_t* values, UINT num32BitValues, UINT offsetIn32BitValues = 0);
    void setRootConstantsCopy(UINT rpIdx, const std::string& varName, const uint32_t* values, UINT num32BitValues);
    template<class T>
    void setRootConstantsCopy(UINT rpIdx, const T& value, UINT offsetIn32BitValues = 0) {
        static_assert(sizeof(T) >= sizeof(uint32_t));
        setRootConstantsCopy(rpIdx, reinterpret_cast<const uint32_t*>(&value), sizeof(T) / sizeof(uint32_t), offsetIn32BitValues);
    }
    template<class T>
    void setRootConstantsCopy(UINT rpIdx, const std::string& varName, const T& value) {
        static_assert(sizeof(T) >= sizeof(uint32_t));
        setRootConstantsCopy(rpIdx, reinterpret_cast<const uint32_t*>(&value), sizeof(T) / sizeof(uint32_t));
    }

    void setConstantBufferView(UINT rpIdx, Resource* resource);
    void setShaderResourceView(UINT rpIdx, Resource* resource);
    void setUnorderedAccessView(UINT rpIdx, Resource* resource);
    void setDescriptorTable(UINT rpIdx, DescriptorAllocation* descriptorAllocation);

    void setRootState(ID3D12GraphicsCommandList* d3d12CommandList);

private:
    Renderer* renderer;
    ShaderModulePtr shaderModule;
    RootParametersPtr rootParameters;
    std::vector<RootParameterValue> rootParameterValues;
};

typedef std::shared_ptr<ComputeData> ComputeDataPtr;

}}

#endif //SGL_D3D12_DATA_HPP
