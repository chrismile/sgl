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

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/WebGPU/Utils/Device.hpp>
#include "Shader.hpp"

namespace sgl { namespace webgpu {

ShaderModule::ShaderModule(WGPUShaderModule shaderModule, ReflectInfo reflectInfo)
        : shaderModule(shaderModule), reflectInfo(std::move(reflectInfo)) {
}

ShaderModule::~ShaderModule() {
    if (shaderModule) {
        wgpuShaderModuleRelease(shaderModule);
        shaderModule = {};
    }
}


static const std::unordered_map<std::string, WGPUTextureViewDimension> typeNameToTextureViewDimensionMap = {
        { "texture_1d", WGPUTextureViewDimension_1D },
        { "texture_2d", WGPUTextureViewDimension_2D },
        { "texture_2d_array", WGPUTextureViewDimension_2DArray },
        { "texture_3d", WGPUTextureViewDimension_3D },
        { "texture_cube", WGPUTextureViewDimension_Cube },
        { "texture_cube_array", WGPUTextureViewDimension_CubeArray },
        { "texture_multisampled_2d", WGPUTextureViewDimension_2D },
        { "texture_depth_multisampled_2d", WGPUTextureViewDimension_2D },
        { "texture_storage_1d", WGPUTextureViewDimension_1D },
        { "texture_storage_2d", WGPUTextureViewDimension_2D },
        { "texture_storage_2d_array", WGPUTextureViewDimension_2DArray },
        { "texture_storage_3d", WGPUTextureViewDimension_3D },
        { "texture_depth_2d", WGPUTextureViewDimension_2D },
        { "texture_depth_2d_array", WGPUTextureViewDimension_2DArray },
        { "texture_depth_cube", WGPUTextureViewDimension_Cube },
        { "texture_depth_cube_array", WGPUTextureViewDimension_CubeArray },
};

static const std::unordered_map<std::string, WGPUTextureFormat> typeNameToTextureFormatMap = {
        // https://www.w3.org/TR/WGSL/#texel-formats
        { "rgba8unorm", WGPUTextureFormat_RGBA8Unorm },
        { "rgba8snorm", WGPUTextureFormat_RGBA8Snorm },
        { "rgba8uint", WGPUTextureFormat_RGBA8Uint },
        { "rgba8sint", WGPUTextureFormat_RGBA8Sint },
        { "rgba16uint", WGPUTextureFormat_RGBA16Uint },
        { "rgba16sint", WGPUTextureFormat_RGBA16Sint },
        { "rgba16float", WGPUTextureFormat_RGBA16Float },
        { "r32uint", WGPUTextureFormat_R32Uint },
        { "r32sint", WGPUTextureFormat_R32Sint },
        { "r32float", WGPUTextureFormat_R32Float },
        { "rg32uint", WGPUTextureFormat_RG32Uint },
        { "rg32sint", WGPUTextureFormat_RG32Sint },
        { "rg32float", WGPUTextureFormat_RG32Float },
        { "rgba32uint", WGPUTextureFormat_RGBA32Uint },
        { "rgba32sint", WGPUTextureFormat_RGBA32Sint },
        { "rgba32float", WGPUTextureFormat_RGBA32Float },
        { "bgra8unorm", WGPUTextureFormat_BGRA8Unorm },
};

ShaderStages::ShaderStages(
        Device* device, const std::vector<ShaderModulePtr>& shaderModules, const std::vector<std::string>& entryPoints)
        : device(device), shaderModules(shaderModules), entryPoints(entryPoints) {
    shaderModuleTypes.resize(shaderModules.size());
    for (size_t i = 0; i < shaderModules.size(); i++) {
        const auto& entryPoint = entryPoints.at(i);
        const auto& reflectInfo = shaderModules.at(i)->reflectInfo;
        auto it = reflectInfo.shaders.find(entryPoint);
        if (it == reflectInfo.shaders.end()) {
            sgl::Logfile::get()->throwError(
                    "Error in ShaderStages::ShaderStages: Invalid shader entry point name \"" + entryPoint + "\".");
        }
        const auto& shaderInfo = it->second;
        if (shaderInfo.shaderType == ShaderType::VERTEX) {
            hasVertexShader = true;
            vertexShaderInputEntries = &shaderInfo.inputs;
            std::vector<uint32_t> locationIndices;
            for (const InOutEntry& vertexInput : shaderInfo.inputs) {
                inputVariableNameLocationMap.insert(std::make_pair(vertexInput.variableName, vertexInput.locationIndex));
                inputLocationVariableNameMap.insert(std::make_pair(vertexInput.locationIndex, vertexInput.variableName));
                locationIndices.push_back(vertexInput.locationIndex);
            }
            std::sort(locationIndices.begin(), locationIndices.end());
            for (uint32_t locationIndex = 0; locationIndex < uint32_t(locationIndices.size()); locationIndex++) {
                uint32_t location = locationIndices.at(locationIndex);
                inputVariableNameLocationIndexMap.insert(std::make_pair(
                        inputLocationVariableNameMap[location], locationIndex));
            }
        }
        shaderModuleTypes.at(i) = shaderInfo.shaderType;

        for (const auto& bindGroupInfo : reflectInfo.bindingGroups) {
            for (const auto& bindingEntryInfo : bindGroupInfo.second) {
                WGPUShaderStage newFlags = WGPUShaderStage_None;
                if (shaderInfo.shaderType == ShaderType::VERTEX) {
                    newFlags = WGPUShaderStage_Vertex;
                } else if (shaderInfo.shaderType == ShaderType::FRAGMENT) {
                    newFlags = WGPUShaderStage_Fragment;
                } else if (shaderInfo.shaderType == ShaderType::COMPUTE) {
                    newFlags = WGPUShaderStage_Compute;
                }
                WGPUShaderStage currentFlags =
                        bindingEntryStageFlags[bindGroupInfo.first][bindingEntryInfo.bindingIndex];
                currentFlags = WGPUShaderStage(currentFlags | newFlags);
                bindingEntryStageFlags[bindGroupInfo.first][bindingEntryInfo.bindingIndex] = currentFlags;
            }
        }

        mergeBindGroupsInfo(reflectInfo.bindingGroups);
    }

    createBindGroupLayouts();
}

void ShaderStages::mergeBindGroupsInfo(const std::map<uint32_t, std::vector<BindingEntry>>& newBindGroupsInfo) {
    for (const auto& itNew : newBindGroupsInfo) {
        const std::vector<BindingEntry>& bindGroupInfoNew = itNew.second;
        std::vector<BindingEntry>& bindGroupInfo = bindGroupsInfo[itNew.first];

        // Merge the descriptors in a map object.
        std::map<uint32_t, BindingEntry> bindGroupInfoMap;
        for (BindingEntry& bindingEntry : bindGroupInfo) {
            bindGroupInfoMap.insert(std::make_pair(bindingEntry.bindingIndex, bindingEntry));
        }
        for (const BindingEntry& bindingEntry : bindGroupInfoNew) {
            auto it = bindGroupInfoMap.find(bindingEntry.bindingIndex);
            if (it == bindGroupInfoMap.end()) {
                bindGroupInfoMap.insert(std::make_pair(bindingEntry.bindingIndex, bindingEntry));
                it = bindGroupInfoMap.find(bindingEntry.bindingIndex);
            } else {
                if (it->second.typeName != bindingEntry.typeName
                        || it->second.bindingEntryType != bindingEntry.bindingEntryType) {
                    throw std::runtime_error(
                            std::string() + "Error in ShaderStages::mergeDescriptorSetsInfo: Attempted to merge "
                            + "incompatible binding entries \"" + it->second.variableName + "\" and \""
                            + bindingEntry.variableName + "\"!");
                }
            }
        }

        // Then, convert the merged descriptors back into a list.
        bindGroupInfo.clear();
        for (const auto& it : bindGroupInfoMap) {
            bindGroupInfo.push_back(it.second);
        }
    }
}

void ShaderStages::createBindGroupLayouts() {
    bindGroupLayouts.resize(bindGroupsInfo.size());
    size_t i = 0;
    for (const auto& bindGroupInfo : bindGroupsInfo) {
        std::vector<WGPUBindGroupLayoutEntry> bindGroupLayoutEntries(bindGroupInfo.second.size());
        size_t entryIdx = 0;
        for (const auto& bindGroupEntry : bindGroupInfo.second) {
            auto& bindGroupLayoutEntry = bindGroupLayoutEntries.at(entryIdx);
            bindGroupLayoutEntry.binding = bindGroupEntry.bindingIndex;
            bindGroupLayoutEntry.visibility =
                    bindingEntryStageFlags[bindGroupInfo.first][bindGroupEntry.bindingIndex];
            if (bindGroupEntry.bindingEntryType == BindingEntryType::UNIFORM_BUFFER) {
                bindGroupLayoutEntry.buffer.type = WGPUBufferBindingType_Uniform;
                bindGroupLayoutEntry.buffer.hasDynamicOffset = false;
                // https://www.w3.org/TR/webgpu/#dom-gpubufferbindinglayout-minbindingsize
                bindGroupLayoutEntry.buffer.minBindingSize = 0;
            } else if (bindGroupEntry.bindingEntryType == BindingEntryType::TEXTURE) {
                auto it = typeNameToTextureViewDimensionMap.find(bindGroupEntry.typeName);
                if (it == typeNameToTextureViewDimensionMap.end()) {
                    sgl::Logfile::get()->throwError(
                            "Error in ShaderStages::ShaderStages: Invalid type \"" + bindGroupEntry.typeName + "\".");
                }
                bindGroupLayoutEntry.texture.viewDimension = it->second;
                bindGroupLayoutEntry.texture.multisampled =
                        bindGroupEntry.typeName == "texture_multisampled_2d"
                        || bindGroupEntry.typeName == "texture_depth_multisampled_2d";
                std::string formatName = bindGroupEntry.textureFormat;
                if (sgl::startsWith(bindGroupEntry.typeName, "texture_depth")) {
                    bindGroupLayoutEntry.texture.sampleType = WGPUTextureSampleType_Depth;
                } else if (formatName == "f32") {
                    // No way to detect WGPUTextureSampleType_UnfilterableFloat?
                    // Might need to check for "float32-filterable" (https://www.w3.org/TR/webgpu/#float32-filterable).
                    bindGroupLayoutEntry.texture.sampleType = WGPUTextureSampleType_Float;
                } else if (formatName == "i32") {
                    bindGroupLayoutEntry.texture.sampleType = WGPUTextureSampleType_Sint;
                } else if (formatName == "u32") {
                    bindGroupLayoutEntry.texture.sampleType = WGPUTextureSampleType_Uint;
                } else {
                    sgl::Logfile::get()->throwError(
                            "Error in ShaderStages::ShaderStages: Unsupported texture modifier.");
                }
            } else if (bindGroupEntry.bindingEntryType == BindingEntryType::SAMPLER) {
                if (bindGroupEntry.typeName == "sampler") {
                    // No way to check for WGPUSamplerBindingType_NonFiltering?
                    bindGroupLayoutEntry.sampler.type = WGPUSamplerBindingType_Filtering;
                } else if (bindGroupEntry.typeName == "sampler_comparison") {
                    bindGroupLayoutEntry.sampler.type = WGPUSamplerBindingType_Comparison;
                }
            } else if (bindGroupEntry.bindingEntryType == BindingEntryType::STORAGE_BUFFER) {
                if (bindGroupEntry.storageModifier == StorageModifier::READ) {
                    bindGroupLayoutEntry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
                } else {
                    bindGroupLayoutEntry.buffer.type = WGPUBufferBindingType_Storage;
                }
                bindGroupLayoutEntry.buffer.hasDynamicOffset = false;
                // https://www.w3.org/TR/webgpu/#dom-gpubufferbindinglayout-minbindingsize
                bindGroupLayoutEntry.buffer.minBindingSize = 0;
            } else if (bindGroupEntry.bindingEntryType == BindingEntryType::STORAGE_TEXTURE) {
                if (bindGroupEntry.storageModifier == StorageModifier::READ) {
                    bindGroupLayoutEntry.storageTexture.access = WGPUStorageTextureAccess_ReadOnly;
                } else if (bindGroupEntry.storageModifier == StorageModifier::WRITE) {
                    bindGroupLayoutEntry.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
                } else {
                    bindGroupLayoutEntry.storageTexture.access = WGPUStorageTextureAccess_ReadWrite;
                }
                auto it = typeNameToTextureViewDimensionMap.find(bindGroupEntry.typeName);
                if (it == typeNameToTextureViewDimensionMap.end()) {
                    sgl::Logfile::get()->throwError(
                            "Error in ShaderStages::ShaderStages: Invalid type \"" + bindGroupEntry.typeName + "\".");
                }
                bindGroupLayoutEntry.storageTexture.viewDimension = it->second;
                std::string formatName = bindGroupEntry.textureFormat;
                auto itFormat = typeNameToTextureFormatMap.find(formatName);
                if (itFormat == typeNameToTextureFormatMap.end()) {
                    sgl::Logfile::get()->throwError(
                            "Error in ShaderStages::ShaderStages: Invalid texture format name \"" + formatName + "\".");
                }
                bindGroupLayoutEntry.storageTexture.format = itFormat->second;
            }
            entryIdx++;
        }

        WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
        bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
        bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();
        bindGroupLayouts.at(i) = wgpuDeviceCreateBindGroupLayout(device->getWGPUDevice(), &bindGroupLayoutDescriptor);
        i++;
    }
}

ShaderStages::~ShaderStages() {
    for (auto& bindGroupLayout : bindGroupLayouts) {
        wgpuBindGroupLayoutRelease(bindGroupLayout);
    }
    bindGroupLayouts.clear();
}

const ShaderModulePtr& ShaderStages::getShaderModule(ShaderType shaderType) const {
    for (size_t i = 0; i < shaderModuleTypes.size(); i++) {
        if (shaderType == shaderModuleTypes.at(i)) {
            return shaderModules.at(i);
        }
    }
    sgl::Logfile::get()->throwError(
            "Error in ShaderStages::getShaderModule: Requested shader type could not be found.");
    return shaderModules.front();
}

const std::string& ShaderStages::getEntryPoint(ShaderType shaderType) const {
    for (size_t i = 0; i < shaderModuleTypes.size(); i++) {
        if (shaderType == shaderModuleTypes.at(i)) {
            return entryPoints.at(i);
        }
    }
    sgl::Logfile::get()->throwError(
            "Error in ShaderStages::getEntryPoint: Requested shader type could not be found.");
    return entryPoints.front();
}



const std::vector<InOutEntry>& ShaderStages::getInputVariableDescriptors() const {
    if (!hasVertexShader) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableDescriptors: No vertex shader exists!");
        return emptySet;
    }

