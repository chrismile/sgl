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

#include <cstring>
#include "../Utils/Device.hpp"
#include "../Buffer/Framebuffer.hpp"
#include "../Texture/Texture.hpp"
#include "RenderPipeline.hpp"
#include "ComputePipeline.hpp"
#include "Data.hpp"
#include "Renderer.hpp"

namespace sgl { namespace webgpu {

Renderer::Renderer(Device* device) : device(device) {
    ;
}

Renderer::~Renderer() {
    ;
}

// @see beginCommandBuffer and @see endCommandBuffer need to be called before calling any other command.
void Renderer::beginCommandBuffer() {
    WGPUCommandEncoderDescriptor commandEncoderDescriptor = {};
    commandEncoderDescriptor.nextInChain = nullptr;
    commandEncoderDescriptor.label.data = "Renderer frame command encoder";
    commandEncoderDescriptor.label.length = strlen(commandEncoderDescriptor.label.data);
    encoder = wgpuDeviceCreateCommandEncoder(device->getWGPUDevice(), &commandEncoderDescriptor);
}

WGPUCommandBuffer Renderer::endCommandBuffer() {
    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    commandBufferDescriptor.label.data = "Renderer frame command buffer";
    commandBufferDescriptor.label.length = strlen(commandBufferDescriptor.label.data);
    commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDescriptor);
    commandBuffersWgpu = { commandBuffer };

    wgpuCommandEncoderRelease(encoder);
    encoder = {};
    return commandBuffer;
}

std::vector<WGPUCommandBuffer> Renderer::getFrameCommandBuffers() {
    return commandBuffersWgpu;
}

void Renderer::freeFrameCommandBuffers() {
    wgpuCommandBufferRelease(commandBuffer);
    commandBuffer = {};
}

void Renderer::render(const RenderDataPtr& renderData) {
    const FramebufferPtr& framebuffer = renderData->getRenderPipeline()->getFramebuffer();
    render(renderData, framebuffer);
}

void Renderer::render(const RenderDataPtr& renderData, const FramebufferPtr& framebuffer) {
    size_t numColorTargets = framebuffer->getColorTargetCount();
    const auto& clearValues = framebuffer->getWGPUClearValues();
    const auto& colorTargetTextureViews = framebuffer->getColorTargetTextureViews();
    const auto& resolveTargetTextureViews = framebuffer->getResolveTargetTextureViews();
    const auto& loadOps = framebuffer->getWGPULoadOps();
    const auto& storeOps = framebuffer->getWGPUStoreOps();
    WGPURenderPassDescriptor renderPassDescriptor{};
    std::vector<WGPURenderPassColorAttachment> renderPassColorAttachments(numColorTargets);
    for (size_t i = 0; i < numColorTargets; i++) {
        WGPURenderPassColorAttachment& renderPassColorAttachment = renderPassColorAttachments.at(i);
        renderPassColorAttachment.view = colorTargetTextureViews.at(i)->getWGPUTextureView();
        if (resolveTargetTextureViews.size() > i && resolveTargetTextureViews.at(i)) {
            renderPassColorAttachment.resolveTarget = resolveTargetTextureViews.at(i)->getWGPUTextureView();
        }
        renderPassColorAttachment.loadOp = loadOps.at(i);
        renderPassColorAttachment.storeOp = storeOps.at(i);
        renderPassColorAttachment.clearValue = clearValues.at(i);
#ifndef WEBGPU_BACKEND_WGPU
        renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
    }
    renderPassDescriptor.colorAttachmentCount = numColorTargets;
    renderPassDescriptor.colorAttachments = renderPassColorAttachments.data();

    WGPURenderPassDepthStencilAttachment depthStencilAttachment{};
    if (framebuffer->getHasDepthStencilTarget()) {
        depthStencilAttachment.view = framebuffer->getDepthStencilTarget()->getWGPUTextureView();
        depthStencilAttachment.depthLoadOp = framebuffer->getDepthLoadOp();
        depthStencilAttachment.depthStoreOp = framebuffer->getDepthStoreOp();
        depthStencilAttachment.depthClearValue = framebuffer->getDepthClearValue();
        depthStencilAttachment.depthReadOnly = !renderData->getRenderPipeline()->getDepthWriteEnabled();
        depthStencilAttachment.stencilLoadOp = framebuffer->getStencilLoadOp();
        depthStencilAttachment.stencilStoreOp = framebuffer->getStencilStoreOp();
        depthStencilAttachment.stencilClearValue = framebuffer->getStencilClearValue();
        depthStencilAttachment.stencilReadOnly = !renderData->getRenderPipeline()->getStencilWriteEnabled();
        renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;
    }

    renderPassDescriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, renderData->getRenderPipeline()->getWGPURenderPipeline());
    wgpuRenderPassEncoderSetViewport(
            renderPassEncoder, 0.0f, 0.0f, float(framebuffer->getWidth()), float(framebuffer->getHeight()),
            0.0f, 1.0f);

