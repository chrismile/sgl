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

#ifndef SGL_COORDINATEAXESOVERLAYWIDGET_HPP
#define SGL_COORDINATEAXESOVERLAYWIDGET_HPP

#include <memory>

namespace sgl {

class Camera;
typedef std::shared_ptr<Camera> CameraPtr;

class CoordinateAxesOverlayWidget {
public:
    void setClearColor(const sgl::Color& clearColorSgl);
    void renderGui(const sgl::CameraPtr& cam);

private:
    glm::vec3 colorBright[3] = {
            sgl::Color(255, 54, 83).getFloatColorRGB(),
            sgl::Color(139, 220, 0).getFloatColorRGB(),
            sgl::Color(44, 143, 255).getFloatColorRGB()
    };
    glm::vec3 colorDark[3] = {
            sgl::Color(148, 54, 68).getFloatColorRGB(),
            sgl::Color(98, 138, 28).getFloatColorRGB(),
            sgl::Color(48, 100, 156).getFloatColorRGB()
    };
    glm::vec3 colorInner[3] = {
            sgl::Color(110, 61, 68).getFloatColorRGB(),
            sgl::Color(82, 101, 50).getFloatColorRGB(),
            sgl::Color(59, 83, 110).getFloatColorRGB()
    };
    ImU32 textColor[3] = {
            sgl::Color(89, 19, 28).getColorRGBA(),
            sgl::Color(48, 76, 0).getColorRGBA(),
            sgl::Color(16, 50, 89).getColorRGBA()
    };
    glm::vec3 clearColor{};

    const float EPSILON = 1e-6f;

    float radiusOverlay = 0.0f;
    float radiusBalls = 0.0f;
    float radiusInnerRing = 0.0f;
    float radiusBallsInner = 0.0f;
    float radiusInner = 0.0f;
    float lineThickness = 0.0f;
};

}

#endif //SGL_COORDINATEAXESOVERLAYWIDGET_HPP