    return *vertexShaderInputEntries;
}

bool ShaderStages::getHasInputVariable(const std::string& varName) const {
    if (!hasVertexShader) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getHasInputVariable: No vertex shader exists!");
        return false;
    }

    return inputVariableNameLocationMap.find(varName) != inputVariableNameLocationMap.end();
}

uint32_t ShaderStages::getInputVariableLocation(const std::string& varName) const {
    if (!hasVertexShader) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableLocation: No vertex shader exists!");
        return -1;
    }

    auto it = inputVariableNameLocationMap.find(varName);
    if (it == inputVariableNameLocationMap.end()) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableLocation: Unknown variable name \"" + varName + "\"!");
        return -1;
    }

    return it->second;
}

uint32_t ShaderStages::getInputVariableLocationIndex(const std::string& varName) const {
    if (!hasVertexShader) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableLocationIndex: No vertex shader exists!");
        return -1;
    }

    auto it = inputVariableNameLocationIndexMap.find(varName);
    if (it == inputVariableNameLocationIndexMap.end()) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableLocationIndex: Unknown variable name \"" + varName + "\"!");
        return -1;
    }

    return it->second;
}

const InOutEntry& ShaderStages::getInputVariableDescriptorFromLocation(uint32_t location) {
    if (!hasVertexShader) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderStages::getInputVariableDescriptorFromLocation: No vertex shader exists!");
    }

    for (const auto& descriptor : *vertexShaderInputEntries) {
        if (location == descriptor.locationIndex) {
            return descriptor;
        }
    }
    sgl::Logfile::get()->throwError(
            "Error in ShaderStages::getInputVariableDescriptorFromLocation: Location not found!");
    return vertexShaderInputEntries->front(); // To get rid of warning...
}

