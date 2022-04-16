/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#include <algorithm>
#include <limits>

#include <Utils/File/Logfile.hpp>

#include "../Utils/Device.hpp"
#include "Shader.hpp"

namespace sgl { namespace vk {

ShaderModule::ShaderModule(
        Device* device, const std::string& shaderModuleId, ShaderModuleType shaderModuleType,
        const std::vector<uint32_t>& spirvCode)
        : device(device), shaderModuleId(shaderModuleId), shaderModuleType(shaderModuleType) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    if (vkCreateShaderModule(device->getVkDevice(), &createInfo, nullptr, &vkShaderModule) != VK_SUCCESS) {
        sgl::Logfile::get()->writeError("Error in ShaderModule::ShaderModule: Failed to create the shader module.");
        exit(1);
    }

    createReflectData(spirvCode);
}

ShaderModule::~ShaderModule() {
    vkDestroyShaderModule(device->getVkDevice(), vkShaderModule, nullptr);
}

bool ShaderModule::getIsRayTracingShader() {
    return shaderModuleType == ShaderModuleType::RAYGEN || shaderModuleType == ShaderModuleType::MISS
            || shaderModuleType == ShaderModuleType::CALLABLE || shaderModuleType == ShaderModuleType::ANY_HIT
            || shaderModuleType == ShaderModuleType::CLOSEST_HIT || shaderModuleType == ShaderModuleType::INTERSECTION;
}

void ShaderModule::createReflectData(const std::vector<uint32_t>& spirvCode) {
    // Create the reflect shader module.
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirvCode.size() * 4, spirvCode.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ShaderModule::createReflectData: spvReflectCreateShaderModule failed!");
    }


    // Get reflection information on the input variables.
    uint32_t numInputVariables = 0;
    result = spvReflectEnumerateInputVariables(&module, &numInputVariables, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ShaderModule::createReflectData: spvReflectEnumerateInputVariables failed!");
    }
    std::vector<SpvReflectInterfaceVariable*> inputVariables(numInputVariables);
    result = spvReflectEnumerateInputVariables(&module, &numInputVariables, inputVariables.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ShaderModule::createReflectData: spvReflectEnumerateInputVariables failed!");
    }

    inputVariableDescriptors.reserve(numInputVariables);
    for (uint32_t varIdx = 0; varIdx < numInputVariables; varIdx++) {
        InterfaceVariableDescriptor varDesc;
        varDesc.location = inputVariables[varIdx]->location;
        varDesc.format = inputVariables[varIdx]->format;
        varDesc.name = inputVariables[varIdx]->name;
        inputVariableDescriptors.push_back(varDesc);
    }


