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
#include <Utils/AppSettings.hpp>
#include <Graphics/Window.hpp>
#include <Graphics/Scene/Camera.hpp>
#include <Graphics/Scene/RenderTarget.hpp>

#include "CameraNavigator2D.hpp"

namespace sgl {

bool CameraNavigator2D::moveCameraMouse(sgl::CameraPtr &camera, float dt) {
    bool reRender = false;

    if (sgl::Mouse->isButtonDown(1) && sgl::Mouse->mouseMoved()) {
        sgl::Point2 pixelMovement = sgl::Mouse->mouseMovement();

        auto renderTarget = camera->getRenderTarget();
        float hPixel;
        if (renderTarget) {
            hPixel = float(renderTarget->getHeight());
        } else {
            sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
            hPixel = float(window->getHeight());
        }
        float hWorld = 2.0f * camera->getPosition().z * std::tan(camera->getFOVy() * 0.5f);

        glm::vec2 translationVector = hWorld / hPixel * glm::vec2(pixelMovement.x, pixelMovement.y);
        camera->translate(glm::vec3(-translationVector.x, translationVector.y, 0.0f));

        reRender = true;
        //if (lineRenderer != nullptr) {
        //    lineRenderer->onHasMoved();
        //}
    }
    if (sgl::Mouse->getScrollWheel() > 0.1 || sgl::Mouse->getScrollWheel() < -0.1) {
        float moveAmount = sgl::Mouse->getScrollWheel() * dt * 4.0f;
        camera->translate(glm::vec3(0.0f, 0.0f, -moveAmount));
        camera->setPosition(glm::vec3(
                camera->getPosition().x, camera->getPosition().y, sgl::max(camera->getPosition().z, 0.002f)));

        reRender = true;
        //if (lineRenderer != nullptr) {
        //    lineRenderer->onHasMoved();
        //}
    }

    return reRender;
}

}
