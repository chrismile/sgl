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

#include <GLFW/glfw3.h>
#include <GLFW/GlfwWindow.hpp>
#include <Utils/AppSettings.hpp>
#include "GlfwGamepad.hpp"

namespace sgl {

struct GlfwGamepadState {
    bool isGamepad = false; // Pure joystick or with additional gamepad functionality?

    GLFWgamepadstate state{};
    std::vector<uint8_t> buttons;
    std::vector<uint8_t> hats;
    std::vector<float> axes;

    GLFWgamepadstate stateOld{};
    std::vector<uint8_t> buttonsOld;
    std::vector<uint8_t> hatsOld;
    std::vector<float> axesOld;
};

GlfwGamepad::GlfwGamepad() {
    initialize();
}

GlfwGamepad::~GlfwGamepad() {
    release();
}

void GlfwGamepad::initialize() {
    gamepads.clear();
    gamepads.resize(GLFW_JOYSTICK_LAST);
    for (int j = GLFW_JOYSTICK_1; j < GLFW_JOYSTICK_LAST; j++) {
        if (glfwJoystickPresent(j)) {
            gamepads.at(j) = std::make_shared<GlfwGamepadState>();
        }
    }
}

void GlfwGamepad::release() {
    gamepads.clear();
}

void GlfwGamepad::update(float dt) {
    for (int j = GLFW_JOYSTICK_1; j < GLFW_JOYSTICK_LAST; j++) {
        if (glfwJoystickPresent(j)) {
            if (!gamepads.at(j)) {
                gamepads.at(j) = std::make_shared<GlfwGamepadState>();
                if (glfwJoystickIsGamepad(j)) {
                    gamepads.at(j)->isGamepad = true;
                }
                //const char* name = glfwGetJoystickName(j);
                //const char* name = glfwGetGamepadName(j);
            } else {
                gamepads.at(j)->stateOld = gamepads.at(j)->state;
                gamepads.at(j)->buttonsOld = gamepads.at(j)->buttons;
                gamepads.at(j)->hatsOld = gamepads.at(j)->hats;
                gamepads.at(j)->axesOld = gamepads.at(j)->axes;
            }
            glfwGetGamepadState(j, &gamepads.at(j)->state);
            int count = 0;
            const uint8_t* buttons = glfwGetJoystickButtons(j, &count);
            gamepads.at(j)->hats = std::vector<uint8_t>(buttons, buttons + count);
            const uint8_t* hats = glfwGetJoystickHats(j, &count);
            gamepads.at(j)->hats = std::vector<uint8_t>(hats, hats + count);
            const float* axes = glfwGetJoystickAxes(j, &count);
            gamepads.at(j)->axes = std::vector<float>(axes, axes + count);
        } else {
            gamepads.at(j) = {};
        }
    }
}

void GlfwGamepad::onJoystick(int jid, int event) {
    if (event == GLFW_CONNECTED) {
        // Already handled by update function.
    } else if (event == GLFW_DISCONNECTED) {
        gamepads.at(jid) = {};
    }
}

int GlfwGamepad::getNumGamepads() {
    // TODO: Change API to support index mapping.
    return gamepads.at(0) ? 1 : 0;
}

const char *GlfwGamepad::getGamepadName(int j) {
    if (gamepads.at(j)->isGamepad) {
        return glfwGetGamepadName(j);
    } else {
        return glfwGetJoystickName(j);
    }
}


// Gamepad buttons
bool GlfwGamepad::isButtonDown(int button, int gamepadIndex /* = 0 */) {
    if ((size_t)gamepadIndex < gamepads.size() && gamepads.at(gamepadIndex) && button < int(gamepads.at(gamepadIndex)->buttons.size()))
        return gamepads.at(gamepadIndex)->buttons[button];
    return false;
}

bool GlfwGamepad::isButtonUp(int button, int gamepadIndex /* = 0 */) {
    return !isButtonDown(button, gamepadIndex);
}

bool GlfwGamepad::buttonPressed(int button, int gamepadIndex /* = 0 */) {
    if ((size_t)gamepadIndex < gamepads.size() && gamepads.at(gamepadIndex) && button < int(gamepads.at(gamepadIndex)->buttons.size()))
        return isButtonDown(button, gamepadIndex) && !gamepads.at(gamepadIndex)->buttonsOld[button];
    return false;
}

bool GlfwGamepad::buttonReleased(int button, int gamepadIndex /* = 0 */) {
    if ((size_t)gamepadIndex < gamepads.size() && gamepads.at(gamepadIndex) && button < int(gamepads.at(gamepadIndex)->buttons.size()))
        return !isButtonDown(button, gamepadIndex) && gamepads.at(gamepadIndex)->buttonsOld[button];
    return false;
}

int GlfwGamepad::getNumButtons(int gamepadIndex /* = 0 */) {
    return int(gamepads.at(gamepadIndex)->buttons.size());
}


// Gamepad control stick axes

float GlfwGamepad::axisX(int stickIndex /* = 0 */, int gamepadIndex /* = 0 */) {
    return gamepads.at(gamepadIndex)->axes.at(stickIndex * 2);
}

float GlfwGamepad::axisY(int stickIndex /* = 0 */, int gamepadIndex /* = 0 */) {
    return gamepads.at(gamepadIndex)->axes.at(stickIndex * 2 + 1);
}

glm::vec2 GlfwGamepad::axis(int stickIndex /* = 0 */, int gamepadIndex /* = 0 */) {
    return { axisX(stickIndex, gamepadIndex), axisY(stickIndex, gamepadIndex) };
}

uint8_t GlfwGamepad::getDirectionPad(int dirPadIndex /* = 0 */, int gamepadIndex /* = 0 */) {
    const auto& gamepad = gamepads.at(gamepadIndex);
    if (int(gamepad->hats.size()) <= dirPadIndex) {
        return 0;
    }
    return gamepad->hats.at(dirPadIndex);
}

uint8_t GlfwGamepad::getDirectionPadPressed(int dirPadIndex /* = 0 */, int gamepadIndex /* = 0 */) {
    const auto& gamepad = gamepads.at(gamepadIndex);
    if (int(gamepad->hats.size()) <= dirPadIndex) {
        return 0;
    }

    // Use bitwise operators to add only new states to "returnValue".
    uint8_t hatCurrentState = gamepad->hats.at(dirPadIndex);
    uint8_t hatOldState = gamepad->hatsOld.at(dirPadIndex);
    uint8_t returnValue = 0;
    if ((hatCurrentState & GLFW_HAT_UP) && !(hatOldState & GLFW_HAT_UP)) returnValue |= GLFW_HAT_UP;
    if ((hatCurrentState & GLFW_HAT_RIGHT) && !(hatOldState & GLFW_HAT_RIGHT)) returnValue |= GLFW_HAT_RIGHT;
    if ((hatCurrentState & GLFW_HAT_DOWN) && !(hatOldState & GLFW_HAT_DOWN)) returnValue |= GLFW_HAT_DOWN;
    if ((hatCurrentState & GLFW_HAT_LEFT) && !(hatOldState & GLFW_HAT_LEFT)) returnValue |= GLFW_HAT_LEFT;

    return returnValue;
}

}
