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

#ifndef SGL_GLFWMOUSE_HPP
#define SGL_GLFWMOUSE_HPP

#include <Input/Mouse.hpp>
#include <Math/Geometry/Point2.hpp>

namespace sgl {

struct DLL_OBJECT GlfwMouseState {
    int buttonState{};
    float scrollWheel{};
    double posX{}, posY{};
};

class DLL_OBJECT GlfwMouse : public MouseInterface {
public:
    GlfwMouse();
    ~GlfwMouse() override;
    void update(float dt) override;

    // GLFW callbacks.
    void onCursorPos(double xpos, double ypos);
    void onCursorEnter(int entered);
    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);

    /// Mouse position
    Point2 getAxis() override;
    int getX() override;
    int getY() override;
    Point2 mouseMovement() override;
    std::pair<double, double> getAxisFractional() override;
    double getXFractional() override;
    double getYFractional() override;
    std::pair<double, double> mouseMovementFractional() override;
    bool mouseMoved() override;
    void warp(const Point2 &windowPosition) override;

    /// Mouse buttons
    bool isButtonDown(int button) override;
    bool isButtonUp(int button) override;
    bool buttonPressed(int button) override;
    bool buttonReleased(int button) override;
    /// -1: Scroll down; 0: No scrolling; 1: Scroll up
    float getScrollWheel() override;

    /// Function for event processing (SDL only supports querying scroll wheel state within the event queue)
    void setScrollWheelValue(int val);

protected:
    /// States in the current and last frame
    GlfwMouseState state, oldState;
    double scrollValueCallback = 0.0f;
};

}

#endif //SGL_GLFWMOUSE_HPP
