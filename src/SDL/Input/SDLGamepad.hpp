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

#ifndef SDL_SDLGAMEPAD_HPP_
#define SDL_SDLGAMEPAD_HPP_

#include <SDL2/SDL.h>
#include <vector>
#include <Input/Gamepad.hpp>
#include <glm/vec2.hpp>

namespace sgl {

class OldGamepadState;

class DLL_OBJECT SDLGamepad : public GamepadInterface
{
public:
    SDLGamepad();
    virtual ~SDLGamepad();
    void initialize();
    void release();
    virtual void update(float dt);
    //! Re-open all gamepads
    virtual void refresh();
    virtual int getNumGamepads();
    virtual const char *getGamepadName(int j);

    //! Gamepad buttons
    virtual bool isButtonDown(int button, int gamepadIndex = 0);
    virtual bool isButtonUp(int button, int gamepadIndex = 0);
    virtual bool buttonPressed(int button, int gamepadIndex = 0);
    virtual bool buttonReleased(int button, int gamepadIndex = 0);
    virtual int getNumButtons(int gamepadIndex = 0);

    //! Gamepad control stick axes
    virtual float axisX(int stickIndex = 0, int gamepadIndex = 0);
    virtual float axisY(int stickIndex = 0, int gamepadIndex = 0);
    virtual glm::vec2 axis(int stickIndex = 0, int gamepadIndex = 0);
    virtual uint8_t getDirectionPad(int dirPadIndex = 0, int gamepadIndex = 0);
    virtual uint8_t getDirectionPadPressed(int dirPadIndex = 0, int gamepadIndex = 0);

    //! Force Feedback support:
    //! time in seconds
    virtual void rumble(float strength, float time, int gamepadIndex = 0);

protected:
    //! Array containing the state of the gamepads
    std::vector<SDL_Joystick*> gamepads;
    //! Array containing the state of the gamepads in the last frame
    std::vector<OldGamepadState*> oldGamepads;
    int numGamepads;

    std::vector<SDL_Haptic*> hapticList;
    std::vector<bool> rumbleInited;
};

}

/*! SDL_SDLGAMEPAD_HPP_ */
#endif
