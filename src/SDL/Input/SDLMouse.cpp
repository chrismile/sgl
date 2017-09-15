/*
 * SDLMouse.cpp
 *
 *  Created on: 13.02.2015
 *      Author: Christoph
 */

#include "SDLMouse.hpp"
#include <Math/Geometry/Point2.hpp>
#include <Utils/AppSettings.hpp>
#include <SDL/SDLWindow.hpp>
#include <Utils/Events/EventManager.hpp>
#include <SDL2/SDL.h>

namespace sgl {

SDLMouse::SDLMouse()
{
}

SDLMouse::~SDLMouse()
{
}

void SDLMouse::update(float dt)
{
	oldState = state;
	state.buttonState = SDL_GetMouseState(&state.pos.x, &state.pos.y);

	/*if (oldState.pos.x != x || oldState.pos.y != y) {
		EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
	}*/
}


// Mouse position
Point2 SDLMouse::getAxis()
{
	return state.pos;
}

int SDLMouse::getX()
{
	return state.pos.x;
}

int SDLMouse::getY()
{
	return state.pos.y;
}

Point2 SDLMouse::mouseMovement()
{
	return Point2(state.pos.x-oldState.pos.x, state.pos.y-oldState.pos.y);
}

bool SDLMouse::mouseMoved()
{
	return state.pos.x-oldState.pos.x != 0 || state.pos.y-oldState.pos.y != 0;
}

void SDLMouse::warp(const Point2 &windowPosition)
{
	SDLWindow *mainWindow = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
	SDL_WarpMouseInWindow(mainWindow->getSDLWindow(), windowPosition.x, windowPosition.y);
	state.pos.x = windowPosition.x;
	state.pos.y = windowPosition.y;
	//EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
}


// Mouse buttons
bool SDLMouse::isButtonDown(int button)
{
	return state.buttonState & SDL_BUTTON(button);
}

bool SDLMouse::isButtonUp(int button)
{
	return !(state.buttonState & SDL_BUTTON(button));
}

bool SDLMouse::buttonPressed(int button)
{
	return (state.buttonState & SDL_BUTTON(button)) && !(oldState.buttonState & SDL_BUTTON(button));
}

bool SDLMouse::buttonReleased(int button)
{
	return !(state.buttonState & SDL_BUTTON(button)) && (oldState.buttonState & SDL_BUTTON(button));
}

// -1: Scroll down; 0: No scrolling; 1: Scroll up
float SDLMouse::getScrollWheel()
{
	return state.scrollWheel;
}

void SDLMouse::setScrollWheelValue(int val)
{
	oldState.scrollWheel = state.scrollWheel;
	state.scrollWheel = val;
}

}