    // Get reflection information on the descriptor sets.
    uint32_t numDescriptorSets = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &numDescriptorSets, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ShaderModule::createReflectData: spvReflectEnumerateDescriptorSets failed!");
    }
    std::vector<SpvReflectDescriptorSet*> descriptorSets(numDescriptorSets);
    result = spvReflectEnumerateDescriptorSets(&module, &numDescriptorSets, descriptorSets.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ShaderModule::createReflectData: spvReflectEnumerateDescriptorSets failed!");
    }

    for (uint32_t descSetIdx = 0; descSetIdx < numDescriptorSets; ++descSetIdx) {
        SpvReflectDescriptorSet* reflectDescriptorSet = descriptorSets.at(descSetIdx);

        std::vector<DescriptorInfo> descriptorsInfo;
        descriptorsInfo.reserve(reflectDescriptorSet->binding_count);
        for (uint32_t bindingIdx = 0; bindingIdx < reflectDescriptorSet->binding_count; bindingIdx++) {
            DescriptorInfo descriptorInfo;
            descriptorInfo.binding = reflectDescriptorSet->bindings[bindingIdx]->binding;
            descriptorInfo.type = VkDescriptorType(reflectDescriptorSet->bindings[bindingIdx]->descriptor_type);
            if (reflectDescriptorSet->bindings[bindingIdx]->type_description
                    && reflectDescriptorSet->bindings[bindingIdx]->type_description->type_name
                    && strlen(reflectDescriptorSet->bindings[bindingIdx]->type_description->type_name) > 0) {
                descriptorInfo.name = reflectDescriptorSet->bindings[bindingIdx]->type_description->type_name;
            } else {
                descriptorInfo.name = reflectDescriptorSet->bindings[bindingIdx]->name;
            }
            descriptorInfo.count = reflectDescriptorSet->bindings[bindingIdx]->count;
            descriptorInfo.size = reflectDescriptorSet->bindings[bindingIdx]->block.size;
            descriptorInfo.shaderStageFlags = uint32_t(shaderModuleType);
            descriptorInfo.readOnly = true;
            descriptorInfo.image = reflectDescriptorSet->bindings[bindingIdx]->image;
            descriptorsInfo.push_back(descriptorInfo);

            if (reflectDescriptorSet->bindings[bindingIdx]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE
                    || reflectDescriptorSet->bindings[bindingIdx]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
                    || reflectDescriptorSet->bindings[bindingIdx]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER
                    || reflectDescriptorSet->bindings[bindingIdx]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                descriptorInfo.readOnly = reflectDescriptorSet->bindings[0]->type_description->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE;
            }
        }

        descriptorSetsInfo.insert(std::make_pair(reflectDescriptorSet->set, descriptorsInfo));
    }


    // Get reflection information on the push constant blocks.
    uint32_t numPushConstantBlocks = 0;
    result = spvReflectEnumeratePushConstantBlocks(&module, &numPushConstantBlocks, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ShaderModule::createReflectData: spvReflectEnumeratePushConstantBlocks failed!");
    }
    std::vector<SpvReflectBlockVariable*> pushConstantBlockVariables(numPushConstantBlocks);
    result = spvReflectEnumeratePushConstantBlocks(&module, &numPushConstantBlocks, pushConstantBlockVariables.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ShaderModule::createReflectData: spvReflectEnumeratePushConstantBlocks failed!");
    }

    pushConstantRanges.resize(numPushConstantBlocks);
    for (uint32_t blockIdx = 0; blockIdx < numPushConstantBlocks; blockIdx++) {
        VkPushConstantRange& pushConstantRange = pushConstantRanges.at(blockIdx);
        SpvReflectBlockVariable* pushConstantBlockVariable = pushConstantBlockVariables.at(blockIdx);
        pushConstantRange.stageFlags = uint32_t(shaderModuleType);
        pushConstantRange.offset = pushConstantBlockVariable->absolute_offset;
        pushConstantRange.size = pushConstantBlockVariable->size;
    }


    spvReflectDestroyShaderModule(&module);
}

const std::vector<InterfaceVariableDescriptor>& ShaderModule::getInputVariableDescriptors() const {
    return inputVariableDescriptors;
}

const std::map<uint32_t, std::vector<DescriptorInfo>>& ShaderModule::getDescriptorSetsInfo() const {
    return descriptorSetsInfo;
}



ShaderStages::ShaderStages(
        Device* device, std::vector<ShaderModulePtr>& shaderModules) : device(device), shaderModules(shaderModules) {
    vkShaderStages.reserve(shaderModules.size());
    for (ShaderModulePtr& shaderModule : shaderModules) {
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.stage = VkShaderStageFlagBits(shaderModule->getShaderModuleType());
        shaderStageCreateInfo.module = shaderModule->getVkShaderModule();
        shaderStageCreateInfo.pName = "main";
        shaderStageCreateInfo.pSpecializationInfo = nullptr;
        vkShaderStages.push_back(shaderStageCreateInfo);

        if (shaderModule->getShaderModuleType() == ShaderModuleType::VERTEX) {
            vertexShaderModule = shaderModule;
            std::vector<uint32_t> locationIndices;
            for (const InterfaceVariableDescriptor& varDesc : vertexShaderModule->getInputVariableDescriptors()) {
                inputVariableNameLocationMap.insert(std::make_pair(varDesc.name, varDesc.location));
                inputLocationVariableNameMap.insert(std::make_pair(varDesc.location, varDesc.name));
                locationIndices.push_back(varDesc.location);
            }

            std::sort(locationIndices.begin(), locationIndices.end());
            for (uint32_t locationIndex = 0; locationIndex < uint32_t(locationIndices.size()); locationIndex++) {
                uint32_t location = locationIndices.at(locationIndex);
                inputVariableNameLocationIndexMap.insert(std::make_pair(
                        inputLocationVariableNameMap[location], locationIndex));
            }
        }

        mergeDescriptorSetsInfo(shaderModule->getDescriptorSetsInfo());
        mergePushConstantRanges(shaderModule->getVkPushConstantRanges());
    }
}

