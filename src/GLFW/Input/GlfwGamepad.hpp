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

#ifndef SGL_GLFWGAMEPAD_HPP
#define SGL_GLFWGAMEPAD_HPP

#include <vector>
#include <memory>
#include <Input/Gamepad.hpp>
#ifdef USE_GLM
#include <glm/vec2.hpp>
#else
#include <Math/Geometry/fallback/vec2.hpp>
#endif

namespace sgl {

struct GlfwGamepadState;

class DLL_OBJECT GlfwGamepad : public GamepadInterface {
public:
    GlfwGamepad();
    ~GlfwGamepad() override;
    void initialize();
    void release();
    void update(float dt) override;

    // GLFW callbacks.
    void onJoystick(int jid, int event);

    /// Re-open all gamepads
    void refresh() override {}
    int getNumGamepads() override;
    const char *getGamepadName(int j) override;

    /// Gamepad buttons
    bool isButtonDown(int button, int gamepadIndex = 0) override;
    bool isButtonUp(int button, int gamepadIndex = 0) override;
    bool buttonPressed(int button, int gamepadIndex = 0) override;
    bool buttonReleased(int button, int gamepadIndex = 0) override;
    int getNumButtons(int gamepadIndex = 0) override;

    /// Gamepad control stick axes
    float axisX(int stickIndex = 0, int gamepadIndex = 0) override;
    float axisY(int stickIndex = 0, int gamepadIndex = 0) override;
    glm::vec2 axis(int stickIndex = 0, int gamepadIndex = 0) override;
    uint8_t getDirectionPad(int dirPadIndex = 0, int gamepadIndex = 0) override;
    uint8_t getDirectionPadPressed(int dirPadIndex = 0, int gamepadIndex = 0) override;

    /// Force Feedback support:
    /// time in seconds
    void rumble(float strength, float time, int gamepadIndex = 0) override {}

protected:
    /// Array containing the state of the gamepads
    std::vector<std::shared_ptr<GlfwGamepadState>> gamepads;
};

}

#endif //SGL_GLFWGAMEPAD_HPP
