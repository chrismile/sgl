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

#ifndef SGL_WEBGPU_RENDERER_HPP
#define SGL_WEBGPU_RENDERER_HPP

#include <vector>
#include <memory>

#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

class Device;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class RenderData;
typedef std::shared_ptr<RenderData> RenderDataPtr;
class ComputeData;
typedef std::shared_ptr<ComputeData> ComputeDataPtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;

class DLL_OBJECT Renderer {
public:
    explicit Renderer(Device* device);
    ~Renderer();

    [[nodiscard]] inline Device* getDevice() { return device; }

    // @see beginCommandBuffer and @see endCommandBuffer need to be called before calling any other command.
    void beginCommandBuffer();
    WGPUCommandBuffer endCommandBuffer();
    std::vector<WGPUCommandBuffer> getFrameCommandBuffers();
    void freeFrameCommandBuffers();
    inline WGPUCommandBuffer getWebGPUCommandBuffer() { return commandBuffer; }
    inline WGPUCommandEncoder getWebGPUCommandEncoder() { return encoder; }

    // Render pipeline.
    void render(const RenderDataPtr& renderData);
    void render(const RenderDataPtr& renderData, const FramebufferPtr& framebuffer);

    // Compute pipeline.
    void dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX);
    void dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void dispatchIndirect(
            const ComputeDataPtr& computeData, const sgl::webgpu::BufferPtr& dispatchIndirectBuffer, uint64_t offset);
    void dispatchIndirect(
            const ComputeDataPtr& computeData, const sgl::webgpu::BufferPtr& dispatchIndirectBuffer);

    // TODO: For testing purposes; will be removed in the future.
    void addTestRenderPass(WGPUTextureView targetView);

private:
    Device* device;
    WGPUCommandEncoder encoder{};
    std::vector<WGPUCommandBuffer> commandBuffersWgpu;
    WGPUCommandBuffer commandBuffer{};
};

}}

#endif //SGL_WEBGPU_RENDERER_HPP
