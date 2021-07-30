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

#include <Graphics/Vulkan/Render/Renderer.hpp>
#include "FrameGraph.hpp"
#include "FrameGraphPass.hpp"

namespace sgl { namespace vk {

FrameGraphPass::FrameGraphPass(FrameGraph* frameGraph) : frameGraph(frameGraph), device(frameGraph->getDevice()) {}

void FrameGraphPass::_addIngoingEdge(FrameGraphEdge& edge) {
    ingoingEdges.push_back(edge);
    std::sort(ingoingEdges.begin(), ingoingEdges.end());
}

void FrameGraphPass::_addOutgoingEdge(FrameGraphEdge& edge) {
    outgoingEdges.push_back(edge);
    std::sort(outgoingEdges.begin(), outgoingEdges.end());
}


RasterPass::RasterPass(FrameGraph* frameGraph) : FrameGraphPass(frameGraph) {}

void RasterPass::createGraphicsPipelineInfoFromFramebuffer(sgl::vk::FramebufferPtr& framebuffer) {
    sgl::vk::GraphicsPipelineInfo graphicsPipelineInfo(shaderStages);
    graphicsPipelineInfo.setFramebuffer(framebuffer);
    setGraphicsPipelineInfo(graphicsPipelineInfo);
}

void RasterPass::_render() {
    vk::Renderer* renderer = frameGraph->getRenderer();
    renderer->render(renderData);
}

}}
