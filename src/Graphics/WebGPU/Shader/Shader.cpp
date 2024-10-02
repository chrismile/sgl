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
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, WGPUShaderStageFlags>> bindingEntryStageFlags;
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
            for (const InOutEntry& vertexInput : shaderInfo.inputs) {
                inputVariableNameLocationMap.insert(std::make_pair(vertexInput.variableName, vertexInput.locationIndex));
                inputLocationVariableNameMap.insert(std::make_pair(vertexInput.locationIndex, vertexInput.variableName));
                inputVariableNameLocationIndexMap.insert(std::make_pair(vertexInput.variableName, vertexInput.locationIndex));
            }
        }
        shaderModuleTypes.at(i) = shaderInfo.shaderType;

        for (const auto& bindGroupInfo : reflectInfo.bindingGroups) {
            bindGroupInfo.first;
            for (const auto& bindingEntryInfo : bindGroupInfo.second) {
                WGPUShaderStageFlags newFlags = WGPUShaderStage_None;
                if (shaderInfo.shaderType == ShaderType::VERTEX) {
                    newFlags = WGPUShaderStage_Vertex;
                } else if (shaderInfo.shaderType == ShaderType::FRAGMENT) {
                    newFlags = WGPUShaderStage_Fragment;
                } else if (shaderInfo.shaderType == ShaderType::COMPUTE) {
                    newFlags = WGPUShaderStage_Compute;
                }
                WGPUShaderStageFlags currentFlags =
                        bindingEntryStageFlags[bindGroupInfo.first][bindingEntryInfo.bindingIndex];
                currentFlags = currentFlags | newFlags;
                bindingEntryStageFlags[bindGroupInfo.first][bindingEntryInfo.bindingIndex] = currentFlags;
            }
        }
    }

    bindGroupLayouts.resize(bindGroupsInfo.size());
    size_t i = 0;
    for (const auto& bindGroupInfo : bindGroupsInfo) {
        std::vector<WGPUBindGroupLayoutEntry> bindGroupLayoutEntries(bindGroupsInfo.size());
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
                if (bindGroupEntry.modifiers.size() != 1) {
                    sgl::Logfile::get()->throwError(
                            "Error in ShaderStages::ShaderStages: Invalid number of texture modifiers.");
                }
                std::string modifier = bindGroupEntry.modifiers.front();
                if (sgl::startsWith(bindGroupEntry.typeName, "texture_depth")) {
                    bindGroupLayoutEntry.texture.sampleType = WGPUTextureSampleType_Depth;
                } else if (modifier == "f32") {
                    // No way to detect WGPUTextureSampleType_UnfilterableFloat?
                    // Might need to check for "float32-filterable" (https://www.w3.org/TR/webgpu/#float32-filterable).
                    bindGroupLayoutEntry.texture.sampleType = WGPUTextureSampleType_Float;
                } else if (modifier == "i32") {
                    bindGroupLayoutEntry.texture.sampleType = WGPUTextureSampleType_Sint;
                } else if (modifier == "u32") {
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
                if (bindGroupEntry.modifiers.size() != 2) {
                    sgl::Logfile::get()->throwError(
                            "Error in ShaderStages::ShaderStages: Invalid number of storage texture modifiers.");
                }
                std::string formatName = bindGroupEntry.modifiers.front();
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
    ;
}

bool ShaderStages::getHasInputVariable(const std::string& varName) const {
    return false;
}

uint32_t ShaderStages::getInputVariableLocation(const std::string& varName) const {
    ;
}

uint32_t ShaderStages::getInputVariableLocationIndex(const std::string& varName) const {
    ;
}

const InOutEntry& ShaderStages::getInputVariableDescriptorFromLocation(uint32_t location) {
    ;
}

const InOutEntry& ShaderStages::getInputVariableDescriptorFromName(const std::string& name) {
    ;
}


const std::map<uint32_t, std::vector<BindingEntry>>& ShaderStages::getBindGroupsInfo() const {
    ;
}

bool ShaderStages::hasBindingEntry(uint32_t groupIdx, const std::string& descName) const {
    ;
}

const BindingEntry& ShaderStages::getBindingEntryByName(uint32_t groupIdx, const std::string& descName) const {
    ;
}

const BindingEntry& ShaderStages::getBindingEntryByIndex(uint32_t groupIdx, uint32_t binding) const {
    ;
}

uint32_t ShaderStages::getBindingIndexByName(uint32_t groupIdx, const std::string& descName) const {
    return 0;
}

bool ShaderStages::getBindingEntryByNameOptional(uint32_t groupIdx, const std::string& descName, uint32_t& bindingIndex) const {
    ;
}


void ShaderStages::mergeBindGroupsInfo(const std::map<uint32_t, std::vector<BindingEntry>>& newBindGroupsInfo) {
    ;
}

void ShaderStages::createBindGroupLayouts() {
    ;
}

}}
