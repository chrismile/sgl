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
#include <Graphics/Color.hpp>
#include <Graphics/Scene/Camera.hpp>
#include <ImGui/imgui.h>
#include <ImGui/imgui_custom.h>
#include <ImGui/ImGuiWrapper.hpp>
#include "CoordinateAxesOverlayWidget.hpp"

namespace sgl {

static float computeZoomFactor(float z, float f) {
    float val = 1.0f / (1.0f - (1.0f / f) * z);
    val = val / f * (f - 1.0f);
    return val;
}

void CoordinateAxesOverlayWidget::setClearColor(const sgl::Color& clearColorSgl) {
    clearColor = clearColorSgl.getFloatColorRGB();
}

void CoordinateAxesOverlayWidget::renderGui(const sgl::CameraPtr& cam) {
    /*
     * This function draws a coordinate axes widget similar to what is used in Blender.
     */
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 windowPos = ImGuiWrapper::get()->getCurrentWindowPosition();
    ImVec2 windowSize = ImGuiWrapper::get()->getCurrentWindowSize();
    ImVec2 offset = ImGuiWrapper::get()->getScaleDependentSize(45, 45);

    const char* AXIS_NAME[3] = { "X", "Y", "Z" };

    // Compute the font sizes. The offset is added as a hack to get perfectly centered text.
    ImFont* fontSmall = ImGuiWrapper::get()->getFontSmall();
    float fontSizeSmall = ImGuiWrapper::get()->getFontSizeSmall();
    ImVec2 textSize[3];
    float minRadius = 0.0f;
    for (int i = 0; i < 3; i++) {
        ImVec2 textSizeAxis = ImGui::CalcTextSize(fontSmall, fontSizeSmall, AXIS_NAME[i]);
        if (i < 2) {
            textSizeAxis.x -= ImGuiWrapper::get()->getScaleDependentSize(2.0f);
            textSizeAxis.y -= ImGuiWrapper::get()->getScaleDependentSize(2.0f);
        } else {
            textSizeAxis.x -= ImGuiWrapper::get()->getScaleDependentSize(1.0f);
        }
        textSize[i] = textSizeAxis;
        float minRadiusAxis =
                0.5f * std::sqrt(textSizeAxis.x * textSizeAxis.x + textSizeAxis.y * textSizeAxis.y)
                + ImGuiWrapper::get()->getScaleDependentSize(1.0f);
        minRadius = std::max(minRadius, minRadiusAxis);
    }

    radiusOverlay = ImGuiWrapper::get()->getScaleDependentSize(60.0f);
    radiusBalls = std::max(minRadius, ImGuiWrapper::get()->getScaleDependentSize(10.0f));
    radiusInnerRing = ImGuiWrapper::get()->getScaleDependentSize(2.0f);
    radiusBallsInner = radiusBalls - radiusInnerRing;
    lineThickness = ImGuiWrapper::get()->getScaleDependentSize(4.0f);
    ImVec2 center = ImVec2(
            windowPos.x + offset.x + radiusOverlay,
            windowPos.y + windowSize.y - offset.y - radiusOverlay);

    glm::vec3 right3d = cam->getCameraRight();
    glm::vec3 up3d = cam->getCameraUp();
    glm::vec3 front3d = cam->getCameraFront();
    right3d.y *= -1.0f;
    up3d.y *= -1.0f;
    front3d.y *= -1.0f;
    right3d.z *= -1.0f;
    up3d.z *= -1.0f;
    front3d.z *= -1.0f;
    glm::vec2 right(right3d.x, right3d.y);
    glm::vec2 up(up3d.x, up3d.y);
    glm::vec2 front(front3d.x, front3d.y);

    glm::vec3 axes3d[3] = { right3d, up3d, front3d };
    glm::vec2 axes2d[3] = { right, up, front };
    float signAxis[3];

    for (int i = 0; i < 3; i++) {
        signAxis[i] = axes3d[i].z > -EPSILON ? 1.0f : -1.0f;
    }

    ImU32 colorPos[3];
    ImU32 colorNeg[3];
    float sizeFactorPos[3];
    float sizeFactorNeg[3];
    ImU32 colorInnerNeg[3];
    for (int i = 0; i < 3; i++) {
        float darkToBrightFactor0 = axes3d[i].z * 0.5f + 0.5f;
        float darkToBrightFactor1 = -axes3d[i].z * 0.5f + 0.5f;
        darkToBrightFactor0 = 1.0f - std::pow(1.0f - darkToBrightFactor0, 3.0f);
        darkToBrightFactor1 = 1.0f - std::pow(1.0f - darkToBrightFactor1, 3.0f);
        glm::vec3 colorPosVec = glm::mix(colorDark[i], colorBright[i], darkToBrightFactor0);
        glm::vec3 colorNegVec = glm::mix(colorDark[i], colorBright[i], darkToBrightFactor1);
        sizeFactorPos[i] = computeZoomFactor(axes3d[i].z, 10.0f);
        sizeFactorNeg[i] = computeZoomFactor(-axes3d[i].z, 10.0f);
        colorPos[i] = sgl::colorFromVec3(colorPosVec).getColorRGBA();
        colorNeg[i] = sgl::colorFromVec3(colorNegVec).getColorRGBA();

        glm::vec3 colorInnerBase = glm::mix(clearColor, colorBright[i], 0.5f);
        float alphaInner = glm::mix(0.5f, 1.0f, darkToBrightFactor1);
        colorInnerNeg[i] = sgl::colorFromVec4(glm::vec4(
                colorInnerBase.x, colorInnerBase.y, colorInnerBase.z, alphaInner)).getColorRGBA();
    }

    // Draw coordinate axes (back).
    for (int i = 0; i < 3; i++) {
        if (signAxis[i] < 0.0f) {
            drawList->AddLine(
                    center, ImVec2(
                            center.x + axes2d[i].x * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y + axes2d[i].y * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    signAxis[i] < 0.0f ? colorPos[i] : colorNeg[i], lineThickness);
        }
    }

    // Draw coordinate axis balls (back).
    for (int i = 0; i < 3; i++) {
        if (signAxis[i] < 0.0f) {
            drawList->AddCircleFilled(
                    ImVec2(
                            center.x - signAxis[i] * axes2d[i].x * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y - signAxis[i] * axes2d[i].y * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    radiusBalls * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                    signAxis[i] < 0.0f ? colorPos[i] : colorNeg[i]);
        } else {
            drawList->AddCircleFilled(
                    ImVec2(
                            center.x - signAxis[i] * axes2d[i].x * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y - signAxis[i] * axes2d[i].y * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    radiusBalls * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                    colorInnerNeg[i]);
            drawList->AddCircle(
                    ImVec2(
                            center.x - signAxis[i] * axes2d[i].x * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y - signAxis[i] * axes2d[i].y * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    radiusBalls * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]) - radiusInnerRing * 0.5f,
                    signAxis[i] < 0.0f ? colorPos[i] : colorNeg[i], 0, radiusInnerRing);
        }
    }

    // Draw "X", "Y", "Z" on coordinate axis balls.
    for (int i = 0; i < 3; i++) {
        if (signAxis[i] < 0.0f) {
            drawList->AddText(
                    fontSmall, fontSizeSmall, ImVec2(
                            center.x + axes2d[i].x * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]) - textSize[i].x * 0.5f,
                            center.y + axes2d[i].y * radiusOverlay
                                       * (signAxis[i] < 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]) - textSize[i].y * 0.5f),
                    textColor[i], AXIS_NAME[i]);
        }
    }

