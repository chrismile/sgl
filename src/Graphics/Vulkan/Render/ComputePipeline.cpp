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
#include "../Shader/Shader.hpp"
#include "ComputePipeline.hpp"

namespace sgl { namespace vk {

ComputePipelineInfo::ComputePipelineInfo(const ShaderStagesPtr& shaderStages) : shaderStages(shaderStages) {
    if (!shaderStages) {
        Logfile::get()->throwError(
                "Error in ComputePipelineInfo::ComputePipelineInfo: shaderStages is not valid.");
    }
}

ComputePipeline::ComputePipeline(Device* device, const ComputePipelineInfo& pipelineInfo)
        : Pipeline(device, pipelineInfo.shaderStages) {
    createPipelineLayout();

    const std::vector<ShaderModulePtr>& shaderModules = pipelineInfo.shaderStages->getShaderModules();
    if (shaderModules.size() != 1 || shaderModules.front()->getShaderModuleType() != ShaderModuleType::COMPUTE) {
        Logfile::get()->throwError(
                "Error in ComputePipeline::ComputePipeline: Expected exactly one compute shader module.");
    }

    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages = pipelineInfo.shaderStages->getVkShaderStages();

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stage = shaderStages.front();
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateComputePipelines(
            device->getVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo,
            nullptr, &pipeline) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ComputePipeline::ComputePipeline: Could not create a Compute pipeline.");
    }
}

ComputePipeline::~ComputePipeline() = default;

}}
