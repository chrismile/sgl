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
#include <Utils/File/FileLoader.hpp>

#include "Shader.hpp"
#include "ShaderManager.hpp"

#ifdef SUPPORT_D3D_COMPILER
#include <dxcapi.h>
#elif USE_LEGACY_D3DCOMPILER
#include <d3dcompiler.h>
#endif

namespace sgl { namespace d3d12 {

#ifdef SUPPORT_D3D_COMPILER
inline const wchar_t* getShaderModuleTypeTarget(ShaderModuleType shaderModuleType) {
    if (shaderModuleType == ShaderModuleType::VERTEX) {
        return L"vs_6_0";
    } else if (shaderModuleType == ShaderModuleType::GEOMETRY) {
        return L"gs_6_0";
    } else if (shaderModuleType == ShaderModuleType::PIXEL) {
        return L"ps_6_0";
    } else if (shaderModuleType == ShaderModuleType::COMPUTE) {
        return L"cs_6_0";
    } else {
        sgl::Logfile::get()->throwError("Error in getShaderModuleTypeTarget: Unsupported shader module type.");
        return L"";
    }
}
#elif USE_LEGACY_D3DCOMPILER
inline const char* getShaderModuleTypeTarget(ShaderModuleType shaderModuleType) {
    if (shaderModuleType == ShaderModuleType::VERTEX) {
        return "vs_5_0";
    } else if (shaderModuleType == ShaderModuleType::GEOMETRY) {
        return "gs_5_0";
    } else if (shaderModuleType == ShaderModuleType::PIXEL) {
        return "ps_5_0";
    } else if (shaderModuleType == ShaderModuleType::COMPUTE) {
        return "cs_5_0";
    } else {
        sgl::Logfile::get()->throwError("Error in getShaderModuleTypeTarget: Unsupported shader module type.");
        return "";
    }
}
#endif

#ifdef USE_LEGACY_D3DCOMPILER
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
#endif

ShaderManagerD3D12::ShaderManagerD3D12() {
#ifdef SUPPORT_D3D_COMPILER
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    if (FAILED(hr)) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerD3D12::ShaderManagerD3D12: Could not create DxcUtils object.");
    }
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr)) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerD3D12::ShaderManagerD3D12: Could not create DxcCompiler object.");
    }
#endif
}

ShaderManagerD3D12::~ShaderManagerD3D12() = default;

ShaderModulePtr ShaderManagerD3D12::loadShaderFromBlobFile(const std::string& shaderPath, ShaderModuleType shaderModuleType) {
#ifdef SUPPORT_D3D_COMPILER
    uint8_t* buffer = nullptr;
    size_t bufferSize = 0;
    if (!loadFileFromSource(shaderPath, buffer, bufferSize, true)) {
        return {};
    }

    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = utils->CreateBlob(buffer, uint32_t(bufferSize), DXC_CP_ACP, sourceBlob.GetAddressOf());
    if(FAILED(hr)) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderManagerD3D12::loadShaderFromHlslFile: CreateBlob failed.");
        return {};
    }
    auto shaderModule = loadShaderFromSourceBlob(
            sourceBlob, shaderPath, shaderModuleType, "", {});

    delete[] buffer;
    return shaderModule;
#elif USE_LEGACY_D3DCOMPILER
    auto shaderPathWideString = stdStringToWideString(shaderPath);
    ComPtr<ID3DBlob> shaderBlob{};
    ThrowIfFailed(D3DReadFileToBlob(shaderPathWideString.c_str(), &shaderBlob));
    return std::make_shared<ShaderModule>(shaderModuleType, shaderBlob, createReflectionData(shaderBlob));
#else
    sgl::Logfile::get()->throwError(
            "Error in ShaderManagerD3D12::loadShaderFromBlobFile: D3D compiler was not enabled during the build.");
    return {};
#endif
}

