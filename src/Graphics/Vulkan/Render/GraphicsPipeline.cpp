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
    if (!shaderStages) {
        Logfile::get()->throwError(
                "Error in GraphicsPipelineInfo::GraphicsPipelineInfo: shaderStages is not valid.");
    }
    reset();
}

void GraphicsPipelineInfo::reset() {
    inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    colorBlendAttachments.resize(1);
    VkPipelineColorBlendAttachmentState& colorBlendAttachment = colorBlendAttachments.front();
    colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    currentBlendModes = { BlendMode::CUSTOM };
    setBlendMode(BlendMode::OVERWRITE);

    colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = uint32_t(colorBlendAttachments.size());
    colorBlendInfo.pAttachments = colorBlendAttachments.data();
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
    rasterizerInfo.frontFace = coordinateOriginBottomLeft ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
}

void GraphicsPipelineInfo::_resizeColorAttachments(size_t newCount) {
    if (colorBlendAttachments.size() != newCount) {
        size_t oldSize = colorBlendAttachments.size();
        colorBlendAttachments.resize(newCount);
        currentBlendModes.resize(newCount);
        for (size_t attachmentIdx = oldSize; attachmentIdx < newCount; attachmentIdx++) {
            VkPipelineColorBlendAttachmentState& colorBlendAttachment = colorBlendAttachments.at(attachmentIdx);
            colorBlendAttachment = {};
            colorBlendAttachment.colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                    | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            setBlendMode(BlendMode::OVERWRITE, uint32_t(attachmentIdx));
        }
        colorBlendInfo.attachmentCount = uint32_t(colorBlendAttachments.size());
        colorBlendInfo.pAttachments = colorBlendAttachments.data();
    }
}

void GraphicsPipelineInfo::setFramebuffer(const FramebufferPtr& framebuffer, uint32_t subpassIndex) {
    this->framebuffer = framebuffer;
    this->subpassIndex = subpassIndex;
    _resizeColorAttachments(framebuffer->getColorAttachmentCount());

    multisamplingInfo.rasterizationSamples = framebuffer->getSampleCount();

    viewport = {};
    viewport.x = 0.0f;
    viewport.width = float(framebuffer->getWidth());
    if (coordinateOriginBottomLeft) {
        viewport.y = float(framebuffer->getHeight());
        viewport.height = -float(framebuffer->getHeight());
    } else {
        viewport.y = 0.0f;
        viewport.height = float(framebuffer->getHeight());
    }
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

void GraphicsPipelineInfo::setColorWriteEnabled(bool enableColorWrite, uint32_t colorAttachmentIndex) {
    if (colorAttachmentIndex >= colorBlendAttachments.size()) {
        _resizeColorAttachments(colorAttachmentIndex + 1);
    }
    VkPipelineColorBlendAttachmentState& colorBlendAttachment = colorBlendAttachments.at(colorAttachmentIndex);
    if (enableColorWrite) {
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    } else {
        colorBlendAttachment.colorWriteMask = 0;
    }
}

void GraphicsPipelineInfo::setBlendMode(BlendMode blendMode, uint32_t colorAttachmentIndex) {
    if (colorAttachmentIndex >= currentBlendModes.size()) {
        _resizeColorAttachments(colorAttachmentIndex + 1);
    }
    VkPipelineColorBlendAttachmentState& colorBlendAttachment = colorBlendAttachments.at(colorAttachmentIndex);
    BlendMode& currentBlendMode = currentBlendModes.at(colorAttachmentIndex);

    currentBlendMode = blendMode;
    if (blendMode == BlendMode::OVERWRITE) {
        colorBlendAttachment.blendEnable = VK_FALSE;
    } else {
        colorBlendAttachment.blendEnable = VK_TRUE;
    }

    if (blendMode == BlendMode::OVERWRITE) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
        // Alpha blending
    else if (blendMode == BlendMode::BACK_TO_FRONT_STRAIGHT_ALPHA) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else if (blendMode == BlendMode::BACK_TO_FRONT_PREMUL_ALPHA) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else if (blendMode == BlendMode::FRONT_TO_BACK_PREMUL_ALPHA) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
        // Additive blending modes & multiplicative blending
    else if (blendMode == BlendMode::BACK_ADDITIVE) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    else if (blendMode == BlendMode::ONE) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    else if (blendMode == BlendMode::BACK_SUBTRACTIVE) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
    }
    else if (blendMode == BlendMode::BACK_MULTIPLICATIVE) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
}

void GraphicsPipelineInfo::setBlendModeCustom(
        VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp,
        VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor, VkBlendOp alphaBlendOp,
        uint32_t colorAttachmentIndex) {
    if (colorAttachmentIndex >= currentBlendModes.size()) {
        _resizeColorAttachments(colorAttachmentIndex + 1);
    }
    VkPipelineColorBlendAttachmentState& colorBlendAttachment = colorBlendAttachments.at(colorAttachmentIndex);
    BlendMode& currentBlendMode = currentBlendModes.at(colorAttachmentIndex);

    currentBlendMode = BlendMode::CUSTOM;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = alphaBlendOp;
}

