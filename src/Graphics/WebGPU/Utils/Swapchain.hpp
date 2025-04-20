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

#ifndef SGL_WEBGPU_SWAPCHAIN_HPP
#define SGL_WEBGPU_SWAPCHAIN_HPP

#include <vector>

#include <webgpu/webgpu.h>

#include <Graphics/Window.hpp>

namespace sgl { namespace webgpu {

class DLL_OBJECT Swapchain {
public:
    /**
     * @param device The device object.
     */
    explicit Swapchain(Device* device);
    ~Swapchain();
    void create(Window* window);

    /// Interface for window class.
    void recreateSwapchain();

    /**
     * Updates of buffers etc. can be performed between beginFrame and renderFrame.
     * beginFrame returns false when wgpuSurfaceGetCurrentTexture failed.
     */
    bool beginFrame();
    void renderFrame(const std::vector<WGPUCommandBuffer>& commandBuffers);

    inline WGPUTextureFormat getSurfaceTextureFormat() { return surfaceFormat; }
    inline WGPUTexture getFrameTexture() { return currentTexture; }
    inline WGPUTextureView getFrameTextureView() { return currentTextureView; }

private:
    /// Only cleans up resources that are reallocated by @see recreate.
    void cleanupRecreate();
    /// Cleans up all resources.
    void cleanup();

    Device* device = nullptr;
    Window* window = nullptr;
    WGPUSurface surface{};
    WGPUTextureFormat surfaceFormat{};
    WGPUTexture currentTexture{};
    WGPUTextureView currentTextureView{};
    bool validPixelSize = false;
    WGPUFuture submittedWorkFuture{};
};

}}

#endif //SGL_WEBGPU_SWAPCHAIN_HPP
