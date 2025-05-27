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

#ifndef SGL_WEBGPU_BLITRENDERPASS_HPP
#define SGL_WEBGPU_BLITRENDERPASS_HPP

#include <vector>
#include <string>

#ifdef USE_GLM
#include <glm/vec4.hpp>
#else
#include <Math/Geometry/fallback/vec4.hpp>
#endif

#include <Math/Geometry/AABB2.hpp>
#include <Graphics/WebGPU/Texture/Texture.hpp>
#include <Graphics/WebGPU/Render/RenderPipeline.hpp>

#include "Pass.hpp"

namespace sgl { namespace webgpu {

class DLL_OBJECT BlitRenderPass : public sgl::webgpu::RenderPass {
public:
    /**
     * Uses the shaders {"Blit.Vertex", "Blit.Fragment"} for blitting.
     * @param frameGraph The frame graph object.
     */
    explicit BlitRenderPass(sgl::webgpu::Renderer* renderer);
    /**
     * Uses the custom shaders for blitting.
     * @param frameGraph The frame graph object.
     */
    BlitRenderPass(sgl::webgpu::Renderer* renderer, std::vector<std::string> customShaderIds);

    // Public interface.
    void setNormalizedCoordinatesAabb(const sgl::AABB2& aabb);
    void setNormalizedCoordinatesAabb(const sgl::AABB2& aabb, bool flipY);
    virtual void setInputSampler(const sgl::webgpu::SamplerPtr& sampler);
    virtual void setInputTextureView(const sgl::webgpu::TextureViewPtr& texture);
    virtual void setOutputTextureView(const sgl::webgpu::TextureViewPtr& textureView);
    inline void setCullMode(sgl::webgpu::CullMode mode) { cullMode = mode; }
    inline void setBlendMode(BlendMode mode) { blendMode = mode; setDataDirty(); }
    void setAttachmentLoadOp(WGPULoadOp op);
    void setAttachmentStoreOp(WGPUStoreOp op);
    void setAttachmentClearColor(const glm::vec4& color);
    void setColorWriteEnabled(bool enable);
    void setDepthWriteEnabled(bool enable);
    void setDepthTestEnabled(bool enable);
    void setDepthCompareFunction(WGPUCompareFunction compareFunction);

    void recreateSwapchain(uint32_t width, uint32_t height) override;

protected:
    void loadShader() override;
    void setRenderPipelineInfo(sgl::webgpu::RenderPipelineInfo& renderPipelineInfo) override;
    void createRenderData(sgl::webgpu::Renderer* renderer, sgl::webgpu::RenderPipelinePtr& renderPipeline) override;
    void _render() override;

    void setupGeometryBuffers();
    std::vector<std::string> shaderIds;
    sgl::webgpu::CullMode cullMode = sgl::webgpu::CullMode::CULL_BACK;
    BlendMode blendMode = BlendMode::OVERWRITE;
    bool enableColorWrite = true;
    bool enableDepthWrite = true;
    bool enableDepthTest = true;
    WGPUCompareFunction depthCompareFunction = WGPUCompareFunction_Less;

    WGPULoadOp loadOp = WGPULoadOp_Clear;
    WGPUStoreOp storeOp = WGPUStoreOp_Store;
    glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float clearColorDepth = 1.0f;
    sgl::webgpu::SamplerPtr inputSampler;
    sgl::webgpu::TextureViewPtr inputTextureView;
    sgl::webgpu::TextureViewPtr outputTextureView;

    sgl::webgpu::BufferPtr indexBuffer;
    sgl::webgpu::BufferPtr vertexBuffer;
};

typedef std::shared_ptr<BlitRenderPass> BlitRenderPassPtr;

}}

#endif //SGL_WEBGPU_BLITRENDERPASS_HPP
