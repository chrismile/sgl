/*
 * AppLogic.cpp
 *
 *  Created on: 02.09.2015
 *      Author: Christoph Neuhauser
 */

#include "AppLogic.hpp"
#include <Utils/File/Logfile.hpp>
#include <Utils/Timer.hpp>
#include <Graphics/Window.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Input/Mouse.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Gamepad.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Graphics/Renderer.hpp>
#include <string>
#include <SDL2/SDL.h>

namespace sgl {

AppLogic::AppLogic()
{
	Timer->setFixedFPS(60, 4);
	running = true;
	screenshot = false;
	fpsCounterEnabled = true;
	fps = 0.0f;
}

AppLogic::~AppLogic()
{
}

void AppLogic::run()
{
	Window *window = AppSettings::get()->getMainWindow();
	float timeToProcess = 0.0f;
	float lastFrame = Timer->getTicks()/1000.0f - 20.0f;
	while (running == true) {
		window->update();
		timeToProcess += Timer->getElapsedRealTime();

		if (timeToProcess >= Timer->getElapsed()) {
			do {
				running = window->processEvents();

				float dt = Timer->getElapsed();
				Mouse->update(dt);
				Keyboard->update(dt);
				Gamepad->update(dt);
				update(dt);
				timeToProcess -= Timer->getElapsed();
			} while(timeToProcess >= Timer->getElapsed() && running);

			window->clear(Color(0, 0, 0));
			render();

			if (fpsCounterEnabled) {
				// TODO: Here the engine could render an FPS counter
				static float fpsTimer = 0.0f;
				if (fabs(fpsTimer - Timer->getTimeInS()) > 1.0f) {
					fpsTimer = Timer->getTimeInS();
					std::cout << fps << std::endl;
				}

			}

			// Check for errors
			Renderer->errorCheck();
			window->errorCheck();

			// Save a screenshot before flipping the backbuffer surfaces if necessary
			if (screenshot) {
				std::string filename = FileUtils::get()->getConfigDirectory() + "Screenshot";
				bool nonExistent = false;
				for (int i = 1; i < 999; ++i) {
					if (!FileUtils::get()->exists(filename + toString(i) + ".png")) {
						filename += toString(i);
						filename += ".png";
						nonExistent = true;
						break;
					}
				}
				if (!nonExistent) filename += "999.png";
				window->saveScreenshot(filename.c_str());
				screenshot = false;
			}
			window->flip();
		}

		if (((Timer->getTicks()/1000.0f) - lastFrame) >= 0.5f) {
			fps = 1.0f/Timer->getElapsed();
			lastFrame = Timer->getTicks()/1000.0f;
		}
	}
	Logfile::get()->write("INFO: End of main loop.", BLUE);
}

void AppLogic::update(float dt)
{
	EventManager::get()->update();
	if (Keyboard->keyPressed(SDLK_PRINTSCREEN)
			|| ((Keyboard->getModifier()&KMOD_CTRL) && Keyboard->keyPressed(SDLK_p))) {
		screenshot = true;
	}
	if (Keyboard->keyPressed(SDLK_RETURN) && (Keyboard->getModifier()&KMOD_ALT)) {
		Logfile::get()->writeInfo("Switching to fullscreen (ALT-TAB)");
		AppSettings::get()->getMainWindow()->toggleFullscreen();
	}
}

void AppLogic::setFPSCounterEnabled(bool enabled)
{
	fpsCounterEnabled = enabled;
	// TODO
}


}
