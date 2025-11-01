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
#include "Pipeline.hpp"

namespace sgl { namespace vk {

Pipeline::Pipeline(Device* device, ShaderStagesPtr shaderStages) : device(device), shaderStages(std::move(shaderStages)) {
#ifdef VK_EXT_shader_64bit_indexing
    if (this->shaderStages && this->shaderStages->getUse64BitIndexing()) {
        pipelineCreateFlags2CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO;
        pipelineCreateFlags2CreateInfo.flags = VK_PIPELINE_CREATE_2_64_BIT_INDEXING_BIT_EXT;
    }
#endif
}

void Pipeline::createPipelineLayout() {
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = shaderStages->getVkDescriptorSetLayouts();
    const std::vector<VkPushConstantRange>& pushConstantRanges = shaderStages->getVkPushConstantRanges();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = uint32_t(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = uint32_t(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    if (vkCreatePipelineLayout(
            device->getVkDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Pipeline::createPipelineLayout: Could not create the pipeline layout.");
    }
}

void Pipeline::setPipelineCreateInfoPNextInternal(const void*& pNext) {
#ifdef VK_EXT_shader_64bit_indexing
    if (shaderStages->getUse64BitIndexing()) {
        pNext = &pipelineCreateFlags2CreateInfo;
    }
#endif
}

Pipeline::~Pipeline() {
    vkDestroyPipeline(device->getVkDevice(), pipeline, nullptr);
    vkDestroyPipelineLayout(device->getVkDevice(), pipelineLayout, nullptr);
}

}}
