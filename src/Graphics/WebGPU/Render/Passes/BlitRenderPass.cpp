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

#include <Graphics/WebGPU/Utils/Device.hpp>
#include <Graphics/WebGPU/Buffer/Framebuffer.hpp>
#include <Graphics/WebGPU/Render/Renderer.hpp>
#include <Graphics/WebGPU/Shader/ShaderManager.hpp>
#include "BlitRenderPass.hpp"

namespace sgl { namespace webgpu {

BlitRenderPass::BlitRenderPass(sgl::webgpu::Renderer* renderer)
        : BlitRenderPass(renderer, {"Blit.Vertex", "Blit.Fragment"}) {
}

BlitRenderPass::BlitRenderPass(sgl::webgpu::Renderer* renderer, std::vector<std::string> customShaderIds)
        : RenderPass(renderer), shaderIds(std::move(customShaderIds)) {
    setupGeometryBuffers();
}

void BlitRenderPass::setInputSampler(const sgl::webgpu::SamplerPtr& sampler) {
    inputSampler = sampler;
    if (renderData) {
        renderData->setSampler(inputSampler, "inputSampler");
    }
}

void BlitRenderPass::setInputTextureView(const sgl::webgpu::TextureViewPtr& textureView) {
    inputTextureView = textureView;
    if (renderData) {
        renderData->setTextureView(inputTextureView, "inputTexture");
    }
}

void BlitRenderPass::setOutputTextureView(const sgl::webgpu::TextureViewPtr& textureView) {
    outputTextureView = textureView;
    if (framebuffer
            && framebuffer->getWidth() == textureView->getTextureSettings().size.width
            && framebuffer->getHeight() == textureView->getTextureSettings().size.height
            && framebuffer->getColorTargetCount() == 1
            && framebuffer->getColorTargetTextureViews().at(0)->getTextureSettings().format == textureView->getTextureSettings().format
            && framebuffer->getColorTargetTextureViews().at(0)->getTextureSettings().sampleCount == textureView->getTextureSettings().sampleCount) {
        // We can directly set the attachment if it is compatible.
        framebuffer->setColorAttachment(outputTextureView, 0, loadOp, storeOp, clearColor);
    }
}

void BlitRenderPass::setAttachmentLoadOp(WGPULoadOp op) {
    loadOp = op;
    if (framebuffer) {
        recreateSwapchain(framebuffer->getWidth(), framebuffer->getHeight());
    }
    setDataDirty();
}

void BlitRenderPass::setAttachmentStoreOp(WGPUStoreOp op) {
    storeOp = op;
    if (framebuffer) {
        recreateSwapchain(framebuffer->getWidth(), framebuffer->getHeight());
    }
    setDataDirty();
}

void BlitRenderPass::setAttachmentClearColor(const glm::vec4& color) {
    clearColor = color;
    if (outputTextureView) {
        bool dataDirtyOld = dataDirty;
        recreateSwapchain(framebuffer->getWidth(), framebuffer->getHeight());
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

void BlitRenderPass::setDepthCompareFunction(WGPUCompareFunction compareFunction) {
    depthCompareFunction = compareFunction;
    setDataDirty();
}

void BlitRenderPass::recreateSwapchain(uint32_t width, uint32_t height) {
    framebuffer = std::make_shared<sgl::webgpu::Framebuffer>(device, width, height);
    framebuffer->setColorAttachment(outputTextureView, 0, loadOp, storeOp, clearColor);
    framebufferDirty = true;
    dataDirty = true;
}


void BlitRenderPass::_render() {
    renderer->render(renderData, framebuffer);
}

/*
 * Needed by WebGPU, as we cannot use negative heights as in sgl::vk::GraphicsPipeline::setFramebuffer.
 */
#define DEFAULT_COORDINATE_ORIGIN_BOTTOM_LEFT true

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
    BufferSettings bufferSettings{};
    bufferSettings.usage = WGPUBufferUsage(WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);
    bufferSettings.sizeInBytes = indexData.size() * sizeof(uint32_t);
    indexBuffer = std::make_shared<sgl::webgpu::Buffer>(device, bufferSettings);
    indexBuffer->write(indexData.data(), indexBuffer->getSizeInBytes(), renderer->getDevice()->getWGPUQueue());
    bufferSettings.usage = WGPUBufferUsage(WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
    bufferSettings.sizeInBytes = vertexData.size() * sizeof(float);
    vertexBuffer = std::make_shared<sgl::webgpu::Buffer>(device, bufferSettings);
    vertexBuffer->write(vertexData.data(), vertexBuffer->getSizeInBytes(), renderer->getDevice()->getWGPUQueue());
}

void BlitRenderPass::setNormalizedCoordinatesAabb(const sgl::AABB2& aabb) {
#if DEFAULT_COORDINATE_ORIGIN_BOTTOM_LEFT
    std::vector<float> vertexData = {
            aabb.min.x, aabb.min.y, 0.0f, 0.0f, 1.0f,
            aabb.max.x, aabb.min.y, 0.0f, 1.0f, 1.0f,
            aabb.max.x, aabb.max.y, 0.0f, 1.0f, 0.0f,
            aabb.min.x, aabb.max.y, 0.0f, 0.0f, 0.0f,
    };
#else
    std::vector<float> vertexData = {
            aabb.min.x, aabb.max.y, 0.0f, 0.0f, 1.0f,
            aabb.max.x, aabb.max.y, 0.0f, 1.0f, 1.0f,
            aabb.max.x, aabb.min.y, 0.0f, 1.0f, 0.0f,
            aabb.min.x, aabb.min.y, 0.0f, 0.0f, 0.0f,
    };
#endif
    vertexBuffer->write(vertexData.data(), vertexData.size() * sizeof(float), renderer->getDevice()->getWGPUQueue());
}

void BlitRenderPass::setNormalizedCoordinatesAabb(const sgl::AABB2& aabb, bool flipY) {
    if (!flipY) {
        setNormalizedCoordinatesAabb(aabb);
        return;
    }
#if DEFAULT_COORDINATE_ORIGIN_BOTTOM_LEFT
    std::vector<float> vertexData = {
            aabb.min.x, aabb.min.y, 0.0f, 0.0f, 0.0f,
            aabb.max.x, aabb.min.y, 0.0f, 1.0f, 0.0f,
            aabb.max.x, aabb.max.y, 0.0f, 1.0f, 1.0f,
            aabb.min.x, aabb.max.y, 0.0f, 0.0f, 1.0f,
    };
#else
    std::vector<float> vertexData = {
            aabb.min.x, aabb.max.y, 0.0f, 0.0f, 0.0f,
            aabb.max.x, aabb.max.y, 0.0f, 1.0f, 0.0f,
            aabb.max.x, aabb.min.y, 0.0f, 1.0f, 1.0f,
            aabb.min.x, aabb.min.y, 0.0f, 0.0f, 1.0f,
    };
#endif
    vertexBuffer->write(vertexData.data(), vertexData.size() * sizeof(float), renderer->getDevice()->getWGPUQueue());
}

void BlitRenderPass::loadShader() {
    shaderStages = sgl::webgpu::ShaderManager->getShaderStagesMultiSource(shaderIds);
}

void BlitRenderPass::createRenderData(sgl::webgpu::Renderer* renderer, sgl::webgpu::RenderPipelinePtr& renderPipeline) {
    renderData = std::make_shared<sgl::webgpu::RenderData>(renderer, renderPipeline);
    renderData->setIndexBuffer(indexBuffer);
    renderData->setVertexBuffer(vertexBuffer, 0);
    renderData->setSampler(inputSampler, "inputSampler");
    renderData->setTextureView(inputTextureView, "inputTexture");
}

void BlitRenderPass::setRenderPipelineInfo(sgl::webgpu::RenderPipelineInfo& renderPipelineInfo) {
    renderPipelineInfo.setIsFrontFaceCcw(true);
    renderPipelineInfo.setVertexBufferBinding(0, sizeof(float) * 5);
    renderPipelineInfo.setInputAttributeDescription(
            0, 0, "vertexPosition");
    if (shaderStages->getHasInputVariable("vertexTexCoord")) {
        renderPipelineInfo.setInputAttributeDescription(
                0, sizeof(float) * 3, "vertexTexCoord");
    }
    renderPipelineInfo.setBlendMode(blendMode);
    renderPipelineInfo.setColorWriteEnabled(enableColorWrite);
    renderPipelineInfo.setDepthWriteEnabled(enableDepthWrite);
    renderPipelineInfo.setDepthTestEnabled(enableDepthTest);
    renderPipelineInfo.setDepthCompareFunction(depthCompareFunction);
    renderPipelineInfo.setCullMode(cullMode);
}

}}
