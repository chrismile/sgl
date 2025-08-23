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

#include <Utils/StringUtils.hpp>
#include <Utils/Dialog.hpp>

#include "Shader.hpp"
#include "ShaderManager.hpp"

#ifdef SUPPORT_D3D_COMPILER
#include <d3dcompiler.h>
#endif

namespace sgl { namespace d3d12 {

inline const char* getShaderModuleTypeTarget(ShaderModuleType shaderModuleType) {
    if (shaderModuleType == ShaderModuleType::VERTEX) {
        return "vs_5_1";
    } else if (shaderModuleType == ShaderModuleType::GEOMETRY) {
        return "gs_5_1";
    } else if (shaderModuleType == ShaderModuleType::PIXEL) {
        return "ps_5_1";
    } else if (shaderModuleType == ShaderModuleType::COMPUTE) {
        return "cs_5_1";
    } else {
        sgl::Logfile::get()->throwError("Error in getShaderModuleTypeTarget: Unsupported shader module type.");
        return "";
    }
}

inline std::vector<D3D_SHADER_MACRO> getShaderMacros(const std::map<std::string, std::string>& preprocessorDefines) {
    std::vector<D3D_SHADER_MACRO> shaderMacros{};
    if (!preprocessorDefines.empty()) {
        shaderMacros.resize(preprocessorDefines.size());
        int i = 0;
        for (const auto& preprocessorDefine : preprocessorDefines) {
            shaderMacros[i].Name = preprocessorDefine.first.c_str();
            shaderMacros[i].Definition = preprocessorDefine.second.c_str();
            i++;
        }
    }
    return shaderMacros;
}

ShaderModulePtr ShaderManagerD3D12::loadShaderFromBlobFile(const std::string& shaderPath, ShaderModuleType shaderModuleType) {
#ifdef SUPPORT_D3D_COMPILER
    auto shaderPathWideString = stdStringToWideString(shaderPath);
    ComPtr<ID3DBlob> shaderBlob{};
    ThrowIfFailed(D3DReadFileToBlob(shaderPathWideString.c_str(), &shaderBlob));
    return std::make_shared<ShaderModule>(shaderModuleType, shaderBlob);
#else
    sgl::Logfile::get()->throwError(
            "Error in ShaderManager::loadShaderFromBlobFile: D3D compiler was not enabled during the build.");
    return {};
#endif
}

ShaderModulePtr ShaderManagerD3D12::loadShaderFromHlslFile(
        const std::string& shaderPath, ShaderModuleType shaderModuleType, const std::string& entrypoint,
        const std::map<std::string, std::string>& preprocessorDefines) {
#ifdef SUPPORT_D3D_COMPILER
    std::vector<D3D_SHADER_MACRO> shaderMacros = getShaderMacros(preprocessorDefines);
    D3D_SHADER_MACRO* shaderMacroPtr = nullptr;
    if (!shaderMacros.empty()) {
        shaderMacros.resize(preprocessorDefines.size());
    }
    const char* target = getShaderModuleTypeTarget(shaderModuleType);

    auto shaderPathWideString = stdStringToWideString(shaderPath);
    ComPtr<ID3DBlob> shaderBlob{};
    ComPtr<ID3DBlob> errorMessagesBlob{};
    auto hr = D3DCompileFromFile(
            shaderPathWideString.c_str(), shaderMacroPtr, nullptr,
            entrypoint.c_str(), target, 0, 0,
            shaderBlob.GetAddressOf(), errorMessagesBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errorMessagesBlob) {
            auto errorString = static_cast<const char*>(errorMessagesBlob->GetBufferPointer());
            sgl::Logfile::get()->writeErrorMultiline(errorString, false);
            auto choice = dialog::openMessageBoxBlocking(
                    "Error occurred", errorString, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
            if (choice == dialog::Button::RETRY) {
                //sgl::d3d12::ShaderManager->invalidateShaderCache();
                return loadShaderFromHlslFile(
                        shaderPath, shaderModuleType, entrypoint, preprocessorDefines);
            } else if (choice == dialog::Button::ABORT) {
                exit(1);
            }
        } else {
            sgl::Logfile::get()->writeError(
                    "Error in ShaderManager::loadShaderFromHlslFile: Unknown HLSL compilation failure.");
        }
        return {};
    }
    return std::make_shared<ShaderModule>(shaderModuleType, shaderBlob);
#else
    sgl::Logfile::get()->throwError(
            "Error in ShaderManager::loadShaderFromHlslFile: D3D compiler was not enabled during the build.");
    return {};
#endif
}

ShaderModulePtr ShaderManagerD3D12::loadShaderFromHlslString(
            const std::string& shaderString, ShaderModuleType shaderModuleType, const std::string& entrypoint,
            const std::map<std::string, std::string>& preprocessorDefines) {
#ifdef SUPPORT_D3D_COMPILER
    std::vector<D3D_SHADER_MACRO> shaderMacros = getShaderMacros(preprocessorDefines);
    D3D_SHADER_MACRO* shaderMacroPtr = nullptr;
    if (!shaderMacros.empty()) {
        shaderMacros.resize(preprocessorDefines.size());
    }
    const char* target = getShaderModuleTypeTarget(shaderModuleType);

    ComPtr<ID3DBlob> shaderBlob{};
    ComPtr<ID3DBlob> errorMessagesBlob{};
    auto hr = D3DCompile(
            shaderString.data(), shaderString.size(), nullptr /* name */, shaderMacroPtr, nullptr,
            entrypoint.c_str(), target, 0, 0,
            shaderBlob.GetAddressOf(), errorMessagesBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errorMessagesBlob) {
            auto errorString = static_cast<const char*>(errorMessagesBlob->GetBufferPointer());
            sgl::Logfile::get()->writeErrorMultiline(errorString, false);
            auto choice = dialog::openMessageBoxBlocking(
                    "Error occurred", errorString, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
            if (choice == dialog::Button::RETRY) {
                //sgl::d3d12::ShaderManager->invalidateShaderCache();
                return loadShaderFromHlslString(
                        shaderString, shaderModuleType, entrypoint, preprocessorDefines);
            } else if (choice == dialog::Button::ABORT) {
                exit(1);
            }
        } else {
            sgl::Logfile::get()->writeError(
                    "Error in ShaderManager::loadShaderFromHlslString: Unknown HLSL compilation failure.");
        }
        return {};
    }
    return std::make_shared<ShaderModule>(shaderModuleType, shaderBlob);
#else
    sgl::Logfile::get()->throwError(
            "Error in ShaderManager::loadShaderFromHlslString: D3D compiler was not enabled during the build.");
    return {};
#endif
}

}}
