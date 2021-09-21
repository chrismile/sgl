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
#include <Utils/AppSettings.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Buffers/Framebuffer.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <memory>
#include "BlitRenderPass.hpp"

namespace sgl { namespace vk {

BlitRenderPass::BlitRenderPass(sgl::vk::Renderer* renderer)
        : BlitRenderPass(renderer, {"Blit.Vertex", "Blit.Fragment"}) {
}

BlitRenderPass::BlitRenderPass(sgl::vk::Renderer* renderer, std::vector<std::string> customShaderIds)
        : RasterPass(renderer), shaderIds(std::move(customShaderIds)) {
    setupGeometryBuffers();
}

void BlitRenderPass::setInputTexture(sgl::vk::TexturePtr& texture) {
    inputTexture = texture;
    if (rasterData) {
        rasterData->setStaticTexture(inputTexture, "inputTexture");
    }
}

void BlitRenderPass::setOutputImage(sgl::vk::ImageViewPtr& imageView) {
    outputImageViews = { imageView };
}

void BlitRenderPass::setOutputImages(std::vector<sgl::vk::ImageViewPtr>& imageViews) {
    if (imageViews.empty()) {
        Logfile::get()->throwError("Error in BlitRenderPass::setOutputImages: imageViews.empty()");
    }
    outputImageViews = imageViews;
}

void BlitRenderPass::setOutputImageLayout(VkImageLayout layout) {
    finalLayout = layout;
}

void BlitRenderPass::recreateSwapchain(uint32_t width, uint32_t height) {
    AttachmentState attachmentState;
    attachmentState.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentState.finalLayout = finalLayout;

    framebuffers.clear();
    for (size_t i = 0; i < outputImageViews.size(); i++) {
        FramebufferPtr framebuffer = std::make_shared<sgl::vk::Framebuffer>(device, width, height);
        framebuffer->setColorAttachment(outputImageViews.at(i), 0, attachmentState);
        framebuffers.push_back(framebuffer);
    }
    framebuffer = framebuffers.front();
    framebufferDirty = true;
    dataDirty = true;
}


void BlitRenderPass::_render() {
    FramebufferPtr framebuffer = this->framebuffer;
    if (framebuffers.size() > 1) {
        Swapchain* swapchain = sgl::AppSettings::get()->getSwapchain();
        uint32_t imageIndex = swapchain ? swapchain->getImageIndex() : 0;
        framebuffer = framebuffers.at(imageIndex);
    }
    renderer->render(rasterData, framebuffer);
}

void BlitRenderPass::setupGeometryBuffers() {
    std::vector<uint32_t> indexData = {
            0, 1, 2,
            0, 2, 3,
    };
#if DEFAULT_COORDINATE_ORIGIN_BOTTOM_LEFT
    std::vector<float> vertexData = {
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
    };
#else
    std::vector<float> vertexData = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    };
#endif
    indexBuffer = std::make_shared<sgl::vk::Buffer>(
            device, indexData.size() * sizeof(uint32_t), indexData.data(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
    vertexBuffer = std::make_shared<sgl::vk::Buffer>(
            device, vertexData.size() * sizeof(float), vertexData.data(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
}

void BlitRenderPass::loadShader() {
    shaderStages = sgl::vk::ShaderManager->getShaderStages(shaderIds);
}

void BlitRenderPass::createRasterData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline) {
    rasterData = std::make_shared<sgl::vk::RasterData>(renderer, graphicsPipeline);
    rasterData->setIndexBuffer(indexBuffer);
    rasterData->setVertexBuffer(vertexBuffer, 0);
    rasterData->setStaticTexture(inputTexture, "inputTexture");
}

void BlitRenderPass::setGraphicsPipelineInfo(sgl::vk::GraphicsPipelineInfo& graphicsPipelineInfo) {
    graphicsPipelineInfo.setIsFrontFaceCcw(true);
    graphicsPipelineInfo.setVertexBufferBinding(0, sizeof(float) * 5);
    graphicsPipelineInfo.setInputAttributeDescription(
            0, 0, "vertexPosition");
    graphicsPipelineInfo.setInputAttributeDescription(
            0, sizeof(float) * 3, "vertexTexCoord");
}

}}
