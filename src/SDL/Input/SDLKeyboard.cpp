/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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
