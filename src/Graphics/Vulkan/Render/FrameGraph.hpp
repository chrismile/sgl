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

#ifndef SGL_FRAMEGRAPH_HPP
#define SGL_FRAMEGRAPH_HPP

#include <functional>
#include <unordered_map>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/GraphicsPipeline.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include <glm/vec3.hpp>
#include "Data.hpp"

namespace sgl { namespace vk {

class RenderPass;
class FrameGraph;

typedef uint32_t PassId;
typedef std::pair<uint32_t, RenderPass*> FrameGraphEdge; ///< Priority, pass ID.

class RenderPass {
public:
    RenderPass(FrameGraph& frameGraph, PassId passId);
    virtual sgl::vk::ShaderStagesPtr loadShader()=0;
    inline PassId getPassId() const { return passId; }

protected:
    FrameGraph& frameGraph;
    vk::Device* device;

private:
    friend class FrameGraph;
    virtual void _render()=0;
    virtual void _addIngoingEdge(FrameGraphEdge& edge);
    virtual void _addOutgoingEdge(FrameGraphEdge& edge);

    PassId passId;
    std::vector<FrameGraphEdge> ingoingEdges;
    std::vector<FrameGraphEdge> outgoingEdges;
};

typedef std::shared_ptr<RenderPass> RenderPassPtr;

class FrameGraph {
public:
    explicit FrameGraph(vk::Renderer* renderer) : renderer(renderer), device(renderer->getDevice()) {}
    void addPass(RenderPassPtr& renderPass);
    void addEdge(PassId passId0, PassId passId1, uint32_t priority = 0);
    void setFinalRenderPass(RenderPassPtr& renderPass);

    void render();
    static inline uint32_t getFinalRenderPassId() { return 0; }

    inline vk::Device* getDevice() { return device; }

private:
    vk::Renderer* renderer;
    vk::Device* device;
    std::vector<RenderPassPtr> renderPasses;
    RenderPassPtr finalRenderPass;

    void _build();
    bool dirty = true; ///< Needs to be rebuilt?
    std::vector<RenderPass*> linearizedRenderPasses;
    std::unordered_map<size_t, std::vector<VkImageMemoryBarrier>> imageDependenciesMap; ///< linearized index -> image memory barriers
    std::unordered_map<size_t, std::vector<VkBufferMemoryBarrier>> bufferDependenciesMap; ///< linearized index -> buffer memory barriers
};


class TestRenderPass : public RenderPass {
public:
    TestRenderPass(FrameGraph& frameGraph, PassId passId) : RenderPass(frameGraph, passId) {}

    sgl::vk::ShaderStagesPtr loadShader() {
        return sgl::vk::ShaderManager->getShaderStages(
                {"TestShader.Vertex", "TestShader.Fragment"});
    }

    void setupGeometryBuffers() {
        std::vector<glm::vec3> vertexPositions = {
                {-1.0f, -1.0f, 0.0f},
                {1.0f, -1.0f, 0.0f},
                {1.0f, 1.0f, 0.0f},
                {-1.0f, -1.0f, 0.0f},
                {1.0f, 1.0f, 0.0f},
                {-1.0f, 1.0f, 0.0f},
        };
        vertexBuffer = sgl::vk::BufferPtr(new sgl::vk::Buffer(
                frameGraph.getDevice(), vertexPositions.size() * sizeof(glm::vec3), vertexPositions.data(),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY));
    }

    void createGraphicsPipelineInfo(sgl::vk::GraphicsPipelineInfo& graphicsPipelineInfo) {
        graphicsPipelineInfo.setVertexBufferBinding(0, sizeof(glm::vec3));
        graphicsPipelineInfo.setInputAttributeDescription(0, 0, "vertexPosition");
    }

private:
    sgl::vk::BufferPtr vertexBuffer;
};

} }

#endif //SGL_FRAMEGRAPH_HPP
