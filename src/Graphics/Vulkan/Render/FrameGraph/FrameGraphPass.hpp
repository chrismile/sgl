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

#ifndef SGL_FRAMEGRAPHPASS_HPP
#define SGL_FRAMEGRAPHPASS_HPP

#include <glm/vec4.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/GraphicsPipeline.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include <Graphics/Vulkan/Render/Data.hpp>

namespace sgl { namespace vk {

class FrameGraphPass;
class FrameGraph;

typedef uint32_t PassId;
typedef std::pair<uint32_t, FrameGraphPass*> FrameGraphEdge; ///< Priority, pass ID.

struct RenderPassAttachmentState {
    VkAttachmentLoadOp  loadOp          = VK_ATTACHMENT_LOAD_OP_LOAD;
    VkAttachmentLoadOp  stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkImageLayout       initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
};

enum class FrameGraphPassType {
    RASTER_PASS, RAYTRACING_PASS, COMPUTE_PASS, COPY_PASS, BLIT_PASS, CUSTOM_PASS
};

class FrameGraphPass {
public:
    explicit FrameGraphPass(FrameGraph* frameGraph);
    virtual ~FrameGraphPass()=default;
    virtual FrameGraphPassType getFrameGraphPassType()=0;

protected:
    FrameGraph* frameGraph;
    vk::Device* device;

private:
    friend class FrameGraph;
    virtual void _render()=0;
    virtual void _addIngoingEdge(FrameGraphEdge& edge);
    virtual void _addOutgoingEdge(FrameGraphEdge& edge);

    std::vector<FrameGraphEdge> ingoingEdges;
    std::vector<FrameGraphEdge> outgoingEdges;
};

typedef std::shared_ptr<FrameGraphPass> FrameGraphPassPtr;

class CustomFrameGraphPass : public FrameGraphPass {
public:
    explicit CustomFrameGraphPass(FrameGraph* frameGraph) : FrameGraphPass(frameGraph) {}
    FrameGraphPassType getFrameGraphPassType() override { return FrameGraphPassType::CUSTOM_PASS; }
};

class RasterPass : public FrameGraphPass {
public:
    explicit RasterPass(FrameGraph* frameGraph);
    virtual void resolutionChanged(uint32_t width, uint32_t height) {}
    FrameGraphPassType getFrameGraphPassType() override { return FrameGraphPassType::RASTER_PASS; }

    // Interface for the frame graph.
    virtual void loadShader()=0;
    virtual void setGraphicsPipelineInfo(sgl::vk::GraphicsPipelineInfo& graphicsPipelineInfo)=0;
    virtual void createRasterData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline)=0;
    inline sgl::vk::ShaderStagesPtr& getShaderStages() { return shaderStages; }

    void createGraphicsPipelineInfoFromFramebuffer(sgl::vk::FramebufferPtr& framebuffer);

protected:
    void setColorAttachment(
            ImageViewPtr& attachmentImageView, int index,
            const RenderPassAttachmentState& attachmentState,
            const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    void setDepthStencilAttachment(
            ImageViewPtr& attachmentImageView,
            const RenderPassAttachmentState& attachmentState,
            float clearDepth = 1.0f, uint32_t clearStencil = 0);
    void setResolveAttachment(
            ImageViewPtr& attachmentImageView,
            const RenderPassAttachmentState& attachmentState);
    void setInputAttachment(
            ImageViewPtr& attachmentImageView,
            const RenderPassAttachmentState& attachmentState);
    inline void setSubpassIndex(uint32_t subpass) { subpassIndex = subpass; }

    void _render() override;

    uint32_t subpassIndex = 0;
    sgl::vk::ShaderStagesPtr shaderStages;
    sgl::vk::RasterDataPtr renderData;
};

typedef std::shared_ptr<RasterPass> RasterPassPtr;

}}

#endif //SGL_FRAMEGRAPHPASS_HPP
