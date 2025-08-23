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

#include "Shader.hpp"

#include <utility>

#ifdef SUPPORT_D3D_COMPILER
#include <d3dcompiler.h>
#endif

namespace sgl { namespace d3d12 {

ShaderModule::ShaderModule(ShaderModuleType shaderModuleType, ComPtr<ID3DBlob> shaderBlob)
        : shaderModuleType(shaderModuleType), shaderBlob(std::move(shaderBlob)) {
#ifdef SUPPORT_D3D_COMPILER
    ComPtr<ID3D12ShaderReflection> reflection;
    HRESULT hr = D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&reflection));
    if (FAILED(hr)) {
        sgl::Logfile::get()->throwError("Error in ShaderModule::ShaderModule: D3DReflect failed.");
    }

    if (shaderModuleType == ShaderModuleType::COMPUTE) {
        reflection->GetThreadGroupSize(&threadGroupSizeX, &threadGroupSizeY, &threadGroupSizeZ);
    }

    // TODO
    /*D3D12_SHADER_DESC desc{};
    reflection->GetDesc(&desc);
    desc.ConstantBuffers;*/
#else
    sgl::Logfile::get()->throwError("Error in ShaderModule::ShaderModule: D3D shader compiler not supported.");
#endif
}

}}
