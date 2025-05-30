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

#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>
#include <Math/Geometry/MatrixUtil.hpp>
#include <Graphics/Scene/Camera.hpp>

#include "FirstPersonNavigator.hpp"

namespace sgl {

bool FirstPersonNavigator::moveCameraKeyboard(sgl::CameraPtr &camera, float dt) {
    bool reRender = false;

    // Rotate scene around camera origin
    if (sgl::Keyboard->isKeyDown(ImGuiKey_Q)) {
        camera->rotateYaw(-1.9f*dt * MOVE_SPEED);
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_E)) {
        camera->rotateYaw(1.9f*dt * MOVE_SPEED);
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_R)) {
        camera->rotatePitch(1.9f*dt * MOVE_SPEED);
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_F)) {
        camera->rotatePitch(-1.9f*dt * MOVE_SPEED);
        reRender = true;
    }

    glm::mat4 rotationMatrix = camera->getRotationMatrix();
    glm::mat4 invRotationMatrix = glm::inverse(rotationMatrix);
    if (sgl::Keyboard->isKeyDown(ImGuiKey_PageDown)) {
        camera->translate(sgl::transformPoint(
                invRotationMatrix, glm::vec3(0.0f, -dt * MOVE_SPEED, 0.0f)));
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_PageUp)) {
        camera->translate(sgl::transformPoint(
                invRotationMatrix, glm::vec3(0.0f, dt * MOVE_SPEED, 0.0f)));
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_DownArrow) || sgl::Keyboard->isKeyDown(ImGuiKey_S)) {
        camera->translate(sgl::transformPoint(
                invRotationMatrix, glm::vec3(0.0f, 0.0f, dt * MOVE_SPEED)));
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_UpArrow) || sgl::Keyboard->isKeyDown(ImGuiKey_W)) {
        camera->translate(sgl::transformPoint(
                invRotationMatrix, glm::vec3(0.0f, 0.0f, -dt * MOVE_SPEED)));
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_LeftArrow) || sgl::Keyboard->isKeyDown(ImGuiKey_A)) {
        camera->translate(sgl::transformPoint(
                invRotationMatrix, glm::vec3(-dt * MOVE_SPEED, 0.0f, 0.0f)));
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(ImGuiKey_RightArrow) || sgl::Keyboard->isKeyDown(ImGuiKey_D)) {
        camera->translate(sgl::transformPoint(
                invRotationMatrix, glm::vec3(dt * MOVE_SPEED, 0.0f, 0.0f)));
        reRender = true;
    }

    return reRender;
}

bool FirstPersonNavigator::moveCameraMouse(sgl::CameraPtr &camera, float dt) {
    bool reRender = false;

    if (!sgl::Keyboard->getModifier(ImGuiKey_ModCtrl) && !sgl::Keyboard->getModifier(ImGuiKey_ModShift)) {
        // Zoom in/out
        if (sgl::Mouse->getScrollWheel() > 0.1 || sgl::Mouse->getScrollWheel() < -0.1) {
            glm::mat4 rotationMatrix = camera->getRotationMatrix();
            glm::mat4 invRotationMatrix = glm::inverse(rotationMatrix);

            float moveAmount = sgl::Mouse->getScrollWheel() * dt * 2.0f;
            camera->translate(sgl::transformPoint(
                    invRotationMatrix, glm::vec3(0.0f, 0.0f, -moveAmount * MOVE_SPEED)));
            reRender = true;
        }

        // Mouse rotation
        if (sgl::Mouse->isButtonDown(1) && sgl::Mouse->mouseMoved()) {
            auto pixelMovement = sgl::Mouse->mouseMovementFractional();
            float yaw = dt * MOUSE_ROT_SPEED * float(pixelMovement.first);
            float pitch = -dt * MOUSE_ROT_SPEED * float(pixelMovement.second);

            //glm::quat rotYaw = glm::quat(glm::vec3(0.0f, yaw, 0.0f));
            //glm::quat rotPitch = glm::quat(
            //        pitch*glm::vec3(rotationMatrix[0][0], rotationMatrix[1][0], rotationMatrix[2][0]));
            camera->rotateYaw(yaw);
            camera->rotatePitch(pitch);
            reRender = true;
        }
    }

    return reRender;
}

}
