/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2018, Christoph Neuhauser
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

#ifndef SGL_IMGUIWRAPPER_HPP
#define SGL_IMGUIWRAPPER_HPP

#include <vector>
#include <Utils/Singleton.hpp>
#include "imgui.h"

#ifdef SUPPORT_VULKAN
#include <vulkan/vulkan.h>

namespace sgl { namespace vk {
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;
}}
#endif

union SDL_Event;

namespace sgl
{

class DLL_OBJECT ImGuiWrapper : public Singleton<ImGuiWrapper>
{
public:
    /**
     * Initializes ImGui for use with SDL and OpenGL.
     * @param fontRangesData The range of the font to be loaded in the texture atlas.
     * For more details @see ImFontGlyphRangesBuilder.
     * @param useDocking Whether to enable docking windows.
     * @param useMultiViewport Whether to enable using multiple viewport windows when the user drags ImGui windows
     * outside of the main window.
     * @param uiScaleFactor A factor for scaling the UI elements. Is multiplied with a high DPI scaling factor.
     * Use @see overwriteHighDPIScaleFactor to manually overwrite the high DPI scaling factor if necessary.
     *
     * To be called by AppSettings.
     */
    void initialize(
            const ImWchar* fontRangesData = nullptr, bool useDocking = true, bool useMultiViewport = true,
            float uiScaleFactor = 1.0f);
    void shutdown(); //< to be called by AppSettings
    inline float getScaleFactor() { return uiScaleFactor; } //< The UI high DPI scale factor

    // Sets the default scale factor.
    inline void setDefaultScaleFactor(float factor) {
        defaultUiScaleFactor = factor;
        sizeScale = uiScaleFactor / defaultUiScaleFactor;
    }

    // Takes into account that defaultUiScaleFactor is used for the sizes.
    void setNextWindowStandardPos(int x, int y);
    void setNextWindowStandardSize(int width, int height);
    void setNextWindowStandardPosSize(int x, int y, int width, int height);

    // Insert your ImGui code between "renderStart" and "renderEnd"
    void renderStart();
    void renderEnd();
    void processSDLEvent(const SDL_Event &event);
    void onResolutionChanged();

    void renderDemoWindow();
    void showHelpMarker(const char* desc);

#ifdef SUPPORT_VULKAN
    void setVkRenderTarget(vk::ImageViewPtr imageView);
    std::vector<VkCommandBuffer>& getVkCommandBuffers() { return imguiCommandBuffers; }
#endif

private:
    float uiScaleFactor;
    float defaultUiScaleFactor = 1.875f;
    float sizeScale = 1.0f;

#ifdef SUPPORT_VULKAN
    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> imguiCommandBuffers;
    vk::FramebufferPtr framebuffer;
    vk::ImageViewPtr imageView;
#endif
};

}

#endif //SGL_IMGUIWRAPPER_HPP
