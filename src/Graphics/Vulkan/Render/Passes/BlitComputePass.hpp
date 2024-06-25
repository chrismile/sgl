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

#ifndef SGL_BLITCOMPUTEPASS_HPP
#define SGL_BLITCOMPUTEPASS_HPP

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <Math/Geometry/AABB2.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include "Pass.hpp"

namespace sgl { namespace vk {

/**
 * A frame graph pass for blitting one image to another image via a compute shader.
 */
class DLL_OBJECT BlitComputePass : public ComputePass {
public:
    /**
     * Uses the shaders {"Blit.Vertex", "Blit.Fragment"} for blitting.
     * @param frameGraph The frame graph object.
     */
    explicit BlitComputePass(sgl::vk::Renderer* renderer);
    /**
     * Uses the custom shaders for blitting.
     * @param frameGraph The frame graph object.
     */
    BlitComputePass(sgl::vk::Renderer* renderer, std::vector<std::string> customShaderIds);

    // Public interface.
    virtual void setInputTexture(sgl::vk::TexturePtr& texture);
    virtual void setOutputImage(sgl::vk::ImageViewPtr& imageView);

protected:
    void loadShader() override;
    void setComputePipelineInfo(sgl::vk::ComputePipelineInfo& pipelineInfo) {}
    void createComputeData(sgl::vk::Renderer* renderer, sgl::vk::ComputePipelinePtr& computePipeline) override;
    void _render() override;

    const uint32_t LOCAL_SIZE_X = 16;
    const uint32_t LOCAL_SIZE_Y = 16;
    std::vector<std::string> shaderIds;
    sgl::vk::TexturePtr inputTexture;
    sgl::vk::ImageViewPtr outputImageView;
};

typedef std::shared_ptr<BlitComputePass> BlitComputePassPtr;

}}

#endif //SGL_BLITCOMPUTEPASS_HPP