void GraphicsPipelineInfo::setInputAssemblyTopology(PrimitiveTopology primitiveTopology, bool primitiveRestartEnable) {
    inputAssemblyInfo.topology = VkPrimitiveTopology(primitiveTopology);
    inputAssemblyInfo.primitiveRestartEnable = primitiveRestartEnable;
}

void GraphicsPipelineInfo::setCullMode(CullMode cullMode) {
    rasterizerInfo.cullMode = VkCullModeFlagBits(cullMode);
}

void GraphicsPipelineInfo::setIsFrontFaceCcw(bool isFrontFaceCcw) {
    if (coordinateOriginBottomLeft) {
        isFrontFaceCcw = !isFrontFaceCcw;
    }
    rasterizerInfo.frontFace = isFrontFaceCcw ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
}

void GraphicsPipelineInfo::setMinSampleShading(bool enableMinSampleShading, float minSampleShading) {
    multisamplingInfo.sampleShadingEnable = enableMinSampleShading;
    multisamplingInfo.minSampleShading = minSampleShading;
}

void GraphicsPipelineInfo::setUseCoordinateOriginBottomLeft(bool bottomLeft) {
    if (coordinateOriginBottomLeft != bottomLeft) {
        if (rasterizerInfo.frontFace == VK_FRONT_FACE_COUNTER_CLOCKWISE) {
            rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        } else {
            rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }
    }
    coordinateOriginBottomLeft = bottomLeft;
}

void GraphicsPipelineInfo::setDepthTestEnabled(bool enableDepthTest) {
    depthStencilInfo.depthTestEnable = enableDepthTest;
}

void GraphicsPipelineInfo::setDepthWriteEnabled(bool enableDepthWrite) {
    depthStencilInfo.depthWriteEnable = enableDepthWrite;
}

void GraphicsPipelineInfo::setDepthCompareOp(VkCompareOp compareOp) {
    depthStencilInfo.depthCompareOp = compareOp;
}

void GraphicsPipelineInfo::setStencilTestEnabled(bool enableStencilTest) {
    depthStencilInfo.stencilTestEnable = enableStencilTest;
}

