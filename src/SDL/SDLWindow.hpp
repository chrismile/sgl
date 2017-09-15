/*
 * SDLWindow.hpp
 *
 *  Created on: 27.08.2017
 *      Author: christoph
 */

#ifndef SRC_SDL_SDLWINDOW_HPP_
#define SRC_SDL_SDLWINDOW_HPP_

#include <Graphics/Window.hpp>
#include <SDL2/SDL.h>

namespace sgl {

class SDLWindow : public Window
{
public:
	SDLWindow();
	virtual void errorCheck(); // Outputs e.g. "SDL_GetError"

	// Initialize/close the window
	virtual void initialize(const WindowSettings &settings);
	virtual void close();

	// Change the window attributes
	virtual void toggleFullscreen(bool nativeFullscreen = true); // Try to keep resolution
	virtual void serializeSettings(SettingsFile &settings);
	virtual WindowSettings deserializeSettings(const SettingsFile &settings);

	// Update the window
	virtual void update();
	virtual bool processEvents(); // Returns false if the game should quit
	virtual void clear(const Color &color = Color(0, 0, 0));
	virtual void flip();

	// Utility functions/getters for the main window attributes
	virtual void saveScreenshot(const char *filename);
	virtual bool isFullscreen() { return windowSettings.fullscreen; }
	virtual int getWidth() { return windowSettings.width; }
	virtual int getHeight() { return windowSettings.height; }

	// Getting SDL specific data
	inline SDL_Window *getSDLWindow() { return sdlWindow; }
	inline SDL_GLContext getGLContext() { return glContext; }

private:
	SDL_Window *sdlWindow;
	SDL_GLContext glContext;
	WindowSettings windowSettings;
	SDL_DisplayMode oldDisplayMode; // For toggle fullscreen: Resolution before going fullscreen
};

}

#endif /* SRC_SDL_SDLWINDOW_HPP_ */
