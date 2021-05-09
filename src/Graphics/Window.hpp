/*!
 * Window.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
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

struct WindowSettings {
    int width;
    int height;
    bool fullscreen;
    bool resizable;
    int multisamples;
    int depthSize;
    int stencilSize;
    bool vSync;
    VSyncMode vSyncMode;
    bool debugContext;
    bool savePosition;
    glm::ivec2 windowPosition;

    WindowSettings() {
        width = 1920;
        height = 1080;
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
    }
};

class SettingsFile;

//! Use AppSettings (Utils/AppSettings.hpp) to create a window using the user's preferred settings

class Window
{
public:
    virtual ~Window() {}
    //! Outputs e.g. "SDL_GetError"
    virtual void errorCheck() {}

    /// Returns whether this window uses
    virtual bool isDebugContext()=0;

    //! Initialize/close the window
    virtual void initialize(const WindowSettings& windowSettings, RenderSystem renderSystem)=0;
    virtual void destroySurface()=0;
    virtual void close()=0;

    //! Change the window attributes
    virtual void toggleFullscreen(bool nativeFullscreen = true)=0;
    virtual void setWindowSize(int width, int height)=0;
    virtual void setWindowPosition(int x, int y)=0;
    virtual void serializeSettings(SettingsFile &settings)=0;
    virtual WindowSettings deserializeSettings(const SettingsFile &settings)=0;

    //! Update the window
    virtual void update()=0;
    //! Returns false if the game should quit
    virtual bool processEvents(std::function<void(const SDL_Event&)> eventHandler)=0;
    virtual void clear(const Color &color = Color(0, 0, 0))=0;
    virtual void flip()=0;

    //! Utility functions and getters for the main window attributes
    virtual void saveScreenshot(const char *filename)=0;
    virtual bool isFullscreen()=0;
    virtual int getWidth()=0;
    virtual int getHeight()=0;
    virtual glm::ivec2 getWindowResolution()=0;
    virtual glm::ivec2 getWindowPosition()=0;
    virtual const WindowSettings& getWindowSettings() const=0;

#ifdef SUPPORT_VULKAN
    virtual VkSurfaceKHR getVkSurface()=0;
#endif
};

}

/*! SRC_GRAPHICS_WINDOW_HPP_ */
#endif