void GraphicsPipelineInfo::setVertexBufferBinding(
        uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
    bool isAlreadySet =
            binding < vertexInputBindingDescriptions.size() && vertexInputBindingDescriptionsUsed.at(binding);
    if (vertexInputBindingDescriptions.size() <= binding) {
        vertexInputBindingDescriptions.resize(binding + 1);
        vertexInputBindingDescriptionsUsed.resize(binding + 1);
    }
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = binding;
    bindingDescription.stride = stride;
    bindingDescription.inputRate = inputRate;
    vertexInputBindingDescriptions.at(binding) = bindingDescription;
    vertexInputBindingDescriptionsUsed.at(binding) = true;

    if (isAlreadySet) {
        vertexInputBindingDescriptionsUnsorted.clear();
        for (size_t i = 0; i < vertexInputBindingDescriptions.size(); i++) {
            if (vertexInputBindingDescriptionsUsed.at(i)) {
                vertexInputBindingDescriptionsUnsorted.push_back(vertexInputBindingDescriptions.at(i));
            }
        }
    } else {
        vertexInputBindingDescriptionsUnsorted.push_back(bindingDescription);
    }

    vertexInputInfo.vertexBindingDescriptionCount = uint32_t(vertexInputBindingDescriptionsUnsorted.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescriptionsUnsorted.data();
}

void GraphicsPipelineInfo::setInputAttributeDescription(
        uint32_t bufferBinding, uint32_t bufferOffset, uint32_t attributeLocation) {
    bool isAlreadySet =
            attributeLocation < vertexInputAttributeDescriptions.size()
            && vertexInputAttributeDescriptionsUsed.at(attributeLocation);
    if (vertexInputAttributeDescriptions.size() <= attributeLocation) {
        vertexInputAttributeDescriptions.resize(attributeLocation + 1);
        vertexInputAttributeDescriptionsUsed.resize(attributeLocation + 1);
    }

    const InterfaceVariableDescriptor& inputVariableDescriptor =
            shaderStages->getInputVariableDescriptorFromLocation(attributeLocation);

    VkVertexInputAttributeDescription attributeDescription = {};
    attributeDescription.location = attributeLocation;
    attributeDescription.binding = bufferBinding;
    attributeDescription.format = VkFormat(inputVariableDescriptor.format);
    attributeDescription.offset = bufferOffset;
    vertexInputAttributeDescriptions.at(attributeLocation) = attributeDescription;
    vertexInputAttributeDescriptionsUsed.at(attributeLocation) = true;

    if (isAlreadySet) {
        vertexInputAttributeDescriptionsUnsorted.clear();
        for (size_t i = 0; i < vertexInputAttributeDescriptions.size(); i++) {
            if (vertexInputAttributeDescriptionsUsed.at(i)) {
                vertexInputAttributeDescriptionsUnsorted.push_back(vertexInputAttributeDescriptions.at(i));
            }
        }
    } else {
        vertexInputAttributeDescriptionsUnsorted.push_back(attributeDescription);
    }

    vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(vertexInputAttributeDescriptionsUnsorted.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptionsUnsorted.data();
}

void GraphicsPipelineInfo::setInputAttributeDescription(
        uint32_t bufferBinding, uint32_t bufferOffset, const std::string& attributeName) {
    const InterfaceVariableDescriptor& inputVariableDescriptor = shaderStages->getInputVariableDescriptorFromName(
            attributeName);
    uint32_t attributeLocation = inputVariableDescriptor.location;

    bool isAlreadySet =
            attributeLocation < vertexInputAttributeDescriptions.size()
            && vertexInputAttributeDescriptionsUsed.at(attributeLocation);
    if (vertexInputAttributeDescriptions.size() <= attributeLocation) {
        vertexInputAttributeDescriptions.resize(attributeLocation + 1);
        vertexInputAttributeDescriptionsUsed.resize(attributeLocation + 1);
    }

    VkVertexInputAttributeDescription attributeDescription = {};
    attributeDescription.location = attributeLocation;
    attributeDescription.binding = bufferBinding;
    attributeDescription.format = VkFormat(inputVariableDescriptor.format);
    attributeDescription.offset = bufferOffset;
    vertexInputAttributeDescriptions.at(attributeLocation) = attributeDescription;
    vertexInputAttributeDescriptionsUsed.at(attributeLocation) = true;

    if (isAlreadySet) {
        vertexInputAttributeDescriptionsUnsorted.clear();
        for (size_t i = 0; i < vertexInputAttributeDescriptions.size(); i++) {
            if (vertexInputAttributeDescriptionsUsed.at(i)) {
                vertexInputAttributeDescriptionsUnsorted.push_back(vertexInputAttributeDescriptions.at(i));
            }
        }
    } else {
        vertexInputAttributeDescriptionsUnsorted.push_back(attributeDescription);
    }

    vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(vertexInputAttributeDescriptionsUnsorted.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptionsUnsorted.data();
}

void GraphicsPipelineInfo::setVertexBufferBindingByLocationIndex(
        const std::string& attributeName, uint32_t stride, VkVertexInputRate inputRate) {
    uint32_t vertexAttributeBinding = shaderStages->getInputVariableLocationIndex(attributeName);
    setVertexBufferBinding(vertexAttributeBinding, stride, inputRate);
    setInputAttributeDescription(vertexAttributeBinding, 0, attributeName);
}

void GraphicsPipelineInfo::setVertexBufferBindingByLocationIndexOptional(
        const std::string& attributeName, uint32_t stride, VkVertexInputRate inputRate) {
    if (shaderStages->getHasInputVariable(attributeName)) {
        uint32_t vertexNormalBinding = shaderStages->getInputVariableLocationIndex(attributeName);
        setVertexBufferBinding(vertexNormalBinding, stride, inputRate);
        setInputAttributeDescription(vertexNormalBinding, 0, attributeName);
    }
}



GraphicsPipeline::GraphicsPipeline(Device* device, const GraphicsPipelineInfo& pipelineInfo)
        : Pipeline(device, pipelineInfo.shaderStages),
          framebuffer(pipelineInfo.framebuffer), subpassIndex(pipelineInfo.subpassIndex),
          vertexInputBindingDescriptions(pipelineInfo.vertexInputBindingDescriptions),
          vertexInputAttributeDescriptions(pipelineInfo.vertexInputAttributeDescriptions) {
    createPipelineLayout();

    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesCreateInfo =
            pipelineInfo.shaderStages->getVkShaderStages();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = uint32_t(shaderStagesCreateInfo.size());
    pipelineCreateInfo.pStages = shaderStagesCreateInfo.data();
    pipelineCreateInfo.pVertexInputState = &pipelineInfo.vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &pipelineInfo.inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &pipelineInfo.viewportStateInfo;
    pipelineCreateInfo.pRasterizationState = &pipelineInfo.rasterizerInfo;
    pipelineCreateInfo.pMultisampleState = &pipelineInfo.multisamplingInfo;
    if (framebuffer->getHasDepthStencilAttachment()) {
        pipelineCreateInfo.pDepthStencilState = &pipelineInfo.depthStencilInfo;
    }
    pipelineCreateInfo.pColorBlendState = &pipelineInfo.colorBlendInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = framebuffer->getVkRenderPass();
    pipelineCreateInfo.subpass = subpassIndex;
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
