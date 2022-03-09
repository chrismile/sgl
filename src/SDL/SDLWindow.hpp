/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

#ifndef SRC_SDL_SDLWINDOW_HPP_
#define SRC_SDL_SDLWINDOW_HPP_

#include <SDL2/SDL.h>
#include <Graphics/Window.hpp>

#ifdef SUPPORT_VULKAN
#include <SDL2/SDL_vulkan.h>
#endif

namespace sgl {

class DLL_OBJECT SDLWindow : public Window {
public:
    SDLWindow();
    ~SDLWindow() override;

    /// Outputs e.g. "SDL_GetError"
    void errorCheck() override;
    static void errorCheckSDL();
    void errorCheckSDLCritical();

    /// Returns whether this window uses
    bool isDebugContext() override { return windowSettings.debugContext; }

    /// Initialize the window
    void initialize(const WindowSettings &settings, RenderSystem renderSystem) override;

    /// Change the window attributes
    /// Try to keep resolution
    void toggleFullscreen(bool nativeFullscreen = true) override;
    void setWindowSize(int width, int height) override;
    void setWindowPosition(int x, int y) override;
    void serializeSettings(SettingsFile &settings) override;
    WindowSettings deserializeSettings(const SettingsFile &settings) override;

    /// Update the window
    void update() override;
    void setEventHandler(std::function<void(const SDL_Event&)> eventHandler) override;
    /// Returns false if the game should quit
    bool processEvents() override;
    void clear(const Color &color = Color(0, 0, 0)) override;
    void flip() override;

    /// Utility functions/getters for the main window attributes
    void saveScreenshot(const char *filename) override;
    bool isFullscreen() override { return windowSettings.fullscreen; }
    int getWidth() override { return windowSettings.width; }
    int getHeight() override { return windowSettings.height; }
    glm::ivec2 getWindowResolution() override { return glm::ivec2(windowSettings.width, windowSettings.height); }
    glm::ivec2 getWindowPosition() override;
    [[nodiscard]] const WindowSettings& getWindowSettings() const override { return windowSettings; }

    /// Getting SDL specific data
    inline SDL_Window *getSDLWindow() { return sdlWindow; }
#ifdef SUPPORT_OPENGL
    inline SDL_GLContext getGLContext() { return glContext; }
#endif
#ifdef SUPPORT_VULKAN
    VkSurfaceKHR getVkSurface() override { return windowSurface; }
#endif

private:
    RenderSystem renderSystem;
    WindowSettings windowSettings;

    bool eventHandlerSet = false;
    std::function<void(const SDL_Event&)> eventHandler;

    /// For toggle fullscreen: Resolution before going fullscreen
    SDL_DisplayMode oldDisplayMode{};
    bool isFirstFrame = true;

    SDL_Window *sdlWindow = nullptr;

#ifdef SUPPORT_OPENGL
    SDL_GLContext glContext = nullptr;
#endif

#ifdef SUPPORT_VULKAN
    VkSurfaceKHR windowSurface{};
#endif
};

}

/*! SRC_SDL_SDLWINDOW_HPP_ */
#endif
