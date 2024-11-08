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
#include <Graphics/WebGPU/Render/Renderer.hpp>
#include <Graphics/WebGPU/Render/ComputePipeline.hpp>
#include <Graphics/WebGPU/Render/RenderPipeline.hpp>

#include "Pass.hpp"

namespace sgl { namespace webgpu {

Pass::Pass(webgpu::Renderer *renderer) : renderer(renderer), device(renderer->getDevice()) {
}

void ComputePass::render() {
    if (shaderDirty || dataDirty) {
        _build();
    }
    _render();
}

void ComputePass::buildIfNecessary() {
    if (shaderDirty || dataDirty) {
        _build();
    }
}

void ComputePass::_render() {
    renderer->dispatch(computeData, groupCountX, groupCountY, groupCountZ);
}

void ComputePass::_build() {
    if (shaderDirty) {
        loadShader();
    }

    if (shaderDirty || dataDirty) {
        sgl::webgpu::ComputePipelineInfo computePipelineInfo(shaderStages);
        setComputePipelineInfo(computePipelineInfo);
        sgl::webgpu::ComputePipelinePtr computePipeline(new sgl::webgpu::ComputePipeline(device, computePipelineInfo));

        createComputeData(renderer, computePipeline);
        dataDirty = false;
    }


    if (shaderDirty) {
        shaderDirty = false;
    }
}


void RenderPass::render() {
    if (shaderDirty || framebufferDirty || dataDirty) {
        _build();
    }
    _render();
}

void RenderPass::buildIfNecessary() {
    if (shaderDirty || framebufferDirty || dataDirty) {
        _build();
    }
}

void RenderPass::_render() {
    renderer->render(renderData);
}

void RenderPass::_build() {
    if (shaderDirty) {
        loadShader();
    }

    if (!framebuffer) {
        Logfile::get()->throwError("Error in RenderPass::_build: No framebuffer object is set.");
    }
    framebufferDirty = false;

    if (shaderDirty || dataDirty) {
        sgl::webgpu::RenderPipelineInfo renderPipelineInfo(shaderStages);
        renderPipelineInfo.setFramebuffer(framebuffer);
        setRenderPipelineInfo(renderPipelineInfo);
        sgl::webgpu::RenderPipelinePtr renderPipeline(new sgl::webgpu::RenderPipeline(device, renderPipelineInfo));

        createRenderData(renderer, renderPipeline);
        dataDirty = false;
    }

    if (shaderDirty) {
        shaderDirty = false;
    }
}

}}
