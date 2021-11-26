/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#ifndef SGL_TURNTABLENAVIGATOR_HPP
#define SGL_TURNTABLENAVIGATOR_HPP

#include "CameraNavigator.hpp"

namespace sgl {

/**
 * A camera navigator similar to what is by default used in Blender.
 * For more details refer to: https://docs.blender.org/manual/en/latest/editors/preferences/navigation.html
 * By default, the left mouse button is used for controlling the camera navigator.
 */
class TurntableNavigator : public CameraNavigator {
public:
    TurntableNavigator(float& MOVE_SPEED, float& MOUSE_ROT_SPEED, int& turntableMouseButtonIndex)
            : CameraNavigator(MOVE_SPEED, MOUSE_ROT_SPEED), turntableMouseButtonIndex(turntableMouseButtonIndex) {}

    /**
     * Navigates the camera using the mouse.
     * @param camera The camera to navigate.
     * @param dt The elapsed time since the last update (in seconds).
     * @return Whether the camera was moved/rotated.
     */
    bool moveCameraMouse(sgl::CameraPtr &camera, float dt) override;

protected:
    int& turntableMouseButtonIndex;
    int cameraInitialUpDirection = 1;
};

}

#endif //SGL_TURNTABLENAVIGATOR_HPP
