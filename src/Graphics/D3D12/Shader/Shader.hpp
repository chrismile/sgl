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

#ifndef SGL_D3D12_SHADER_HPP
#define SGL_D3D12_SHADER_HPP

#include <unordered_map>

#include "../Utils/d3d12.hpp"
#include "ShaderModuleType.hpp"

#ifdef SUPPORT_D3D_COMPILER
#endif

struct IDxcBlob;

namespace sgl { namespace d3d12 {

class Device;

struct DLL_OBJECT ShaderBindingInfo {
    uint32_t space;
    uint32_t binding;
    uint32_t size; // optional
};
struct DLL_OBJECT ShaderVarInfo {
    uint32_t space;
    uint32_t binding;
    uint32_t offset;
    uint32_t size;
};

class DLL_OBJECT ShaderModule {
public:
#ifdef SUPPORT_D3D_COMPILER
    ShaderModule(
            ShaderModuleType shaderModuleType, ComPtr<IDxcBlob> shaderBlob,
            const ComPtr<ID3D12ShaderReflection>& reflection);
    inline IDxcBlob* getBlobPtr() { return shaderBlob.Get(); }
#elif USE_LEGACY_D3DCOMPILER
    ShaderModule(
            ShaderModuleType shaderModuleType, ComPtr<ID3DBlob> shaderBlob,
            const ComPtr<ID3D12ShaderReflection>& reflection);
    inline ID3DBlob* getBlobPtr() { return shaderBlob.Get(); }
#endif
    LPVOID getBlobBufferPointer();
    SIZE_T getBlobBufferSize();
    [[nodiscard]] inline uint32_t getThreadGroupSizeX() const { return threadGroupSizeX; }
    [[nodiscard]] inline uint32_t getThreadGroupSizeY() const { return threadGroupSizeY; }
    [[nodiscard]] inline uint32_t getThreadGroupSizeZ() const { return threadGroupSizeZ; }

    bool hasBindingName(const std::string& name);
    const ShaderBindingInfo& getBindingInfoByName(const std::string& name);
    bool hasVarName(const std::string& name);
    const ShaderVarInfo& getVarInfoByName(const std::string& name);

private:
    void queryReflectionData(const ComPtr<ID3D12ShaderReflection>& reflection);
    ShaderModuleType shaderModuleType;
#ifdef SUPPORT_D3D_COMPILER
    ComPtr<IDxcBlob> shaderBlob;
#elif USE_LEGACY_D3DCOMPILER
    ComPtr<ID3DBlob> shaderBlob;
#endif

    union {
        // Vertex/fragment shader data.
        struct {
            uint32_t numInputs;
            uint32_t numOutputs;
        };
        // Compute shader data.
        struct {
            uint32_t threadGroupSizeX;
            uint32_t threadGroupSizeY;
            uint32_t threadGroupSizeZ;
        };
    };

    std::unordered_map<std::string, ShaderBindingInfo> bindingNameToInfoMap;
    std::unordered_map<std::string, ShaderVarInfo> variableNameToInfoMap;
};

typedef std::shared_ptr<ShaderModule> ShaderModulePtr;

}}

#endif //SGL_D3D12_SHADER_HPP
