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

#include "SDLGamepad.hpp"
#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <SDL2/SDL.h>

namespace sgl {

struct DLL_OBJECT OldBallDelta
{
    int dx;
    int dy;
};

class DLL_OBJECT OldGamepadState
{
public:
    OldGamepadState(SDL_Joystick *joystick);
    ~OldGamepadState();
    void updateOldState();
    bool getButtonDown(int button);
    Uint8 getDirpadState(int iDirpadIndex);

private:
    SDL_Joystick *joy;
    Uint8  *buttons;          // Button states
    Sint16 *axes;             // Axis states
    Uint8  *hats;             // Hat states
    OldBallDelta *ballDeltas; // Ball motion deltas
};
// Initialisiert die Werte von OldJoystick
OldGamepadState::OldGamepadState(SDL_Joystick *_joy)
{
    joy  = _joy;
    if (joy != NULL) {
        buttons    = new Uint8 [SDL_JoystickNumButtons(joy)];
        axes       = new Sint16[SDL_JoystickNumAxes(joy)];
        hats       = new Uint8 [SDL_JoystickNumHats(joy)];
        ballDeltas = new OldBallDelta[SDL_JoystickNumBalls(joy)];
    }
}

OldGamepadState::~OldGamepadState()
{
    if (joy != NULL) {
        delete[] buttons;
        delete[] axes;
        delete[] hats;
        delete[] ballDeltas;
    }
}

// Setzt altes State auf neues
void OldGamepadState::updateOldState()
{
    if (joy != NULL) {
        for(int i = 0; i < SDL_JoystickNumButtons(joy); i++) {
            buttons[i] = SDL_JoystickGetButton(joy, i);
        }
        for(int i = 0; i < SDL_JoystickNumHats(joy); i++) {
            hats[i] = SDL_JoystickGetHat(joy, i);
        }
    }
}

bool OldGamepadState::getButtonDown(int button)
{
    if (joy == NULL)
        return false;
    return (buttons[button] ? true : false);
}


Uint8 OldGamepadState::getDirpadState(int iDirpadIndex)
{
    if (joy == NULL)
        return 0;
    return hats[iDirpadIndex];
}


SDLGamepad::SDLGamepad()
{
    initialize();
}

SDLGamepad::~SDLGamepad()
{
    release();
}

void SDLGamepad::initialize()
{
    numGamepads = SDL_NumJoysticks();
    for (int j = 0; j < numGamepads; j++) {
        gamepads.push_back(static_cast<SDL_Joystick*>(0));
        oldGamepads.push_back(static_cast<OldGamepadState*>(0));
        hapticList.push_back(static_cast<SDL_Haptic*>(0));
        gamepads.at(j) = SDL_JoystickOpen(j);
        oldGamepads.at(j) = new OldGamepadState(gamepads.at(j));

        // Does the gamepad support force feedback?
        if (SDL_JoystickIsHaptic(gamepads.at(j))) {
            hapticList.at(j) = SDL_HapticOpenFromJoystick(gamepads.at(j));
        }

        // Does initializing force feedback work for this gamepad?
        if (SDL_HapticRumbleInit(hapticList.at(j)) != 0) {
            Logfile::get()->write(std::string() + "WARNING: SDLGamepad::initialize: SDL_HapticRumbleInit(hapticList.at("
                    + toString(j) + ")) != 0", ORANGE);
            rumbleInited.push_back(false);
        } else {
            rumbleInited.push_back(true);
        }

        Logfile::get()->write(std::string() + "INFO: SDLGamepad::initialize: Adress of Joystick #"
                + toString(j+1) + ": " + toString(gamepads.at(j)), BLUE);
    }
}

void SDLGamepad::release()
{
    for (size_t j = 0; j < gamepads.size(); j++) {
        if (hapticList.at(j) != NULL) {
            SDL_HapticClose(hapticList.at(j));
        }
        SDL_JoystickClose(gamepads.at(j));
        delete oldGamepads.at(j);
    }
    gamepads.clear();
    hapticList.clear();
    oldGamepads.clear();
}

void SDLGamepad::update(float dt)
{
    for (size_t j = 0; j < gamepads.size(); j++) {
        if (gamepads.at(j) == NULL) break;
        oldGamepads.at(j)->updateOldState();
    }
}

// Re-open all gamepads
void SDLGamepad::refresh()
{
    release();

    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    initialize();
}


int SDLGamepad::getNumGamepads()
{
    return SDL_NumJoysticks();
}

const char *SDLGamepad::getGamepadName(int j)
{
    return SDL_JoystickNameForIndex(j);
}


// Gamepad buttons
bool SDLGamepad::isButtonDown(int button, int gamepadIndex /* = 0 */)
{
    if ((size_t)gamepadIndex < gamepads.size() && button < SDL_JoystickNumButtons(gamepads.at(gamepadIndex)))
        return SDL_JoystickGetButton(gamepads.at(gamepadIndex), button);
    return false;
}

bool SDLGamepad::isButtonUp(int button, int gamepadIndex /* = 0 */)
{
    return !isButtonDown(button, gamepadIndex);
}

bool SDLGamepad::buttonPressed(int button, int gamepadIndex /* = 0 */)
{
    if ((size_t)gamepadIndex < gamepads.size() && button < SDL_JoystickNumButtons(gamepads.at(gamepadIndex)))
        return isButtonDown(button, gamepadIndex) && !oldGamepads.at(gamepadIndex)->getButtonDown(button);
    return false;
}

bool SDLGamepad::buttonReleased(int button, int gamepadIndex /* = 0 */)
{
    if ((size_t)gamepadIndex < gamepads.size() && button < SDL_JoystickNumButtons(gamepads.at(gamepadIndex)))
        return !isButtonDown(button, gamepadIndex) && oldGamepads.at(gamepadIndex)->getButtonDown(button);
    return false;
}

int SDLGamepad::getNumButtons(int gamepadIndex /* = 0 */)
{
    return SDL_JoystickNumButtons(gamepads.at(gamepadIndex));
}


// Gamepad control stick axes

// Remap the axis of an analog gamepad stick.
// Example for function call: "axis = remap(axis, 0.05f, 0.95f);"
// -1.0f <= in <= 1.0f; 0.0f <= min <= max <= 1.0f
float remapAnalogStickAxis(float in, float min, float max) {
    if (in < min && in > -min) { // Near 0.0f
        return 0.0f;
    } else if (in > max) { // Near 1.0f
        return max;
    } else if (in < -max) { // Near -1.0f
        return -max;
    }
    // Between 0.0f and 1.0f
    return (in - min) / (max - min); // Remap from min == 0.0f to max == +/-1.0f
}

float SDLGamepad::axisX(int stickIndex /* = 0 */, int gamepadIndex /* = 0 */)
{
    Sint16 axisX = SDL_JoystickGetAxis(gamepads.at(gamepadIndex), stickIndex*2);
    return remapAnalogStickAxis(axisX/32768.0f, 0.05f, 0.95f);
}

float SDLGamepad::axisY(int stickIndex /* = 0 */, int gamepadIndex /* = 0 */)
{
    Sint16 axisY = SDL_JoystickGetAxis(gamepads.at(gamepadIndex), stickIndex*2+1);
    return remapAnalogStickAxis(axisY/32768.0f, 0.05f, 0.95f);
}

glm::vec2 SDLGamepad::axis(int stickIndex /* = 0 */, int gamepadIndex /* = 0 */)
{
    return glm::vec2(axisX(stickIndex, gamepadIndex), axisY(stickIndex, gamepadIndex));
}

Uint8 SDLGamepad::getDirectionPad(int dirPadIndex /* = 0 */, int gamepadIndex /* = 0 */)
{
    SDL_Joystick *gamepad = gamepads.at(gamepadIndex);
    if (SDL_JoystickNumHats(gamepad) <= dirPadIndex) {
        return 0;
    }
    return SDL_JoystickGetHat(gamepad, dirPadIndex);
}

Uint8 SDLGamepad::getDirectionPadPressed(int dirPadIndex /* = 0 */, int gamepadIndex /* = 0 */)
{
    SDL_Joystick *gamepad = gamepads.at(gamepadIndex);
    if (SDL_JoystickNumHats(gamepad) <= dirPadIndex) {
        return 0;
    }

    // Use bitwise operators to add only new states to "returnValue"
    Uint8 returnValue = 0, hatCurrentState = SDL_JoystickGetHat(gamepad, dirPadIndex),
            hatOldState = oldGamepads.at(gamepadIndex)->getDirpadState(dirPadIndex);
    if ((hatCurrentState & SDL_HAT_UP) && !(hatOldState & SDL_HAT_UP)) returnValue |= SDL_HAT_UP;
    if ((hatCurrentState & SDL_HAT_RIGHT) && !(hatOldState & SDL_HAT_RIGHT)) returnValue |= SDL_HAT_RIGHT;
    if ((hatCurrentState & SDL_HAT_DOWN) && !(hatOldState & SDL_HAT_DOWN)) returnValue |= SDL_HAT_DOWN;
    if ((hatCurrentState & SDL_HAT_LEFT) && !(hatOldState & SDL_HAT_LEFT)) returnValue |= SDL_HAT_LEFT;

    return returnValue;
}


// Force Feedback support:
void SDLGamepad::rumble(float strength, float time, int gamepadIndex /* = 0 */) // time in seconds
{
    if (rumbleInited.at(gamepadIndex)) {
        SDL_HapticRumblePlay(hapticList.at(gamepadIndex), strength, uint32_t(time*1000));
    }
}

}
