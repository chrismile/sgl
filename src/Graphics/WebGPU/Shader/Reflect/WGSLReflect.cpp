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

#include <set>
#include <algorithm>

#include <Utils/Convert.hpp>
#include <Utils/StringUtils.hpp>

#include "WGSLReflectInternal.hpp"
#include "WGSLReflect.hpp"

namespace sgl { namespace webgpu {

static const std::set<std::string> builtinTypes = {
        "bool", "f16", "f32", "f64", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
        "vec2", "vec3", "vec4", // < with templates.
        "vec2i", "vec3i", "vec4i", "vec2u", "vec3u", "vec4u",
        "vec2f", "vec3f", "vec4f", "vec2h", "vec3h", "vec4h",
        "mat2x2f", "mat2x3f", "mat2x4f", "mat3x2f", "mat3x3f", "mat3x4f", "mat4x2f", "mat4x3f", "mat4x4f",
        "mat2x2h", "mat2x3h", "mat2x4h", "mat3x2h", "mat3x3h", "mat3x4h", "mat4x2h", "mat4x3h", "mat4x4h",
        "ref", "pointer", "atomic", //< With templates.
        "array" // <E,N> for fixed-size N or <E> for runtime-sized array.
};
static bool isTypeBuiltin(const wgsl_type& type) {
    return builtinTypes.find(type.name) != builtinTypes.end();
}

/**
 * @param attributes The set of attributes.
 * @param name The name of the searched attribute.
 * @return A pointer to the attribute if found or a null pointer otherwise.
 */
static const wgsl_attribute* findAttributeByName(
        const std::vector<wgsl_attribute>& attributes, const std::string& name) {
    for (const auto& attribute : attributes) {
        if (attribute.name == name) {
            return &attribute;
        }
    }
    return nullptr;
}

}}

