/*
 * SDLKeyboard.cpp
 *
 *  Created on: 13.02.2015
 *      Author: Christoph Neuhauser
 */

#include "SDLKeyboard.hpp"
#include <SDL2/SDL.h>

namespace sgl {

SDLKeyboard::SDLKeyboard()
{
	// Get the number of keys
	SDL_GetKeyboardState(&numKeys);

	// Allocate and initialize the memory for the keystates
	oldKeystate = new Uint8[numKeys];
	keystate = new Uint8[numKeys];
	memset((void*)oldKeystate, 0, numKeys);
	memset((void*)keystate, 0, numKeys);
	modifier = KMOD_NONE;
}

SDLKeyboard::~SDLKeyboard()
{
	delete[] oldKeystate;
	delete[] keystate;
}

void SDLKeyboard::update(float dt)
{
	// Copy the the keystates to the oldKeystate array
	memcpy((void*)oldKeystate, keystate, numKeys);

	// Get the new keystates from SDL
	const Uint8 *sdlKeystate = SDL_GetKeyboardState(&numKeys);
	memcpy((void*)keystate, sdlKeystate, numKeys);
	modifier = SDL_GetModState();
}


// Keyboard keys
bool SDLKeyboard::isKeyDown(int button)
{
	return keystate[SDL_GetScancodeFromKey(button)];
}

bool SDLKeyboard::isKeyUp(int button)
{
	return !keystate[SDL_GetScancodeFromKey(button)];
}

bool SDLKeyboard::keyPressed(int button)
{
	return keystate[SDL_GetScancodeFromKey(button)] && !oldKeystate[SDL_GetScancodeFromKey(button)];
}

bool SDLKeyboard::keyReleased(int button)
{
	return !keystate[SDL_GetScancodeFromKey(button)] && oldKeystate[SDL_GetScancodeFromKey(button)];
}

bool SDLKeyboard::isScancodeDown(int button)
{
	return keystate[button];
}

bool SDLKeyboard::isScancodeUp(int button)
{
	return !keystate[button];
}

bool SDLKeyboard::scancodePressed(int button)
{
	return keystate[button] && !oldKeystate[button];
}

bool SDLKeyboard::scancodeReleased(int button)
{
	return !keystate[button] && oldKeystate[button];
}

int SDLKeyboard::getNumKeys()
{
	return numKeys;
}

SDL_Keymod SDLKeyboard::getModifier()
{
	return modifier;
}


// To support non-standard input methods a key buffer is needed.
// It contains the chars that were typed this frame as UTF-8 chars.
const char *SDLKeyboard::getKeyBuffer() const
{
	return utf8KeyBuffer.c_str();
}

void SDLKeyboard::clearKeyBuffer()
{
	utf8KeyBuffer = "";
}

void SDLKeyboard::addToKeyBuffer(const char *str)
{
	utf8KeyBuffer += str;
}

}
