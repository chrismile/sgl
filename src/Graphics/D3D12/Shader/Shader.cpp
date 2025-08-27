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

#include <iostream>
#include <utility>

#include "Shader.hpp"

#ifdef SUPPORT_D3D_COMPILER
#include <dxcapi.h>
#elif USE_LEGACY_D3DCOMPILER
#include <d3dcompiler.h>
#endif

namespace sgl { namespace d3d12 {

#ifdef SUPPORT_D3D_COMPILER

ShaderModule::ShaderModule(
        ShaderModuleType shaderModuleType, ComPtr<IDxcBlob> shaderBlob,
        const ComPtr<ID3D12ShaderReflection>& reflection)
        : shaderModuleType(shaderModuleType), shaderBlob(std::move(shaderBlob)) {
    queryReflectionData(reflection);
}

LPVOID ShaderModule::getBlobBufferPointer() {
    return shaderBlob->GetBufferPointer();
}

SIZE_T ShaderModule::getBlobBufferSize() {
    return shaderBlob->GetBufferSize();
}

#elif USE_LEGACY_D3DCOMPILER

ShaderModule::ShaderModule(
        ShaderModuleType shaderModuleType, ComPtr<ID3DBlob> shaderBlob,
        const ComPtr<ID3D12ShaderReflection>& reflection)
        : shaderModuleType(shaderModuleType), shaderBlob(std::move(shaderBlob)) {
    queryReflectionData(reflection);
}

LPVOID ShaderModule::getBlobBufferPointer() {
    return shaderBlob->GetBufferPointer();
}

SIZE_T ShaderModule::getBlobBufferSize() {
    return shaderBlob->GetBufferSize();
}

#else

LPVOID ShaderModule::getBlobBufferPointer() {
    sgl::Logfile::get()->throwError(
            "Error in ShaderModule::getBlobBufferPointer: D3D compiler was not enabled during the build.");
    return nullptr;
}

SIZE_T ShaderModule::getBlobBufferSize() {
    sgl::Logfile::get()->throwError(
            "Error in ShaderModule::getBlobBufferSize: D3D compiler was not enabled during the build.");
    return 0;
}

#endif

#if defined(SUPPORT_D3D_COMPILER) || defined(USE_LEGACY_D3DCOMPILER)
void ShaderModule::queryReflectionData(const ComPtr<ID3D12ShaderReflection>& reflection) {
    if (shaderModuleType == ShaderModuleType::COMPUTE) {
        reflection->GetThreadGroupSize(&threadGroupSizeX, &threadGroupSizeY, &threadGroupSizeZ);
    }

    D3D12_SHADER_DESC desc{};
    reflection->GetDesc(&desc);

    D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
    for (UINT resIdx = 0; resIdx < desc.BoundResources; resIdx++) {
        reflection->GetResourceBindingDesc(resIdx, &bindDesc);
        ShaderBindingInfo bindingInfo{};
        bindingInfo.space = bindDesc.Space;
        bindingInfo.binding = bindDesc.BindPoint;
        bindingNameToInfoMap.insert(std::make_pair(std::string(bindDesc.Name), bindingInfo));
    }

    D3D12_SHADER_BUFFER_DESC bufferDesc{};
    D3D12_SHADER_VARIABLE_DESC varDesc{};
    for (UINT cbIdx = 0; cbIdx < desc.ConstantBuffers; cbIdx++) {
        ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(cbIdx);
        cb->GetDesc(&bufferDesc);
        auto it = bindingNameToInfoMap.find(bufferDesc.Name);
        if (it == bindingNameToInfoMap.end()) {
            continue;
        }
        it->second.size = bufferDesc.Size;
        for (UINT varIdx = 0; varIdx < bufferDesc.Variables; varIdx++) {
            ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(varIdx);
            var->GetDesc(&varDesc);
            ShaderVarInfo varInfo{};
            varInfo.space = it->second.space;
            varInfo.binding = it->second.binding;
            varInfo.offset = varDesc.StartOffset;
            varInfo.size = varDesc.Size;
            variableNameToInfoMap.insert(std::make_pair(std::string(varDesc.Name), varInfo));
        }
    }

    //D3D12CreateVersionedRootSignatureDeserializer // formerly D3D12CreateRootSignatureDeserializer
    /*ComPtr<ID3D12RootSignatureDeserializer> deserializer;
    HRESULT hr = D3D12CreateRootSignatureDeserializer(
            getBlobBufferPointer(), getBlobBufferSize(), IID_PPV_ARGS(&deserializer));
    if (hr == E_INVALIDARG) {
        ;
    } else if (FAILED(hr)) {
        ;
    }
    const D3D12_ROOT_SIGNATURE_DESC *rootSignatureDesc = deserializer->GetRootSignatureDesc();*/
}
#endif

bool ShaderModule::hasBindingName(const std::string& name) {
    return bindingNameToInfoMap.find(name) != bindingNameToInfoMap.end();
}

const ShaderBindingInfo& ShaderModule::getBindingInfoByName(const std::string& name) {
    const auto it = bindingNameToInfoMap.find(name);
    if (it == bindingNameToInfoMap.end()) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderModule::getBindingInfoByName: No binding with name '" + name + "'.");
    }
    return it->second;
}

bool ShaderModule::hasVarName(const std::string& name) {
    return variableNameToInfoMap.find(name) != variableNameToInfoMap.end();
}

const ShaderVarInfo& ShaderModule::getVarInfoByName(const std::string& name) {
    const auto it = variableNameToInfoMap.find(name);
    if (it == variableNameToInfoMap.end()) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderModule::getVarInfoByName: No variable with name '" + name + "'.");
    }
    return it->second;
}