ShaderStages::ShaderStages(
        Device* device, std::vector<ShaderModulePtr>& shaderModules, const std::vector<std::string>& functionNames)
        : device(device), shaderModules(shaderModules) {
    vkShaderStages.reserve(shaderModules.size());
    for (size_t i = 0; i < shaderModules.size(); i++) {
        ShaderModulePtr& shaderModule = shaderModules.at(i);
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.stage = VkShaderStageFlagBits(shaderModule->getShaderModuleType());
        shaderStageCreateInfo.module = shaderModule->getVkShaderModule();
        shaderStageCreateInfo.pName = functionNames.at(i).c_str();
        shaderStageCreateInfo.pSpecializationInfo = nullptr;
        vkShaderStages.push_back(shaderStageCreateInfo);

        if (shaderModule->getShaderModuleType() == ShaderModuleType::VERTEX) {
            vertexShaderModule = shaderModule;
            std::vector<uint32_t> locationIndices;
            for (const InterfaceVariableDescriptor& varDesc : vertexShaderModule->getInputVariableDescriptors()) {
                inputVariableNameLocationMap.insert(std::make_pair(varDesc.name, varDesc.location));
                inputLocationVariableNameMap.insert(std::make_pair(varDesc.location, varDesc.name));
                locationIndices.push_back(varDesc.location);
            }

            std::sort(locationIndices.begin(), locationIndices.end());
            for (uint32_t locationIndex = 0; locationIndex < uint32_t(locationIndices.size()); locationIndex++) {
                uint32_t location = locationIndices.at(locationIndex);
                inputVariableNameLocationIndexMap.insert(std::make_pair(
                        inputLocationVariableNameMap[location], locationIndex));
            }
        }

        mergeDescriptorSetsInfo(shaderModule->getDescriptorSetsInfo());
    }
    createDescriptorSetLayouts();
}

ShaderStages::~ShaderStages() {
    for (VkDescriptorSetLayout& descriptorSetLayout : descriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(device->getVkDevice(), descriptorSetLayout, nullptr);
    }
    descriptorSetLayouts.clear();
}

void ShaderStages::mergeDescriptorSetsInfo(const std::map<uint32_t, std::vector<DescriptorInfo>>& newDescriptorSetsInfo) {
    for (auto& itNew : newDescriptorSetsInfo) {
        numDescriptorSets = std::max(numDescriptorSets, itNew.first + 1);
        const std::vector<DescriptorInfo>& descriptorsInfoNew = itNew.second;
        std::vector<DescriptorInfo>& descriptorsInfo = descriptorSetsInfo[itNew.first];

        // Merge the descriptors in a map object.
        std::map<uint32_t, DescriptorInfo> descriptorsInfoMap;
        for (DescriptorInfo& descInfo : descriptorsInfo) {
            descriptorsInfoMap.insert(std::make_pair(descInfo.binding, descInfo));
        }
        for (const DescriptorInfo& descInfo : descriptorsInfoNew) {
            auto it = descriptorsInfoMap.find(descInfo.binding);
            if (it == descriptorsInfoMap.end()) {
                descriptorsInfoMap.insert(std::make_pair(descInfo.binding, descInfo));
                it = descriptorsInfoMap.find(descInfo.binding);
            } else {
                if (it->second.type != descInfo.type) {
                    throw std::runtime_error(
                            std::string() + "Error in ShaderStages::mergeDescriptorSetsInfo: Attempted to merge "
                            + "incompatible descriptors \"" + it->second.name + "\" and \"" + descInfo.name + "\"!");
                }
                it->second.shaderStageFlags |= descInfo.shaderStageFlags;
            }
            if (itNew.first == 1 && descInfo.binding == 0 && descInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                // Hard-coded: MVP matrix block.
                it->second.shaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                if (device->getPhysicalDeviceFeatures().geometryShader) {
                    it->second.shaderStageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
                }
                if (device->getPhysicalDeviceMeshShaderFeaturesNV().meshShader) {
                    it->second.shaderStageFlags |= VK_SHADER_STAGE_MESH_BIT_NV;
                }
            }
        }

        // Then, convert the merged descriptors back into a list.
        descriptorsInfo.clear();
        for (const auto& it : descriptorsInfoMap) {
            descriptorsInfo.push_back(it.second);
        }
    }
    createDescriptorSetLayouts();
}

void ShaderStages::mergePushConstantRanges(const std::vector<VkPushConstantRange>& newPushConstantRanges) {
    if (pushConstantRanges.empty()) {
        pushConstantRanges = newPushConstantRanges;
        return;
    }

    for (const VkPushConstantRange& newPushConstantRange : newPushConstantRanges) {
        bool foundIdenticalRange = false;
        for (VkPushConstantRange& pushConstantRange : pushConstantRanges) {
            // Identical?
            if (pushConstantRange.offset == newPushConstantRange.offset
                    && pushConstantRange.size == newPushConstantRange.size) {
                pushConstantRange.stageFlags = pushConstantRange.stageFlags | newPushConstantRange.stageFlags;
                foundIdenticalRange = true;
                break;
            }
        }
        if (!foundIdenticalRange) {
            pushConstantRanges.push_back(newPushConstantRange);
        }
    }
}