const InOutEntry& ShaderStages::getInputVariableDescriptorFromName(const std::string& name) {
    if (!hasVertexShader) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderStages::getInputVariableDescriptorFromName: No vertex shader exists!");
    }

    for (const auto& descriptor : *vertexShaderInputEntries) {
        if (name == descriptor.variableName) {
            return descriptor;
        }
    }
    sgl::Logfile::get()->throwError(
            "Error in ShaderStages::getInputVariableDescriptorFromName: Location not found!");
    return vertexShaderInputEntries->front(); // To get rid of warning...
}


const std::map<uint32_t, std::vector<BindingEntry>>& ShaderStages::getBindGroupsInfo() const {
    return bindGroupsInfo;
}


bool ShaderStages::hasBindingEntry(uint32_t groupIdx, const std::string& descName) const {
    auto it = bindGroupsInfo.find(groupIdx);
    if (it == bindGroupsInfo.end()) {
        return false;
    }
    const std::vector<BindingEntry>& descriptorSetInfo = it->second;
    for (const BindingEntry& bindingEntry : descriptorSetInfo) {
        if (bindingEntry.variableName == descName) {
            return true;
        }
    }
    return false;
}

const BindingEntry& ShaderStages::getBindingEntryByName(uint32_t groupIdx, const std::string& descName) const {
    auto it = bindGroupsInfo.find(groupIdx);
    if (it == bindGroupsInfo.end()) {
        Logfile::get()->throwError(
                "Error in ShaderStages::getBindingEntryByName: No binding group #" + std::to_string(groupIdx)
                + " is used in these shaders.");
    }
    const std::vector<BindingEntry>& descriptorSetInfo = it->second;
    for (const BindingEntry& bindingEntry : descriptorSetInfo) {
        if (bindingEntry.variableName == descName) {
            return bindingEntry;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::getBindingEntryByName: Couldn't find descriptor with name \"" + descName
            + "\" for binding group index " + std::to_string(groupIdx) + ".");
    return descriptorSetInfo.front(); // To get rid of warning...
}

const BindingEntry& ShaderStages::getBindingEntryByIndex(uint32_t groupIdx, uint32_t bindingIndex) const {
    auto it = bindGroupsInfo.find(groupIdx);
    if (it == bindGroupsInfo.end()) {
        Logfile::get()->throwError(
                "Error in ShaderStages::getBindingEntryByIndex: No binding group #" + std::to_string(groupIdx)
                + " is used in these shaders.");
    }
    const std::vector<BindingEntry>& descriptorSetInfo = it->second;
    for (const BindingEntry& bindingEntry : descriptorSetInfo) {
        if (bindingEntry.bindingIndex == bindingIndex) {
            return bindingEntry;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::getBindingEntryByIndex: Couldn't find descriptor with binding \""
            + std::to_string(bindingIndex) + "\" for binding group index " + std::to_string(groupIdx) + ".");
    return descriptorSetInfo.front(); // To get rid of warning...
}

uint32_t ShaderStages::getBindingIndexByName(uint32_t groupIdx, const std::string& descName) const {
    auto it = bindGroupsInfo.find(groupIdx);
    if (it == bindGroupsInfo.end()) {
        Logfile::get()->throwError(
                "Error in ShaderStages::getDescriptorBindingByName: No binding group #" + std::to_string(groupIdx)
                + " is used in these shaders.");
    }
    const std::vector<BindingEntry>& descriptorSetInfo = it->second;
    for (const BindingEntry& bindingEntry : descriptorSetInfo) {
        if (bindingEntry.variableName == descName) {
            return bindingEntry.bindingIndex;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::getDescriptorBindingByName: Couldn't find descriptor with name \"" + descName
            + "\" for binding group index " + std::to_string(groupIdx) + ".");
    return std::numeric_limits<uint32_t>::max(); // To get rid of warning...
}

bool ShaderStages::getBindingEntryByNameOptional(uint32_t groupIdx, const std::string& descName, uint32_t& bindingIndex) const {
    auto it = bindGroupsInfo.find(groupIdx);
    if (it == bindGroupsInfo.end()) {
        return false;
    }
    const std::vector<BindingEntry>& descriptorSetInfo = it->second;
    for (const BindingEntry& bindingEntry : descriptorSetInfo) {
        if (bindingEntry.variableName == descName) {
            bindingIndex = bindingEntry.bindingIndex;
            return true;
        }
    }
    return false;
}

}}
