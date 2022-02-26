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

#ifndef SGL_BLITRENDERPASS_HPP
#define SGL_BLITRENDERPASS_HPP

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include "Pass.hpp"

namespace sgl { namespace vk {

/**
 * A frame graph pass for blitting one image to another image via a vertex and fragment shader.
 */
class DLL_OBJECT BlitRenderPass : public RasterPass {
public:
    /**
     * Uses the shaders {"Blit.Vertex", "Blit.Fragment"} for blitting.
     * @param frameGraph The frame graph object.
     */
    explicit BlitRenderPass(sgl::vk::Renderer* renderer);
    /**
     * Uses the custom shaders for blitting.
     * @param frameGraph The frame graph object.
     */
    BlitRenderPass(sgl::vk::Renderer* renderer, std::vector<std::string> customShaderIds);

    // Public interface.
    virtual void setInputTexture(sgl::vk::TexturePtr& texture);
    virtual void setOutputImage(sgl::vk::ImageViewPtr& imageView);
    virtual void setOutputImages(std::vector<sgl::vk::ImageViewPtr>& imageViews);
    virtual void setOutputImageLayout(VkImageLayout layout);
    inline void setBlendMode(BlendMode mode) { blendMode = mode; setDataDirty(); }
    void setAttachmentLoadOp(VkAttachmentLoadOp op);
    void setAttachmentClearColor(const glm::vec4& color);

    void recreateSwapchain(uint32_t width, uint32_t height) override;

protected:
    void loadShader() override;
    void setGraphicsPipelineInfo(sgl::vk::GraphicsPipelineInfo& graphicsPipelineInfo) override;
    void createRasterData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline) override;
    void _render() override;

    void setupGeometryBuffers();
    std::vector<std::string> shaderIds;
    BlendMode blendMode = BlendMode::OVERWRITE;

    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentLoadOp attachmentLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    sgl::vk::TexturePtr inputTexture;
    std::vector<sgl::vk::ImageViewPtr> outputImageViews;
    std::vector<sgl::vk::FramebufferPtr> framebuffers;

    sgl::vk::BufferPtr indexBuffer;
    sgl::vk::BufferPtr vertexBuffer;
};

typedef std::shared_ptr<BlitRenderPass> BlitRenderPassPtr;

}}

#endif //SGL_BLITRENDERPASS_HPP
