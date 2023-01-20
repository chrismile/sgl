/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Christoph Neuhauser
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

#ifndef CORRERENDER_NANOVGWRAPPER_HPP
#define CORRERENDER_NANOVGWRAPPER_HPP

#include <vector>
#include <memory>

#ifdef SUPPORT_OPENGL
namespace sgl {
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
class ShaderProgram;
typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;
class FramebufferObject;
typedef std::shared_ptr<FramebufferObject> FramebufferObjectPtr;
class RenderbufferObject;
typedef std::shared_ptr<RenderbufferObject> RenderbufferObjectPtr;
}
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/libs/volk/volk.h>

namespace sgl { namespace vk {
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;
class CommandBuffer;
typedef std::shared_ptr<CommandBuffer> CommandBufferPtr;
class BlitRenderPass;
typedef std::shared_ptr<BlitRenderPass> BlitRenderPassPtr;
class Renderer;
}}
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
namespace sgl {
class InteropSyncVkGl;
typedef std::shared_ptr<InteropSyncVkGl> InteropSyncVkGlPtr;
}
#endif

struct NVGcontext;
typedef struct NVGcontext NVGcontext;

namespace sgl {

/**
 * A different backend than the render system can be used.
 * In this case, resource sharing is used between OpenGL and Vulkan.
 */
enum class NanoVGBackend {
    OPENGL, VULKAN
};

struct DLL_OBJECT NanoVGSettings {
    NanoVGSettings();
    NanoVGBackend nanoVgBackend;
    bool useDebugging;
    bool shallClearBeforeRender = true;
    bool useMsaa = false;
    bool useStencilStrokes = false;
    int numMsaaSamples = 8;
    int supersamplingFactor = 4;
};

class DLL_OBJECT NanoVGWidget {
public:
    explicit NanoVGWidget(NanoVGSettings nanoVgSettings = {});
    void setSettings(NanoVGSettings nanoVgSettings);
    virtual ~NanoVGWidget();

    virtual void update(float dt) {}
    void render();

    /// Returns whether the mouse is over the area of the window.
    [[nodiscard]] bool isMouseOverDiagram() const;
    [[nodiscard]] bool isMouseOverDiagram(int parentX, int parentY, int parentWidth, int parentHeight) const;

#ifdef SUPPORT_OPENGL
    [[nodiscard]] inline const sgl::TexturePtr& getRenderTargetTextureGl() { return renderTargetGl; }
    void blitToTargetGl(sgl::FramebufferObjectPtr& sceneFramebuffer);
#endif

#ifdef SUPPORT_VULKAN
    [[nodiscard]] inline const vk::TexturePtr& getRenderTargetTextureVk() { return renderTargetTextureVk; }
    void setRendererVk(vk::Renderer* renderer) { rendererVk = renderer; }
    void setBlitTargetVk(vk::ImageViewPtr& blitTargetVk, VkImageLayout initialLayout, VkImageLayout finalLayout);
    void blitToTargetVk();
#endif

protected:
    void _initialize();
    void renderStart();
    void renderEnd();
    void onWindowSizeChanged();

    /// This function can be overridden by derived classes to add NanoVG calls.
    virtual void renderBase() {}

    NVGcontext* vg = nullptr;
    float windowWidth = 1.0f, windowHeight = 1.0f;

    [[nodiscard]] inline float getWindowOffsetX() const { return windowOffsetX; }
    [[nodiscard]] inline float getWindowOffsetY() const { return windowOffsetY; }
    [[nodiscard]] inline float getScaleFactor() const { return scaleFactor; }

private:
    void _initializeFont(NVGcontext* vgCurrent);
    NanoVGBackend nanoVgBackend = NanoVGBackend::VULKAN;
    int flags = 0;
    bool shallClearBeforeRender = true;

    float windowOffsetX = 20, windowOffsetY = 20;
    float scaleFactor = 1.0f;
    int fboWidthInternal = 1, fboHeightInternal = 1;
    int fboWidthDisplay = 1, fboHeightDisplay = 1;
    bool useMsaa = false;
    int numMsaaSamples = 8;
    int supersamplingFactor = 4;

#ifdef SUPPORT_OPENGL
    sgl::FramebufferObjectPtr framebufferGl;
    sgl::TexturePtr renderTargetGl;
    sgl::RenderbufferObjectPtr depthStencilRbo;
    sgl::ShaderProgramPtr blitShader, blitMsaaShader, blitDownscaleShader, blitDownscaleMsaaShader;
#endif

#ifdef SUPPORT_VULKAN
    bool initialized = false;
    std::vector<NVGcontext*> vgArray;
    vk::Renderer* rendererVk = nullptr;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    //std::vector<sgl::vk::CommandBufferPtr> nanovgCommandBuffers;
    std::vector<VkCommandBuffer> nanovgCommandBuffers;
    vk::FramebufferPtr framebufferVk;
    vk::ImageViewPtr renderTargetImageViewVk;
    vk::TexturePtr renderTargetTextureVk;

    void _createBlitRenderPass();
    sgl::vk::BlitRenderPassPtr blitPassVk;
    vk::ImageViewPtr blitTargetVk;
    VkImageLayout blitInitialLayoutVk;
    VkImageLayout blitFinalLayoutVk;
    sgl::vk::BufferPtr blitMatrixBuffer;
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    std::vector<sgl::vk::CommandBufferPtr> commandBuffersPost;
    sgl::InteropSyncVkGlPtr interopSyncVkGl;
#endif
};

}

#endif //CORRERENDER_NANOVGWRAPPER_HPP
