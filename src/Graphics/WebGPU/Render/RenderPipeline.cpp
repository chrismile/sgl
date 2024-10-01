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

#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "../Buffer/Framebuffer.hpp"
#include "../Texture/Texture.hpp"
#include "../Shader/Shader.hpp"
#include "RenderPipeline.hpp"

namespace sgl { namespace webgpu {

RenderPipelineInfo::RenderPipelineInfo(const ShaderStagesPtr& shaderStages) {
    reset();
}

void RenderPipelineInfo::reset() {
    primitiveState = {};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.stripIndexFormat = WGPUIndexFormat_Uint32;
    primitiveState.frontFace = coordinateOriginBottomLeft ? WGPUFrontFace_CW : WGPUFrontFace_CCW;
    primitiveState.cullMode = WGPUCullMode_Back;

    depthStencilState.format = WGPUTextureFormat_Undefined;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.stencilFront = {};
    depthStencilState.stencilBack = {};
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;
    depthStencilState.depthBias = 0.0f;
    depthStencilState.depthBiasSlopeScale = 0.0f;
    depthStencilState.depthBiasClamp = 0.0f;

    multisampleState.count = framebuffer ? framebuffer->getSampleCount() : 1;
    multisampleState.mask = ~0u;
    multisampleState.alphaToCoverageEnabled = false;

    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    primitiveState.cullMode = WGPUCullMode_None;

    currentBlendModes.clear();
    blendStates.clear();
    colorTargetStates.clear();
    _resizeColorTargets(framebuffer->getColorTargetCount());

    auto colorTargetCount = framebuffer->getColorTargetCount();
    const auto& colorTargetTextureViews = framebuffer->getColorTargetTextureViews();
    colorTargetStates.resize(colorTargetCount);
    for (size_t i = 0; i < colorTargetCount; i++) {
        WGPUColorTargetState& colorTargetState = colorTargetStates.at(i);
        colorTargetState.format = colorTargetTextureViews.at(i)->getTextureSettings().format;
        colorTargetState.blend = &blendStates.at(i);
        colorTargetState.writeMask = WGPUColorWriteMask_All; // TODO
    }

    constantEntriesMap.clear();

    vertexBuffersAttributes.clear();
    vertexBufferLayouts.clear();
}

void RenderPipelineInfo::addConstantEntry(ShaderType shaderType, const std::string& key, double value) {
    constantEntriesMap[shaderType][key] = value;
}

void RenderPipelineInfo::removeConstantEntry(ShaderType shaderType, const std::string& key) {
    constantEntriesMap[shaderType].erase(key);
}

void RenderPipelineInfo::_resizeColorTargets(size_t newCount) {
    if (colorTargetStates.size() != newCount) {
        size_t oldSize = colorTargetStates.size();
        colorTargetStates.resize(newCount);
        blendStates.resize(newCount);
        currentBlendModes.resize(newCount);
        for (size_t attachmentIdx = oldSize; attachmentIdx < newCount; attachmentIdx++) {
            auto& colorTargetState = colorTargetStates.at(attachmentIdx);
            colorTargetState = {};
            colorTargetState.writeMask = WGPUColorWriteMask_All;
            setBlendMode(BlendMode::OVERWRITE, uint32_t(attachmentIdx));
        }
        for (size_t i = 0; i < colorTargetStates.size(); i++) {
            WGPUColorTargetState& colorTargetState = colorTargetStates.at(i);
            if (currentBlendModes.at(i) == BlendMode::OVERWRITE) {
                colorTargetState.blend = nullptr;
            } else {
                colorTargetState.blend = &blendStates.at(i);
            }
        }
    }
}

void RenderPipelineInfo::setFramebuffer(const FramebufferPtr& framebuffer) {
    this->framebuffer = framebuffer;
    _resizeColorTargets(framebuffer->getColorTargetCount());

    if (framebuffer->getHasDepthStencilTarget()) {
        depthStencilState.format = framebuffer->getDepthStencilTarget()->getTextureSettings().format;
    } else {
        depthStencilState.format = WGPUTextureFormat_Undefined;
    }
    multisampleState.count = framebuffer->getSampleCount();
}

void RenderPipelineInfo::setColorWriteEnabled(bool enableColorWrite, uint32_t colorAttachmentIndex) {
    if (colorAttachmentIndex >= colorTargetStates.size()) {
        _resizeColorTargets(colorAttachmentIndex + 1);
    }
    auto& colorTargetState = colorTargetStates.at(colorAttachmentIndex);
    if (enableColorWrite) {
        colorTargetState.writeMask = WGPUColorWriteMask_All;
    } else {
        colorTargetState.writeMask = WGPUColorWriteMask_None;
    }
}

void RenderPipelineInfo::setBlendMode(BlendMode blendMode, uint32_t colorAttachmentIndex) {
    if (colorAttachmentIndex >= currentBlendModes.size()) {
        _resizeColorTargets(colorAttachmentIndex + 1);
    }
    WGPUColorTargetState& colorTargetState = colorTargetStates.at(colorAttachmentIndex);
    BlendMode& currentBlendMode = currentBlendModes.at(colorAttachmentIndex);
    auto& blendState = blendStates.at(colorAttachmentIndex);

    currentBlendMode = blendMode;
    if (blendMode == BlendMode::OVERWRITE) {
        colorTargetState.blend = nullptr;
    }

    if (blendMode == BlendMode::OVERWRITE) {
        blendState.color.srcFactor = WGPUBlendFactor_One;
        blendState.color.dstFactor = WGPUBlendFactor_Zero;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_One;
        blendState.alpha.dstFactor = WGPUBlendFactor_Zero;
        blendState.alpha.operation = WGPUBlendOperation_Add;
    }
    // Alpha blending
    else if (blendMode == BlendMode::BACK_TO_FRONT_STRAIGHT_ALPHA) {
        blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_One;
        blendState.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.alpha.operation = WGPUBlendOperation_Add;
    } else if (blendMode == BlendMode::BACK_TO_FRONT_PREMUL_ALPHA) {
        blendState.color.srcFactor = WGPUBlendFactor_One;
        blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_One;
        blendState.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.alpha.operation = WGPUBlendOperation_Add;
    } else if (blendMode == BlendMode::FRONT_TO_BACK_PREMUL_ALPHA) {
        blendState.color.srcFactor = WGPUBlendFactor_OneMinusDstAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_One;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_OneMinusDstAlpha;
        blendState.alpha.dstFactor = WGPUBlendFactor_One;
        blendState.alpha.operation = WGPUBlendOperation_Add;
    }
    // Additive blending modes & multiplicative blending
    else if (blendMode == BlendMode::BACK_ADDITIVE) {
        blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_One;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.alpha.dstFactor = WGPUBlendFactor_One;
        blendState.alpha.operation = WGPUBlendOperation_Add;
    }
    else if (blendMode == BlendMode::ONE) {
        blendState.color.srcFactor = WGPUBlendFactor_One;
        blendState.color.dstFactor = WGPUBlendFactor_One;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_One;
        blendState.alpha.dstFactor = WGPUBlendFactor_One;
        blendState.alpha.operation = WGPUBlendOperation_Add;
    }
    else if (blendMode == BlendMode::BACK_SUBTRACTIVE) {
        blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_One;
        blendState.color.operation = WGPUBlendOperation_ReverseSubtract;
        blendState.alpha.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.alpha.dstFactor = WGPUBlendFactor_One;
        blendState.alpha.operation = WGPUBlendOperation_ReverseSubtract;
    }
    else if (blendMode == BlendMode::BACK_MULTIPLICATIVE) {
        blendState.color.srcFactor = WGPUBlendFactor_Dst;
        blendState.color.dstFactor = WGPUBlendFactor_Zero;
        blendState.color.operation = WGPUBlendOperation_ReverseSubtract;
        blendState.alpha.srcFactor = WGPUBlendFactor_DstAlpha;
        blendState.alpha.dstFactor = WGPUBlendFactor_Zero;
        blendState.alpha.operation = WGPUBlendOperation_Add;
    }
}

void RenderPipelineInfo::setBlendModeCustom(
        WGPUBlendFactor srcColorBlendFactor, WGPUBlendFactor dstColorBlendFactor, WGPUBlendOperation colorBlendOp,
        WGPUBlendFactor srcAlphaBlendFactor, WGPUBlendFactor dstAlphaBlendFactor, WGPUBlendOperation alphaBlendOp,
        uint32_t colorAttachmentIndex) {
    if (colorAttachmentIndex >= currentBlendModes.size()) {
        _resizeColorTargets(colorAttachmentIndex + 1);
    }
    auto& colorTargetState = colorTargetStates.at(colorAttachmentIndex);
    BlendMode& currentBlendMode = currentBlendModes.at(colorAttachmentIndex);
    auto& blendState = blendStates.at(colorAttachmentIndex);

    currentBlendMode = BlendMode::CUSTOM;
    colorTargetState.blend = &blendStates.at(colorAttachmentIndex);
    blendState.color.srcFactor = srcColorBlendFactor;
    blendState.color.dstFactor = dstColorBlendFactor;
    blendState.color.operation = colorBlendOp;
    blendState.alpha.srcFactor = srcAlphaBlendFactor;
    blendState.alpha.dstFactor = dstAlphaBlendFactor;
    blendState.alpha.operation = alphaBlendOp;
}

void RenderPipelineInfo::setPrimitiveTopology(
        PrimitiveTopology primitiveTopology, WGPUIndexFormat stripIndexFormat) {
    primitiveState.topology = WGPUPrimitiveTopology(primitiveTopology);
    primitiveState.stripIndexFormat = stripIndexFormat;
}

void RenderPipelineInfo::setCullMode(CullMode cullMode) {
    primitiveState.cullMode = WGPUCullMode(cullMode);
}

void RenderPipelineInfo::setIsFrontFaceCcw(bool isFrontFaceCcw) {
    if (coordinateOriginBottomLeft) {
        isFrontFaceCcw = !isFrontFaceCcw;
    }
    primitiveState.frontFace = isFrontFaceCcw ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
}

void RenderPipelineInfo::setDepthWriteEnabled(bool enableDepthWrite) {
    depthStencilState.depthWriteEnabled = enableDepthWrite;
}

void RenderPipelineInfo::setDepthCompareFunction(WGPUCompareFunction compareFunction) {
    depthStencilState.depthCompare = compareFunction;
}

/*
 * Currently not used:
 * depthStencilState.stencilFront;
 * depthStencilState.stencilBack;
 * depthStencilState.stencilReadMask;
 * depthStencilState.stencilWriteMask;
 * depthStencilState.depthBias;
 * depthStencilState.depthBiasSlopeScale;
 * depthStencilState.depthBiasClamp;
 */

void RenderPipelineInfo::setVertexBufferBinding(
        uint32_t binding, uint32_t stride, WGPUVertexStepMode stepMode) {
    if (vertexBufferLayouts.size() <= binding) {
        vertexBufferLayouts.resize(binding + 1);
        vertexBuffersAttributes.resize(binding + 1);
        for (size_t bufferIdx = 0; bufferIdx < vertexBufferLayouts.size(); bufferIdx++) {
            auto& vertexBufferLayout = vertexBufferLayouts.at(bufferIdx);
            vertexBufferLayout.attributeCount = vertexBuffersAttributes.at(bufferIdx).size();
            vertexBufferLayout.attributes =
                    vertexBuffersAttributes.at(bufferIdx).empty() ? nullptr :
                    vertexBuffersAttributes.at(bufferIdx).data();
        }
    }
    auto& vertexBufferLayout = vertexBufferLayouts.at(binding);
    vertexBufferLayout.arrayStride = stride;
    vertexBufferLayout.stepMode = stepMode;
}

void RenderPipelineInfo::setInputAttributeDescription(
        uint32_t bufferBinding, uint32_t bufferOffset, uint32_t attributeLocation) {
    auto& vertexBufferAttributes = vertexBuffersAttributes.at(bufferBinding);
    WGPUVertexAttribute vertexAttribute;
    vertexAttribute.format = shaderStages->getInputVariableDescriptorFromLocation(attributeLocation).vertexFormat;
    vertexAttribute.offset = bufferOffset;
    vertexAttribute.shaderLocation = attributeLocation;
    vertexBufferAttributes.push_back(vertexAttribute);

    auto& vertexBufferLayout = vertexBufferLayouts.at(bufferBinding);
    vertexBufferLayout.attributeCount = vertexBuffersAttributes.at(bufferBinding).size();
    vertexBufferLayout.attributes =
            vertexBuffersAttributes.at(bufferBinding).empty() ? nullptr :
            vertexBuffersAttributes.at(bufferBinding).data();
}

void RenderPipelineInfo::setInputAttributeDescription(
        uint32_t bufferBinding, uint32_t bufferOffset, const std::string& attributeName) {
    auto& vertexBufferAttributes = vertexBuffersAttributes.at(bufferBinding);
    const auto& inputVariableDescriptor = shaderStages->getInputVariableDescriptorFromName(attributeName);
    WGPUVertexAttribute vertexAttribute;
    vertexAttribute.format = inputVariableDescriptor.vertexFormat;
    vertexAttribute.offset = bufferOffset;
    vertexAttribute.shaderLocation = inputVariableDescriptor.locationIndex;
    vertexBufferAttributes.push_back(vertexAttribute);

    auto& vertexBufferLayout = vertexBufferLayouts.at(bufferBinding);
    vertexBufferLayout.attributeCount = vertexBuffersAttributes.at(bufferBinding).size();
    vertexBufferLayout.attributes =
            vertexBuffersAttributes.at(bufferBinding).empty() ? nullptr :
            vertexBuffersAttributes.at(bufferBinding).data();
}

void RenderPipelineInfo::setVertexBufferBindingByLocationIndex(
        const std::string& attributeName, uint32_t stride, WGPUVertexStepMode stepMode) {
    uint32_t vertexAttributeBinding = shaderStages->getInputVariableLocationIndex(attributeName);
    setVertexBufferBinding(vertexAttributeBinding, stride, stepMode);
    setInputAttributeDescription(vertexAttributeBinding, 0, attributeName);
}

void RenderPipelineInfo::setVertexBufferBindingByLocationIndexOptional(
        const std::string& attributeName, uint32_t stride, WGPUVertexStepMode stepMode) {
    if (shaderStages->getHasInputVariable(attributeName)) {
        uint32_t vertexNormalBinding = shaderStages->getInputVariableLocationIndex(attributeName);
        setVertexBufferBinding(vertexNormalBinding, stride, stepMode);
        setInputAttributeDescription(vertexNormalBinding, 0, attributeName);
    }
}


RenderPipeline::RenderPipeline(Device* device, const RenderPipelineInfo& pipelineInfo) {
    const auto& shaderStages = pipelineInfo.shaderStages;

    WGPURenderPipelineDescriptor pipelineDesc{};

    WGPUPipelineLayoutDescriptor pipelineLayoutDesc{};
    pipelineLayoutDesc.bindGroupLayoutCount = shaderStages->getWGPUBindGroupLayouts().size();
    pipelineLayoutDesc.bindGroupLayouts = shaderStages->getWGPUBindGroupLayouts().data();
    pipelineLayout = wgpuDeviceCreatePipelineLayout(device->getWGPUDevice(), &pipelineLayoutDesc);
    pipelineDesc.layout = pipelineLayout;

    std::vector<WGPUConstantEntry> constantEntriesVertex;
    auto itVert = pipelineInfo.constantEntriesMap.find(ShaderType::VERTEX);
    if (itVert != pipelineInfo.constantEntriesMap.end()) {
        constantEntriesVertex.reserve(itVert->second.size());
        for (const auto& constantEntryPair : itVert->second) {
            WGPUConstantEntry constantEntry{};
            constantEntry.key = constantEntryPair.first.c_str();
            constantEntry.value = constantEntryPair.second;
            constantEntriesVertex.push_back(constantEntry);
        }
    }

    // Update vertex buffer layout.
    /*std::vector<WGPUVertexBufferLayout> vertexBufferLayouts = pipelineInfo.vertexBufferLayouts;
    for (size_t bufferIdx = 0; bufferIdx < vertexBufferLayouts.size(); bufferIdx++) {
        auto& vertexBufferLayout = vertexBufferLayouts.at(bufferIdx);
        vertexBufferLayout.attributeCount = pipelineInfo.vertexBuffersAttributes.at(bufferIdx).size();
        vertexBufferLayout.attributes = pipelineInfo.vertexBuffersAttributes.at(bufferIdx).data();
    }
    pipelineDesc.vertex.bufferCount = vertexBufferLayouts.size();
    pipelineDesc.vertex.buffers = vertexBufferLayouts.data();*/
    pipelineDesc.vertex.bufferCount = pipelineInfo.vertexBufferLayouts.size();
    pipelineDesc.vertex.buffers = pipelineInfo.vertexBufferLayouts.data();
    pipelineDesc.vertex.constantCount = constantEntriesVertex.size();
    pipelineDesc.vertex.constants = constantEntriesVertex.empty() ? nullptr : constantEntriesVertex.data();
    pipelineDesc.vertex.module = shaderStages->getShaderModule(ShaderType::VERTEX)->getWGPUShaderModule();
    pipelineDesc.vertex.entryPoint = shaderStages->getEntryPoint(ShaderType::VERTEX).c_str();

    pipelineDesc.primitive = pipelineInfo.primitiveState;

    std::vector<WGPUConstantEntry> constantEntriesFragment;
    auto itFrag = pipelineInfo.constantEntriesMap.find(ShaderType::FRAGMENT);
    if (itFrag != pipelineInfo.constantEntriesMap.end()) {
        constantEntriesFragment.reserve(itFrag->second.size());
        for (const auto& constantEntryPair : itFrag->second) {
            WGPUConstantEntry constantEntry{};
            constantEntry.key = constantEntryPair.first.c_str();
            constantEntry.value = constantEntryPair.second;
            constantEntriesFragment.push_back(constantEntry);
        }
    }

    WGPUFragmentState fragmentState{};
    fragmentState.module = shaderStages->getShaderModule(ShaderType::FRAGMENT)->getWGPUShaderModule();
    fragmentState.entryPoint = shaderStages->getEntryPoint(ShaderType::FRAGMENT).c_str();
    fragmentState.constantCount = constantEntriesFragment.size();
    fragmentState.constants = constantEntriesFragment.empty() ? nullptr : constantEntriesFragment.data();
    fragmentState.targetCount = pipelineInfo.colorTargetStates.size();
    fragmentState.targets = pipelineInfo.colorTargetStates.data();
    pipelineDesc.fragment = &fragmentState;

    if (pipelineInfo.framebuffer->getHasDepthStencilTarget()) {
        pipelineDesc.depthStencil = &pipelineInfo.depthStencilState;
    }

    pipelineDesc.multisample = pipelineInfo.multisampleState;

    depthWriteEnabled = pipelineInfo.depthStencilState.depthWriteEnabled;
    stencilWriteEnabled = pipelineInfo.depthStencilState.stencilWriteMask != 0;

    pipeline = wgpuDeviceCreateRenderPipeline(device->getWGPUDevice(), &pipelineDesc);
    if (!pipeline) {
        Logfile::get()->throwError(
                "Error in RenderPipeline::RenderPipeline: wgpuDeviceCreateRenderPipeline failed.");
    }

    // Build vertexAttributeLocationToBufferStrideMap to enable validity check of number of vertices per bound buffer.
    vertexBufferStrides.reserve(pipelineInfo.vertexBufferLayouts.size());
    for (size_t bufferIdx = 0; bufferIdx < pipelineInfo.vertexBufferLayouts.size(); bufferIdx++) {
        const WGPUVertexBufferLayout& vertexBufferLayout = pipelineInfo.vertexBufferLayouts.at(bufferIdx);
        vertexBufferStrides.push_back(vertexBufferLayout.arrayStride);
    }
}

RenderPipeline::~RenderPipeline() {
    if (pipeline) {
        wgpuRenderPipelineRelease(pipeline);
        pipeline = {};
    }
    if (pipelineLayout) {
        wgpuPipelineLayoutRelease(pipelineLayout);
        pipelineLayout = {};
    }
}

}}
