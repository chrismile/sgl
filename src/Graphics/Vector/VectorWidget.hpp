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

#ifndef SGL_VECTORWIDGET_HPP
#define SGL_VECTORWIDGET_HPP

#include <map>
#include <memory>

#include <glm/vec4.hpp>

#include "VectorBackend.hpp"

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

namespace sgl {

struct DLL_OBJECT VectorWidgetSettings {
    bool shallClearBeforeRender = true;
    glm::vec4 clearColor = glm::vec4(0.0f);
};

class DLL_OBJECT VectorWidget {
public:
    explicit VectorWidget(const VectorWidgetSettings& vectorWidgetSettings = {});
    virtual ~VectorWidget();
    void setDefaultBackendId(const std::string& defaultId);

    virtual void update(float dt) {}
    void render();
    virtual bool renderGuiPropertyEditor(sgl::PropertyEditor& propertyEditor);
    virtual void copyVectorWidgetSettingsFrom(sgl::VectorWidget* vectorWidget);
    bool getIsFirstRender();

    // Returns whether the mouse is over the area of the window.
    /// Assumes the rendering area is the main window and aligned with position (0,0) of the mouse.
    [[nodiscard]] bool getIsMouseOverDiagram() const;
    /// Assumes the rendering area of the passed size is aligned with position (parentX,parentY) of the mouse.
    [[nodiscard]] bool getIsMouseOverDiagram(int parentX, int parentY, int parentWidth, int parentHeight) const;
    /// Assumes the rendering area is aligned with position (0,0) of the mouse.
    [[nodiscard]] bool getIsMouseOverDiagram(const glm::ivec2& mousePositionPx) const;


#ifdef SUPPORT_OPENGL
    [[nodiscard]] inline const sgl::TexturePtr& getRenderTargetTextureGl() { return renderTargetGl; }
    void blitToTargetGl(sgl::FramebufferObjectPtr& sceneFramebuffer);
#endif

#ifdef SUPPORT_VULKAN
    [[nodiscard]] inline const vk::TexturePtr& getRenderTargetTextureVk() { return renderTargetTextureVk; }
    inline void setRendererVk(vk::Renderer* renderer) { rendererVk = renderer; }
    void setBlitTargetVk(vk::ImageViewPtr& blitTargetVk, VkImageLayout initialLayout, VkImageLayout finalLayout);
    void blitToTargetVk();
#endif

    void setBlitTargetSupersamplingFactor(int f);
    std::pair<uint32_t, uint32_t> getBlitTargetSize();

    // public only for VectorBackend subclasses.
    void onWindowSizeChanged();
    void setSupersamplingFactor(int _supersamplingFactor, bool recomputeWindowSize = true);

    /// Must be called for Vulkan backend before @see onWindowSizeChanged to make sure textures are no longer in use.
    void syncRendererWithCpu();

protected:
    void _initialize();
    virtual void onBackendCreated() {}
    virtual void onBackendDestroyed() {}

    template <typename T>
    void registerRenderBackendIfSupported(std::function<void()> renderFunctor) {
        bool isSupported = T::checkIsSupported();
        if (!isSupported) {
            return;
        }

        VectorBackendFactory factory;
        factory.id = T::getClassID();
        factory.createBackendFunctor = [this]() { return new T(this); };
        factory.renderFunctor = std::move(renderFunctor);
        factories.insert(std::make_pair(factory.id, factory));
    }

    template <typename T, typename BackendSettings>
    void registerRenderBackendIfSupported(std::function<void()> renderFunctor, const BackendSettings& backendSettings) {
        bool isSupported = T::checkIsSupported();
        if (!isSupported) {
            return;
        }

        VectorBackendFactory factory;
        factory.id = T::getClassID();
        factory.createBackendFunctor = [this, backendSettings]() { return new T(this, backendSettings); };
        factory.renderFunctor = std::move(renderFunctor);
        factories.insert(std::make_pair(factory.id, factory));
    }

    [[nodiscard]] inline float getWindowOffsetX() const { return windowOffsetX; }
    [[nodiscard]] inline float getWindowOffsetY() const { return windowOffsetY; }
    [[nodiscard]] inline float getScaleFactor() const { return scaleFactor; }

    float windowWidth = 1.0f, windowHeight = 1.0f;
    float windowOffsetX = 20, windowOffsetY = 20;
    float customScaleFactor = 0.0f;

    int fboWidthInternal = 1, fboHeightInternal = 1;
    int fboWidthDisplay = 1, fboHeightDisplay = 1;
    float scaleFactor = 1.0f;
    int supersamplingFactor = 4;
    int blitTargetSupersamplingFactor = 1;

    VectorBackend* vectorBackend = nullptr;

private:
    bool initialized = false;
    bool shallClearBeforeRender = true;
    glm::vec4 clearColor = glm::vec4(0.0f);
    bool isFirstRender = true; // First rendered frame since reset (e.g. through resize)?

    void createDefaultBackend();
    void onSelectedBackendIdxChanged();
    std::string defaultBackendId;
    std::map<std::string, VectorBackendFactory> factories;
    std::vector<std::string> vectorBackendIds;
    int selectedVectorBackendIdx = 0;

#ifdef SUPPORT_OPENGL
    sgl::TexturePtr renderTargetGl;
    sgl::ShaderProgramPtr blitShader, blitMsaaShader, blitDownscaleShader, blitDownscaleMsaaShader;
#endif

#ifdef SUPPORT_VULKAN
    vk::Renderer* rendererVk = nullptr;
    vk::ImageViewPtr renderTargetImageViewVk;
    vk::TexturePtr renderTargetTextureVk;

    void _createBlitRenderPass();
    bool blitRenderPassCreateLater = false; ///< If backend creates texture while rendering.
    int cachedBlitPassSupersampling = 0;
    bool cachedBlitPassMsaa = false;
    sgl::vk::BlitRenderPassPtr blitPassVk;
    vk::ImageViewPtr blitTargetVk;
    VkImageLayout blitInitialLayoutVk;
    VkImageLayout blitFinalLayoutVk;
    sgl::vk::BufferPtr blitMatrixBuffer;
#endif
};

}

#endif //SGL_VECTORWIDGET_HPP