ShaderModulePtr ShaderManagerD3D12::loadShaderFromHlslFile(
        const std::string& shaderPath, ShaderModuleType shaderModuleType, const std::string& entrypoint,
        const std::map<std::string, std::string>& preprocessorDefines) {
#ifdef SUPPORT_D3D_COMPILER
    uint8_t* buffer = nullptr;
    size_t bufferSize = 0;
    if (!loadFileFromSource(shaderPath, buffer, bufferSize, false)) {
        return {};
    }

    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = utils->CreateBlob(buffer, uint32_t(bufferSize), CP_UTF8, sourceBlob.GetAddressOf());
    if(FAILED(hr)) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderManagerD3D12::loadShaderFromHlslFile: CreateBlob failed.");
        return {};
    }
    auto shaderModule = loadShaderFromSourceBlob(
            sourceBlob, shaderPath, shaderModuleType, entrypoint, preprocessorDefines);

    delete[] buffer;
    return shaderModule;
#elif USE_LEGACY_D3DCOMPILER
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
                    "Error in ShaderManagerD3D12::loadShaderFromHlslFile: Unknown HLSL compilation failure.");
        }
        return {};
    }
    return std::make_shared<ShaderModule>(shaderModuleType, shaderBlob, createReflectionData(shaderBlob));
#else
    sgl::Logfile::get()->throwError(
            "Error in ShaderManagerD3D12::loadShaderFromHlslFile: D3D compiler was not enabled during the build.");
    return {};
#endif
}

ShaderModulePtr ShaderManagerD3D12::loadShaderFromHlslString(
        const std::string& shaderString, const std::string& shaderName,
        ShaderModuleType shaderModuleType, const std::string& entrypoint,
        const std::map<std::string, std::string>& preprocessorDefines) {
#ifdef SUPPORT_D3D_COMPILER
    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = utils->CreateBlob(
            shaderString.data(), uint32_t(shaderString.size()), CP_UTF8, sourceBlob.GetAddressOf());
    if(FAILED(hr)) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderManagerD3D12::loadShaderFromHlslString: CreateBlob failed.");
        return {};
    }
    return loadShaderFromSourceBlob(sourceBlob, shaderName, shaderModuleType, entrypoint, preprocessorDefines);
#elif USE_LEGACY_D3DCOMPILER
    std::vector<D3D_SHADER_MACRO> shaderMacros = getShaderMacros(preprocessorDefines);
    D3D_SHADER_MACRO* shaderMacroPtr = nullptr;
    if (!shaderMacros.empty()) {
        shaderMacroPtr = shaderMacros.data();
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
                    "Error in ShaderManagerD3D12::loadShaderFromHlslString: Unknown HLSL compilation failure.");
        }
        return {};
    }
    return std::make_shared<ShaderModule>(shaderModuleType, shaderBlob, createReflectionData(shaderBlob));
#else
    sgl::Logfile::get()->throwError(
            "Error in ShaderManagerD3D12::loadShaderFromHlslString: D3D compiler was not enabled during the build.");
    return {};
#endif
}

