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

#include "Graphics/D3D12/Shader/Shader.hpp"

namespace sgl { namespace d3d12 {

RootParameters::RootParameters() {
    ;
}

RootParameters::RootParameters(ShaderModulePtr shaderModule) : shaderModule(shaderModule) {
    ;
}

void RootParameters::pushConstants(
        UINT num32BitValues, UINT shaderRegister, UINT registerSpace,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushConstants(
        const std::string& varName,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushConstantBufferView(
        UINT shaderRegister, UINT registerSpace,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushConstantBufferView(
        const std::string& varName,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushShaderResourceView(
        UINT shaderRegister, UINT registerSpace,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushShaderResourceView(
        const std::string& varName,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushUnorderedAccessView(
        UINT shaderRegister, UINT registerSpace,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushUnorderedAccessView(
        const std::string& varName,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushDescriptorTable(
        UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::pushDescriptorTable(
        const std::string& varName,
        D3D12_SHADER_VISIBILITY visibility) {
    checkPush();
}

void RootParameters::checkPush() {
    if (rootSignature) {
        sgl::Logfile::get()->throwError(
                "Error: RootParameters::push* can only be called before RootParameters::build.");
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
            rootParameters.size(), d3d12RootParameters,
            staticSamplers.size(), d3d12StaticSamplers, rootSignatureFlags);

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
    auto* shaderBlob = shaderModule->getBlobPtr();
    pipelineStateStream.CS = { shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize() };
    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
            sizeof(ComputePipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(d3d12Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)));
}


ComputeData::ComputeData(Renderer* renderer, const RootParametersPtr& rootParameters)
        : renderer(renderer), rootParameters(rootParameters) {
    rootParameters->build(renderer->getDevice());
}

void ComputeData::setRootState(ID3D12GraphicsCommandList* d3d12CommandList) {
    d3d12CommandList->SetPipelineState(rootParameters->getD3D12PipelineStatePtr());
    d3d12CommandList->SetComputeRootSignature(rootParameters->getD3D12RootSignaturePtr());

    /*d3d12CommandList->SetComputeRoot32BitConstant();
    d3d12CommandList->SetComputeRootUnorderedAccessView(idx, bufferAddress);
    d3d12CommandList->SetComputeRootDescriptorTable(idx, baseDescriptor);*/
}

}}
