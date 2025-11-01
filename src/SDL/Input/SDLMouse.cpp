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

#ifdef SUPPORT_SDL3
#define SDL_ENABLE_OLD_NAMES
#endif
#include "SDLMouse.hpp"
#include <Math/Geometry/Point2.hpp>
#include <Utils/AppSettings.hpp>
#include <SDL/SDLWindow.hpp>
#include <Utils/Events/EventManager.hpp>
#ifdef SUPPORT_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

namespace sgl {

SDLMouse::SDLMouse() {
}

SDLMouse::~SDLMouse() {
}

void SDLMouse::update(float dt) {
    oldState = state;
    state.buttonState = SDL_GetMouseState(&state.pos.x, &state.pos.y);

    /*if (oldState.pos.x != x || oldState.pos.y != y) {
        EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
    }*/
}


// Mouse position
Point2 SDLMouse::getAxis() {
#ifdef SUPPORT_SDL3
    return Point2(int(state.pos.x), int(state.pos.y));
#else
    return state.pos;
#endif
}

int SDLMouse::getX() {
    return int(state.pos.x);
}

int SDLMouse::getY() {
    return int(state.pos.y);
}

Point2 SDLMouse::mouseMovement() {
    return Point2(int(state.pos.x - oldState.pos.x), int(state.pos.y - oldState.pos.y));
}

#ifdef SUPPORT_SDL3
std::pair<float, float> SDLMouse::getAxisFractional() {
    return { state.pos.x, state.pos.y };
}

float SDLMouse::getXFractional() {
    return state.pos.x;
}

float SDLMouse::getYFractional() {
    return state.pos.y;
}

std::pair<float, float> SDLMouse::mouseMovementFractional() {
    return { state.pos.x - oldState.pos.x, state.pos.y - oldState.pos.y };
}
#endif

bool SDLMouse::mouseMoved() {
    return state.pos.x - oldState.pos.x != 0 || state.pos.y - oldState.pos.y != 0;
}

void SDLMouse::warp(const Point2& windowPosition) {
    auto* mainWindow = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
    SDL_WarpMouseInWindow(mainWindow->getSDLWindow(), SCROLL_TYPE(windowPosition.x), SCROLL_TYPE(windowPosition.y));
    state.pos.x = SCROLL_TYPE(windowPosition.x);
    state.pos.y = SCROLL_TYPE(windowPosition.y);
    //EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
}

#ifdef SUPPORT_SDL3
void SDLMouse::warpFractional(const std::pair<float, float>& windowPosition) {
    auto* mainWindow = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
    SDL_WarpMouseInWindow(mainWindow->getSDLWindow(), windowPosition.first, windowPosition.second);
    state.pos.x = SCROLL_TYPE(windowPosition.first);
    state.pos.y = SCROLL_TYPE(windowPosition.second);
    //EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
}
#endif


// Mouse buttons
bool SDLMouse::isButtonDown(int button) {
    return state.buttonState & SDL_BUTTON(button);
}

bool SDLMouse::isButtonUp(int button) {
    return !(state.buttonState & SDL_BUTTON(button));
}

bool SDLMouse::buttonPressed(int button) {
    return (state.buttonState & SDL_BUTTON(button)) && !(oldState.buttonState & SDL_BUTTON(button));
}

bool SDLMouse::buttonReleased(int button) {
    return !(state.buttonState & SDL_BUTTON(button)) && (oldState.buttonState & SDL_BUTTON(button));
}

// -1: Scroll down; 0: No scrolling; 1: Scroll up
float SDLMouse::getScrollWheel() {
    return float(state.scrollWheel);
}

void SDLMouse::setScrollWheelValue(SCROLL_TYPE val) {
    oldState.scrollWheel = state.scrollWheel;
    state.scrollWheel = val;
}

}