void ShaderStages::createDescriptorSetLayouts() {
    for (VkDescriptorSetLayout& descriptorSetLayout : descriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(device->getVkDevice(), descriptorSetLayout, nullptr);
    }
    descriptorSetLayouts.resize(numDescriptorSets);

    for (uint32_t setIdx = 0; setIdx < numDescriptorSets; setIdx++) {
        VkDescriptorSetLayout& descriptorSetLayout = descriptorSetLayouts.at(setIdx);
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        auto it = descriptorSetsInfo.find(setIdx);
        if (it != descriptorSetsInfo.end()) {
            bindings.resize(it->second.size());
            for (size_t j = 0; j < it->second.size(); j++) {
                VkDescriptorSetLayoutBinding& binding = bindings.at(j);
                DescriptorInfo& descriptorInfo = it->second.at(j);
                binding.binding = descriptorInfo.binding;
                binding.descriptorCount = descriptorInfo.count;
                binding.descriptorType = descriptorInfo.type;
                binding.pImmutableSamplers = nullptr;
                binding.stageFlags = descriptorInfo.shaderStageFlags;
            }
            descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            descriptorSetLayoutInfo.pBindings = bindings.data();
        } else {
            descriptorSetLayoutInfo.bindingCount = 0;
            descriptorSetLayoutInfo.pBindings = nullptr;
        }

        if (vkCreateDescriptorSetLayout(
                device->getVkDevice(), &descriptorSetLayoutInfo, nullptr,
                &descriptorSetLayout) != VK_SUCCESS) {
            Logfile::get()->throwError(
                    "Error in GraphicsPipeline::GraphicsPipeline: Failed to create descriptor set layout!");
        }
    }
}

const std::vector<InterfaceVariableDescriptor>& ShaderStages::getInputVariableDescriptors() const {
    if (!vertexShaderModule) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableDescriptors: No vertex shader exists!");
        return emptySet;
    }

    return vertexShaderModule->getInputVariableDescriptors();
}

bool ShaderStages::getHasInputVariable(const std::string& varName) const {
    if (!vertexShaderModule) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableLocation: No vertex shader exists!");
        return false;
    }

    return inputVariableNameLocationMap.find(varName) != inputVariableNameLocationMap.end();
}

