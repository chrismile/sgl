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

#ifndef SGL_D3D12_SHADERMANAGER_HPP
#define SGL_D3D12_SHADERMANAGER_HPP

#include <map>

#include "../Utils/d3d12.hpp"
#include "ShaderModuleType.hpp"

struct IDxcUtils;
struct IDxcCompiler;
struct IDxcBlob;
struct IDxcBlobEncoding;

namespace sgl { namespace d3d12 {

class Device;
class ShaderModule;
typedef std::shared_ptr<ShaderModule> ShaderModulePtr;

//struct ShaderModuleSettings {
//    std::string entryName = "main";
//};

class DLL_OBJECT ShaderManagerD3D12 {
public:
    ShaderManagerD3D12();
    ~ShaderManagerD3D12();
    ShaderModulePtr loadShaderFromBlobFile(const std::string& shaderPath, ShaderModuleType shaderModuleType);
    ShaderModulePtr loadShaderFromHlslFile(
            const std::string& shaderPath, ShaderModuleType shaderModuleType, const std::string& entrypoint,
            const std::map<std::string, std::string>& preprocessorDefines = {});
    ShaderModulePtr loadShaderFromHlslString(
            const std::string& shaderString, const std::string& shaderName,
            ShaderModuleType shaderModuleType, const std::string& entrypoint,
            const std::map<std::string, std::string>& preprocessorDefines = {});
    //    ShaderStagesPtr getShaderStagesWithSettings(
    //            const std::vector<std::string>& shaderIds,
    //            const std::map<std::string, std::string>& customPreprocessorDefines,
    //            const std::vector<ShaderStageSettings>& settings, bool dumpTextDebug = false);

private:
#ifdef SUPPORT_D3D_COMPILER
    ShaderModulePtr loadShaderFromSourceBlob(
            const ComPtr<IDxcBlobEncoding>& sourceBlob, const std::string& shaderName,
            ShaderModuleType shaderModuleType, const std::string& entrypoint,
            const std::map<std::string, std::string>& preprocessorDefines = {});
    ComPtr<ID3D12ShaderReflection> createReflectionData(const ComPtr<IDxcBlob>& shaderBlob);
    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcCompiler> compiler;
#elif USE_LEGACY_D3DCOMPILER
    ComPtr<ID3D12ShaderReflection> createReflectionData(const ComPtr<ID3DBlob>& shaderBlob);
#endif
};

DLL_OBJECT extern ShaderManagerD3D12* ShaderManager;

}}

#endif //SGL_D3D12_SHADERMANAGER_HPP
