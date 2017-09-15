/*!
 * Window.hpp
 *
 *  Created on: 27.08.2017
 *      Author: christoph
 */

#ifndef SRC_GRAPHICS_WINDOW_HPP_
#define SRC_GRAPHICS_WINDOW_HPP_

#include <Defs.hpp>
#include <Graphics/Color.hpp>

namespace sgl {

const uint32_t RESOLUTION_CHANGED_EVENT = 74561634U;

struct WindowSettings {
	int width;
	int height;
	bool fullscreen;
	bool resizable;
	int multisamples;
	int depthSize;
	bool vSync;
	bool debugContext;

	WindowSettings() {
		width = 1024;
		height = 768;
		fullscreen = false;
		resizable = true;
		multisamples = 16;
		depthSize = 16;
		vSync = true;
#ifdef _DEBUG
		debugContext = true;
#else
		debugContext = false;
#endif
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

	//! Initialize/close the window
	virtual void initialize(const WindowSettings&)=0;
	virtual void close()=0;

	//! Change the window attributes
	virtual void toggleFullscreen(bool nativeFullscreen = true)=0;
	virtual void serializeSettings(SettingsFile &settings)=0;
	virtual WindowSettings deserializeSettings(const SettingsFile &settings)=0;

	//! Update the window
	virtual void update()=0;
	//! Returns false if the game should quit
	virtual bool processEvents()=0;
	virtual void clear(const Color &color = Color(0, 0, 0))=0;
	virtual void flip()=0;

	//! Utility functions and getters for the main window attributes
	virtual void saveScreenshot(const char *filename)=0;
	virtual bool isFullscreen()=0;
	virtual int getWidth()=0;
	virtual int getHeight()=0;
};

}



/*! SRC_GRAPHICS_WINDOW_HPP_ */
#endif
