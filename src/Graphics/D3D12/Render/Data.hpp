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
typedef std::shared_ptr<Resource> ResourcePtr;
class DescriptorAllocation;
typedef std::shared_ptr<DescriptorAllocation> DescriptorAllocationPtr;
class ShaderModule;
typedef std::shared_ptr<ShaderModule> ShaderModulePtr;
class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;
class Renderer;

enum class RootParameterType {
        UNDEFINED = 0, CONSTANTS_PTR, CONSTANTS_VALUE, CONSTANTS_COPY, CBV, SRV, UAV, DESCRIPTOR_TABLE
};

class DLL_OBJECT RootParameters {
public:
    RootParameters();
    // Passing the shader module allows for doing shader reflection.
    explicit RootParameters(const ShaderModulePtr& shaderModule);
    explicit RootParameters(const ShaderStagesPtr& shaderStages);

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
    [[nodiscard]] const ShaderStagesPtr& getShaderStages() const { return shaderStages; }
    [[nodiscard]] const std::vector<CD3DX12_ROOT_PARAMETER1>& getRootParameters() const { return rootParameters; }
    [[nodiscard]] const std::vector<D3D12_STATIC_SAMPLER_DESC>& getStaticSamplers() const { return staticSamplers; }
    ID3D12RootSignature* getD3D12RootSignaturePtr() { return rootSignature.Get(); }

protected:
    ShaderModulePtr shaderModule;
    ShaderStagesPtr shaderStages;
    void checkPush();
    void checkShaderModule();
    std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
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

class DLL_OBJECT Data {
public:
    Data(Device* device, const RootParametersPtr& rootParameters, const ShaderStagesPtr& shaderStages);
    virtual ~Data();

    RootParametersPtr getRootParameters() { return rootParameters; }

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

    virtual void setRootState(ID3D12GraphicsCommandList* d3d12CommandList)=0;

protected:
    Device* device;
    RootParametersPtr rootParameters;
    ShaderStagesPtr shaderStages;
    std::vector<RootParameterValue> rootParameterValues;
};

class DLL_OBJECT ComputePipelineState {
public:
    explicit ComputePipelineState(const RootParametersPtr& rootParameters);
    ComputePipelineState(const RootParametersPtr& rootParameters, const ShaderModulePtr& shaderModule);
    RootParametersPtr getRootParameters() { return rootParameters; }
    ShaderModulePtr getShaderModule() { return shaderModule; }
    ID3D12PipelineState* getD3D12PipelineStatePtr() { return pipelineState.Get(); }
    void build(Device* device);

private:
    RootParametersPtr rootParameters;
    ShaderModulePtr shaderModule;
    ComPtr<ID3D12PipelineState> pipelineState;
};

typedef std::shared_ptr<ComputePipelineState> ComputePipelineStatePtr;

class DLL_OBJECT ComputeData : public Data {
public:
    ComputeData(Device* device, const ComputePipelineStatePtr& computePipelineState);
    ComputeData(Device* device, const RootParametersPtr& rootParameters);
    ComputeData(Device* device, const RootParametersPtr& rootParameters, const ShaderModulePtr& shaderModule);
    ~ComputeData() override;
    void setRootState(ID3D12GraphicsCommandList* d3d12CommandList) override;

private:
    ComputePipelineStatePtr computePipelineState;
};

typedef std::shared_ptr<ComputeData> ComputeDataPtr;


class DLL_OBJECT RasterPipelineState {
public:
    explicit RasterPipelineState(const RootParametersPtr& rootParameters);
    RasterPipelineState(const RootParametersPtr& rootParameters, const ShaderStagesPtr& shaderStages);

    RootParametersPtr getRootParameters() { return rootParameters; }
    ShaderStagesPtr getShaderStages() { return shaderStages; }
    ID3D12PipelineState* getD3D12PipelineStatePtr() { return pipelineState.Get(); }
    [[nodiscard]] D3D12_PRIMITIVE_TOPOLOGY getPrimitiveTopology() const { return primitiveTopology; }
    [[nodiscard]] uint32_t getNumRenderTargets() const { return rtFormats.NumRenderTargets; }
    [[nodiscard]] bool getHasDepthStencil() const { return hasDepthStencil; }
    [[nodiscard]] DXGI_FORMAT getDepthStencilFormat() const { return dsFormat; }

    void pushInputElementDesc(
            const std::string& semanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot,
            UINT alignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            UINT instanceDataStepRate = 0);

    void setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY _primitiveTopology);

    void setRenderTargetFormat(DXGI_FORMAT format, UINT index = 0);
    void setDepthStencilFormat(DXGI_FORMAT format);

    void build(Device* device);

private:
    RootParametersPtr rootParameters;
    ShaderStagesPtr shaderStages;
    ComPtr<ID3D12PipelineState> pipelineState;
    std::vector<std::string> inputElementSemanticNames;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
    D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    D3D12_RT_FORMAT_ARRAY rtFormats{};
    bool hasDepthStencil = false;
    DXGI_FORMAT dsFormat = DXGI_FORMAT_D32_FLOAT;
};

typedef std::shared_ptr<RasterPipelineState> RasterPipelineStatePtr;

class DLL_OBJECT RasterData : public Data {
public:
    RasterData(Renderer* renderer, const RasterPipelineStatePtr& computePipelineState);
    ~RasterData() override;

    void setVertexBuffer(const ResourcePtr& buffer, UINT slot, size_t strideInBytes);
    void setIndexBuffer(const ResourcePtr& buffer, DXGI_FORMAT format);
    void setNumInstances(uint32_t _numInstances) { numInstances = _numInstances; }
    [[nodiscard]] bool getHasIndexBuffer() const { return indexBuffer.get() != nullptr; }
    [[nodiscard]] size_t getNumIndices() const { return numIndices; }
    [[nodiscard]] size_t getNumVertices() const { return numVertices; }
    [[nodiscard]] uint32_t getNumInstances() const { return numInstances; }

    void setDepthStencilView(const ResourcePtr& image, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
    void setRenderTargetView(const ResourcePtr& image, UINT index = 0);
    void setClearColor(const glm::vec4& colorVal, UINT index = std::numeric_limits<UINT>::max());
    void setClearDepthStencil(float depthVal = 1.0f, uint8_t stencilVal = 0);
    void disableClearColor(UINT index = std::numeric_limits<UINT>::max());
    void disableClearDepthStencil();

    void setRootState(ID3D12GraphicsCommandList* d3d12CommandList) override;

private:
    Renderer* renderer;
    RasterPipelineStatePtr rasterPipelineState;

    // Index and vertex data.
    ResourcePtr indexBuffer;
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;
    size_t numIndices = 0;
    std::vector<ResourcePtr> vertexBuffers;
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
    size_t numVertices = 0;
    uint32_t numInstances = 1;

    // RTV and DSV data.
    std::vector<ResourcePtr> renderTargetImages;
    DescriptorAllocationPtr descriptorAllocationRtv;
    std::vector<bool> shallClearColors;
    std::vector<glm::vec4> colorClearValues;
    bool shallClearColorDefault = false;
    glm::vec4 colorClearValueDefault = glm::vec4(0.0f);
    ResourcePtr depthStencilImage;
    DescriptorAllocationPtr descriptorAllocationDsv;
    bool shallClearDepthStencil = false;
    float depthClearValue = 1.0f;
    uint8_t stencilClearValue = 0;
    uint32_t renderTargetWidth = 0, renderTargetHeight = 0;
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissorRect{};
};

typedef std::shared_ptr<ComputeData> ComputeDataPtr;

}}

#endif //SGL_D3D12_DATA_HPP
