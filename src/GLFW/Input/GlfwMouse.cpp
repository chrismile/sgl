/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#include <cmath>
#include <GLFW/glfw3.h>
#include <GLFW/GlfwWindow.hpp>
#include <Utils/AppSettings.hpp>
#include "GlfwMouse.hpp"

#define MAP_SDL_BUTTON(X) (1 << ((X)-1))

namespace sgl {

GlfwMouse::GlfwMouse() {
}

GlfwMouse::~GlfwMouse() {
}

void GlfwMouse::update(float dt) {
    auto* glfwWindow = static_cast<GlfwWindow*>(AppSettings::get()->getMainWindow())->getGlfwWindow();
    oldState = state;
    glfwGetCursorPos(glfwWindow, &state.posX, &state.posY);
    state.buttonState = 0;
    for (int i = GLFW_MOUSE_BUTTON_1; i <= GLFW_MOUSE_BUTTON_LAST; i++) {
        int val = glfwGetMouseButton(glfwWindow, i);
        state.buttonState |= (val << i);
    }
    state.scrollWheel = float(scrollValueCallback);
    scrollValueCallback = 0.0;

    /*if (oldState.posX != x || oldState.posY != y) {
        EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
    }*/
}

void GlfwMouse::onCursorPos(double xpos, double ypos) {
    ;
}

void GlfwMouse::onCursorEnter(int entered) {
    ;
}

void GlfwMouse::onMouseButton(int button, int action, int mods) {
    ;
}

void GlfwMouse::onScroll(double xoffset, double yoffset) {
    scrollValueCallback = yoffset;
}


// Mouse position
std::pair<float, float> GlfwMouse::getAxisFractional() {
    return { state.posX, state.posY };
}

float GlfwMouse::getXFractional() {
    return state.posX;
}

float GlfwMouse::getYFractional() {
    return state.posY;
}

std::pair<float, float> GlfwMouse::mouseMovementFractional() {
    return { state.posX - oldState.posX, state.posY - oldState.posY };
}

Point2 GlfwMouse::getAxis() {
    return { getX(), getY() };
}

int GlfwMouse::getX() {
    return int(std::round(getXFractional()));
}

int GlfwMouse::getY() {
    return int(std::round(getYFractional()));
}

Point2 GlfwMouse::mouseMovement() {
    return { int(state.posX - oldState.posX), int(state.posY - oldState.posY) };
}

bool GlfwMouse::mouseMoved() {
    return state.posX - oldState.posX != 0 || state.posY - oldState.posY != 0;
}

void GlfwMouse::warp(const Point2 &windowPosition) {
    auto* mainWindow = static_cast<GlfwWindow*>(AppSettings::get()->getMainWindow());
    glfwSetCursorPos(mainWindow->getGlfwWindow(), double(windowPosition.x), double(windowPosition.y));
    state.posX = double(windowPosition.x);
    state.posY = double(windowPosition.y);
    //EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
}

void GlfwMouse::warpFractional(const std::pair<float, float>& windowPosition) {
    auto* mainWindow = static_cast<GlfwWindow*>(AppSettings::get()->getMainWindow());
    glfwSetCursorPos(mainWindow->getGlfwWindow(), double(windowPosition.first), double(windowPosition.second));
    state.posX = double(windowPosition.first);
    state.posY = double(windowPosition.second);
    //EventManager::get()->queueEvent(EventDataPtr(new EventData(MOUSE_MOVED_EVENT)));
}


// Mouse buttons
bool GlfwMouse::isButtonDown(int button) {
    return state.buttonState & MAP_SDL_BUTTON(button);
}

bool GlfwMouse::isButtonUp(int button) {
    return !(state.buttonState & MAP_SDL_BUTTON(button));
}

bool GlfwMouse::buttonPressed(int button) {
    return (state.buttonState & MAP_SDL_BUTTON(button)) && !(oldState.buttonState & MAP_SDL_BUTTON(button));
}

bool GlfwMouse::buttonReleased(int button) {
    return !(state.buttonState & MAP_SDL_BUTTON(button)) && (oldState.buttonState & MAP_SDL_BUTTON(button));
}

// -1: Scroll down; 0: No scrolling; 1: Scroll up
float GlfwMouse::getScrollWheel() {
    return float(state.scrollWheel);
}

void GlfwMouse::setScrollWheelValue(int val) {
    oldState.scrollWheel = state.scrollWheel;
    state.scrollWheel = float(val);
}

}
