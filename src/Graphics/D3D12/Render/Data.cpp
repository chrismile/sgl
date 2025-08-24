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

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
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

    // Create the pipeline state object.
    struct ComputePipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;
    pipelineStateStream.pRootSignature = rootSignature.Get();
    pipelineStateStream.CS = {
        shaderModule->getBlobBufferPointer(), shaderModule->getBlobBufferSize()
    };
    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
            sizeof(ComputePipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(d3d12Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)));
}


ComputeData::ComputeData(Renderer* renderer, const RootParametersPtr& rootParameters)
        : renderer(renderer), shaderModule(rootParameters->getShaderModule()), rootParameters(rootParameters) {
    rootParameters->build(renderer->getDevice());
}

ComputeData::ComputeData(Renderer* renderer, const ShaderModulePtr& shaderModule, const RootParametersPtr& rootParameters)
        : renderer(renderer), shaderModule(shaderModule), rootParameters(rootParameters) {
    rootParameters->build(renderer->getDevice());
}

ComputeData::~ComputeData() {
    for (const auto& rpValue : rootParameterValues) {
        if (rpValue.type == RootParameterType::CONSTANTS_COPY) {
            delete[] static_cast<const uint32_t*>(rpValue.dataPointer);
        }
    }
}

void ComputeData::setRootConstantValue(UINT rpIdx, uint32_t value, UINT offsetIn32BitValues) {
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

void ComputeData::setRootConstantValue(UINT rpIdx, const std::string& varName, uint32_t value) {
    if (!shaderModule->hasVarName(varName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantValue: No variable called '" + varName + "'.");
        return;
    }
    const auto& varInfo = shaderModule->getVarInfoByName(varName);
    if (varInfo.size != sizeof(uint32_t)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantValue: Size mismatch for variable '" + varName + "'.");
        return;
    }
    setRootConstantValue(rpIdx, value, varInfo.offset / sizeof(uint32_t));
}

void ComputeData::setRootConstants(UINT rpIdx, const uint32_t* values, UINT num32BitValues, UINT offsetIn32BitValues) {
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

void ComputeData::setRootConstants(UINT rpIdx, const std::string& varName, const uint32_t* values, UINT num32BitValues) {
    if (!shaderModule->hasVarName(varName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstants: No variable called '" + varName + "'.");
        return;
    }
    const auto& varInfo = shaderModule->getVarInfoByName(varName);
    if (varInfo.size != sizeof(uint32_t) * num32BitValues) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstants: Size mismatch for variable '" + varName + "'.");
        return;
    }
    setRootConstants(rpIdx, values, num32BitValues, varInfo.offset / sizeof(uint32_t));
}

void ComputeData::setRootConstantsCopy(UINT rpIdx, const uint32_t* values, UINT num32BitValues, UINT offsetIn32BitValues) {
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

void ComputeData::setRootConstantsCopy(UINT rpIdx, const std::string& varName, const uint32_t* values, UINT num32BitValues) {
    if (!shaderModule->hasVarName(varName)) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantsCopy: No variable called '" + varName + "'.");
        return;
    }
    const auto& varInfo = shaderModule->getVarInfoByName(varName);
    if (varInfo.size != sizeof(uint32_t) * num32BitValues) {
        sgl::Logfile::get()->throwError(
                "Error in RootParameters::setRootConstantsCopy: Size mismatch for variable '" + varName + "'.");
        return;
    }
    setRootConstantsCopy(rpIdx, values, num32BitValues, varInfo.offset / sizeof(uint32_t));
}

void ComputeData::setConstantBufferView(UINT rpIdx, Resource* resource) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::CBV;
    rpValue.resource = resource;
    rootParameterValues.at(rpIdx) = rpValue;
}

void ComputeData::setShaderResourceView(UINT rpIdx, Resource* resource) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::SRV;
    rpValue.resource = resource;
    rootParameterValues.at(rpIdx) = rpValue;
}

void ComputeData::setUnorderedAccessView(UINT rpIdx, Resource* resource) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::UAV;
    rpValue.resource = resource;
    rootParameterValues.at(rpIdx) = rpValue;
}

void ComputeData::setDescriptorTable(UINT rpIdx, DescriptorAllocation* descriptorAllocation) {
    if (UINT(rootParameterValues.size()) <= rpIdx) {
        rootParameterValues.resize(rpIdx + 1);
    }
    RootParameterValue rpValue{};
    rpValue.rpIdx = rpIdx;
    rpValue.type = RootParameterType::DESCRIPTOR_TABLE;
    rpValue.descriptorAllocation = descriptorAllocation;
    rootParameterValues.at(rpIdx) = rpValue;
}


void ComputeData::setRootState(ID3D12GraphicsCommandList* d3d12CommandList) {
    d3d12CommandList->SetPipelineState(rootParameters->getD3D12PipelineStatePtr());
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

}}
