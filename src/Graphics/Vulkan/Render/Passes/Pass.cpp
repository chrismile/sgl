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
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/ComputePipeline.hpp>
#include <Graphics/Vulkan/Render/RayTracingPipeline.hpp>
#include "Pass.hpp"

namespace sgl { namespace vk {

Pass::Pass(vk::Renderer *renderer) : renderer(renderer), device(renderer->getDevice()) {
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
        sgl::vk::ComputePipelineInfo computePipelineInfo(shaderStages);
        setComputePipelineInfo(computePipelineInfo);
        sgl::vk::ComputePipelinePtr computePipeline(new sgl::vk::ComputePipeline(device, computePipelineInfo));

        createComputeData(renderer, computePipeline);
        dataDirty = false;
    }


    if (shaderDirty) {
        shaderDirty = false;
    }
}


void RasterPass::render() {
    if (shaderDirty || framebufferDirty || dataDirty) {
        _build();
    }
    _render();
}

void RasterPass::buildIfNecessary() {
    if (shaderDirty || framebufferDirty || dataDirty) {
        _build();
    }
}

void RasterPass::_render() {
    renderer->render(rasterData);
}

void RasterPass::_build() {
    if (shaderDirty) {
        loadShader();
    }

    if (!framebuffer) {
        Logfile::get()->throwError("Error in RasterPass::_build: No framebuffer object is set.");
    }
    framebufferDirty = false;

    if (shaderDirty || dataDirty) {
        sgl::vk::GraphicsPipelineInfo graphicsPipelineInfo(shaderStages);
        graphicsPipelineInfo.setFramebuffer(framebuffer);
        setGraphicsPipelineInfo(graphicsPipelineInfo);
        sgl::vk::GraphicsPipelinePtr graphicsPipeline(new sgl::vk::GraphicsPipeline(device, graphicsPipelineInfo));

        createRasterData(renderer, graphicsPipeline);
        dataDirty = false;
    }

    if (shaderDirty) {
        shaderDirty = false;
    }
}


void RayTracingPass::render() {
    if (shaderDirty || dataDirty) {
        _build();
    }
    _render();
}

void RayTracingPass::buildIfNecessary() {
    if (shaderDirty || dataDirty) {
        _build();
    }
}

void RayTracingPass::_render() {
    renderer->traceRays(rayTracingData, launchSizeX, launchSizeY, launchSizeZ);
}

void RayTracingPass::recreateSwapchain(uint32_t width, uint32_t height) {
    launchSizeX = width;
    launchSizeY = height;
}

sgl::vk::RayTracingPipelinePtr RayTracingPass::createRayTracingPipeline() {
    sgl::vk::ShaderBindingTable sbt = sgl::vk::ShaderBindingTable::generateSimpleShaderBindingTable(shaderStages);
    sgl::vk::RayTracingPipelineInfo rayTracingPipelineInfo(sbt);
    return std::make_shared<sgl::vk::RayTracingPipeline>(device, rayTracingPipelineInfo);
}

void RayTracingPass::_build() {
    if (shaderDirty) {
        loadShader();
    }

    if (shaderDirty || dataDirty) {
        sgl::vk::RayTracingPipelinePtr rayTracingPipeline = createRayTracingPipeline();

        createRayTracingData(renderer, rayTracingPipeline);
        dataDirty = false;
    }

    if (shaderDirty) {
        shaderDirty = false;
    }
}

}}