    if (renderData->getHasIndexBuffer()) {
        wgpuRenderPassEncoderSetIndexBuffer(
                renderPassEncoder, renderData->getWGPUIndexBuffer(), renderData->getIndexFormat(),
                0, renderData->getIndexBuffer()->getSizeInBytes());
    }

    const std::vector<BufferPtr>& vertexBuffers = renderData->getVertexBuffers();
    const std::vector<uint32_t>& vertexBufferSlots = renderData->getVertexBufferSlots();
    for (size_t i = 0; i < vertexBuffers.size(); i++) {
        const auto& vertexBuffer = vertexBuffers.at(i);
        uint32_t slot = vertexBufferSlots.at(i);
        wgpuRenderPassEncoderSetVertexBuffer(
                renderPassEncoder, slot, vertexBuffer->getWGPUBuffer(), 0, vertexBuffer->getSizeInBytes());
    }

    renderData->_updateBindingGroups();
    WGPUBindGroup bindGroup = renderData->getWGPUBindGroup();
    if (bindGroup != nullptr) {
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0, bindGroup, 0, nullptr);
    }

    if (renderData->getUseIndirectDraw()) {
        if (renderData->getHasIndexBuffer()) {
            wgpuRenderPassEncoderDrawIndexedIndirect(
                    renderPassEncoder, renderData->getWGPUIndirectDrawBuffer(),
                    renderData->getIndirectDrawBufferOffset());
        } else {
            wgpuRenderPassEncoderDrawIndirect(
                    renderPassEncoder, renderData->getWGPUIndirectDrawBuffer(),
                    renderData->getIndirectDrawBufferOffset());
        }
    } else {
        if (renderData->getHasIndexBuffer()) {
            //uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) WGPU_FUNCTION_ATTRIBUTE;
            wgpuRenderPassEncoderDrawIndexed(
                    renderPassEncoder, uint32_t(renderData->getNumIndices()), uint32_t(renderData->getNumInstances()),
                    0, 0, 0); // first index, base vertex, first instance currently forced to be zero.
        } else {
            wgpuRenderPassEncoderDraw(
                    renderPassEncoder, uint32_t(renderData->getNumVertices()), uint32_t(renderData->getNumInstances()),
                    0, 0); // first vertex, first instance currently forced to be zero.
        }
    }

    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}


void Renderer::dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX) {
    dispatch(computeData, groupCountX, 1, 1);
}

void Renderer::dispatch(
        const ComputeDataPtr& computeData, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    WGPUComputePassDescriptor computePassDescriptor{};
    WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(encoder, &computePassDescriptor);

    computeData->_updateBindingGroups();
    WGPUBindGroup bindGroup = computeData->getWGPUBindGroup();
    if (bindGroup != nullptr) {
        wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, bindGroup, 0, nullptr);
    }

    wgpuComputePassEncoderSetPipeline(computePassEncoder, computeData->getComputePipeline()->getWGPUPipeline());
    wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder, groupCountX, groupCountY, groupCountZ);
    wgpuComputePassEncoderEnd(computePassEncoder);
    wgpuComputePassEncoderRelease(computePassEncoder);
}

void Renderer::dispatchIndirect(
        const ComputeDataPtr& computeData, const sgl::webgpu::BufferPtr& dispatchIndirectBuffer, uint64_t offset) {
    WGPUComputePassDescriptor computePassDescriptor{};
    WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(encoder, &computePassDescriptor);

    computeData->_updateBindingGroups();
    WGPUBindGroup bindGroup = computeData->getWGPUBindGroup();
    if (bindGroup != nullptr) {
        wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, bindGroup, 0, nullptr);
    }

    wgpuComputePassEncoderSetPipeline(computePassEncoder, computeData->getComputePipeline()->getWGPUPipeline());
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(
            computePassEncoder, dispatchIndirectBuffer->getWGPUBuffer(), offset);
    wgpuComputePassEncoderEnd(computePassEncoder);
    wgpuComputePassEncoderRelease(computePassEncoder);
}

void Renderer::dispatchIndirect(
        const ComputeDataPtr& computeData, const sgl::webgpu::BufferPtr& dispatchIndirectBuffer) {
    dispatchIndirect(computeData, dispatchIndirectBuffer, 0);
}


void Renderer::addTestRenderPass(WGPUTextureView targetView) {
    WGPURenderPassDescriptor renderPassDescriptor{};
    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{ 1.0, 1.0, 0.5, 1.0 };
#ifndef WEBGPU_BACKEND_WGPU
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &renderPassColorAttachment;
    renderPassDescriptor.depthStencilAttachment = nullptr;
    renderPassDescriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);

    // TODO
    //wgpuRenderPassEncoderSetPipeline(renderPassEncoder, pipeline);
    //wgpuRenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}

}}
