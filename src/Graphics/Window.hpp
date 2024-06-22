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

#ifndef SRC_GRAPHICS_WINDOW_HPP_
#define SRC_GRAPHICS_WINDOW_HPP_

#include <functional>
#include <glm/vec2.hpp>

#ifdef SUPPORT_VULKAN
#include <SDL2/SDL_vulkan.h>
namespace sgl { namespace vk { class Swapchain; } }
#endif

#include <Defs.hpp>
#include <Graphics/Color.hpp>
#include <Utils/AppSettings.hpp>

union SDL_Event;

namespace sgl {

const uint32_t RESOLUTION_CHANGED_EVENT = 74561634U;

/**
 * If one of the modes is not available, the next lower one is used.
 * On OpenGL, the following swap intervals are used.
 * - IMMEDIATE: 0
 * - FIFO: 1
 * - FIFO_RELAXED & VSYNC_MODE_MAILBOX: -1
 */
enum VSyncMode {
    VSYNC_MODE_IMMEDIATE, // no vsync
    VSYNC_MODE_FIFO, // normal vsync
    VSYNC_MODE_FIFO_RELAXED, // vsync if fps >= refresh rate
    VSYNC_MODE_MAILBOX // vsync, replace oldest image
};

struct DLL_OBJECT WindowSettings {
    int width;
    int height;
    // Pixel width and height may differ on macOS.
    int pixelWidth;
    int pixelHeight;
    bool fullscreen;
    bool resizable;
    int multisamples;
    int depthSize;
    int stencilSize;
    bool vSync;
    VSyncMode vSyncMode;
    bool debugContext;
    bool savePosition;
    glm::ivec2 windowPosition{};
    bool useDownloadSwapchain;

    WindowSettings() {
        width = 1920;
        height = 1080;
        pixelWidth = width;
        pixelHeight = height;
        fullscreen = false;
        resizable = true;
        multisamples = 16;
        depthSize = 24;
        stencilSize = 8;
        vSync = true;
        vSyncMode = VSYNC_MODE_FIFO_RELAXED;
#ifdef _DEBUG
        debugContext = true;
#else
        debugContext = false;
#endif
        savePosition = false;
        windowPosition = glm::ivec2(std::numeric_limits<int>::min());
        useDownloadSwapchain = false;
    }
};

/// Cursor type, see https://wiki.libsdl.org/SDL2/SDL_SystemCursor.
enum class CursorType {
    DEFAULT, ARROW, IBEAM, WAIT, CROSSHAIR, WAITARROW, SIZENWSE, SIZENESW, SIZEWE, SIZENS, SIZEALL, NO, HAND
};

class SettingsFile;

/// Use AppSettings (Utils/AppSettings.hpp) to create a window using the user's preferred settings

class DLL_OBJECT Window {
public:
    virtual ~Window() = default;
    /// Outputs e.g. "SDL_GetError"
    virtual void errorCheck() {}

    /// Returns whether this window uses
    virtual bool isDebugContext()=0;

    /// Initialize the window
    virtual void initialize(const WindowSettings& windowSettings, RenderSystem renderSystem)=0;

    /// Change the window attributes
    virtual void toggleFullscreen(bool nativeFullscreen = true)=0;
    virtual void setWindowPosition(int x, int y)=0;
    virtual void serializeSettings(SettingsFile &settings)=0;
    virtual WindowSettings deserializeSettings(const SettingsFile &settings)=0;

    /// Update the window.
    virtual void update()=0;
    virtual void setEventHandler(std::function<void(const SDL_Event&)> eventHandler)=0;
    /// Returns false if the game should quit.
    virtual bool processEvents()=0;
    virtual void clear(const Color &color = Color(0, 0, 0))=0;
    virtual void flip()=0;

    /// Sets the window icon.
    virtual void setWindowIconFromFile(const std::string& imageFilename) {}

    /// Sets the window cursor.
    virtual void setCursorType(CursorType cursorType) {}
    virtual void setShowCursor(bool _show) {}

    /// Utility functions and getters & setters for the main window attributes.
    // Virtual and pixel size is equivalent on Linux and Windows, but not on macOS.
    virtual void saveScreenshot(const char *filename)=0;
    virtual bool isFullscreen()=0;
    virtual int getVirtualWidth()=0;
    virtual int getVirtualHeight()=0;
    virtual int getPixelWidth()=0;
    virtual int getPixelHeight()=0;
    virtual glm::ivec2 getWindowVirtualResolution()=0;
    virtual glm::ivec2 getWindowPixelResolution()=0;
    virtual glm::ivec2 getWindowPosition()=0;
    [[nodiscard]] virtual const WindowSettings& getWindowSettings() const=0;
    virtual void setWindowVirtualSize(int width, int height)=0;
    virtual void setWindowPixelSize(int width, int height)=0;

    // Legacy, may make problems on macOS.
    virtual int getWidth()=0;
    virtual int getHeight()=0;
    virtual glm::ivec2 getWindowResolution()=0;
    virtual void setWindowSize(int width, int height)=0;

    /// Whether to download all images from the GPU instead of using a swapchain.
    [[nodiscard]] virtual bool getUseDownloadSwapchain() const { return false; }

    /// Returns whether the Wayland backend is used.
    [[nodiscard]] virtual bool getUsesX11Backend() const { return false; }
    [[nodiscard]] virtual bool getUsesWaylandBackend() const { return false; }
    [[nodiscard]] virtual bool getUsesXWaylandBackend() const { return false; }
    [[nodiscard]] virtual bool getUsesX11OrWaylandBackend() const { return false; }
    [[nodiscard]] virtual bool getUsesAnyWaylandBackend() const { return false; }

#ifdef SUPPORT_OPENGL
    virtual void* getOpenGLFunctionPointer(const char* functionName)=0;
#endif

#ifdef SUPPORT_VULKAN
    virtual VkSurfaceKHR getVkSurface()=0;
#endif
};

}

/*! SRC_GRAPHICS_WINDOW_HPP_ */
#endif
