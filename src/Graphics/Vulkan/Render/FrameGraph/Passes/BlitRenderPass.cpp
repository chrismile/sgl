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
#include <Graphics/Vulkan/Buffers/Framebuffer.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/FrameGraph/FrameGraph.hpp>
#include "BlitRenderPass.hpp"

namespace sgl { namespace vk {

BlitRenderPass::BlitRenderPass(FrameGraph* frameGraph)
        : RasterPass(frameGraph), shaderIds({"BlitShader.Vertex", "BlitShader.Fragment"}) {
    setupGeometryBuffers();
}

BlitRenderPass::BlitRenderPass(FrameGraph* frameGraph, std::vector<std::string> customShaderIds)
        : RasterPass(frameGraph), shaderIds(std::move(customShaderIds)) {
    setupGeometryBuffers();
}

void BlitRenderPass::setInputTexture(sgl::vk::TexturePtr& texture) {
    inputTexture = texture;
}

void BlitRenderPass::setOutputImage(sgl::vk::ImageViewPtr& imageView) {
    RenderPassAttachmentState attachmentState;
    attachmentState.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    setColorAttachment(imageView, 0, attachmentState);
}

void BlitRenderPass::setOutputImages(std::vector<sgl::vk::ImageViewPtr>& imageViews) {
    if (imageViews.empty()) {
        Logfile::get()->throwError("Error in BlitRenderPass::setOutputImages: imageViews.empty()");
    }
    RenderPassAttachmentState attachmentState;
    attachmentState.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    setColorAttachment(imageViews.at(0), 0, attachmentState);
}

void BlitRenderPass::_render() {
    FramebufferPtr framebuffer = renderData->getGraphicsPipeline()->getFramebuffer();
    //framebuffer->copy();
    vk::Renderer* renderer = frameGraph->getRenderer();
    renderer->render(renderData, framebuffer);
}

void BlitRenderPass::setupGeometryBuffers() {
    std::vector<glm::vec3> vertexPositions = {
            {-1.0f, -1.0f, 0.0f},
            {1.0f, -1.0f, 0.0f},
            {1.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            {1.0f, 1.0f, 0.0f},
            {-1.0f, 1.0f, 0.0f},
    };
    vertexPositionBuffer = std::make_shared<sgl::vk::Buffer>(
            device, vertexPositions.size() * sizeof(glm::vec3), vertexPositions.data(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
}

void BlitRenderPass::loadShader() {
    shaderStages = sgl::vk::ShaderManager->getShaderStages(shaderIds);
}

void BlitRenderPass::createRasterData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline) {
    renderData = std::make_shared<sgl::vk::RasterData>(renderer, graphicsPipeline);
    renderData->setVertexBuffer(vertexPositionBuffer, "vertexPosition");
    renderData->setStaticTexture(inputTexture, "inputTexture");
}

void BlitRenderPass::setGraphicsPipelineInfo(sgl::vk::GraphicsPipelineInfo& graphicsPipelineInfo) {
    graphicsPipelineInfo.setVertexBufferBinding(0, sizeof(glm::vec3));
    graphicsPipelineInfo.setInputAttributeDescription(0, 0, "vertexPosition");
}

}}
