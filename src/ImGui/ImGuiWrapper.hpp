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
#include <glm/vec4.hpp>

#include <Utils/Singleton.hpp>

#include "imgui.h"

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/libs/volk/volk.h>
#include "imgui_impl_vulkan.h"

namespace sgl { namespace vk {
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;
class Renderer;
}}
#endif

#ifdef SUPPORT_WEBGPU
namespace sgl { namespace webgpu {
class Renderer;
}}
#endif

union SDL_Event;

namespace sgl {

// For calls to @see ImGuiWrapper::setNextWindowStandardPosSizeLocation.
const int LOCATION_LEFT = 0x1;
const int LOCATION_RIGHT = 0x2;
const int LOCATION_TOP = 0x4;
const int LOCATION_BOTTOM = 0x8;

class DLL_OBJECT ImGuiWrapper : public Singleton<ImGuiWrapper> {
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
    [[nodiscard]] inline float getScaleFactor() const { return uiScaleFactor; } //< The UI high DPI scale factor.
    [[nodiscard]] inline float getSizeScale() const { return sizeScale; } //< The normalized UI high DPI scale factor.

    [[nodiscard]] inline float getFontSizeNormal() const { return fontSizeNormal; }
    [[nodiscard]] inline float getFontSizeSmall() const { return fontSizeSmall; }
    inline ImFont* getFontNormal() { return fontNormal; }
    inline ImFont* getFontSmall() { return fontSmall; }

    // Sets the default scale factor.
    inline void setDefaultScaleFactor(float factor) {
        defaultUiScaleFactor = factor;
        sizeScale = uiScaleFactor / defaultUiScaleFactor;
    }

    // Takes into account that defaultUiScaleFactor is used for the sizes.
    void setNextWindowStandardPos(int x, int y);
    void setNextWindowStandardSize(int width, int height);
    void setNextWindowStandardPosSize(int x, int y, int width, int height);
    void setNextWindowStandardPosSizeLocation(int location, int offsetX, int offsetY, int width, int height);
    float getScaleDependentSize(float width);
    ImVec2 getScaleDependentSize(int width, int height);

    // Insert your ImGui code between "renderStart" and "renderEnd".
    void renderStart();
    void renderEnd();
    void processSDLEvent(const SDL_Event &event);
    void onResolutionChanged();

    // Utility functions for dock space mode.
    [[nodiscard]] inline bool getUseDockSpaceMode() const { return useDockSpaceMode; }
    inline void setUseDockSpaceMode(bool _useDockSpaceMode) { useDockSpaceMode = _useDockSpaceMode; }
    ImGuiViewport* getCurrentWindowViewport() { return windowViewports.at(currentWindowIdx); }
    void setWindowViewport(size_t windowIdx, ImGuiViewport* windowViewport) {
        if (windowViewports.size() <= windowIdx) {
            windowViewports.resize(windowIdx + 1);
        }
        currentWindowIdx = windowIdx;
        windowViewports.at(currentWindowIdx) = windowViewport;
    }
    const ImVec2& getCurrentWindowPosition() { return windowPositions.at(currentWindowIdx); }
    const ImVec2& getCurrentWindowSize() { return windowSizes.at(currentWindowIdx); }
    void setWindowPosAndSize(size_t windowIdx, const ImVec2& windowPosition, const ImVec2& windowSize) {
        if (windowPositions.size() <= windowIdx) {
            windowPositions.resize(windowIdx + 1);
            windowSizes.resize(windowIdx + 1);
        }
        currentWindowIdx = windowIdx;
        windowPositions.at(currentWindowIdx) = windowPosition;
        windowSizes.at(currentWindowIdx) = windowSize;
    }

    //
    /**
     * In dockspace mode, the background may not be covered completely due to fractional scaling.
     * In this case, we do not want to use the clear color of the dock windows, but a color matching the ImGui style.
     * @return The best background clear color for dockspace mode.
     */
    [[nodiscard]] inline const glm::vec4& getBackgroundClearColor() const { return backgroundClearColor; }

    void renderDemoWindow();
    void showHelpMarker(const char* desc);

#ifdef SUPPORT_VULKAN
    void setVkRenderTarget(vk::ImageViewPtr& imageView);
    void setRendererVk(vk::Renderer* renderer) { rendererVk = renderer; }
    std::vector<VkCommandBuffer>& getVkCommandBuffers() { return imguiCommandBuffers; }
    VkDescriptorPool getVkDescriptorPool() { return imguiDescriptorPool; }
    void freeDescriptorSet(VkDescriptorSet descriptorSet);
#endif

#ifdef SUPPORT_WEBGPU
    void setRendererWgpu(webgpu::Renderer* renderer) { rendererWgpu = renderer; }
#endif

private:
    float uiScaleFactor;
    float defaultUiScaleFactor = 1.875f;
    float sizeScale = 1.0f;

    float fontSizeNormal = 0.0f;
    float fontSizeSmall = 0.0f;
    ImFont* fontNormal = nullptr;
    ImFont* fontSmall = nullptr;

    // Dock space mode.
    bool useDockSpaceMode = false;
    size_t currentWindowIdx = 0;
    std::vector<ImGuiViewport*> windowViewports;
    std::vector<ImVec2> windowPositions;
    std::vector<ImVec2> windowSizes;
    glm::vec4 backgroundClearColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

#ifdef SUPPORT_VULKAN
    bool initialized = false;
    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    vk::Renderer* rendererVk = nullptr;
    std::vector<VkCommandBuffer> imguiCommandBuffers;
    vk::FramebufferPtr framebuffer;
    vk::ImageViewPtr renderTargetImageView;
    ImGui_ImplVulkanH_Window mainWindowData;
#endif

#ifdef SUPPORT_WEBGPU
    webgpu::Renderer* rendererWgpu = nullptr;
#endif
};

}

#endif //SGL_IMGUIWRAPPER_HPP
