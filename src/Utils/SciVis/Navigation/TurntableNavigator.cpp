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

#include <algorithm>
#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>
#include <Math/Geometry/MatrixUtil.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Window.hpp>
#include <Graphics/Scene/Camera.hpp>
#include <Graphics/Scene/RenderTarget.hpp>
#include <ImGui/ImGuiWrapper.hpp>

#include "TurntableNavigator.hpp"

namespace sgl {

bool TurntableNavigator::moveCameraMouse(sgl::CameraPtr &camera, float dt) {
    bool reRender = false;

    // Invert the rotation direction if the camera is upside-down.
    if (sgl::Mouse->buttonPressed(turntableMouseButtonIndex)) {
        cameraInitialUpDirection =
                glm::dot(camera->getCameraUp(), camera->getCameraGlobalUp()) > 0.0f ? 1 : -1;
    }

    if (!sgl::Keyboard->getModifier(ImGuiKey_ModCtrl) && !sgl::Keyboard->getModifier(ImGuiKey_ModShift)) {
        // Zoom in/out.
        if (sgl::Mouse->getScrollWheel() > 0.1 || sgl::Mouse->getScrollWheel() < -0.1) {
            // The scrolling distance depends on the distance between camera and look-at position.
            float clippedDt = std::min(dt, 1.0f / 30.0f);
            float lookDist = glm::length(camera->getPosition() - camera->getLookAtLocation());
            float moveAmount = sgl::Mouse->getScrollWheel() * MOVE_SPEED * clippedDt * 80.0f * lookDist;
            glm::vec3 translation =
                    std::min(moveAmount, lookDist - 1e-3f) * normalize(
                            camera->getLookAtLocation() - camera->getPosition());
            camera->setLookAtViewMatrix(
                    camera->getPosition() + translation,
                    camera->getLookAtLocation(),
                    camera->getCameraUp());
            reRender = true;
        }

        // Mouse rotation.
        if (sgl::Mouse->isButtonDown(turntableMouseButtonIndex) && sgl::Mouse->mouseMoved()) {
            sgl::Point2 mouseDiff = sgl::Mouse->mouseMovement();
            mouseDiff.x *= cameraInitialUpDirection;

            const float pixelsForCompleteRotation =
                    sgl::ImGuiWrapper::get()->getScaleDependentSize(1000.0f / MOUSE_ROT_SPEED * 0.05f);

            float theta = sgl::TWO_PI * float(mouseDiff.x) / pixelsForCompleteRotation;
            float phi   = sgl::TWO_PI * float(mouseDiff.y) / pixelsForCompleteRotation;

            glm::mat4 rotTheta = glm::rotate(glm::mat4(1.0f), -theta, {0.0f, 1.0f, 0.0f});

            glm::vec3 rotPhiAxis;
            glm::vec3 cameraUp = camera->getCameraUp();
            glm::vec3 cameraLookAt = camera->getLookAtLocation();
            glm::vec3 cameraPosition = camera->getPosition();

            rotPhiAxis = glm::cross(cameraUp, cameraLookAt - cameraPosition);
            rotPhiAxis = glm::normalize(rotPhiAxis);

            glm::mat4 rotPhi = glm::rotate(glm::mat4(1.0f), phi, rotPhiAxis);

            cameraPosition = cameraPosition - cameraLookAt;
            cameraPosition = glm::vec3(rotTheta * rotPhi * glm::vec4(cameraPosition, 1.0f));
            cameraUp = glm::vec3(rotTheta * rotPhi * glm::vec4(cameraUp, 1.0f));
            cameraPosition = cameraPosition + cameraLookAt;

            camera->setLookAtViewMatrix(cameraPosition, cameraLookAt, cameraUp);
            reRender = true;
        }
    }

    // Move look-at position.
    if (sgl::Mouse->isButtonDown(turntableMouseButtonIndex) && sgl::Mouse->mouseMoved()
            && sgl::Keyboard->getModifier(ImGuiKey_ModShift)) {
        sgl::Point2 mouseDiff = sgl::Mouse->mouseMovement();
        glm::vec3 lookAt = camera->getLookAtLocation();
        glm::vec3 lookOffset = camera->getPosition() - camera->getLookAtLocation();
        float lookOffsetLength = glm::length(lookOffset);

        auto renderTarget = camera->getRenderTarget();
        float wPixel;
        float hPixel;
        if (renderTarget) {
            wPixel = float(renderTarget->getWidth());
            hPixel = float(renderTarget->getHeight());
        } else {
            sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
            wPixel = float(window->getWidth());
            hPixel = float(window->getHeight());
        }
        float wWorld = 2.0f * lookOffsetLength * std::tan(camera->getFOVx() * 0.5f);
        float hWorld = 2.0f * lookOffsetLength * std::tan(camera->getFOVy() * 0.5f);
        float shiftX = -float(mouseDiff.x) / wPixel * wWorld;
        float shiftY = float(mouseDiff.y) / hPixel * hWorld;
        lookAt = lookAt + camera->getCameraRight() * shiftX + camera->getCameraUp() * shiftY;

        camera->setLookAtViewMatrix(lookOffset + lookAt, lookAt, camera->getCameraUp());
        reRender = true;
    }

    return reRender;
}

}