uint32_t ShaderStages::getInputVariableLocation(const std::string& varName) const {
    if (!vertexShaderModule) {
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
    if (!vertexShaderModule) {
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

const InterfaceVariableDescriptor& ShaderStages::getInputVariableDescriptorFromLocation(uint32_t location) {
    if (!vertexShaderModule) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderStages::getInputVariableDescriptorFromLocation: No vertex shader exists!");
    }

    for (const InterfaceVariableDescriptor& descriptor : vertexShaderModule->getInputVariableDescriptors()) {
        if (location == descriptor.location) {
            return descriptor;
        }
    }
    sgl::Logfile::get()->throwError(
            "Error in ShaderStages::getInputVariableDescriptorFromLocation: Location not found!");
    return vertexShaderModule->getInputVariableDescriptors().front(); // To get rid of warning...
}

const InterfaceVariableDescriptor& ShaderStages::getInputVariableDescriptorFromName(const std::string& name) {
    if (!vertexShaderModule) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderStages::getInputVariableDescriptorFromName: No vertex shader exists!");
    }

    for (const InterfaceVariableDescriptor& descriptor : vertexShaderModule->getInputVariableDescriptors()) {
        if (name == descriptor.name) {
            return descriptor;
        }
    }
    sgl::Logfile::get()->throwError(
            "Error in ShaderStages::getInputVariableDescriptorFromName: Location not found!");
    return vertexShaderModule->getInputVariableDescriptors().front(); // To get rid of warning...
}

const std::map<uint32_t, std::vector<DescriptorInfo>>& ShaderStages::getDescriptorSetsInfo() const {
    return descriptorSetsInfo;
}

bool ShaderStages::hasDescriptorBinding(uint32_t setIdx, const std::string &descName) const {
    auto it = descriptorSetsInfo.find(setIdx);
    if (it == descriptorSetsInfo.end()) {
        return false;
    }
    const std::vector<DescriptorInfo>& descriptorSetInfo = it->second;
    for (const DescriptorInfo& descriptorInfo : descriptorSetInfo) {
        if (descriptorInfo.name == descName) {
            return true;
        }
    }
    return false;
}

const DescriptorInfo& ShaderStages::getDescriptorInfoByName(uint32_t setIdx, const std::string& descName) const {
    auto it = descriptorSetsInfo.find(setIdx);
    if (it == descriptorSetsInfo.end()) {
        Logfile::get()->throwError(
                "Error in ShaderStages::getDescriptorInfoByName: No descriptor set #" + std::to_string(setIdx)
                + " is used in these shaders.");
    }
    const std::vector<DescriptorInfo>& descriptorSetInfo = it->second;
    for (const DescriptorInfo& descriptorInfo : descriptorSetInfo) {
        if (descriptorInfo.name == descName) {
            return descriptorInfo;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::getDescriptorInfoByName: Couldn't find descriptor with name \"" + descName
            + "\" for descriptor set index " + std::to_string(setIdx) + ".");
    return descriptorSetInfo.front(); // To get rid of warning...
}

const DescriptorInfo& ShaderStages::getDescriptorInfoByBinding(uint32_t setIdx, uint32_t binding) const {
    auto it = descriptorSetsInfo.find(setIdx);
    if (it == descriptorSetsInfo.end()) {
        Logfile::get()->throwError(
                "Error in ShaderStages::getDescriptorInfoByBinding: No descriptor set #" + std::to_string(setIdx)
                + " is used in these shaders.");
    }
    const std::vector<DescriptorInfo>& descriptorSetInfo = it->second;
    for (const DescriptorInfo& descriptorInfo : descriptorSetInfo) {
        if (descriptorInfo.binding == binding) {
            return descriptorInfo;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::getDescriptorInfoByBinding: Couldn't find descriptor with binding \""
            + std::to_string(binding) + "\" for descriptor set index " + std::to_string(setIdx) + ".");
    return descriptorSetInfo.front(); // To get rid of warning...
}

uint32_t ShaderStages::getDescriptorBindingByName(uint32_t setIdx, const std::string& descName) const {
    auto it = descriptorSetsInfo.find(setIdx);
    if (it == descriptorSetsInfo.end()) {
        Logfile::get()->throwError(
                "Error in ShaderStages::getDescriptorBindingByName: No descriptor set #" + std::to_string(setIdx)
                + " is used in these shaders.");
    }
    const std::vector<DescriptorInfo>& descriptorSetInfo = it->second;
    for (const DescriptorInfo& descriptorInfo : descriptorSetInfo) {
        if (descriptorInfo.name == descName) {
            return descriptorInfo.binding;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::getDescriptorBindingByName: Couldn't find descriptor with name \"" + descName
            + "\" for descriptor set index " + std::to_string(setIdx) + ".");
    return std::numeric_limits<uint32_t>::max(); // To get rid of warning...
}

bool ShaderStages::getDescriptorBindingByNameOptional(
        uint32_t setIdx, const std::string& descName, uint32_t& binding) const {
    auto it = descriptorSetsInfo.find(setIdx);
    if (it == descriptorSetsInfo.end()) {
        return false;
    }
    const std::vector<DescriptorInfo>& descriptorSetInfo = it->second;
    for (const DescriptorInfo& descriptorInfo : descriptorSetInfo) {
        if (descriptorInfo.name == descName) {
            binding = descriptorInfo.binding;
            return true;
        }
    }
    return false;
}

ShaderModulePtr ShaderStages::findModuleId(const std::string& shaderModuleId) {
    for (ShaderModulePtr& shaderModule : shaderModules) {
        if (shaderModule->getShaderModuleId() == shaderModuleId) {
            return shaderModule;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::findModuleId: Could not find a module with the passed ID \""
            + shaderModuleId + "\".");
    return {};
}

size_t ShaderStages::findModuleIndexFromId(const std::string& shaderModuleId) const {
    for (size_t moduleIdx = 0; moduleIdx < shaderModules.size(); moduleIdx++) {
        if (shaderModules.at(moduleIdx)->getShaderModuleId() == shaderModuleId) {
            return moduleIdx;
        }
    }
    Logfile::get()->throwError(
            "Error in ShaderStages::findModuleIndexFromId: Could not find a module with the passed ID \""
            + shaderModuleId + "\".");
    return std::numeric_limits<size_t>::max();
}

}}
