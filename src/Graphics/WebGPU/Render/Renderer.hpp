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

#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

class Device;

class DLL_OBJECT Renderer {
public:
    explicit Renderer(Device* device);
    ~Renderer();

    // @see beginCommandBuffer and @see endCommandBuffer need to be called before calling any other command.
    void beginCommandBuffer();
    WGPUCommandBuffer endCommandBuffer();
    std::vector<WGPUCommandBuffer> getFrameCommandBuffers();
    void freeFrameCommandBuffers();
    inline WGPUCommandBuffer getWebGPUCommandBuffer() { return commandBuffer; }
    inline WGPUCommandEncoder getWebGPUCommandEncoder() { return encoder; }

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