#ifdef SUPPORT_D3D_COMPILER
ShaderModulePtr ShaderManagerD3D12::loadShaderFromSourceBlob(
        const ComPtr<IDxcBlobEncoding>& sourceBlob, const std::string& shaderName,
        ShaderModuleType shaderModuleType, const std::string& entrypoint,
        const std::map<std::string, std::string>& preprocessorDefines) {
    std::vector<DxcDefine> shaderDefines;
    std::vector<std::wstring> shaderDefineNames;
    std::vector<std::wstring> shaderDefineValues;
    DxcDefine* shaderDefinesPtr = nullptr;
    if (!shaderDefines.empty()) {
        shaderDefines.resize(preprocessorDefines.size());
        shaderDefineNames.resize(preprocessorDefines.size());
        shaderDefineNames.resize(preprocessorDefines.size());
        int i = 0;
        for (const auto& preprocessorDefine : preprocessorDefines) {
            shaderDefineNames[i] = sgl::stdStringToWideString(preprocessorDefine.first);
            shaderDefineValues[i] = sgl::stdStringToWideString(preprocessorDefine.second);
            shaderDefines[i].Name = shaderDefineNames[i].c_str();
            shaderDefines[i].Value = shaderDefineValues[i].c_str();
            i++;
        }
        shaderDefinesPtr = shaderDefines.data();
    }

    auto shaderNameWideString = sgl::stdStringToWideString(shaderName);
    const wchar_t* shaderNamePtr = nullptr;
    if (!shaderNameWideString.empty()) {
        shaderNamePtr = shaderNameWideString.data();
    }

    auto entrypointWideString = sgl::stdStringToWideString(entrypoint);
    const wchar_t* entrypointPtr = nullptr;
    if (!shaderNameWideString.empty()) {
        entrypointPtr = entrypointWideString.data();
    }

    const wchar_t* target = getShaderModuleTypeTarget(shaderModuleType);

    ComPtr<IDxcOperationResult> result;
    HRESULT hr = compiler->Compile(
            sourceBlob.Get(),
            shaderNamePtr,
            entrypointPtr,
            target,
            nullptr, 0, // arguments
            shaderDefinesPtr, uint32_t(shaderDefines.size()),
            nullptr, // include handler
            &result);
    if(SUCCEEDED(hr)) {
        result->GetStatus(&hr);
    }
    if(FAILED(hr)) {
        if(result) {
            ComPtr<IDxcBlobEncoding> errorsBlob;
            hr = result->GetErrorBuffer(&errorsBlob);
            if (SUCCEEDED(hr) && errorsBlob) {
                auto errorString = static_cast<const char*>(errorsBlob->GetBufferPointer());
                sgl::Logfile::get()->writeErrorMultiline(errorString, false);
                auto choice = dialog::openMessageBoxBlocking(
                        "Error occurred", errorString, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
                if (choice == dialog::Button::RETRY) {
                    //sgl::d3d12::ShaderManager->invalidateShaderCache();
                    // TODO
                    return {};
                } else if (choice == dialog::Button::ABORT) {
                    exit(1);
                }
            } else {
                sgl::Logfile::get()->writeError(
                        "Error in ShaderManagerD3D12::loadShaderFromSourceBlob: "
                        "Unknown HLSL compilation failure (no error blob).");
            }
        } else {
            sgl::Logfile::get()->writeError(
                    "Error in ShaderManagerD3D12::loadShaderFromSourceBlob: "
                    "Unknown HLSL compilation failure (no result).");
        }
        return {};
    }
    ComPtr<IDxcBlob> shaderBlob;
    hr = result->GetResult(&shaderBlob);
    if (FAILED(hr)) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderManagerD3D12::loadShaderFromSourceBlob: GetResult failed.");
    }

    return std::make_shared<ShaderModule>(shaderModuleType, shaderBlob, createReflectionData(shaderBlob));
}

ComPtr<ID3D12ShaderReflection> ShaderManagerD3D12::createReflectionData(const ComPtr<IDxcBlob>& shaderBlob) {
    ComPtr<ID3D12ShaderReflection> reflection;
    DxcBuffer shaderBuffer{};
    shaderBuffer.Ptr = shaderBlob->GetBufferPointer();
    shaderBuffer.Size = shaderBlob->GetBufferSize();
    shaderBuffer.Encoding = 0;
    HRESULT hr = utils->CreateReflection(&shaderBuffer, IID_PPV_ARGS(&reflection));
    if (FAILED(hr)) {
        sgl::Logfile::get()->throwError("Error in ShaderManagerD3D12::createReflectionData: CreateReflection failed.");
    }
    return reflection;
}
#elif USE_LEGACY_D3DCOMPILER
ComPtr<ID3D12ShaderReflection> ShaderManagerD3D12::createReflectionData(const ComPtr<ID3DBlob>& shaderBlob) {
    ComPtr<ID3D12ShaderReflection> reflection;
    HRESULT hr = D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&reflection));
    if (FAILED(hr)) {
        sgl::Logfile::get()->throwError("Error in ShaderManagerD3D12::createReflectionData: D3DReflect failed.");
    }
    return reflection;
}
#endif

}}
