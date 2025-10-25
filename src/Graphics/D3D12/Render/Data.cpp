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
#include "Renderer.hpp"
#include "Data.hpp"

#include "DescriptorAllocator.hpp"
#include "Graphics/D3D12/Shader/Shader.hpp"
#include "Graphics/D3D12/Utils/Resource.hpp"
#include "Math/Math.hpp"

namespace sgl { namespace d3d12 {

RootParameters::RootParameters() {
}

RootParameters::RootParameters(const ShaderModulePtr& shaderModule) : shaderModule(shaderModule) {
    shaderStages = std::make_shared<ShaderStages>(std::vector<ShaderModulePtr>{ shaderModule });
}

RootParameters::RootParameters(const ShaderStagesPtr& shaderStages) : shaderStages(shaderStages) {
    if (shaderStages->getShaderModules().size() == 1) {
        shaderModule = shaderStages->getShaderModules().front();
    }
}

UINT RootParameters::pushConstants(
        UINT num32BitValues, UINT shaderRegister, UINT registerSpace,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsConstants(num32BitValues, shaderRegister, registerSpace, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushConstants(
        const std::string& bindingName,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    checkShaderModule();
    if (!shaderStages->hasBindingName(bindingName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::pushShaderResourceView: No binding called '" + bindingName + "'.");
        return std::numeric_limits<UINT>::max();
    }
    const auto& bindingInfo = shaderStages->getBindingInfoByName(bindingName);
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsConstants(
            bindingInfo.size / sizeof(uint32_t), bindingInfo.binding, bindingInfo.space, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushConstantBufferView(
        UINT shaderRegister, UINT registerSpace,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsConstantBufferView(shaderRegister, registerSpace, flags, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushConstantBufferView(
        const std::string& bindingName,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    checkShaderModule();
    if (!shaderStages->hasBindingName(bindingName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::pushShaderResourceView: No binding called '" + bindingName + "'.");
        return std::numeric_limits<UINT>::max();
    }
    const auto& bindingInfo = shaderStages->getBindingInfoByName(bindingName);
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsConstantBufferView(bindingInfo.binding, bindingInfo.space, flags, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushShaderResourceView(
        UINT shaderRegister, UINT registerSpace,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsShaderResourceView(shaderRegister, registerSpace, flags, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushShaderResourceView(
        const std::string& bindingName,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    checkShaderModule();
    if (!shaderStages->hasBindingName(bindingName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::pushShaderResourceView: No binding called '" + bindingName + "'.");
        return std::numeric_limits<UINT>::max();
    }
    const auto& bindingInfo = shaderStages->getBindingInfoByName(bindingName);
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsShaderResourceView(bindingInfo.binding, bindingInfo.space, flags, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushUnorderedAccessView(
        UINT shaderRegister, UINT registerSpace,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsUnorderedAccessView(shaderRegister, registerSpace, flags, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushUnorderedAccessView(
        const std::string& bindingName,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    checkShaderModule();
    if (!shaderStages->hasBindingName(bindingName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::pushShaderResourceView: No binding called '" + bindingName + "'.");
        return std::numeric_limits<UINT>::max();
    }
    const auto& bindingInfo = shaderStages->getBindingInfoByName(bindingName);
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsUnorderedAccessView(bindingInfo.binding, bindingInfo.space, flags, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

UINT RootParameters::pushDescriptorTable(
        UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE1* descriptorRanges,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsDescriptorTable(numDescriptorRanges, descriptorRanges, visibility);
    rootParameters.emplace_back(rootParameter);
    return UINT(rootParameters.size() - 1);
}

void RootParameters::pushStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc) {
    checkPush();
    staticSamplers.push_back(staticSamplerDesc);
}

void RootParameters::checkPush() {
    if (rootSignature) {
        sgl::Logfile::get()->throwError(
                "Error: RootParameters::push* can only be called before RootParameters::build.");
    }
}

void RootParameters::checkShaderModule() {
    if (!shaderModule) {
        sgl::Logfile::get()->throwError(
                "Error: RootParameters::push* taking variable names need to be created with a shader module.");
    }
}

void RootParameters::build(Device* device) {
    if (rootSignature) {
        return;
    }
    auto* d3d12Device = device->getD3D12Device2();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureDataRootSignature = {};
    featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(d3d12Device->CheckFeatureSupport(
            D3D12_FEATURE_ROOT_SIGNATURE, &featureDataRootSignature, sizeof(featureDataRootSignature)))) {
        featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    const CD3DX12_ROOT_PARAMETER1* d3d12RootParameters = nullptr;
    const D3D12_STATIC_SAMPLER_DESC* d3d12StaticSamplers = nullptr;
    if (!rootParameters.empty()) {
        d3d12RootParameters = rootParameters.data();
    }
    if (!staticSamplers.empty()) {
        d3d12StaticSamplers = staticSamplers.data();
    }

    /*
     * TODO: Allow more flags:
     * D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
     * D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
     * D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
     * D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
     * D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
     * D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT
     */
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    if (shaderStages && shaderStages->hasShaderModuleType(ShaderModuleType::PIXEL)) {
        rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(
            uint32_t(rootParameters.size()), d3d12RootParameters,
            uint32_t(staticSamplers.size()), d3d12StaticSamplers, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
            featureDataRootSignature.HighestVersion, &rootSignatureBlob, &errorBlob));
    ThrowIfFailed(d3d12Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}


Data::Data(Device* device, const RootParametersPtr& rootParameters, const ShaderStagesPtr& shaderStages)
        : device(device), rootParameters(rootParameters), shaderStages(shaderStages) {
    rootParameters->build(device);
}

Data::~Data() {
    for (const auto& rpValue : rootParameterValues) {
        if (rpValue.type == RootParameterType::CONSTANTS_COPY) {
            delete[] static_cast<const uint32_t*>(rpValue.dataPointer);
        }
    }
}

void Data::setRootConstantValue(UINT rpIdx, uint32_t value, UINT offsetIn32BitValues) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::CONSTANTS_VALUE;
    rpValue.value = value;
    rpValue.num32BitValues = 1;
    rpValue.offsetIn32BitValues = offsetIn32BitValues;
    rootParameterValues.at(rpIdx) = rpValue;
}

void Data::setRootConstantValue(UINT rpIdx, const std::string& varName, uint32_t value) {
    if (!shaderStages->hasVarName(varName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantValue: No variable called '" + varName + "'.");
        return;
    }
    const auto& varInfo = shaderStages->getVarInfoByName(varName);
    if (varInfo.size != sizeof(uint32_t)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantValue: Size mismatch for variable '" + varName + "'.");
        return;
    }
    setRootConstantValue(rpIdx, value, varInfo.offset / sizeof(uint32_t));
}

void Data::setRootConstants(UINT rpIdx, const uint32_t* values, UINT num32BitValues, UINT offsetIn32BitValues) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::CONSTANTS_PTR;
    rpValue.dataPointer = values;
    rpValue.num32BitValues = num32BitValues;
    rpValue.offsetIn32BitValues = offsetIn32BitValues;
    rootParameterValues.at(rpIdx) = rpValue;
}

void Data::setRootConstants(UINT rpIdx, const std::string& varName, const uint32_t* values, UINT num32BitValues) {
    if (!shaderStages->hasVarName(varName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstants: No variable called '" + varName + "'.");
        return;
    }
    const auto& varInfo = shaderStages->getVarInfoByName(varName);
    if (varInfo.size != sizeof(uint32_t) * num32BitValues) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstants: Size mismatch for variable '" + varName + "'.");
        return;
    }
    setRootConstants(rpIdx, values, num32BitValues, varInfo.offset / sizeof(uint32_t));
}

void Data::setRootConstantsCopy(UINT rpIdx, const uint32_t* values, UINT num32BitValues, UINT offsetIn32BitValues) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    if (rootParameterValues.at(rpIdx).type == RootParameterType::CONSTANTS_COPY) {
        delete[] static_cast<const uint32_t*>(rootParameterValues.at(rpIdx).dataPointer);
    }
    auto* valuesCopy = new uint32_t[num32BitValues];
    memcpy(valuesCopy, values, num32BitValues * sizeof(uint32_t));
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::CONSTANTS_COPY;
    rpValue.dataPointer = valuesCopy;
    rpValue.num32BitValues = num32BitValues;
    rpValue.offsetIn32BitValues = offsetIn32BitValues;
    rootParameterValues.at(rpIdx) = rpValue;
}

void Data::setRootConstantsCopy(UINT rpIdx, const std::string& varName, const uint32_t* values, UINT num32BitValues) {
    if (!shaderStages->hasVarName(varName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantsCopy: No variable called '" + varName + "'.");
        return;
    }
    const auto& varInfo = shaderStages->getVarInfoByName(varName);
    if (varInfo.size != sizeof(uint32_t) * num32BitValues) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantsCopy: Size mismatch for variable '" + varName + "'.");
        return;
    }
    setRootConstantsCopy(rpIdx, values, num32BitValues, varInfo.offset / sizeof(uint32_t));
}

void Data::setConstantBufferView(UINT rpIdx, Resource* resource) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::CBV;
    rpValue.resource = resource;
    rootParameterValues.at(rpIdx) = rpValue;
}

void Data::setShaderResourceView(UINT rpIdx, Resource* resource) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::SRV;
    rpValue.resource = resource;
    rootParameterValues.at(rpIdx) = rpValue;
}

void Data::setUnorderedAccessView(UINT rpIdx, Resource* resource) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::UAV;
    rpValue.resource = resource;
    rootParameterValues.at(rpIdx) = rpValue;
}

void Data::setDescriptorTable(UINT rpIdx, DescriptorAllocation* descriptorAllocation) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::DESCRIPTOR_TABLE;
    rpValue.descriptorAllocation = descriptorAllocation;
    rootParameterValues.at(rpIdx) = rpValue;
}


ComputePipelineState::ComputePipelineState(const RootParametersPtr& rootParameters) : rootParameters(rootParameters) {
    shaderModule = rootParameters->getShaderModule();
}

ComputePipelineState::ComputePipelineState(const RootParametersPtr& rootParameters, const ShaderModulePtr& shaderModule)
        : rootParameters(rootParameters), shaderModule(shaderModule) {
}

void ComputePipelineState::build(Device* device) {
    if (pipelineState) {
        return;
    }
    auto* d3d12Device = device->getD3D12Device2();
    rootParameters->build(device);

    // Compute pipeline.
    struct ComputePipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;
    pipelineStateStream.rootSignature = rootParameters->getD3D12RootSignaturePtr();
    pipelineStateStream.CS = {
        shaderModule->getBlobBufferPointer(), shaderModule->getBlobBufferSize()
    };
    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(ComputePipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(d3d12Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)));
}


ComputeData::ComputeData(Device* device, const ComputePipelineStatePtr& computePipelineState)
        : Data(device, computePipelineState->getRootParameters(), std::make_shared<ShaderStages>(std::vector<ShaderModulePtr>{ computePipelineState->getShaderModule() })),
          computePipelineState(computePipelineState) {
    computePipelineState->build(device);
}

ComputeData::ComputeData(Device* device, const RootParametersPtr& rootParameters)
        : Data(device, rootParameters, rootParameters->getShaderStages() ? rootParameters->getShaderStages() : std::make_shared<ShaderStages>(std::vector<ShaderModulePtr>{ rootParameters->getShaderModule() })) {
    computePipelineState = std::make_shared<ComputePipelineState>(rootParameters, rootParameters->getShaderModule());
    computePipelineState->build(device);
}

ComputeData::ComputeData(Device* device, const RootParametersPtr& rootParameters, const ShaderModulePtr& shaderModule)
        :  Data(device, rootParameters, rootParameters->getShaderStages() ? rootParameters->getShaderStages() : std::make_shared<ShaderStages>(std::vector<ShaderModulePtr>{ shaderModule })) {
    computePipelineState = std::make_shared<ComputePipelineState>(rootParameters, shaderModule);
    computePipelineState->build(device);
}

ComputeData::~ComputeData() = default;

void ComputeData::setRootState(ID3D12GraphicsCommandList* d3d12CommandList) {
    d3d12CommandList->SetPipelineState(computePipelineState->getD3D12PipelineStatePtr());
    d3d12CommandList->SetComputeRootSignature(rootParameters->getD3D12RootSignaturePtr());

    for (const auto& rpValue : rootParameterValues) {
        if (rpValue.type == RootParameterType::CONSTANTS_PTR
                || rpValue.type == RootParameterType::CONSTANTS_COPY) {
            if (rpValue.num32BitValues == 1) {
                d3d12CommandList->SetComputeRoot32BitConstant(
                        rpValue.rpIdx, *static_cast<const uint32_t*>(rpValue.dataPointer), rpValue.offsetIn32BitValues);
            } else {
                d3d12CommandList->SetComputeRoot32BitConstants(
                        rpValue.rpIdx, rpValue.num32BitValues, rpValue.dataPointer, rpValue.offsetIn32BitValues);
            }
        } else if (rpValue.type == RootParameterType::CONSTANTS_VALUE) {
            d3d12CommandList->SetComputeRoot32BitConstant(rpValue.rpIdx, rpValue.value, rpValue.offsetIn32BitValues);
        } else if (rpValue.type == RootParameterType::CBV) {
            d3d12CommandList->SetComputeRootConstantBufferView(
                    rpValue.rpIdx, rpValue.resource->getGPUVirtualAddress());
        } else if (rpValue.type == RootParameterType::SRV) {
            d3d12CommandList->SetComputeRootShaderResourceView(
                    rpValue.rpIdx, rpValue.resource->getGPUVirtualAddress());
        } else if (rpValue.type == RootParameterType::UAV) {
            d3d12CommandList->SetComputeRootUnorderedAccessView(
                    rpValue.rpIdx, rpValue.resource->getGPUVirtualAddress());
        } else if (rpValue.type == RootParameterType::DESCRIPTOR_TABLE) {
            d3d12CommandList->SetComputeRootDescriptorTable(
                    rpValue.rpIdx, rpValue.descriptorAllocation->getGPUDescriptorHandle());
        } else {
            sgl::Logfile::get()->throwErrorVar(
                    "Error in ComputeData::setRootState: Root parameter '", rpValue.rpIdx, "' not set.");
        }
    }
}


RasterPipelineState::RasterPipelineState(const RootParametersPtr& rootParameters) : rootParameters(rootParameters) {
    shaderStages = rootParameters->getShaderStages();
}

RasterPipelineState::RasterPipelineState(const RootParametersPtr& rootParameters, const ShaderStagesPtr& shaderStages)
        : rootParameters(rootParameters), shaderStages(shaderStages) {
}

void RasterPipelineState::pushInputElementDesc(
        const std::string& semanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot,
        UINT alignedByteOffset, D3D12_INPUT_CLASSIFICATION inputSlotClass, UINT instanceDataStepRate) {
    bool resetStringPointers = inputElementDescs.capacity() == inputElementDescs.size() && !inputElementDescs.empty();
    inputElementSemanticNames.push_back(semanticName);
    inputElementDescs.push_back(D3D12_INPUT_ELEMENT_DESC {
        inputElementSemanticNames.back().c_str(), semanticIndex, format, inputSlot,
        alignedByteOffset, inputSlotClass, instanceDataStepRate
    });
    if (resetStringPointers) {
        for (size_t i = 0; i < inputElementDescs.size() - 1; i++) {
            inputElementDescs[i].SemanticName = inputElementSemanticNames[i].c_str();
        }
    }
}

void RasterPipelineState::setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY _primitiveTopology) {
    primitiveTopology = _primitiveTopology;

    if (primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED) {
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    } else if (primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_POINTLIST) {
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    } else if (primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_LINELIST || primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP
            || primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ ||primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ) {
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    } else if (primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST || primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
            || primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN || primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ
            || primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ) {
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    } else if (int(primitiveTopology) >= int(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)
            && int(primitiveTopology) <= int(D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST)) {
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    } else {
        sgl::Logfile::get()->throwErrorVar(
                "Error in ComputeData::setPrimitiveTopology: Invalid primitive topology index ", primitiveTopology, ".");
    }
}

void RasterPipelineState::setRenderTargetFormat(DXGI_FORMAT format, UINT index) {
    if (index >= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
        sgl::Logfile::get()->throwError("Error in RasterPipelineState::setRenderTargetViewFormat: Invalid index.");
    }
    rtFormats.NumRenderTargets = std::max(rtFormats.NumRenderTargets, index + 1);
    rtFormats.RTFormats[index] = format;
}

void RasterPipelineState::setDepthStencilFormat(DXGI_FORMAT format) {
    dsFormat = format;
    hasDepthStencil = true;
}

class PipelineStateStream {
public:
    ~PipelineStateStream() {
        if (memPtr) {
            free(memPtr);
            memPtr = nullptr;
        }
    }

    void reserve(size_t newSize) {
        if (newSize > currentCapability) {
            constexpr size_t GRANULARITY = 64;
            currentCapability = sgl::sizeceil(newSize, GRANULARITY) * GRANULARITY;
            memPtr = static_cast<uint8_t*>(realloc(memPtr, currentCapability));
        }
    }

    [[nodiscard]] size_t size() const { return currentSize; }
    [[nodiscard]] const void* data() const { return memPtr; }
    [[nodiscard]] void* data() { return memPtr; }

    template<class T>
    void push(const T& streamSubobject) {
        static_assert(sizeof(T) % sizeof(void*) == 0);
        reserve(currentSize + sizeof(T));
        memcpy(memPtr + currentSize, std::addressof(streamSubobject), sizeof(T));
        currentSize += sizeof(T);
    }

private:
    size_t currentCapability = 0;
    size_t currentSize = 0;
    uint8_t* memPtr = nullptr;
};

void RasterPipelineState::build(Device* device) {
    if (pipelineState) {
        return;
    }
    auto* d3d12Device = device->getD3D12Device2();
    rootParameters->build(device);

    if (!shaderStages->hasShaderModuleType(ShaderModuleType::VERTEX)) {
        // TODO: Can be removed once mesh shaders are supported.
        sgl::Logfile::get()->throwError("Error in RasterPipelineState::build: No vertex shader specified.");
    }
    if (!shaderStages->hasShaderModuleType(ShaderModuleType::PIXEL)) {
        sgl::Logfile::get()->throwError("Error in RasterPipelineState::build: No pixel shader specified.");
    }
    auto vertexShaderModule = shaderStages->getShaderModule(ShaderModuleType::VERTEX);
    auto pixelShaderModule = shaderStages->getShaderModule(ShaderModuleType::PIXEL);

    PipelineStateStream pipelineStateStream;
    pipelineStateStream.reserve(
        sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE)
        + sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT)
        + sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY)
        + sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS)
        + sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS)
        + sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT)
        + sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS)
    );

    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature = rootParameters->getD3D12RootSignaturePtr();
    pipelineStateStream.push(rootSignature);
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
    inputLayout = { inputElementDescs.data(), uint32_t(inputElementDescs.size()) };
    pipelineStateStream.push(inputLayout);
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopologyObj = primitiveTopologyType;
    pipelineStateStream.push(primitiveTopologyObj);
    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
    VS = {
        vertexShaderModule->getBlobBufferPointer(), vertexShaderModule->getBlobBufferSize()
    };
    pipelineStateStream.push(VS);
    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
    PS = {
        pixelShaderModule->getBlobBufferPointer(), pixelShaderModule->getBlobBufferSize()
    };
    pipelineStateStream.push(PS);
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat = dsFormat;
    if (getHasDepthStencil()) {
        pipelineStateStream.push(dsvFormat);
    }
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats = rtFormats;
    if (getNumRenderTargets() > 0) {
        pipelineStateStream.push(rtvFormats);
    }

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        pipelineStateStream.size(), pipelineStateStream.data()
    };
    ThrowIfFailed(d3d12Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)));
}


inline uint32_t getIndexFormatSizeInBytes(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UINT:
            return 1;
        case DXGI_FORMAT_R16_UINT:
            return 2;
        case DXGI_FORMAT_R32_UINT:
            return 4;
        default:
            sgl::Logfile::get()->throwError("Error in getIndexFormatSizeInBytes: Invalid index buffer format.");
            return 0;
    }
}

RasterData::RasterData(Renderer* renderer, const RasterPipelineStatePtr& rasterPipelineState)
        : Data(renderer->getDevice(), rasterPipelineState->getRootParameters(), rasterPipelineState->getShaderStages()),
          renderer(renderer), rasterPipelineState(rasterPipelineState) {
    rasterPipelineState->build(device);

    if (rasterPipelineState->getNumRenderTargets() > 0) {
        auto* descriptorAllocatorRtv = renderer->getDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        descriptorAllocationRtv = descriptorAllocatorRtv->allocate(rasterPipelineState->getNumRenderTargets());
    }
    if (rasterPipelineState->getHasDepthStencil()) {
        auto* descriptorAllocatorDsv = renderer->getDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        descriptorAllocationDsv = descriptorAllocatorDsv->allocate(1);
    }
}

RasterData::~RasterData() = default;

void RasterData::setVertexBuffer(const ResourcePtr& buffer, UINT slot, size_t strideInBytes) {
    if (strideInBytes == 0) {
        sgl::Logfile::get()->throwError("Error in setVertexBuffer: Zero stride is not valid.");
    }
    if (size_t(slot) >= vertexBuffers.size()) {
        vertexBuffers.resize(slot + 1);
        vertexBufferViews.resize(slot + 1);
    }
    vertexBuffers[slot] = buffer;
    auto& vertexBufferView = vertexBufferViews[slot];
    vertexBufferView.BufferLocation = buffer->getGPUVirtualAddress();
    vertexBufferView.SizeInBytes = UINT(buffer->getD3D12ResourceDesc().Width);
    vertexBufferView.StrideInBytes = UINT(strideInBytes);
    size_t numVerticesNew = buffer->getD3D12ResourceDesc().Width / strideInBytes;
    if (numVertices != 0 && numVertices != numVerticesNew) {
        sgl::Logfile::get()->throwError("Error in setVertexBuffer: Mismatching number of vertices.");
    }
    numVertices = numVerticesNew;
}

void RasterData::setIndexBuffer(const ResourcePtr& buffer, DXGI_FORMAT format) {
    indexBuffer = buffer;
    indexFormat = format;
    numIndices = buffer->getD3D12ResourceDesc().Width / getIndexFormatSizeInBytes(format);
    indexBufferView.BufferLocation = indexBuffer->getGPUVirtualAddress();
    indexBufferView.Format = indexFormat;
    indexBufferView.SizeInBytes = UINT(indexBuffer->getD3D12ResourceDesc().Width);
}

void RasterData::setDepthStencilView(const ResourcePtr& image, D3D12_DSV_FLAGS flags) {
    const auto& resourceDesc = image->getD3D12ResourceDesc();
    if (renderTargetWidth == 0 && renderTargetHeight == 0) {
        renderTargetWidth = uint32_t(resourceDesc.Width);
        renderTargetHeight = resourceDesc.Height;
    } else if (renderTargetWidth != uint32_t(resourceDesc.Width) || renderTargetHeight != resourceDesc.Height) {
        sgl::Logfile::get()->throwError("Error in RasterData::setDepthStencilView: Render target resolution mismatch.");
    }
    depthStencilImage = image;

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
    depthStencilViewDesc.Format = image->getD3D12ResourceDesc().Format;
    depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Flags = flags;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
    device->getD3D12Device2()->CreateDepthStencilView(
            image->getD3D12ResourcePtr(), &depthStencilViewDesc,
            descriptorAllocationDsv->getCPUDescriptorHandle());
}

void RasterData::setRenderTargetView(const ResourcePtr& image, UINT index) {
    const auto& resourceDesc = image->getD3D12ResourceDesc();
    if (renderTargetWidth == 0 && renderTargetHeight == 0) {
        renderTargetWidth = uint32_t(resourceDesc.Width);
        renderTargetHeight = resourceDesc.Height;
    } else if (renderTargetWidth != uint32_t(resourceDesc.Width) || renderTargetHeight != resourceDesc.Height) {
        sgl::Logfile::get()->throwError("Error in RasterData::setRenderTargetView: Render target resolution mismatch.");
    }
    if (size_t(index) >= rasterPipelineState->getNumRenderTargets()) {
        sgl::Logfile::get()->throwError("Error in RasterData::setRenderTargetView: Mismatching number of render targets.");
    }
    if (size_t(index) >= renderTargetImages.size()) {
        renderTargetImages.resize(index + 1);
        shallClearColors.resize(index + 1, shallClearColorDefault);
        colorClearValues.resize(index + 1, colorClearValueDefault);
    }
    renderTargetImages[index] = image;

    D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
    renderTargetViewDesc.Format = image->getD3D12ResourceDesc().Format;
    renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;
    renderTargetViewDesc.Texture2D.PlaneSlice = 0;
    device->getD3D12Device2()->CreateRenderTargetView(
            image->getD3D12ResourcePtr(), &renderTargetViewDesc,
            descriptorAllocationRtv->getCPUDescriptorHandle(index));
}

void RasterData::setClearColor(const glm::vec4& colorVal, UINT index) {
    if (index == std::numeric_limits<UINT>::max() || renderTargetImages.size() <= 1) {
        shallClearColorDefault = true;
        colorClearValueDefault = colorVal;
        for (size_t i = 0; i < shallClearColors.size(); i++) {
            shallClearColors[i] = true;
            colorClearValues[i] = colorVal;
        }
    } else {
        shallClearColors[index] = true;
        colorClearValues[index] = colorVal;
    }
}

void RasterData::setClearDepthStencil(float depthVal, uint8_t stencilVal) {
    shallClearDepthStencil = true;
    depthClearValue = depthVal;
    stencilClearValue = stencilVal;
}

void RasterData::disableClearColor(UINT index) {
    if (index == std::numeric_limits<UINT>::max() || renderTargetImages.size() <= 1) {
        for (size_t i = 0; i < shallClearColors.size(); i++) {
            shallClearColors[i] = false;
        }
    } else {
        shallClearColors[index] = false;
    }
}

void RasterData::disableClearDepthStencil() {
    shallClearDepthStencil = false;
}

void RasterData::setRootState(ID3D12GraphicsCommandList* d3d12CommandList) {
    d3d12CommandList->SetPipelineState(rasterPipelineState->getD3D12PipelineStatePtr());
    d3d12CommandList->SetGraphicsRootSignature(rootParameters->getD3D12RootSignaturePtr());

    for (const auto& rpValue : rootParameterValues) {
        if (rpValue.type == RootParameterType::CONSTANTS_PTR
                || rpValue.type == RootParameterType::CONSTANTS_COPY) {
            if (rpValue.num32BitValues == 1) {
                d3d12CommandList->SetGraphicsRoot32BitConstant(
                        rpValue.rpIdx, *static_cast<const uint32_t*>(rpValue.dataPointer), rpValue.offsetIn32BitValues);
            } else {
                d3d12CommandList->SetGraphicsRoot32BitConstants(
                        rpValue.rpIdx, rpValue.num32BitValues, rpValue.dataPointer, rpValue.offsetIn32BitValues);
            }
        } else if (rpValue.type == RootParameterType::CONSTANTS_VALUE) {
            d3d12CommandList->SetGraphicsRoot32BitConstant(rpValue.rpIdx, rpValue.value, rpValue.offsetIn32BitValues);
        } else if (rpValue.type == RootParameterType::CBV) {
            d3d12CommandList->SetGraphicsRootConstantBufferView(
                    rpValue.rpIdx, rpValue.resource->getGPUVirtualAddress());
        } else if (rpValue.type == RootParameterType::SRV) {
            d3d12CommandList->SetGraphicsRootShaderResourceView(
                    rpValue.rpIdx, rpValue.resource->getGPUVirtualAddress());
        } else if (rpValue.type == RootParameterType::UAV) {
            d3d12CommandList->SetGraphicsRootUnorderedAccessView(
                    rpValue.rpIdx, rpValue.resource->getGPUVirtualAddress());
        } else if (rpValue.type == RootParameterType::DESCRIPTOR_TABLE) {
            d3d12CommandList->SetGraphicsRootDescriptorTable(
                    rpValue.rpIdx, rpValue.descriptorAllocation->getGPUDescriptorHandle());
        } else {
            sgl::Logfile::get()->throwErrorVar(
                    "Error in RasterData::setRootState: Root parameter '", rpValue.rpIdx, "' not set.");
        }
    }

    d3d12CommandList->IASetPrimitiveTopology(rasterPipelineState->getPrimitiveTopology());
    if (!vertexBufferViews.empty()) {
        d3d12CommandList->IASetVertexBuffers(0, uint32_t(vertexBufferViews.size()), vertexBufferViews.data());
    }
    if (getHasIndexBuffer()) {
        d3d12CommandList->IASetIndexBuffer(&indexBufferView);
    }

    viewport.Width = float(renderTargetWidth);
    viewport.Height = float(renderTargetHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    d3d12CommandList->RSSetViewports(1, &viewport);

    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = LONG(renderTargetWidth);
    scissorRect.bottom = LONG(renderTargetHeight);
    d3d12CommandList->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleRtv, descriptorHandleDsv;
    D3D12_CPU_DESCRIPTOR_HANDLE *descriptorHandleRtvPtr = nullptr, *descriptorHandleDsvPtr = nullptr;
    if (rasterPipelineState->getNumRenderTargets() > 0) {
        descriptorHandleRtv = descriptorAllocationRtv->getCPUDescriptorHandle();
        descriptorHandleRtvPtr = &descriptorHandleRtv;
    }
    if (rasterPipelineState->getHasDepthStencil()) {
        descriptorHandleDsv = descriptorAllocationDsv->getCPUDescriptorHandle();
        descriptorHandleDsvPtr = &descriptorHandleDsv;
    }
    d3d12CommandList->OMSetRenderTargets(1, descriptorHandleRtvPtr, true, descriptorHandleDsvPtr);

    if (rasterPipelineState->getHasDepthStencil() && shallClearDepthStencil) {
        D3D12_CLEAR_FLAGS clearFlags{};
        auto dsFormat = rasterPipelineState->getDepthStencilFormat();
        if (dsFormat == DXGI_FORMAT_D16_UNORM || dsFormat == DXGI_FORMAT_D32_FLOAT) {
            clearFlags = D3D12_CLEAR_FLAG_DEPTH;
        } else if (dsFormat == DXGI_FORMAT_D24_UNORM_S8_UINT || dsFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
            clearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
        } else {
            sgl::Logfile::get()->throwError("Error in RasterData::setRootState: Unexpected depth-stencil format.");
        }
        d3d12CommandList->ClearDepthStencilView(
                descriptorHandleDsv, clearFlags,
                depthClearValue, stencilClearValue, 1, &scissorRect);
    }
    for (size_t i = 0; i < renderTargetImages.size(); i++) {
        if (shallClearColors.at(i)) {
            d3d12CommandList->ClearRenderTargetView(
                    descriptorAllocationRtv->getCPUDescriptorHandle(uint32_t(i)),
                    &colorClearValues.at(i).x, 1, &scissorRect);
        }
    }
}

}}
