/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_WGSLREFLECT_HPP
#define SGL_WGSLREFLECT_HPP

#include <vector>
#include <map>
#include <string>
#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

enum class BindingEntryType : uint32_t {
    UNKNOWN = 0, UNIFORM_BUFFER, TEXTURE, SAMPLER, STORAGE_BUFFER, STORAGE_TEXTURE
};
// Only for BindingEntryType::STORAGE_BUFFER and BindingEntryType::STORAGE_TEXTURE.
enum class StorageModifier : uint32_t {
    UNKNOWN = 0, READ = 1, WRITE = 2, READ_WRITE = 3
};

struct DLL_OBJECT BindingEntry {
    uint32_t bindingIndex;
    std::string variableName;
    std::string typeName;
    std::vector<std::string> modifiers;
    BindingEntryType bindingEntryType;
    // Only for BindingEntryType::STORAGE_BUFFER and BindingEntryType::STORAGE_TEXTURE.
    StorageModifier storageModifier;
};

struct DLL_OBJECT InOutEntry {
    uint32_t locationIndex;
    std::string variableName;
    WGPUVertexFormat vertexFormat;
};

enum class ShaderType : uint32_t {
    VERTEX, FRAGMENT, COMPUTE
};

struct DLL_OBJECT ShaderInfo {
    ShaderType shaderType;
    std::vector<InOutEntry> inputs; // Only reported for vertex shaders.
    std::vector<InOutEntry> outputs; // Only reported for fragment shaders.
};

struct DLL_OBJECT ReflectInfo {
    std::map<uint32_t, std::vector<BindingEntry>> bindingGroups;
    // Maps from entry point to shader info.
    std::map<std::string, ShaderInfo> shaders;
};

/**
 * Creates reflection information about the content of a WGSL shader file.
 * @param fileContent The content of the shader file.
 * @param reflectInfo The information about the interface of the shaders in the file.
 * @param errorString Used for storing the error string if the function failed and returns false.
 * @return Whether the extraction of the reflect info was successful.
 */
DLL_OBJECT bool wgslCodeReflect(const std::string& fileContent, ReflectInfo& reflectInfo, std::string& errorString);

}}

#endif //SGL_WGSLREFLECT_HPP
