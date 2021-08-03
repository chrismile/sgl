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

#ifndef SGL_PASS_HPP
#define SGL_PASS_HPP

#include <glm/vec4.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/GraphicsPipeline.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include <Graphics/Vulkan/Render/Data.hpp>

namespace sgl { namespace vk {

class ComputePipelineInfo;
class GraphicsPipelineInfo;
class RayTracingPipelineInfo;

enum class PassType {
    RASTER_PASS, RAYTRACING_PASS, COMPUTE_PASS, COPY_PASS, BLIT_PASS, CUSTOM_PASS
};

class Pass {
public:
    explicit Pass(sgl::vk::Renderer* renderer);
    virtual ~Pass()=default;
    virtual PassType getPassType()=0;

    virtual void render()=0;
    virtual void recreateSwapchain(uint32_t width, uint32_t height) {}

protected:
    virtual void loadShader()=0;
    vk::Renderer* renderer;
    vk::Device* device;
};

class ComputePass : public Pass {
public:
    explicit ComputePass(sgl::vk::Renderer* renderer) : Pass(renderer) {}
    PassType getPassType() override { return PassType::RASTER_PASS; }

    inline sgl::vk::ShaderModulePtr& getShaderModule() { return computeShader; }

    void render() override;

protected:
    virtual void setComputePipelineInfo(sgl::vk::ComputePipelineInfo& pipelineInfo)=0;
    virtual void createComputeData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline)=0;

    vk::ShaderModulePtr computeShader;
    vk::ComputeDataPtr computeData;
    uint32_t groupCountX = 1;
    uint32_t groupCountY = 1;
    uint32_t groupCountZ = 1;
};

class RenderPass : public Pass {
public:
    explicit RenderPass(sgl::vk::Renderer* renderer) : Pass(renderer) {}
    inline sgl::vk::ShaderStagesPtr& getShaderStages() { return shaderStages; }

protected:
    sgl::vk::ShaderStagesPtr shaderStages;
    sgl::vk::FramebufferPtr framebuffer;
    bool shaderDirty = true;
    bool framebufferDirty = true;
};

class RasterPass : public RenderPass {
public:
    explicit RasterPass(sgl::vk::Renderer* renderer) : RenderPass(renderer) {}
    PassType getPassType() override { return PassType::RASTER_PASS; }

    void render() final;

protected:
    virtual void setGraphicsPipelineInfo(sgl::vk::GraphicsPipelineInfo& pipelineInfo)=0;
    virtual void createRasterData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline)=0;
    virtual void _render();

    sgl::vk::RasterDataPtr rasterData;

private:
    void _build();
};

}}

#endif //SGL_PASS_HPP
