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

#ifndef SGL_VECTORBACKEND_HPP
#define SGL_VECTORBACKEND_HPP

#include <string>
#include <functional>
#include <memory>

#include <Utils/AppSettings.hpp>

#ifdef SUPPORT_OPENGL
namespace sgl {
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
}
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/libs/volk/volk.h>

namespace sgl { namespace vk {
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
class Renderer;
}}
#endif

namespace sgl {

class VectorBackend;
class VectorWidget;

/**
 * Render system vs. render backend:
 * A different backend than the render system can be used.
 * In this case, resource sharing is used between OpenGL and Vulkan.
 */
struct DLL_OBJECT VectorBackendCapabilities {
    bool supportsOpenGL = false;
    bool supportsVulkan = false;
    bool supportsMsaa = false;
};

struct DLL_OBJECT VectorBackendFactory {
    std::string id;
    std::function<VectorBackend*()> createBackendFunctor;
    std::function<void()> renderFunctor;
};

class DLL_OBJECT VectorBackend {
public:
    [[nodiscard]] virtual const char* getID() const=0;
    explicit VectorBackend(VectorWidget* vectorWidget) : vectorWidget(vectorWidget) {}
    virtual ~VectorBackend()=default;
    virtual void initialize()=0;
    virtual void destroy()=0;
    virtual void onResize()=0;
    virtual void renderStart()=0;
    virtual void renderEnd()=0;

    [[nodiscard]] RenderSystem getRenderBackend() const { return renderBackend; }

#ifdef SUPPORT_OPENGL
    [[nodiscard]] inline const sgl::TexturePtr& getRenderTargetTextureGl() const { return renderTargetGl; }
#endif

#ifdef SUPPORT_VULKAN
    [[nodiscard]] inline const sgl::vk::TexturePtr& getRenderTargetTextureVk() const { return renderTargetTextureVk; }
    inline void setRendererVk(vk::Renderer* renderer) { rendererVk = renderer; }
#endif

    void setWidgetSize(
            float _scaleFactor, int _supersamplingFactor, float _windowWidth, float _windowHeight,
            int _fboWidthInternal, int _fboHeightInternal, int _fboWidthDisplay, int _fboHeightDisplay) {
        scaleFactor = _scaleFactor;
        supersamplingFactor = _supersamplingFactor;
        windowWidth = _windowWidth;
        windowHeight = _windowHeight;
        fboWidthInternal = _fboWidthInternal;
        fboHeightInternal = _fboHeightInternal;
        fboWidthDisplay = _fboWidthDisplay;
        fboHeightDisplay = _fboHeightDisplay;
    }

protected:
    VectorWidget* vectorWidget = nullptr;
    bool initialized = false;
    RenderSystem renderBackend = RenderSystem::VULKAN;
    bool shallClearBeforeRender = true;
    glm::vec4 clearColor = glm::vec4(0.0f);

    float scaleFactor = 1.0f;
    int supersamplingFactor = 4;
    float windowWidth = 1.0f, windowHeight = 1.0f;
    int fboWidthInternal = 1, fboHeightInternal = 1;
    int fboWidthDisplay = 1, fboHeightDisplay = 1;

#ifdef SUPPORT_OPENGL
    sgl::TexturePtr renderTargetGl;
#endif

#ifdef SUPPORT_VULKAN
    vk::Renderer* rendererVk = nullptr;
    sgl::vk::ImageViewPtr renderTargetImageViewVk;
    sgl::vk::TexturePtr renderTargetTextureVk;
#endif
};

}

#endif //SGL_VECTORBACKEND_HPP
