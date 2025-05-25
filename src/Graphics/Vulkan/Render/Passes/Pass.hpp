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

#ifdef USE_GLM
#include <glm/vec4.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif
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

class DLL_OBJECT Pass {
public:
    explicit Pass(sgl::vk::Renderer* renderer);
    virtual ~Pass()=default;
    virtual PassType getPassType()=0;

    virtual void render()=0;
    virtual void buildIfNecessary()=0;
    virtual void recreateSwapchain(uint32_t width, uint32_t height) {}

    inline void setShaderDirty() { shaderDirty = true; }
    inline void setDataDirty() { dataDirty = true; }
    inline sgl::vk::ShaderStagesPtr& getShaderStages() { return shaderStages; }

protected:
    virtual void loadShader()=0;
    vk::Renderer* renderer;
    vk::Device* device;
    sgl::vk::ShaderStagesPtr shaderStages;
    bool shaderDirty = true;
    bool dataDirty = true;
};

class DLL_OBJECT ComputePass : public Pass {
public:
    explicit ComputePass(sgl::vk::Renderer* renderer) : Pass(renderer) {}
    PassType getPassType() override { return PassType::COMPUTE_PASS; }
    [[nodiscard]] inline const sgl::vk::ComputePipelinePtr& getComputePipeline() const {
        return computeData->getComputePipeline();
    }

    inline sgl::vk::ShaderModulePtr& getShaderModule() { return shaderStages->getShaderModules().front(); }

    void render() final;
    void buildIfNecessary() final;

protected:
    virtual void setComputePipelineInfo(sgl::vk::ComputePipelineInfo& pipelineInfo) {}
    virtual void createComputeData(sgl::vk::Renderer* renderer, sgl::vk::ComputePipelinePtr& computePipeline)=0;
    virtual void _render();

    vk::ComputeDataPtr computeData;
    uint32_t groupCountX = 1;
    uint32_t groupCountY = 1;
    uint32_t groupCountZ = 1;

private:
    void _build();
};

class DLL_OBJECT RasterPass : public Pass {
public:
    explicit RasterPass(sgl::vk::Renderer* renderer) : Pass(renderer) {}
    PassType getPassType() override { return PassType::RASTER_PASS; }
    [[nodiscard]] inline const sgl::vk::GraphicsPipelinePtr& getGraphicsPipeline() const {
        return rasterData->getGraphicsPipeline();
    }

    void render() final;
    void buildIfNecessary() final;

protected:
    virtual void setGraphicsPipelineInfo(sgl::vk::GraphicsPipelineInfo& pipelineInfo)=0;
    virtual void createRasterData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline)=0;
    virtual void _render();

    sgl::vk::RasterDataPtr rasterData;
    sgl::vk::FramebufferPtr framebuffer;
    bool framebufferDirty = true;

private:
    void _build();
};

class DLL_OBJECT RayTracingPass : public Pass {
public:
    explicit RayTracingPass(sgl::vk::Renderer* renderer) : Pass(renderer) {}
    PassType getPassType() override { return PassType::RAYTRACING_PASS; }
    [[nodiscard]] inline const sgl::vk::RayTracingPipelinePtr& getRayTracingPipeline() const {
        return rayTracingData->getRayTracingPipeline();
    }

    void render() final;
    void buildIfNecessary() final;

    /**
     * Sets launchSizeX and launchSizeY to the screen width and height.
     * Override this function if you want to change this!
     * @param width The width of the swapchain surface.
     * @param height The height of the swapchain surface.
     */
    void recreateSwapchain(uint32_t width, uint32_t height) override;

protected:
    virtual sgl::vk::RayTracingPipelinePtr createRayTracingPipeline();
    virtual void createRayTracingData(sgl::vk::Renderer* renderer, sgl::vk::RayTracingPipelinePtr& rayTracingPipeline)=0;
    virtual void _render();

    sgl::vk::RayTracingDataPtr rayTracingData;
    uint32_t launchSizeX = 1, launchSizeY = 1, launchSizeZ = 1;

private:
    void _build();
};

}}

#endif //SGL_PASS_HPP
