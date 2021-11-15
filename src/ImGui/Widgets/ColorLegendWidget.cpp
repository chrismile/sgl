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

#include <iostream>
#include <algorithm>

#include <ImGui/ImGuiWrapper.hpp>
#include <ImGui/imgui_verticaltext.h>

#include "NumberFormatting.hpp"
#include "ColorLegendWidget.hpp"

namespace sgl {

float ColorLegendWidget::regionHeightStandard = -1;
float ColorLegendWidget::regionHeight = regionHeightStandard;
const float ColorLegendWidget::fontScaleStandard = 0.75f;
float ColorLegendWidget::fontScaleResetValue = ColorLegendWidget::fontScaleStandard;
float ColorLegendWidget::fontScale = fontScaleStandard;
float ColorLegendWidget::textRegionWidthStandard = -1;
float ColorLegendWidget::textRegionWidth = textRegionWidthStandard;

ColorLegendWidget::ColorLegendWidget() {
    if (regionHeightStandard == -1) {
        float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor() / 1.875f;
        regionHeightStandard = (300.0f - 2.0f) * scaleFactor;
        textRegionWidthStandard = 85 * fontScale / fontScaleStandard * scaleFactor;
    }

    transferFunctionColorMap.reserve(256);
    for (int i = 0; i < 256; i++) {
        float pct = float(i) / float(255);
        // Test data.
        auto green = uint8_t(glm::clamp(pct * 255.0f, 0.0f, 255.0f));
        auto blue = uint8_t(glm::clamp((1.0f - pct) * 255.0f, 0.0f, 255.0f));
        transferFunctionColorMap.emplace_back(sgl::Color(0, green, blue));
    }
    regionHeight = regionHeightStandard;
}

void ColorLegendWidget::setClearColor(const sgl::Color& _clearColor) {
    this->clearColor = _clearColor;
    this->textColor = sgl::Color(255 - clearColor.getR(), 255 - clearColor.getG(), 255 - clearColor.getB());
}

void ColorLegendWidget::renderGui() {
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor() / 1.875f;

    const float barWidth = 25 * scaleFactor;
    const float totalWidth = barWidth + textRegionWidth;
    const int numTicks = 5;
    const float tickWidth = 10;

    float textHeight = 0;

    bool useDockSpaceMode = ImGuiWrapper::get()->getUseDockSpaceMode();
    ImVec2 oldCursorPos = ImGui::GetCursorPos();
    float contentOffset = ImGui::GetCursorPos().x;

    ImVec2 windowSize = ImVec2(totalWidth + 3, regionHeight + 30 * scaleFactor);
    float windowOffset = (windowSize.x + 8) * float(numPositionsTotal - positionIndex - 1);

    ImVec2 windowPos;
    if (useDockSpaceMode) {
        ImVec2 currentWindowSize = sgl::ImGuiWrapper::get()->getCurrentWindowSize();
        windowPos = ImVec2(
                currentWindowSize.x - totalWidth - 18 - windowOffset,
                currentWindowSize.y - regionHeight - 46 * scaleFactor);
    } else {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        windowPos = ImVec2(
                viewport->Pos.x + viewport->Size.x - totalWidth - 12 - windowOffset,
                viewport->Pos.y + viewport->Size.y - regionHeight - 40 * scaleFactor);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(windowSize);
    }

    glm::vec3 clearColorFlt(clearColor.getFloatR(), clearColor.getFloatG(), clearColor.getFloatB());
    glm::vec3 textColorFlt(textColor.getFloatR(), textColor.getFloatG(), textColor.getFloatB());
    glm::vec3 bgColor = glm::mix(clearColorFlt, textColorFlt, 0.1);
    ImVec4 bgColorImGui = ImVec4(bgColor.r, bgColor.g, bgColor.b, 0.7f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColorImGui);
    bool showContent = true;
    if (!useDockSpaceMode) {
        std::string windowId = std::string() + "##" + attributeDisplayName;
        showContent = ImGui::Begin(
                windowId.c_str(), &showWindow,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    }
    if (showContent) {
        ImGui::SetWindowFontScale(fontScale); // Make font slightly smaller.
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 startPos = ImGui::GetCursorScreenPos();
        if (useDockSpaceMode) {
            ImVec2 cursorPos = ImGui::GetCursorPos();
            startPos = ImVec2(windowPos.x - cursorPos.x + startPos.x, windowPos.y - cursorPos.y + startPos.y);
        }

        if (useDockSpaceMode) {
            ImVec2 bgPos = ImVec2(startPos.x - contentOffset, startPos.y - contentOffset);
            drawList->AddRectFilled(
                    bgPos, ImVec2(bgPos.x + windowSize.x, bgPos.y + windowSize.y),
                    ImColor(bgColorImGui), 1.0f);
            ImColor borderColor = ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border));
            drawList->AddRect(
                    bgPos, ImVec2(bgPos.x + windowSize.x, bgPos.y + windowSize.y),
                    borderColor, 3.0f);
        }

        // Draw color bar.
        ImVec2 pos = ImVec2(startPos.x + 1, startPos.y + 1);
        const float lineHeightFactor = 1.0f / float(transferFunctionColorMap.size() - 1);
        const size_t numColorMapEntries = transferFunctionColorMap.size();
        for (size_t i = 0; i < transferFunctionColorMap.size(); i++) {
            sgl::Color16 color = transferFunctionColorMap[numColorMapEntries - i - 1];
            ImU32 colorImgui = ImColor(color.getFloatR(), color.getFloatG(), color.getFloatB());
            drawList->AddLine(
                    ImVec2(pos.x, pos.y), ImVec2(pos.x + barWidth, pos.y), colorImgui,
                    2.0f * regionHeight * lineHeightFactor);
            pos.y += regionHeight * lineHeightFactor;
        }

        ImVec2 textSize = ImGui::CalcTextSizeVertical(attributeDisplayName.c_str());
        textHeight = textSize.y;
        ImVec2 textPos = ImVec2(
                startPos.x + barWidth + 31 * fontScale / fontScaleStandard * scaleFactor,
                //startPos.y + regionHeight / 2.0f - textSize.y / 2.0f + 1);
                startPos.y + regionHeight / 2.0f + textSize.y / 2.0f + 1);
        ImGui::AddTextVertical(
                drawList, textPos, textColor.getColorRGBA(), attributeDisplayName.c_str(),
                nullptr, true);

        ImU32 textColorImgui = textColor.getColorRGBA();

        // Add min/max value text to color bar.
        float textHeightLocal = ImGui::CalcTextSize(attributeDisplayName.c_str()).y;
        std::string minText = getNiceNumberString(attributeMinValue, 3);
        std::string maxText = getNiceNumberString(attributeMaxValue, 3);
        drawList->AddText(
                ImVec2(startPos.x + barWidth + 10, startPos.y + regionHeight - textHeightLocal/2.0f + 1),
                textColorImgui, minText.c_str());
        drawList->AddText(
                ImVec2(startPos.x + barWidth + 10, startPos.y - textHeightLocal/2.0f + 1),
                textColorImgui, maxText.c_str());

        ImVec2 rangeSize = ImGui::CalcTextSize(minText.c_str());
        textRegionWidth = std::max(textRegionWidth, 30 * fontScale / fontScaleStandard * scaleFactor + rangeSize.x);

        // Add ticks to the color bar.
        for (int tick = 0; tick < numTicks; tick++) {
            float xpos = startPos.x + barWidth;
            float ypos = startPos.y + float(tick) / float(numTicks - 1) * regionHeight + 1;
            drawList->AddLine(
                    ImVec2(xpos - tickWidth / 2.0f, ypos),
                    ImVec2(xpos + tickWidth / 2.0f, ypos),
                    textColorImgui, 2
            );
        }
    }

    if (!useDockSpaceMode) {
        ImGui::End();
    }
    ImGui::PopStyleColor();

    // Enlarge the height of the widget if one widget needs more vertical space for the text.
    regionHeight = std::max(regionHeight, textHeight + 50 * fontScale / fontScaleStandard);
}

}
