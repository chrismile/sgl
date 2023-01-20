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

void BlitRenderPass::setOutputImageInitialLayout(VkImageLayout layout) {
    initialLayout = layout;
}

void BlitRenderPass::setOutputImageFinalLayout(VkImageLayout layout) {
    finalLayout = layout;
}

void BlitRenderPass::setAttachmentLoadOp(VkAttachmentLoadOp op) {
    attachmentLoadOp = op;
    if (!framebuffers.empty()) {
        recreateSwapchain(framebuffers.front()->getWidth(), framebuffers.front()->getHeight());
    }
    setDataDirty();
}

void BlitRenderPass::setAttachmentStoreOp(VkAttachmentStoreOp op) {
    attachmentStoreOp = op;
    if (!framebuffers.empty()) {
        recreateSwapchain(framebuffers.front()->getWidth(), framebuffers.front()->getHeight());
    }
    setDataDirty();
}

void BlitRenderPass::setAttachmentClearColor(const glm::vec4& color) {
    clearColor = color;
    if (!outputImageViews.empty()) {
        bool dataDirtyOld = dataDirty;
        recreateSwapchain(framebuffers.front()->getWidth(), framebuffers.front()->getHeight());
        dataDirty = dataDirtyOld;
    } else {
        setDataDirty();
    }
}

void BlitRenderPass::setColorWriteEnabled(bool enable) {
    enableColorWrite = enable;
    setDataDirty();
}

void BlitRenderPass::setDepthWriteEnabled(bool enable) {
    enableDepthWrite = enable;
    setDataDirty();
}

void BlitRenderPass::setDepthTestEnabled(bool enable) {
    enableDepthTest = enable;
    setDataDirty();
}

void BlitRenderPass::setDepthCompareOp(VkCompareOp compareOp) {
    depthCompareOp = compareOp;
    setDataDirty();
}

void BlitRenderPass::recreateSwapchain(uint32_t width, uint32_t height) {
    AttachmentState attachmentState;
    attachmentState.loadOp = attachmentLoadOp;
    attachmentState.storeOp = attachmentStoreOp;
    attachmentState.initialLayout = initialLayout;
    attachmentState.finalLayout = finalLayout;

    framebuffers.clear();
    for (size_t i = 0; i < outputImageViews.size(); i++) {
        FramebufferPtr framebuffer = std::make_shared<sgl::vk::Framebuffer>(device, width, height);
        sgl::vk::ImageViewPtr outputImageView = outputImageViews.at(i);
        if (outputImageView->getVkImageAspectFlags() == VK_IMAGE_ASPECT_COLOR_BIT) {
            framebuffer->setColorAttachment(outputImageViews.at(i), 0, attachmentState, clearColor);
        } else if ((outputImageView->getVkImageAspectFlags() & VK_IMAGE_ASPECT_DEPTH_BIT) != 0) {
            framebuffer->setDepthStencilAttachment(outputImageViews.at(i), attachmentState, clearColorDepth);
        } else {
            sgl::Logfile::get()->throwError(
                    "Error in BlitRenderPass::recreateSwapchain: Invalid image aspect flags.");
        }
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

void BlitRenderPass::setNormalizedCoordinatesAabb(const sgl::AABB2& aabb) {
    std::vector<float> vertexData = {
            aabb.min.x, aabb.max.y, 0.0f, 0.0f, 1.0f,
            aabb.max.x, aabb.max.y, 0.0f, 1.0f, 1.0f,
            aabb.max.x, aabb.min.y, 0.0f, 1.0f, 0.0f,
            aabb.min.x, aabb.min.y, 0.0f, 0.0f, 0.0f,
    };
    vertexBuffer->updateData(
            vertexData.size() * sizeof(float), vertexData.data(),
            renderer->getVkCommandBuffer());
    renderer->insertBufferMemoryBarrier(
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            vertexBuffer);
}

void BlitRenderPass::setNormalizedCoordinatesAabb(const sgl::AABB2& aabb, bool flipY) {
    if (!flipY) {
        setNormalizedCoordinatesAabb(aabb);
        return;
    }
    std::vector<float> vertexData = {
            aabb.min.x, aabb.max.y, 0.0f, 0.0f, 0.0f,
            aabb.max.x, aabb.max.y, 0.0f, 1.0f, 0.0f,
            aabb.max.x, aabb.min.y, 0.0f, 1.0f, 1.0f,
            aabb.min.x, aabb.min.y, 0.0f, 0.0f, 1.0f,
    };
    vertexBuffer->updateData(
            vertexData.size() * sizeof(float), vertexData.data(),
            renderer->getVkCommandBuffer());
    renderer->insertBufferMemoryBarrier(
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            vertexBuffer);
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
    if (shaderStages->getHasInputVariable("vertexTexCoord")) {
        graphicsPipelineInfo.setInputAttributeDescription(
                0, sizeof(float) * 3, "vertexTexCoord");
    }
    graphicsPipelineInfo.setBlendMode(blendMode);
    graphicsPipelineInfo.setColorWriteEnabled(enableColorWrite);
    graphicsPipelineInfo.setDepthWriteEnabled(enableDepthWrite);
    graphicsPipelineInfo.setDepthTestEnabled(enableDepthTest);
    graphicsPipelineInfo.setDepthCompareOp(depthCompareOp);
    graphicsPipelineInfo.setCullMode(cullMode);
}

}}
