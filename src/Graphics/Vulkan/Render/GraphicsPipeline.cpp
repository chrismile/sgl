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
#include "../Buffers/Framebuffer.hpp"
#include "GraphicsPipeline.hpp"

namespace sgl { namespace vk {

GraphicsPipelineInfo::GraphicsPipelineInfo(const ShaderStagesPtr& shaderStages) : shaderStages(shaderStages) {
    assert(shaderStages.get() != nullptr);
    reset();
}

void GraphicsPipelineInfo::reset() {
    inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

    rasterizerInfo = {};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo.lineWidth = 1.0f;
    rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.depthBiasConstantFactor = 0.0f;
    rasterizerInfo.depthBiasClamp = 0.0f;
    rasterizerInfo.depthBiasSlopeFactor = 0.0f;

    multisamplingInfo = {};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.minSampleShading = 1.0f;
    multisamplingInfo.rasterizationSamples = framebuffer ? framebuffer->getSampleCount() : VK_SAMPLE_COUNT_1_BIT;
    multisamplingInfo.pSampleMask = nullptr;
    multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingInfo.alphaToOneEnable = VK_FALSE;

    depthStencilInfo = {};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;
    depthStencilInfo.stencilTestEnable = VK_FALSE;
    depthStencilInfo.front = {};
    depthStencilInfo.back = {};

    colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;
}

void GraphicsPipelineInfo::setFramebuffer(FramebufferPtr framebuffer) {
    this->framebuffer = framebuffer;

    multisamplingInfo.rasterizationSamples = framebuffer->getSampleCount();

    viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = float(framebuffer->getWidth());
    viewport.height = float(framebuffer->getHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { uint32_t(framebuffer->getWidth()), uint32_t(framebuffer->getHeight()) };

    viewportStateInfo = {};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;
}

void GraphicsPipelineInfo::setBlendMode(BlendMode blendMode) {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    if (blendMode == BLENDING_MODE_OVERWRITE) {
        colorBlendAttachment.blendEnable = VK_FALSE;
    } else {
        colorBlendAttachment.blendEnable = VK_TRUE;
    }

    if (blendMode == BLENDING_MODE_OVERWRITE) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
        // Alpha blending
    else if (blendMode == BLENDING_MODE_BACK_TO_FRONT_STRAIGHT_ALPHA){
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else if (blendMode == BLENDING_MODE_BACK_TO_FRONT_PREMUL_ALPHA){
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else if (blendMode == BLENDING_MODE_FRONT_TO_BACK_PREMUL_ALPHA){
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
        // Additive blending modes & multiplicative blending
    else if (blendMode == BLENDING_MODE_BACK_ADDITIVE){
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    else if (blendMode == BLENDING_MODE_BACK_SUBTRACTIVE){
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
    }
    else if (blendMode == BLENDING_MODE_BACK_MULTIPLICATIVE){
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;
}

void GraphicsPipelineInfo::setInputAssemblyTopology(PrimitiveTopology primitiveTopology, bool primitiveRestartEnable) {
    inputAssemblyInfo.topology = VkPrimitiveTopology(primitiveTopology);
    inputAssemblyInfo.primitiveRestartEnable = primitiveRestartEnable;
}

void GraphicsPipelineInfo::setCullMode(CullMode cullMode) {
    rasterizerInfo.cullMode = VkCullModeFlagBits(cullMode);
}

void GraphicsPipelineInfo::setIsFrontFaceCcw(bool isFrontFaceCcw) {
    rasterizerInfo.frontFace = isFrontFaceCcw ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
}

void GraphicsPipelineInfo::setEnableMinSampleShading(bool enableMinSampleShading, float minSampleShading) {
    multisamplingInfo.sampleShadingEnable = enableMinSampleShading;
    multisamplingInfo.minSampleShading = minSampleShading;
}

void GraphicsPipelineInfo::setEnableDepthTest(bool enableDepthTest) {
    depthStencilInfo.depthTestEnable = enableDepthTest;
}

void GraphicsPipelineInfo::setEnableDepthWrite(bool enableDepthWrite) {
    depthStencilInfo.depthWriteEnable = enableDepthWrite;
}

void GraphicsPipelineInfo::setEnableStencilTest(bool enableStencilTest) {
    depthStencilInfo.stencilTestEnable = enableStencilTest;
}

void GraphicsPipelineInfo::setVertexBufferBinding(
        uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
    if (vertexInputBindingDescriptions.size() <= binding) {
        vertexInputBindingDescriptions.resize(binding + 1);
    }
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = binding;
    bindingDescription.stride = stride;
    bindingDescription.inputRate = inputRate;
    vertexInputBindingDescriptions.at(binding) = bindingDescription;

    vertexInputInfo.vertexBindingDescriptionCount = uint32_t(vertexInputBindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
}

void GraphicsPipelineInfo::setInputAttributeDescription(
        uint32_t bufferBinding, uint32_t bufferOffset, uint32_t attributeLocation) {
    if (vertexInputAttributeDescriptions.size() <= attributeLocation) {
        vertexInputAttributeDescriptions.resize(attributeLocation + 1);
    }

    const InterfaceVariableDescriptor& inputVariableDescriptor = shaderStages->getInputVariableDescriptorFromLocation(
            attributeLocation);

    VkVertexInputAttributeDescription attributeDescription = {};
    attributeDescription.location = attributeLocation;
    attributeDescription.binding = bufferBinding;
    attributeDescription.format = VkFormat(inputVariableDescriptor.format);
    attributeDescription.offset = bufferOffset;
    vertexInputAttributeDescriptions.at(attributeLocation) = attributeDescription;
}

void GraphicsPipelineInfo::setInputAttributeDescription(
        uint32_t bufferBinding, uint32_t bufferOffset, const std::string& attributeName) {
    const InterfaceVariableDescriptor& inputVariableDescriptor = shaderStages->getInputVariableDescriptorFromName(
            attributeName);
    uint32_t attributeLocation = inputVariableDescriptor.location;

    if (vertexInputAttributeDescriptions.size() <= attributeLocation) {
        vertexInputAttributeDescriptions.resize(attributeLocation + 1);
    }

    VkVertexInputAttributeDescription attributeDescription = {};
    attributeDescription.location = attributeLocation;
    attributeDescription.binding = bufferBinding;
    attributeDescription.format = VkFormat(inputVariableDescriptor.format);
    attributeDescription.offset = bufferOffset;
    vertexInputAttributeDescriptions.at(attributeLocation) = attributeDescription;

    vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(vertexInputAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
}




GraphicsPipeline::GraphicsPipeline(Device* device, const GraphicsPipelineInfo& pipelineInfo)
        : Pipeline(device, pipelineInfo.shaderStages), framebuffer(pipelineInfo.framebuffer),
          vertexInputBindingDescriptions(pipelineInfo.vertexInputBindingDescriptions),
          vertexInputAttributeDescriptions(pipelineInfo.vertexInputAttributeDescriptions) {
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts =
            pipelineInfo.shaderStages->getVkDescriptorSetLayouts();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(
            device->getVkDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in GraphicsPipeline::GraphicsPipeline: Could not create a pipeline layout.");
    }

    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesCreateInfo =
            pipelineInfo.shaderStages->getVkShaderStages();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = shaderStagesCreateInfo.size();
    pipelineCreateInfo.pStages = shaderStagesCreateInfo.data();
    pipelineCreateInfo.pVertexInputState = &pipelineInfo.vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &pipelineInfo.inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &pipelineInfo.viewportStateInfo;
    pipelineCreateInfo.pRasterizationState = &pipelineInfo.rasterizerInfo;
    pipelineCreateInfo.pMultisampleState = &pipelineInfo.multisamplingInfo;
    pipelineCreateInfo.pDepthStencilState = &pipelineInfo.depthStencilInfo;
    pipelineCreateInfo.pColorBlendState = &pipelineInfo.colorBlendInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = framebuffer->getVkRenderPass();
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(
            device->getVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo,
            nullptr, &pipeline) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in GraphicsPipeline::GraphicsPipeline: Could not create a graphics pipeline.");
    }
}

}}
