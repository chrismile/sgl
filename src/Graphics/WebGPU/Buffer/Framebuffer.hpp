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

#ifndef SGL_WEBGPU_FRAMEBUFFER_HPP
#define SGL_WEBGPU_FRAMEBUFFER_HPP

#include <vector>
#include <memory>
#include <glm/vec4.hpp>
#include <webgpu/webgpu.h>

#include <Utils/File/Logfile.hpp>
#include "../Texture/Texture.hpp"

namespace sgl { namespace webgpu {

class Device;
class TextureView;
typedef std::shared_ptr<TextureView> TextureViewPtr;

class DLL_OBJECT Framebuffer {
public:
    Framebuffer(Device* device, uint32_t width, uint32_t height);
    ~Framebuffer();

    void setColorAttachment(
            TextureViewPtr& attachmentTextureView, int index, WGPULoadOp loadOp, WGPUStoreOp storeOp,
            const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) {
        if (int(colorTargets.size()) <= index) {
            colorTargets.resize(index + 1);
            colorTargetsLoadOp.resize(index + 1);
            colorTargetsStoreOp.resize(index + 1);
            clearValues.resize(index + 1);
        }
        colorTargets.at(index) = attachmentTextureView;
        colorTargetsLoadOp.at(index) = loadOp;
        colorTargetsStoreOp.at(index) = storeOp;
        clearValues.at(index) = WGPUColor{ clearColor.x, clearColor.y, clearColor.z, clearColor.w };

        if (width != attachmentTextureView->getTextureSettings().size.width
                || height != attachmentTextureView->getTextureSettings().size.height) {
            sgl::Logfile::get()->throwError("Error in Framebuffer::setColorAttachment: Invalid texture view sizes.");
        }
    }
    inline void setResolveAttachment(
            TextureViewPtr& attachmentImageView, int index) {
        if (int(resolveTargets.size()) <= index) {
            resolveTargets.resize(index + 1);
        }
        resolveTargets.at(index) = attachmentImageView;
    }
    inline void setDepthStencilAttachment(
            TextureViewPtr& attachmentTextureView,
            WGPULoadOp _depthLoadOp, WGPUStoreOp _depthStoreOp,
            WGPULoadOp _stencilLoadOp, WGPUStoreOp _stencilStoreOp,
            float clearDepth = 1.0f, uint32_t clearStencil = 0) {
        depthLoadOp = _depthLoadOp;
        depthStoreOp = _depthStoreOp;
        stencilLoadOp = _stencilLoadOp;
        stencilStoreOp = _stencilStoreOp;
        depthClearValue = clearDepth;
        stencilClearValue = clearStencil;
    }

    // Get size of the framebuffer attachments.
    [[nodiscard]] inline uint32_t getWidth() const { return width; }
    [[nodiscard]] inline uint32_t getHeight() const { return height; }

    /// Returns the number of color targets.
    [[nodiscard]] inline size_t getColorTargetCount() const { return colorTargets.size(); }
    [[nodiscard]] inline const std::vector<TextureViewPtr>& getColorTargetTextureViews() const { return colorTargets; }
    [[nodiscard]] inline const std::vector<TextureViewPtr>& getResolveTargetTextureViews() const { return resolveTargets; }
    [[nodiscard]] inline const std::vector<WGPUColor>& getWGPUClearValues() const { return clearValues; }
    [[nodiscard]] inline const std::vector<WGPULoadOp>& getWGPULoadOps() const { return colorTargetsLoadOp; }
    [[nodiscard]] inline const std::vector<WGPUStoreOp>& getWGPUStoreOps() const { return colorTargetsStoreOp; }

    /// Returns whether this framebuffer has a depth-stencil target.
    [[nodiscard]] inline bool getHasDepthStencilTarget() const { return depthStencilTarget.get() != nullptr; }
    [[nodiscard]] inline const TextureViewPtr& getDepthStencilTarget() const { return depthStencilTarget; }
    [[nodiscard]] inline WGPULoadOp getDepthLoadOp() const { return depthLoadOp; }
    [[nodiscard]] inline WGPUStoreOp getDepthStoreOp() const { return depthStoreOp; }
    [[nodiscard]] inline float getDepthClearValue() const { return depthClearValue; }
    [[nodiscard]] inline WGPULoadOp getStencilLoadOp() const { return stencilLoadOp; }
    [[nodiscard]] inline WGPUStoreOp getStencilStoreOp() const { return stencilStoreOp; }
    [[nodiscard]] inline uint32_t getStencilClearValue() const { return stencilClearValue; }

    /// Returns the number of samples used by the attachments.
    [[nodiscard]] inline uint32_t getSampleCount() const { return sampleCount; }

private:
    Device* device;
    uint32_t width, height;

    std::vector<TextureViewPtr> colorTargets;
    std::vector<WGPUColor> clearValues;
    std::vector<WGPULoadOp> colorTargetsLoadOp;
    std::vector<WGPUStoreOp> colorTargetsStoreOp;

    uint32_t sampleCount = 1;
    std::vector<TextureViewPtr> resolveTargets;

    TextureViewPtr depthStencilTarget;
    WGPULoadOp depthLoadOp = WGPULoadOp_Undefined;
    WGPUStoreOp depthStoreOp = WGPUStoreOp_Undefined;
    float depthClearValue = 1.0f;
    WGPULoadOp stencilLoadOp = WGPULoadOp_Undefined;
    WGPUStoreOp stencilStoreOp = WGPUStoreOp_Undefined;
    uint32_t stencilClearValue = 0;
};

}}

#endif //SGL_WEBGPU_FRAMEBUFFER_HPP