    // Draw coordinate axes (front).
    for (int i = 0; i < 3; i++) {
        if (signAxis[i] > 0.0f) {
            drawList->AddLine(
                    center, ImVec2(
                            center.x + axes2d[i].x * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y + axes2d[i].y * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    signAxis[i] > 0.0f ? colorPos[i] : colorNeg[i], lineThickness);
        }
    }

    // Draw coordinate axis balls (front).
    for (int i = 0; i < 3; i++) {
        if (signAxis[i] > 0.0f) {
            drawList->AddCircleFilled(
                    ImVec2(
                            center.x + signAxis[i] * axes2d[i].x * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y + signAxis[i] * axes2d[i].y * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    radiusBalls * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                    signAxis[i] > 0.0f ? colorPos[i] : colorNeg[i]);
        } else {
            drawList->AddCircleFilled(
                    ImVec2(
                            center.x + signAxis[i] * axes2d[i].x * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y + signAxis[i] * axes2d[i].y * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    radiusBalls * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                    colorInnerNeg[i]);
            drawList->AddCircle(
                    ImVec2(
                            center.x + signAxis[i] * axes2d[i].x * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]),
                            center.y + signAxis[i] * axes2d[i].y * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i])),
                    radiusBalls * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]) - radiusInnerRing * 0.5f,
                    signAxis[i] > 0.0f ? colorPos[i] : colorNeg[i], 0, radiusInnerRing);
        }
    }

    // Draw "X", "Y", "Z" on coordinate axis balls.
    for (int i = 0; i < 3; i++) {
        if (signAxis[i] > 0.0f) {
            drawList->AddText(
                    fontSmall, fontSizeSmall, ImVec2(
                            center.x + axes2d[i].x * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]) - textSize[i].x * 0.5f,
                            center.y + axes2d[i].y * radiusOverlay
                                       * (signAxis[i] > 0.0f ? sizeFactorPos[i] : sizeFactorNeg[i]) - textSize[i].y * 0.5f),
                    textColor[i], AXIS_NAME[i]);
        }
    }
}

}
