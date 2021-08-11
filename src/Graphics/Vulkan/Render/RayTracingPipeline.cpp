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
#include <Math/Math.hpp>
#include "../Utils/Instance.hpp"
#include "../Utils/Device.hpp"
#include "../Buffers/Buffer.hpp"
#include "../Shader/Shader.hpp"
#include "RayTracingPipeline.hpp"

namespace sgl { namespace vk {

RayTracingPipelineInfo::RayTracingPipelineInfo(const ShaderStagesPtr& shaderStages) : shaderStages(shaderStages) {
    assert(shaderStages.get() != nullptr);
    reset();
}

void RayTracingPipelineInfo::reset() {
    maxPipelineRayRecursionDepth = 1;
}

RayTracingPipeline::RayTracingPipeline(Device* device, const RayTracingPipelineInfo& pipelineInfo)
        : Pipeline(device, pipelineInfo.shaderStages) {
    createPipelineLayout();

    const std::vector<ShaderModulePtr>& shaderModules = pipelineInfo.shaderStages->getShaderModules();
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages = pipelineInfo.shaderStages->getVkShaderStages();

#if VK_VERSION_1_2 && VK_HEADER_VERSION >= 162
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties =
            device->getPhysicalDeviceRayTracingPipelineProperties();

    if (pipelineInfo.maxPipelineRayRecursionDepth > rayTracingPipelineProperties.maxRayRecursionDepth) {
        Logfile::get()->throwError(
                "Error in RayTracingPipelineInfo::RayTracingPipelineInfo: The maximum pipeline ray recursion depth"
                "is larger than the maximum ray recursion depth supported.");
    }

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
    for (size_t shaderIdx = 0; shaderIdx < shaderModules.size(); shaderIdx++) {
        const ShaderModulePtr& shaderModule = shaderModules.at(shaderIdx);
        ShaderModuleType shaderModuleType = shaderModule->getShaderModuleType();

        VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo{};
        shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        shaderGroupCreateInfo.type = shaderModule->getRayTracingShaderGroupType();
        if (shaderModuleType == ShaderModuleType::RAYGEN || shaderModuleType == ShaderModuleType::MISS
                || shaderModuleType == ShaderModuleType::CALLABLE) {
            shaderGroupCreateInfo.generalShader = uint32_t(shaderIdx);
        } else if (shaderModuleType == ShaderModuleType::ANY_HIT) {
            shaderGroupCreateInfo.anyHitShader = uint32_t(shaderIdx);
        } else if (shaderModuleType == ShaderModuleType::CLOSEST_HIT) {
            shaderGroupCreateInfo.closestHitShader = uint32_t(shaderIdx);
        } else if (shaderModuleType == ShaderModuleType::INTERSECTION) {
            shaderGroupCreateInfo.intersectionShader = uint32_t(shaderIdx);
        } else {
            Logfile::get()->throwError(
                    "Error in RayTracingPipelineInfo::RayTracingPipelineInfo: Unsupported shader type.");
        }

        shaderGroups.push_back(shaderGroupCreateInfo);
    }

    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = uint32_t(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.groupCount = uint32_t(shaderGroups.size());
    pipelineCreateInfo.pGroups = shaderGroups.data();
    pipelineCreateInfo.maxPipelineRayRecursionDepth = pipelineInfo.maxPipelineRayRecursionDepth;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateRayTracingPipelinesKHR(
            device->getVkDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE,
            1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in RayTracingPipeline::RayTracingPipeline: Could not create a RayTracing pipeline.");
    }

    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t baseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
    const uint32_t handleSizeAligned = uint32_t(sgl::iceil(int(handleSize), int(handleAlignment))) * handleAlignment;

    const uint32_t shaderBindingTableSize = uint32_t(shaderGroups.size()) * handleSizeAligned;
    std::vector<uint8_t> shaderGroupHandleData(shaderBindingTableSize);
    auto _vkGetRayTracingShaderGroupHandlesKHR =
            (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(
                    device->getInstance()->getVkInstance(), "vkGetRayTracingShaderGroupHandlesKHR");
    if (_vkGetRayTracingShaderGroupHandlesKHR != nullptr) {
        if (_vkGetRayTracingShaderGroupHandlesKHR(
                device->getVkDevice(), pipeline, 0, uint32_t(shaderGroups.size()),
                shaderBindingTableSize, shaderGroupHandleData.data()) != VK_SUCCESS) {
            sgl::Logfile::get()->writeError(
                    "Error in RayTracingPipeline::RayTracingPipeline: Failed to retrieve shader group handles.");
        }
    } else {
        sgl::Logfile::get()->writeError(
                "Error in RayTracingPipeline::RayTracingPipeline: Missing vkGetRayTracingShaderGroupHandlesKHR.");
    }

    BufferPtr buffer(new Buffer(
            device, rayTracingPipelineProperties.shaderGroupHandleSize,
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
            | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU));

#endif
}

RayTracingPipeline::~RayTracingPipeline() = default;

}}