template<class T0, class T1>
void mergeMaps(std::unordered_map<T0, T1>& dstMap, const std::unordered_map<T0, T1>& srcMap) {
    for (const auto& entry : srcMap) {
        auto it = dstMap.find(entry.first);
        if (it == dstMap.end()) {
            dstMap.insert(entry);
        } else if (entry.second != it->second) {
            sgl::Logfile::get()->throwError(
                    "Error in ShaderStages::mergeMaps: Mismatching entries for \"" + sgl::toString(entry.first) + "\".");
        }
    }
}

ShaderStages::ShaderStages(const std::vector<ShaderModulePtr>& shaderModules) : shaderModules(shaderModules) {
    if (shaderModules.size() == 1) {
        bindingNameToInfoMap = shaderModules.front()->bindingNameToInfoMap;
        variableNameToInfoMap = shaderModules.front()->variableNameToInfoMap;
    } else {
        for (auto& shaderModule : shaderModules) {
            mergeMaps(bindingNameToInfoMap, shaderModule->bindingNameToInfoMap);
            mergeMaps(variableNameToInfoMap, shaderModule->variableNameToInfoMap);
        }
    }
}

bool ShaderStages::hasShaderModuleType(ShaderModuleType shaderModuleType) const {
    for (const auto& shaderModule : shaderModules) {
        if (shaderModule->getType() == shaderModuleType) {
            return true;
        }
    }
    return false;
}

ShaderModulePtr ShaderStages::getShaderModule(ShaderModuleType shaderModuleType) const {
    for (const auto& shaderModule : shaderModules) {
        if (shaderModule->getType() == shaderModuleType) {
            return shaderModule;
        }
    }
    return {};
}

bool ShaderStages::hasBindingName(const std::string& name) {
    return bindingNameToInfoMap.find(name) != bindingNameToInfoMap.end();
}

const ShaderBindingInfo& ShaderStages::getBindingInfoByName(const std::string& name) {
    const auto it = bindingNameToInfoMap.find(name);
    if (it == bindingNameToInfoMap.end()) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderModule::getBindingInfoByName: No binding with name '" + name + "'.");
    }
    return it->second;
}

bool ShaderStages::hasVarName(const std::string& name) {
    return variableNameToInfoMap.find(name) != variableNameToInfoMap.end();
}

const ShaderVarInfo& ShaderStages::getVarInfoByName(const std::string& name) {
    const auto it = variableNameToInfoMap.find(name);
    if (it == variableNameToInfoMap.end()) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderModule::getVarInfoByName: No variable with name '" + name + "'.");
    }
    return it->second;
}

}}
