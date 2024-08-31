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

#ifndef SGL_GLFWWINDOW_HPP
#define SGL_GLFWWINDOW_HPP

#include <Graphics/Window.hpp>

#ifdef SUPPORT_WEBGPU
typedef struct WGPUSurfaceImpl* WGPUSurface;
#endif

typedef struct GLFWwindow GLFWwindow;
typedef struct GLFWcursor GLFWcursor;

namespace sgl {

class DLL_OBJECT GlfwWindow : public Window {
public:
    GlfwWindow();
    ~GlfwWindow() override;
    [[nodiscard]] WindowBackend getBackend() const override { return WindowBackend::GLFW_IMPL; }

    /// Outputs, e.g., "glfwGetError".
    void errorCheck() override;
    static void errorCheckGlfw();

    /// Returns whether this window uses.
    bool isDebugContext() override { return windowSettings.debugContext; }

    /// Initialize the window.
    void initialize(const WindowSettings &settings, RenderSystem renderSystem) override;

    /// Change the window attributes.
    /// Try to keep resolution.
    void toggleFullscreen(bool nativeFullscreen = true) override;
    void setWindowPosition(int x, int y) override;
    void serializeSettings(SettingsFile &settings) override;
    WindowSettings deserializeSettings(const SettingsFile &settings) override;

    /// Update the window.
    void update() override;
    /// Returns false if the game should quit.
    bool processEvents() override;
    void clear(const Color &color = Color(0, 0, 0)) override;
    void flip() override;

    // Event Callbacks.
    void setRefreshRateCallback(std::function<void(int)> callback);
    void setOnKeyCallback(std::function<void(int, int, int, int)> callback);
    void setOnDropCallback(std::function<void(const std::vector<std::string>&)> callback);

    /// Utility functions and getters & setters for the main window attributes.
    // Virtual and pixel size is equivalent on Linux and Windows, but not on macOS.
    void saveScreenshot(const char *filename) override;
    bool isFullscreen() override { return windowSettings.fullscreen; }
    int getVirtualWidth() override { return windowSettings.width; }
    int getVirtualHeight() override { return windowSettings.height; }
    int getPixelWidth() override { return windowSettings.pixelWidth; }
    int getPixelHeight() override { return windowSettings.pixelHeight; }
    glm::ivec2 getWindowVirtualResolution() override { return {windowSettings.width, windowSettings.height}; }
    glm::ivec2 getWindowPixelResolution() override { return {windowSettings.pixelWidth, windowSettings.pixelHeight}; }
    glm::ivec2 getWindowPosition() override;
    [[nodiscard]] const WindowSettings& getWindowSettings() const override { return windowSettings; }
    void setWindowVirtualSize(int width, int height) override;
    void setWindowPixelSize(int width, int height) override;

    // Legacy, may make problems on macOS.
    int getWidth() override { return windowSettings.pixelWidth; }
    int getHeight() override { return windowSettings.pixelHeight; }
    glm::ivec2 getWindowResolution() override { return {windowSettings.pixelWidth, windowSettings.pixelHeight}; }
    void setWindowSize(int width, int height) override { setWindowPixelSize(width, height); }

    /// Sets the window icon.
    void setWindowIconFromFile(const std::string& imageFilename) override;

    /// Sets the window cursor.
    void setCursorType(CursorType _cursorType) override;
    void setShowCursor(bool _show) override;
    void setCaptureMouse(bool _capture);

#ifdef SUPPORT_OPENGL
    void* getOpenGLFunctionPointer(const char* functionName) override;
#endif

    /// Whether to download all images from the GPU instead of using a swapchain.
    [[nodiscard]] bool getUseDownloadSwapchain() const override { return windowSettings.useDownloadSwapchain; }

    /// Returns whether the Wayland backend is used.
    [[nodiscard]] bool getUsesX11Backend() const override { return usesX11Backend; }
    [[nodiscard]] bool getUsesWaylandBackend() const override { return usesWaylandBackend; }
    [[nodiscard]] bool getUsesXWaylandBackend() const override { return usesXWaylandBackend; }
    [[nodiscard]] bool getUsesX11OrWaylandBackend() const override { return usesX11Backend || usesWaylandBackend; }
    [[nodiscard]] bool getUsesAnyWaylandBackend() const override { return usesWaylandBackend || usesXWaylandBackend; }

    /// Getting GLFW specific data
    inline GLFWwindow *getGlfwWindow() { return glfwWindow; }
#ifdef SUPPORT_VULKAN
    VkSurfaceKHR getVkSurface() override { return windowSurface; }
#endif
#ifdef SUPPORT_WEBGPU
    WGPUSurface getWebGPUSurface() override { return webgpuSurface; }
#endif

private:
    RenderSystem renderSystem = RenderSystem::VULKAN;
    WindowSettings windowSettings;
    bool usesX11Backend = false;
    bool usesWaylandBackend = false;
    bool usesXWaylandBackend = false;

    // Callbacks.
    void onKey(int key, int scancode, int action, int mods);
    void onChar(unsigned int codepoint);
    void onCharMods(unsigned int codepoint, int mods);
    void onCursorPos(double xpos, double ypos);
    void onCursorEnter(int entered);
    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);
    void onDrop(int count, const char** paths);
    void onFramebufferSize(int width, int height);
    void onWindowContentScale(float xscale, float yscale);
    std::function<void(int)> refreshRateCallback;
    std::function<void(int, int, int, int)> onKeyCallback;
    std::function<void(const std::vector<std::string>&)> onDropCallback;

    bool isFirstFrame = true;
    bool isRunning = true;

    // For fullscreen.
    int widthOld = 0, heightOld = 0;

    /// Application cursor type.
    std::unordered_map<CursorType, GLFWcursor*> cursors;
    CursorType currentCursorType = CursorType::DEFAULT;
    bool showCursor = true;
    bool captureMouse = false;

    GLFWwindow* glfwWindow = nullptr;

#ifdef SUPPORT_VULKAN
    VkSurfaceKHR windowSurface{};
#endif

#ifdef SUPPORT_WEBGPU
    WGPUSurface webgpuSurface{};
#endif
};

}

#endif //SGL_GLFWWINDOW_HPP
