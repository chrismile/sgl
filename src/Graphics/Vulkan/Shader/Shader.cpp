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

#include <Utils/File/Logfile.hpp>

#include "../Utils/Device.hpp"
#include "Shader.hpp"

namespace sgl { namespace vk {

ShaderModule::ShaderModule(
        Device* device, const std::string& shaderModuleId, ShaderModuleType shaderModuleType,
        const std::vector<uint32_t>& spirvCode) : device(device), shaderModuleType(shaderModuleType) {
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

void ShaderModule::createReflectData(const std::vector<uint32_t>& spirvCode) {
    // Create the reflect shader module.
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirvCode.size() * 4, spirvCode.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectCreateShaderModule failed!");
    }

    // Get reflection information on the input variables.
    uint32_t numInputVariables = 0;
    result = spvReflectEnumerateInputVariables(&module, &numInputVariables, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectEnumerateInputVariables failed!");
    }
    std::vector<SpvReflectInterfaceVariable*> inputVariables(numInputVariables);
    result = spvReflectEnumerateInputVariables(&module, &numInputVariables, inputVariables.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectEnumerateInputVariables failed!");
    }

    inputVariableDescriptors.reserve(numInputVariables);
    for (uint32_t varIdx = 0; varIdx < numInputVariables; varIdx++) {
        InterfaceVariableDescriptor varDesc;
        varDesc.location = inputVariables[varIdx]->location;
        varDesc.format = inputVariables[varIdx]->format;
        varDesc.name = inputVariables[varIdx]->name;
        inputVariableDescriptors.push_back(varDesc);
    }

    // Get reflection information on the descriptor sets (TODO).
    uint32_t numDescriptorSets = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &numDescriptorSets, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectEnumerateDescriptorSets failed!");
    }
    std::vector<SpvReflectDescriptorSet*> descriptorSets(numDescriptorSets);
    result = spvReflectEnumerateDescriptorSets(&module, &numDescriptorSets, descriptorSets.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectEnumerateDescriptorSets failed!");
    }

    //descriptorSetsInfo.resize(numDescriptorSets);
    for (uint32_t descSetIdx = 0; descSetIdx < numDescriptorSets; ++descSetIdx) {
        auto p_set = descriptorSets.at(descSetIdx);

        std::vector<DescriptorInfo> descriptorsInfo;
        descriptorsInfo.reserve(p_set->binding_count);
        for (uint32_t bindingIdx = 0; bindingIdx < p_set->binding_count; bindingIdx++) {
            DescriptorInfo descriptorInfo;
            descriptorInfo.binding = p_set->bindings[bindingIdx]->binding;
            descriptorInfo.type = VkDescriptorType(p_set->bindings[bindingIdx]->descriptor_type);
            descriptorInfo.name = p_set->bindings[bindingIdx]->name;
            descriptorInfo.count = p_set->bindings[bindingIdx]->count;
            descriptorInfo.shaderStageFlags = uint32_t(shaderModuleType);
            descriptorsInfo.push_back(descriptorInfo);
        }

        descriptorSetsInfo.insert(std::make_pair(p_set->set, descriptorsInfo));
    }

    spvReflectDestroyShaderModule(&module);
}

const std::vector<InterfaceVariableDescriptor>& ShaderModule::getInputVariableDescriptors() const {
    return inputVariableDescriptors;
}

const std::map<int, std::vector<DescriptorInfo>>& ShaderModule::getDescriptorSetsInfo() const {
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
            for (const InterfaceVariableDescriptor& varDesc : vertexShaderModule->getInputVariableDescriptors()) {
                inputVariableNameMap.insert(std::make_pair(varDesc.name, varDesc.location));
            }
        }

        mergeDescriptorSetsInfo(shaderModule->getDescriptorSetsInfo());
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
            for (const InterfaceVariableDescriptor& varDesc : vertexShaderModule->getInputVariableDescriptors()) {
                inputVariableNameMap.insert(std::make_pair(varDesc.name, varDesc.location));
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

void ShaderStages::mergeDescriptorSetsInfo(const std::map<int, std::vector<DescriptorInfo>>& newDescriptorSetsInfo) {
    for (auto& it : newDescriptorSetsInfo) {
        const std::vector<DescriptorInfo>& descriptorsInfoNew = it.second;
        std::vector<DescriptorInfo>& descriptorsInfo = descriptorSetsInfo[it.first];

        // Merge the descriptors in a map object.
        std::map<int, DescriptorInfo> descriptorsInfoMap;
        for (DescriptorInfo& descInfo : descriptorsInfo) {
            descriptorsInfoMap.insert(std::make_pair(descInfo.binding, descInfo));
        }
        for (const DescriptorInfo& descInfo : descriptorsInfoNew) {
            auto it = descriptorsInfoMap.find(descInfo.binding);
            if (it == descriptorsInfoMap.end()) {
                descriptorsInfoMap.insert(std::make_pair(descInfo.binding, descInfo));
            } else {
                if (it->second.type != descInfo.type) {
                    throw std::runtime_error(
                            std::string() + "Error in ShaderStages::mergeDescriptorSetsInfo: Attempted to merge "
                            + "incompatible descriptors \"" + it->second.name + "\" and \"" + descInfo.name + "\"!");
                }
                it->second.shaderStageFlags |= descInfo.shaderStageFlags;
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

void ShaderStages::createDescriptorSetLayouts() {
    descriptorSetLayouts.resize(descriptorSetsInfo.size());

    size_t i = 0;
    for (auto& entry : descriptorSetsInfo) {
        VkDescriptorSetLayout& descriptorSetLayout = descriptorSetLayouts.at(i);

        std::vector<VkDescriptorSetLayoutBinding> bindings(entry.second.size());
        for (size_t j = 0; j < entry.second.size(); j++) {
            VkDescriptorSetLayoutBinding& binding = bindings.at(j);
            DescriptorInfo& descriptorInfo = entry.second.at(j);
            binding.binding = descriptorInfo.binding;
            binding.descriptorCount = descriptorInfo.count;
            binding.descriptorType = descriptorInfo.type;
            binding.pImmutableSamplers = nullptr;
            binding.stageFlags = descriptorInfo.shaderStageFlags;
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(
                device->getVkDevice(), &descriptorSetLayoutInfo, nullptr,
                &descriptorSetLayout) != VK_SUCCESS) {
            Logfile::get()->throwError(
                    "Error in GraphicsPipeline::GraphicsPipeline: Failed to create descriptor set layout!");
        }

        i++;
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

int ShaderStages::getInputVariableLocation(const std::string& varName) const {
    if (!vertexShaderModule) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableLocation: No vertex shader exists!");
        return -1;
    }

    auto it = inputVariableNameMap.find(varName);
    if (it == inputVariableNameMap.end()) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderStages::getInputVariableLocation: Unknown variable name \"" + varName + "\"!");
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
}

const std::map<int, std::vector<DescriptorInfo>>& ShaderStages::getDescriptorSetsInfo() const {
    return descriptorSetsInfo;
}

}}
