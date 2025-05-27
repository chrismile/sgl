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

#ifndef SGL_VECTORBACKENDNANOVG_HPP
#define SGL_VECTORBACKENDNANOVG_HPP

#ifdef USE_GLM
#include <glm/vec4.hpp>
#else
#include <Math/Geometry/fallback/vec4.hpp>
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Buffers/Framebuffer.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/CommandBuffer.hpp>
#endif

#include "VectorBackend.hpp"

#ifdef SUPPORT_OPENGL
namespace sgl {
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
class FramebufferObject;
typedef std::shared_ptr<FramebufferObject> FramebufferObjectPtr;
class RenderbufferObject;
typedef std::shared_ptr<RenderbufferObject> RenderbufferObjectPtr;
}
#endif

#ifdef SUPPORT_VULKAN
namespace sgl { namespace vk {
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;
class CommandBuffer;
typedef std::shared_ptr<CommandBuffer> CommandBufferPtr;
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

enum class NanoVgAAMode {
    OFF, INTERNAL, MSAA
};

struct DLL_OBJECT NanoVGSettings {
    NanoVGSettings();
    RenderSystem renderBackend;
    NanoVgAAMode msaaMode = NanoVgAAMode::INTERNAL;
    int numMsaaSamples = 8; ///< If useMsaa == NanoVgAAMode::MSAA.
    int supersamplingFactor = 4;
    bool useStencilStrokes = false;
    bool useDebugging;
};

class DLL_OBJECT VectorBackendNanoVG : public VectorBackend {
public:
    static const char* getClassID() { return "NanoVG"; }
    [[nodiscard]] const char* getID() const override { return getClassID(); }
    static bool checkIsSupported();

    explicit VectorBackendNanoVG(VectorWidget* vectorWidget, const NanoVGSettings& nanoVgSettings = NanoVGSettings());
    void initialize() override;
    void destroy() override;
    void onResize() override;
    void renderStart() override;
    void renderEnd() override;
    bool renderGuiPropertyEditor(sgl::PropertyEditor& propertyEditor) override;
    void copyVectorBackendSettingsFrom(VectorBackend* backend) override;
#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    // Adds OpenGL image to backend (only valid for the current frame).
    void addImageGl(const sgl::TexturePtr& texture, VkImageLayout srcLayout, VkImageLayout dstLayout) override;
#endif

    [[nodiscard]] inline NVGcontext* getContext() { return vg; }

private:
    static void _initializeFont(NVGcontext* vgCurrent);

    int flags = 0;
    NVGcontext* vg = nullptr;

    NanoVgAAMode msaaMode = NanoVgAAMode::INTERNAL;
    int numMsaaSamples = 8;
    bool useStencilStrokes = false;
    bool useDebugging = false;

#ifdef SUPPORT_OPENGL
    sgl::FramebufferObjectPtr framebufferGl;
    sgl::RenderbufferObjectPtr depthStencilRbo;
#endif

#ifdef SUPPORT_VULKAN
    std::vector<NVGcontext*> vgArray;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> nanovgCommandBuffers;
    vk::FramebufferPtr framebufferVk;
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    std::vector<sgl::vk::CommandBufferPtr> commandBuffersPost;
    sgl::InteropSyncVkGlPtr interopSyncVkGl;
    std::vector<VectorBackendTextureInteropInfo> interopTextures;
#endif
};

}

#endif //SGL_VECTORBACKENDNANOVG_HPP
