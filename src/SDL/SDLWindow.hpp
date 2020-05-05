/*!
 * SDLWindow.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_SDL_SDLWINDOW_HPP_
#define SRC_SDL_SDLWINDOW_HPP_

#include <SDL2/SDL.h>
#include <Graphics/Window.hpp>

namespace sgl {

class SDLWindow : public Window
{
public:
    SDLWindow();
    //! Outputs e.g. "SDL_GetError"
    virtual void errorCheck();

    /// Returns whether this window uses
    virtual bool isDebugContext() { return windowSettings.debugContext; }

    //! Initialize/close the window
    virtual void initialize(const WindowSettings &settings);
    virtual void close();

    //! Change the window attributes
    //! Try to keep resolution
    virtual void toggleFullscreen(bool nativeFullscreen = true);
    virtual void setWindowSize(int width, int height);
    virtual void serializeSettings(SettingsFile &settings);
    virtual WindowSettings deserializeSettings(const SettingsFile &settings);

    //! Update the window
    virtual void update();
    //! Returns false if the game should quit
    virtual bool processEvents(std::function<void(const SDL_Event&)> eventHandler);
    virtual void clear(const Color &color = Color(0, 0, 0));
    virtual void flip();

    //! Utility functions/getters for the main window attributes
    virtual void saveScreenshot(const char *filename);
    virtual bool isFullscreen() { return windowSettings.fullscreen; }
    virtual int getWidth() { return windowSettings.width; }
    virtual int getHeight() { return windowSettings.height; }
    virtual glm::ivec2 getWindowResolution() { return glm::ivec2(windowSettings.width, windowSettings.height); }

    //! Getting SDL specific data
    inline SDL_Window *getSDLWindow() { return sdlWindow; }
    inline SDL_GLContext getGLContext() { return glContext; }

private:
    SDL_Window *sdlWindow;
    SDL_GLContext glContext;
    WindowSettings windowSettings;
    //! For toggle fullscreen: Resolution before going fullscreen
    SDL_DisplayMode oldDisplayMode;
};

}

/*! SRC_SDL_SDLWINDOW_HPP_ */
#endif
