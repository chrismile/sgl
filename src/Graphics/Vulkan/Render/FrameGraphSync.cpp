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

#include <unordered_map>
#include <Utils/File/Logfile.hpp>
#include "Pipeline.hpp"
#include "FrameGraphSync.hpp"
#include "GraphicsPipeline.hpp"

namespace sgl { namespace vk {

typedef std::unordered_map<VkShaderStageFlagBits, VkPipelineStageFlags2KHR> ShaderStageFlagsToPipelineFlagsMap;
static const ShaderStageFlagsToPipelineFlagsMap pipelineStageSrcMap = {
        { VK_SHADER_STAGE_VERTEX_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_GEOMETRY_BIT, VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_FRAGMENT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_MISS_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_INTERSECTION_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_CALLABLE_BIT_KHR, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR },
        { VK_SHADER_STAGE_TASK_BIT_NV, VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV },
        { VK_SHADER_STAGE_MESH_BIT_NV, VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV },
};

static const ShaderStageFlagsToPipelineFlagsMap pipelineStageDstMap = {
        { VK_SHADER_STAGE_VERTEX_BIT, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR },
        { VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_GEOMETRY_BIT, VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_FRAGMENT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_MISS_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_INTERSECTION_BIT_KHR, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR },
        { VK_SHADER_STAGE_CALLABLE_BIT_KHR, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR },
        { VK_SHADER_STAGE_TASK_BIT_NV, VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV },
        { VK_SHADER_STAGE_MESH_BIT_NV, VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV },
};

VkPipelineStageFlagBits2KHR mapToAccessMask(const ResourceAccess& resourceAccess, bool source) {
    ResourceUsage resourceUsage = resourceAccess.resourceUsage;
    VkShaderStageFlagBits shaderStage = resourceAccess.shaderStage;

    if (resourceUsage == ResourceUsage::INPUT_VARIABLE || resourceUsage == ResourceUsage::INDEX_BUFFER) {
        return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR;
    } else if (resourceUsage == ResourceUsage::DESCRIPTOR_BINDING) {
        ShaderStageFlagsToPipelineFlagsMap::const_iterator it;
        if (source) {
            it = pipelineStageSrcMap.find(shaderStage);
            if (it == pipelineStageSrcMap.end()) {
                Logfile::get()->throwError("Error in mapToAccessMask: Invalid shader stage.");
            }
        } else {
            it = pipelineStageDstMap.find(shaderStage);
            if (it == pipelineStageDstMap.end()) {
                Logfile::get()->throwError("Error in mapToAccessMask: Invalid shader stage.");
            }
        }
        return it->second;
    } else if (resourceUsage == ResourceUsage::OUTPUT_ATTACHMENT) {
        if (resourceAccess.outputAttachmentType == OutputAttachmentType::COLOR
                || resourceAccess.outputAttachmentType == OutputAttachmentType::RESOLVE) {
            return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        } else if (resourceAccess.outputAttachmentType == OutputAttachmentType::DEPTH_STENCIL) {
            if (source) {
                return VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
            } else {
                return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR;
            }
        }
    } else if (resourceUsage == ResourceUsage::COPY_SRC || resourceUsage == ResourceUsage::COPY_DST) {
        return VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
    } else if (resourceUsage == ResourceUsage::BLIT_SRC || resourceUsage == ResourceUsage::BLIT_DST) {
        return VK_PIPELINE_STAGE_2_BLIT_BIT_KHR;
    }
}

VkAccessFlagBits2KHR mapToStageMask(
        const ResourceAccess& resourceAccess, bool source, GraphicsPipelineInfo* pipelineInfo) {
    ResourceUsage resourceUsage = resourceAccess.resourceUsage;
    VkShaderStageFlagBits shaderStage = resourceAccess.shaderStage;
    VkDescriptorType descriptorType = resourceAccess.descriptorType;

    if (resourceUsage == ResourceUsage::INPUT_VARIABLE) {
        return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR;
    } else if (resourceUsage == ResourceUsage::INDEX_BUFFER) {
        return VK_ACCESS_2_INDEX_READ_BIT_KHR;
    } else if (resourceUsage == ResourceUsage::DESCRIPTOR_BINDING) {
        if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
                || descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                || descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
                || descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            if (resourceAccess.writeAccess) {
                return VK_ACCESS_2_SHADER_WRITE_BIT_KHR | VK_ACCESS_2_SHADER_READ_BIT_KHR;
            } else {
                return VK_ACCESS_2_SHADER_READ_BIT_KHR;
            }
        } else if (descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            return VK_ACCESS_2_UNIFORM_READ_BIT_KHR;
        } else if (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                || descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                || descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) {
            return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT_KHR;
        } else if (descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
            return VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT_KHR;
        } else if (descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
            if (resourceAccess.writeAccess) {
                return VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            } else {
                return VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            }
        }
    } else if (resourceUsage == ResourceUsage::OUTPUT_ATTACHMENT) {
        if (resourceAccess.outputAttachmentType == OutputAttachmentType::COLOR
                || resourceAccess.outputAttachmentType == OutputAttachmentType::RESOLVE) {
            if (pipelineInfo->getIsBlendEnabled()) {
                return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
            } else {
                return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR;
            }
        } else if (resourceAccess.outputAttachmentType == OutputAttachmentType::DEPTH_STENCIL) {
            if (pipelineInfo->getDepthWriteEnabled()) {
                return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR
                        | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR;
            } else {
                return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR;
            }
        }
    } else if (resourceUsage == ResourceUsage::COPY_SRC || resourceUsage == ResourceUsage::BLIT_SRC) {
        return VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
    } else if (resourceUsage == ResourceUsage::COPY_DST || resourceUsage == ResourceUsage::BLIT_DST) {
        return VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
    }
}

} }
