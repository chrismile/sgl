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

#include "../Utils/Device.hpp"
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
    commandEncoderDescriptor.label = "Renderer frame command encoder";
    encoder = wgpuDeviceCreateCommandEncoder(device->getWGPUDevice(), &commandEncoderDescriptor);
}

WGPUCommandBuffer Renderer::endCommandBuffer() {
    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    commandBufferDescriptor.label = "Renderer frame command buffer";
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

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);

    // TODO
    //wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
    //wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);
}

}}
