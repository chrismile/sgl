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

#ifndef SGL_SHADER_HPP
#define SGL_SHADER_HPP

#include <vector>
#include <memory>
#include <map>

#include <vulkan/vulkan.h>
#include <Graphics/Vulkan/libs/SPIRV-Reflect/spirv_reflect.h>

namespace sgl { namespace vk {

enum class ShaderModuleType {
    VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
    TESSELATION_CONTROL = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
    TESSELATION_EVALUATION = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
    GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT,
    FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
    COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT,
    RAYGEN = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    ANY_HIT = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
    CLOSEST_HIT = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    MISS = VK_SHADER_STAGE_MISS_BIT_KHR,
    INTERSECTION = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
    CALLABLE = VK_SHADER_STAGE_CALLABLE_BIT_KHR,
    TASK = VK_SHADER_STAGE_TASK_BIT_NV,
    MESH = VK_SHADER_STAGE_MESH_BIT_NV
};

struct DLL_OBJECT InterfaceVariableDescriptor {
    uint32_t location;
    SpvReflectFormat format; ///< SpvReflectFormat is a subset of VkFormat valid for interface variables.
    std::string name;
};

struct DLL_OBJECT DescriptorInfo {
    uint32_t binding;
    VkDescriptorType type;
    std::string name;
    VkShaderStageFlags shaderStageFlags;
    uint32_t count;
    bool readOnly;
};
//struct DescriptorSetInfo {
//    int descriptorSetIndex;
//    std::vector<DescriptorInfo> descriptorInfo;
//};

class Device;

class DLL_OBJECT ShaderModule {
public:
    // Called by ShaderManager.
    ShaderModule(
            Device* device, const std::string& shaderModuleId, ShaderModuleType shaderModuleType,
            const std::vector<uint32_t>& spirvCode);
    ~ShaderModule();

    bool getIsRayTracingShader();
    void setRayTracingShaderGroupType(VkRayTracingShaderGroupTypeKHR groupType);
    inline VkRayTracingShaderGroupTypeKHR getRayTracingShaderGroupType() const { return rayTracingShaderGroupType; }

    /**
     * Returns, e.g.: {InterfaceVariableDescriptor(0, SPV_REFLECT_FORMAT_R32G32B32_SFLOAT, "vertexPosition")}
     * ... for a shader with a single interface variable "vertexPosition" defined by, e.g.,
     * "layout(location = 0) in vec3 vertexPosition;" in GLSL.
     *
     * @return A list of input interface variable descriptors for this shader module.
     */
    const std::vector<InterfaceVariableDescriptor>& getInputVariableDescriptors() const;

    const std::map<uint32_t, std::vector<DescriptorInfo>>& getDescriptorSetsInfo() const;

    inline const std::vector<VkPushConstantRange>& getVkPushConstantRanges() const { return pushConstantRanges; }

    inline const std::string& getShaderModuleId() const { return shaderModuleId; }
    inline ShaderModuleType getShaderModuleType() const { return shaderModuleType; }

    // Get Vulkan data.
    inline VkShaderModule getVkShaderModule() { return vkShaderModule; }

private:
    void createReflectData(const std::vector<uint32_t>& spirvCode);

    // General information.
    Device* device;
    std::string shaderModuleId;
    ShaderModuleType shaderModuleType;

    // Vulkan data.
    VkShaderModule vkShaderModule;

    // SPIR-V reflection data.
    std::vector<InterfaceVariableDescriptor> inputVariableDescriptors;
    std::map<uint32_t, std::vector<DescriptorInfo>> descriptorSetsInfo; ///< set index -> descriptor set info
    std::vector<VkPushConstantRange> pushConstantRanges;

    /// The ray tracing shader group type can be specified if this is a ray tracing shader module.
    VkRayTracingShaderGroupTypeKHR rayTracingShaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
};

typedef std::shared_ptr<ShaderModule> ShaderModulePtr;


/**
 * This class represents a selection of shader modules unified into a list of shader stages.
 * It will be used in the VkGraphicsPipelineCreateInfo struct.
 */
class DLL_OBJECT ShaderStages {
public:
    ShaderStages(
            Device* device, std::vector<ShaderModulePtr>& shaderModules);
    ShaderStages(
            Device* device, std::vector<ShaderModulePtr>& shaderModules, const std::vector<std::string>& functionNames);
    ~ShaderStages();

    /// Returns the input variable descriptors of the vertex shader. NOTE: A vertex shader must exist for this to work!
    const std::vector<InterfaceVariableDescriptor>& getInputVariableDescriptors() const;
    uint32_t getInputVariableLocation(const std::string& varName) const;
    const InterfaceVariableDescriptor& getInputVariableDescriptorFromLocation(uint32_t location);
    const InterfaceVariableDescriptor& getInputVariableDescriptorFromName(const std::string& name);
    const std::map<uint32_t, std::vector<DescriptorInfo>>& getDescriptorSetsInfo() const;
    const DescriptorInfo& getDescriptorInfoByName(uint32_t setIdx, const std::string& descName) const;
    const DescriptorInfo& getDescriptorInfoByBinding(uint32_t setIdx, uint32_t binding) const;

    inline std::vector<ShaderModulePtr>& getShaderModules() { return shaderModules; }

    inline const std::vector<VkPipelineShaderStageCreateInfo>& getVkShaderStages() const { return vkShaderStages; }
    inline const std::vector<VkDescriptorSetLayout>& getVkDescriptorSetLayouts() const { return descriptorSetLayouts; }
    inline const std::vector<VkPushConstantRange>& getVkPushConstantRanges() const { return pushConstantRanges; }

private:
    void mergeDescriptorSetsInfo(const std::map<uint32_t, std::vector<DescriptorInfo>>& newDescriptorSetsInfo);
    void mergePushConstantRanges(const std::vector<VkPushConstantRange>& newPushConstantRanges);
    void createDescriptorSetLayouts();

    Device* device;
    std::vector<ShaderModulePtr> shaderModules;
    ShaderModulePtr vertexShaderModule; // Optional
    std::map<uint32_t, std::vector<DescriptorInfo>> descriptorSetsInfo; ///< set index -> descriptor set info
    std::map<std::string, std::vector<DescriptorInfo>> descriptorSetNameBindingMap; ///< name -> binding
    std::map<std::string, uint32_t> inputVariableNameMap; ///< input interface variable name -> location
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts; ///< created from descriptorSetsInfo for use in Vulkan
    std::vector<VkPushConstantRange> pushConstantRanges;
    uint32_t numDescriptorSets = 0;
    std::vector<VkPipelineShaderStageCreateInfo> vkShaderStages;

    // for getInputVariableDescriptors.
    std::vector<InterfaceVariableDescriptor> emptySet;
};

typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;

}}

#endif //SGL_SHADER_HPP