namespace sgl { namespace webgpu {

// Completely removes comments.
/*std::string removeCStyleComments(const std::string& stringWithComments) {
    bool isInLineComment = false;
    bool isInMultiLineComment = false;
    size_t strSize = stringWithComments.size();
    std::string stringWithoutComments;
    std::string::size_type currPos = 0;
    while (currPos < strSize) {
        char c0 = stringWithComments.at(currPos);
        char c1 = currPos + 1 < strSize ? stringWithComments.at(currPos + 1) : '\0';
        if (isInLineComment) {
            if (c0 == '\r' && c1 == '\n') {
                isInLineComment = false;
                currPos += 2;
            } else if (c0 == '\r' || c0 == '\n') {
                isInLineComment = false;
                currPos += 1;
            } else {
                currPos += 1;
            }
        } else if (isInMultiLineComment) {
            if (c0 == '*' && c1 == '/') {
                isInMultiLineComment = false;
                currPos += 2;
            } else {
                currPos += 1;
            }
        } else {
            if (c0 == '/' && c1 == '*') {
                isInMultiLineComment = true;
                currPos += 2;
            } else if (c0 == '/' && c1 == '/') {
                isInLineComment = true;
                currPos += 2;
            } else {
                stringWithoutComments += c0;
                currPos += 1;
            }
        }
    }
    return stringWithoutComments;
}*/

// Replaces any char in comment with space. This leaves char offsets valid.
std::string removeCStyleComments(const std::string& stringWithComments) {
    bool isInLineComment = false;
    bool isInMultiLineComment = false;
    size_t strSize = stringWithComments.size();
    std::string stringWithoutComments;
    std::string::size_type currPos = 0;
    while (currPos < strSize) {
        char c0 = stringWithComments.at(currPos);
        char c1 = currPos + 1 < strSize ? stringWithComments.at(currPos + 1) : '\0';
        if (isInLineComment) {
            if (c0 == '\r' && c1 == '\n') {
                isInLineComment = false;
                stringWithoutComments += "\r\n";
                currPos += 2;
            } else if (c0 == '\r' || c0 == '\n') {
                isInLineComment = false;
                stringWithoutComments += c0;
                currPos += 1;
            } else {
                stringWithoutComments += ' ';
                currPos += 1;
            }
        } else if (isInMultiLineComment) {
            if (c0 == '*' && c1 == '/') {
                isInMultiLineComment = false;
                stringWithoutComments += "  ";
                currPos += 2;
            } else {
                if (c0 == '\r' || c0 == '\n') {
                    stringWithoutComments += c0;
                } else {
                    stringWithoutComments += ' ';
                }
                currPos += 1;
            }
        } else {
            if (c0 == '/' && c1 == '*') {
                isInMultiLineComment = true;
                stringWithoutComments += "  ";
                currPos += 2;
            } else if (c0 == '/' && c1 == '/') {
                isInLineComment = true;
                stringWithoutComments += "  ";
                currPos += 2;
            } else {
                stringWithoutComments += c0;
                currPos += 1;
            }
        }
    }
    return stringWithoutComments;
}

/// Converts wgsl_entry objects to maps.
class wgsl_reflect_visitor : public boost::static_visitor<> {
public:
    wgsl_reflect_visitor(
            std::map<std::string, wgsl_struct>& structs,
            std::map<std::string, wgsl_variable>& variables,
            std::map<std::string, wgsl_constant>& constants,
            std::map<std::string, wgsl_function>& functions,
            std::map<std::string, std::set<std::string>>& directives)
            : structs(structs), variables(variables), constants(constants),
              functions(functions), directives(directives) {
    }
    void operator()(const wgsl_struct& _struct) const {
        structs.insert(std::make_pair(_struct.name, _struct));
    }
    void operator()(const wgsl_variable& variable) const {
        variables.insert(std::make_pair(variable.name, variable));
    }
    void operator()(const wgsl_constant& constant) const {
        constants.insert(std::make_pair(constant.name, constant));
    }
    void operator()(const wgsl_function& function) const {
        functions.insert(std::make_pair(function.name, function));
    }
    void operator()(const wgsl_directive& directive) const {
        for (const std::string& value : directive.values) {
            directives[directive.directive_type].insert(value);
        }
    }

private:
    std::map<std::string, wgsl_struct>& structs;
    std::map<std::string, wgsl_variable>& variables;
    std::map<std::string, wgsl_constant>& constants;
    std::map<std::string, wgsl_function>& functions;
    std::map<std::string, std::set<std::string>>& directives;
};

/**
 * Retrieves information about the line containing the char with the passed index.
 */
void getLineInfo(
        const std::string& fileContent, size_t charIdx, std::string& lineStr, size_t& lineIdx, size_t& charIdxInLine) {
    auto lineStart = fileContent.rfind('\n', charIdx);
    auto lineEnd = fileContent.find('\n', charIdx);
    if (lineStart == std::string::npos) {
        lineStart = 0;
    } else {
        lineStart++;
    }
    if (lineEnd == std::string::npos) {
        lineEnd = fileContent.size();
    }
    lineStr = fileContent.substr(lineStart, lineEnd - lineStart);
    charIdxInLine = charIdx - lineStart;
    lineIdx = size_t(std::count(fileContent.begin(), fileContent.begin() + charIdx, '\n')) + 1;
}

static const std::unordered_map<std::string, WGPUVertexFormat> typeNameVertexFormatMap = {
        { "f32", WGPUVertexFormat_Float32 },
        { "vec2f", WGPUVertexFormat_Float32x2 },
        { "vec3f", WGPUVertexFormat_Float32x3 },
        { "vec4f", WGPUVertexFormat_Float32x4 },
        { "i32", WGPUVertexFormat_Sint32 },
        { "vec2i", WGPUVertexFormat_Sint32x2 },
        { "vec3i", WGPUVertexFormat_Sint32x3 },
        { "vec4i", WGPUVertexFormat_Sint32x4 },
        { "u32", WGPUVertexFormat_Uint32 },
        { "vec2u", WGPUVertexFormat_Uint32x2 },
        { "vec3u", WGPUVertexFormat_Uint32x3 },
        { "vec4u", WGPUVertexFormat_Uint32x4 },
        { "vec2h", WGPUVertexFormat_Float16x2 },
        { "vec4h", WGPUVertexFormat_Float16x4 },
};
WGPUVertexFormat WGSLTypeToWGPUVertexFormat(const wgsl_type& type, std::string& errorString) {
    std::unordered_map<std::string, WGPUVertexFormat>::const_iterator it;
    if (type.name == "vec2" || type.name == "vec3" || type.name == "vec4") {
        std::string vecTypeName = type.name;
        if (!type.template_parameters.has_value()) {
            errorString = "Vector vertex format without type template parameter.";
            return WGPUVertexFormat_Undefined;
        }
        const std::vector<wgsl_type>& templateParameters = type.template_parameters.get();
        if (templateParameters.size() != 1) {
            errorString = "Vector vertex format with incorrect number of template parameters.";
            return WGPUVertexFormat_Undefined;
        }
        const std::string& templateTypeName = templateParameters.front().name;
        if (templateTypeName == "f32") {
            vecTypeName += "f";
        } else if (templateTypeName == "i32") {
            vecTypeName += "i";
        } else if (templateTypeName == "u32") {
            vecTypeName += "u";
        } else {
            errorString = "Vector vertex format with unsupported template parameter \"" + templateTypeName + "\".";
            return WGPUVertexFormat_Undefined;
        }
        it = typeNameVertexFormatMap.find(vecTypeName);
    } else {
        it = typeNameVertexFormatMap.find(type.name);
    }
    type.template_parameters->size();
    if (it == typeNameVertexFormatMap.end()) {
        errorString = "Could not match type \"" + type.name + "\" to a vertex format.";
        return WGPUVertexFormat_Undefined;
    }
    return it->second;
}

/// Processes vertex shader inputs (via function parameters) or fragment shader outputs (via return type).
bool processInOut(
        const std::map<std::string, wgsl_struct>& structs, std::string& errorString,
        const std::string& entryPointName, const std::string& inOutName,
        std::vector<InOutEntry>& inOutEntries, const wgsl_type& type, const std::vector<wgsl_attribute>& attributes) {
    // Is this a built-in type? If yes, check if it has the "location" attribute.
    if (isTypeBuiltin(type)) {
        const auto* locationAttribute = findAttributeByName(attributes, "location");
        if (locationAttribute) {
            InOutEntry inOutEntry{};
            inOutEntry.variableName = inOutName;
            inOutEntry.locationIndex = sgl::fromString<uint32_t>(locationAttribute->expression);
            inOutEntry.vertexFormat = WGSLTypeToWGPUVertexFormat(type, errorString);
            if (inOutEntry.vertexFormat == WGPUVertexFormat_Undefined) {
                return false;
            }
            inOutEntries.push_back(inOutEntry);
        }
        return true;
    }
    // If this is not a built-in type, it must be a valid struct type.
    auto itStruct = structs.find(type.name);
    if (itStruct == structs.end()) {
        errorString =
                "Found unresolved type \"" + type.name
                + "\" when parsing \"" + entryPointName + "\".";
        return false;
    }
    const auto& _struct = itStruct->second;

    // Iterate over all struct type entries and check if they have the "location" attribute set.
    for (const auto& entry : _struct.entries) {
        if (isTypeBuiltin(entry.type)) {
            const auto* locationAttribute = findAttributeByName(entry.attributes, "location");
            if (locationAttribute) {
                InOutEntry inOutEntry{};
                inOutEntry.variableName = entry.name;
                inOutEntry.locationIndex = sgl::fromString<uint32_t>(locationAttribute->expression);
                inOutEntry.vertexFormat = WGSLTypeToWGPUVertexFormat(entry.type, errorString);
                if (inOutEntry.vertexFormat == WGPUVertexFormat_Undefined) {
                    return false;
                }
                inOutEntries.push_back(inOutEntry);
            }
            continue;
        }
    }
    return true;
}

/// Returns the binding group entry type for a given WGSL type and "var" modifier.
static BindingEntryType getBindingEntryType(
        const wgsl_type& type, const boost::optional<std::vector<std::string>>& modifiersOpt) {
    // UNIFORM_BUFFER, TEXTURE, SAMPLER, STORAGE_BUFFER, STORAGE_TEXTURE
    BindingEntryType bindingEntryType = BindingEntryType::UNKNOWN;
    if (modifiersOpt.has_value()) {
        const auto& modifiers = modifiersOpt.get();
        if (std::find(modifiers.begin(), modifiers.end(), "uniform") != modifiers.end()) {
            // Example: var<uniform> settings: Settings;
            bindingEntryType = BindingEntryType::UNIFORM_BUFFER;
        } else if (std::find(modifiers.begin(), modifiers.end(), "storage") != modifiers.end()) {
            // Example: @group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
            // Example: @group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,64>;
            bindingEntryType = BindingEntryType::STORAGE_BUFFER;
        }
    } else {
        if (sgl::startsWith(type.name, "texture_storage_")) {
            // Example: @group(0) @binding(1) var nextMipLevel: texture_storage_2d<rgba8unorm,write>;
            bindingEntryType = BindingEntryType::STORAGE_TEXTURE;
        } else if (sgl::startsWith(type.name, "texture_")) {
            // Example: @group(0) @binding(1) var gradientTexture: texture_2d<f32>;
            bindingEntryType = BindingEntryType::TEXTURE;
        } else if (type.name == "sampler") {
            // Example: @group(0) @binding(2) var textureSampler: sampler;
            bindingEntryType = BindingEntryType::SAMPLER;
        }
    }
    return bindingEntryType;
}

bool wgslCodeReflect(const std::string& fileContent, ReflectInfo& reflectInfo, std::string& errorString) {
    std::string fileContentNoComments = removeCStyleComments(fileContent);

    // The AST.
    wgsl_content content{};
    //if (!wgslReflectParseQi(fileContentNoComments, content, errorString)) {
    //    return false;
    //}
    if (!wgslReflectParseX3(fileContentNoComments, content, errorString)) {
        return false;
    }

    std::map<std::string, wgsl_struct> structs;
    std::map<std::string, wgsl_variable> variables;
    std::map<std::string, wgsl_constant> constants;
    std::map<std::string, wgsl_function> functions;
    std::map<std::string, std::set<std::string>> directives;
    wgsl_reflect_visitor visitor(structs, variables, constants, functions, directives);
    for (const auto& entry : content) {
        boost::apply_visitor(visitor, entry);
    }

    // Debug output.
    /*for (auto& _struct : structs) {
        std::cout << "struct: " << _struct.first << std::endl;
    }
    for (auto& variable : variables) {
        std::cout << "variable: " << variable.first << std::endl;
    }
    for (auto& constant : constants) {
        std::cout << "constant: " << constant.first << std::endl;
    }
    for (auto& function : functions) {
        std::cout << "function: " << function.first << std::endl;
    }
    for (auto& directive : directives) {
        std::cout << "directive: " << directive.first << std::endl;
    }*/

    // Find all shader entry points.
    for (const auto& functionPair : functions) {
        const auto& function = functionPair.second;
        // Is this function a shader entry point, i.e., attribute "vertex", "fragment", or "compute"?
        ShaderType shaderType;
        bool foundShaderType = false;
        for (const auto& attribute : function.attributes) {
            if (attribute.name == "vertex") {
                shaderType = ShaderType::VERTEX;
                foundShaderType = true;
                break;
            } else if (attribute.name == "fragment") {
                shaderType = ShaderType::FRAGMENT;
                foundShaderType = true;
                break;
            } else if (attribute.name == "compute") {
                shaderType = ShaderType::COMPUTE;
                foundShaderType = true;
                break;
            }
        }
        if (!foundShaderType) {
            continue;
        }

        ShaderInfo shaderInfo{};
        shaderInfo.shaderType = shaderType;

        // Create reflection information on vertex shader inputs and fragment shader outputs.
        if (shaderType == ShaderType::VERTEX) {
            for (const auto& parameter : function.parameters) {
                if (!processInOut(
                        structs, errorString, function.name, parameter.name,
                        shaderInfo.inputs, parameter.type, parameter.attributes)) {
                    return false;
                }
            }
            std::sort(
                    shaderInfo.inputs.begin(), shaderInfo.inputs.end(),
                    [](const InOutEntry &entry0, const InOutEntry &entry1) { return entry0.locationIndex < entry1.locationIndex; });
        } else if (shaderType == ShaderType::FRAGMENT) {
            if (function.return_type.has_value()) {
                if (!processInOut(
                        structs, errorString, function.name, "",
                        shaderInfo.outputs, function.return_type.get(), function.return_type_attributes)) {
                    return false;
                }
            }
            std::sort(
                    shaderInfo.outputs.begin(), shaderInfo.outputs.end(),
                    [](const InOutEntry &entry0, const InOutEntry &entry1) { return entry0.locationIndex < entry1.locationIndex; });
        }

        reflectInfo.shaders.insert(std::make_pair(function.name, shaderInfo));
    }

    // Create reflection information on binding groups.
    for (const auto& variablePair : variables) {
        const auto& variable = variablePair.second;
        const auto* bindingAttribute = findAttributeByName(variable.attributes, "binding");
        if (bindingAttribute) {
            BindingEntry bindingEntry{};
            uint32_t groupIndex = 0;
            const auto* groupAttribute = findAttributeByName(variable.attributes, "group");
            if (groupAttribute) {
                groupIndex = sgl::fromString<uint32_t>(groupAttribute->expression);
            }
            bindingEntry.bindingIndex = sgl::fromString<uint32_t>(bindingAttribute->expression);
            bindingEntry.variableName = variable.name;
            bindingEntry.bindingEntryType = getBindingEntryType(variable.type, variable.modifiers);
            bindingEntry.typeName = variable.type.name;
            if (variable.modifiers.has_value()) {
                bindingEntry.modifiers = variable.modifiers.get();
            }
            if (bindingEntry.bindingEntryType == BindingEntryType::STORAGE_BUFFER
                    || bindingEntry.bindingEntryType == BindingEntryType::STORAGE_TEXTURE) {
                bool hasReadModifier = false;
                bool hasWriteModifier = false;
                if (variable.modifiers.has_value()) {
                    const auto& modifiers = variable.modifiers.get();
                    if (std::find(modifiers.begin(), modifiers.end(), "read") != modifiers.end()) {
                        hasReadModifier = true;
                    }
                    if (std::find(modifiers.begin(), modifiers.end(), "write") != modifiers.end()) {
                        hasWriteModifier = true;
                    }
                    if (std::find(modifiers.begin(), modifiers.end(), "read_write") != modifiers.end()) {
                        hasReadModifier = true;
                        hasWriteModifier = true;
                    }
                }
                if (hasReadModifier && hasWriteModifier) {
                    bindingEntry.storageModifier = StorageModifier::READ_WRITE;
                } else if (hasReadModifier) {
                    bindingEntry.storageModifier = StorageModifier::READ;
                } else if (hasWriteModifier) {
                    bindingEntry.storageModifier = StorageModifier::WRITE;
                }
            }
            if (bindingEntry.bindingEntryType == BindingEntryType::UNKNOWN) {
                errorString =
                        "Could not resolve binding entry type for \"var " + variable.name + "\".";
                return false;
            }
            reflectInfo.bindingGroups[groupIndex].push_back(bindingEntry);
        }
    }
    for (auto& bindingGroup : reflectInfo.bindingGroups) {
        std::sort(
                bindingGroup.second.begin(), bindingGroup.second.end(),
                [](const BindingEntry &entry0, const BindingEntry &entry1) { return entry0.bindingIndex < entry1.bindingIndex; }
        );
    }

    return true;
}

}}
