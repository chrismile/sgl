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
#include "Pass.hpp"

namespace sgl { namespace vk {

Pass::Pass(vk::Renderer *renderer) : renderer(renderer), device(renderer->getDevice()) {
}

void ComputePass::render() {
    renderer->dispatch(computeData, groupCountX, groupCountY, groupCountZ);
}


void RasterPass::render() {
    if (shaderDirty || framebufferDirty) {
        _build();
    }
    _render();
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

    sgl::vk::GraphicsPipelineInfo graphicsPipelineInfo(shaderStages);
    graphicsPipelineInfo.setFramebuffer(framebuffer);
    setGraphicsPipelineInfo(graphicsPipelineInfo);
    sgl::vk::GraphicsPipelinePtr graphicsPipeline(new sgl::vk::GraphicsPipeline(device, graphicsPipelineInfo));

    createRasterData(renderer, graphicsPipeline);
}

}}
