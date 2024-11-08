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

#ifndef SGL_WEBGPU_PASS_HPP
#define SGL_WEBGPU_PASS_HPP

#include <memory>
#include <cstdint>
#include "../Data.hpp"
#include "../../Shader/Shader.hpp"

namespace sgl { namespace webgpu {

class Device;
class Renderer;
class ShaderModule;
typedef std::shared_ptr<ShaderModule> ShaderModulePtr;
class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;
class ComputePipelineInfo;
class ComputePipeline;
typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;
class ComputeData;
typedef std::shared_ptr<ComputeData> ComputeDataPtr;
class RenderPipelineInfo;
class RenderPipeline;
typedef std::shared_ptr<RenderPipeline> RenderPipelinePtr;
class RenderData;
typedef std::shared_ptr<RenderData> RenderDataPtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;

class DLL_OBJECT Pass {
public:
    explicit Pass(sgl::webgpu::Renderer* renderer);
    virtual ~Pass()=default;

    virtual void render()=0;
    virtual void buildIfNecessary()=0;
    virtual void recreateSwapchain(uint32_t width, uint32_t height) {}

    inline void setShaderDirty() { shaderDirty = true; }
    inline void setDataDirty() { dataDirty = true; }
    inline sgl::webgpu::ShaderStagesPtr& getShaderStages() { return shaderStages; }
    inline const sgl::webgpu::ShaderModulePtr& getVertexShaderModule() { return shaderStages->getShaderModule(webgpu::ShaderType::VERTEX); }
    inline const sgl::webgpu::ShaderModulePtr& getFragmentShaderModule() { return shaderStages->getShaderModule(webgpu::ShaderType::FRAGMENT); }

protected:
    virtual void loadShader()=0;
    webgpu::Renderer* renderer;
    webgpu::Device* device;
    sgl::webgpu::ShaderStagesPtr shaderStages;
    bool shaderDirty = true;
    bool dataDirty = true;
};

class DLL_OBJECT ComputePass : public Pass {
public:
    explicit ComputePass(sgl::webgpu::Renderer* renderer) : Pass(renderer) {}
    [[nodiscard]] inline const sgl::webgpu::ComputePipelinePtr& getComputePipeline() const {
        return computeData->getComputePipeline();
    }

    inline const sgl::webgpu::ShaderModulePtr& getShaderModule() { return shaderStages->getShaderModule(webgpu::ShaderType::COMPUTE); }

    void render() final;
    void buildIfNecessary() final;

protected:
    virtual void setComputePipelineInfo(sgl::webgpu::ComputePipelineInfo& pipelineInfo) {}
    virtual void createComputeData(sgl::webgpu::Renderer* renderer, sgl::webgpu::ComputePipelinePtr& computePipeline)=0;
    virtual void _render();

    webgpu::ComputeDataPtr computeData;
    uint32_t groupCountX = 1;
    uint32_t groupCountY = 1;
    uint32_t groupCountZ = 1;

private:
    void _build();
};

class DLL_OBJECT RenderPass : public Pass {
public:
    explicit RenderPass(sgl::webgpu::Renderer* renderer) : Pass(renderer) {}
    [[nodiscard]] inline const sgl::webgpu::RenderPipelinePtr& getRenderPipeline() const {
        return renderData->getRenderPipeline();
    }

    void render() final;
    void buildIfNecessary() final;

protected:
    virtual void setRenderPipelineInfo(sgl::webgpu::RenderPipelineInfo& pipelineInfo)=0;
    virtual void createRenderData(sgl::webgpu::Renderer* renderer, sgl::webgpu::RenderPipelinePtr& RenderPipeline)=0;
    virtual void _render();

    sgl::webgpu::RenderDataPtr renderData;
    sgl::webgpu::FramebufferPtr framebuffer;
    bool framebufferDirty = true;

private:
    void _build();
};

}}

#endif //SGL_WEBGPU_PASS_HPP
