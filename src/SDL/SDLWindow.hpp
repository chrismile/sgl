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

class DLL_OBJECT SDLWindow : public Window
{
public:
    SDLWindow();
    //! Outputs e.g. "SDL_GetError"
    virtual void errorCheck();

    /// Returns whether this window uses
    virtual bool isDebugContext() { return windowSettings.debugContext; }

    //! Initialize/close the window
    virtual void initialize(const WindowSettings &settings, RenderSystem renderSystem);
    virtual void destroySurface();
    virtual void close();

    //! Change the window attributes
    //! Try to keep resolution
    virtual void toggleFullscreen(bool nativeFullscreen = true);
    virtual void setWindowSize(int width, int height);
    virtual void setWindowPosition(int x, int y);
    virtual void serializeSettings(SettingsFile &settings);
    virtual WindowSettings deserializeSettings(const SettingsFile &settings);

    //! Update the window
    virtual void update();
    virtual void setEventHandler(std::function<void(const SDL_Event&)> eventHandler);
    //! Returns false if the game should quit
    virtual bool processEvents();
    virtual void clear(const Color &color = Color(0, 0, 0));
    virtual void flip();

    //! Utility functions/getters for the main window attributes
    virtual void saveScreenshot(const char *filename);
    virtual bool isFullscreen() { return windowSettings.fullscreen; }
    virtual int getWidth() { return windowSettings.width; }
    virtual int getHeight() { return windowSettings.height; }
    virtual glm::ivec2 getWindowResolution() { return glm::ivec2(windowSettings.width, windowSettings.height); }
    virtual glm::ivec2 getWindowPosition();
    virtual const WindowSettings& getWindowSettings() const { return windowSettings; }

    //! Getting SDL specific data
    inline SDL_Window *getSDLWindow() { return sdlWindow; }
#ifdef SUPPORT_OPENGL
    inline SDL_GLContext getGLContext() { return glContext; }
#endif
#ifdef SUPPORT_VULKAN
    virtual VkSurfaceKHR getVkSurface() { return windowSurface; }
#endif

private:
    RenderSystem renderSystem;
    WindowSettings windowSettings;

    bool eventHandlerSet = false;
    std::function<void(const SDL_Event&)> eventHandler;

    //! For toggle fullscreen: Resolution before going fullscreen
    SDL_DisplayMode oldDisplayMode;
    bool isFirstFrame = true;

    SDL_Window *sdlWindow = nullptr;

#ifdef SUPPORT_OPENGL
    SDL_GLContext glContext = nullptr;
#endif

#ifdef SUPPORT_VULKAN
    VkSurfaceKHR windowSurface;
#endif
};

}

/*! SRC_SDL_SDLWINDOW_HPP_ */
#endif
