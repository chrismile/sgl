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

#ifndef SGL_COLORLEGENDWIDGET_HPP
#define SGL_COLORLEGENDWIDGET_HPP

#include <Graphics/Color.hpp>
#include <string>
#include <vector>

namespace sgl {

/**
 * A color legend widget for multi-variable data.
 */
class DLL_OBJECT ColorLegendWidget {
public:
    ColorLegendWidget();
    void renderGui();
    void setClearColor(const sgl::Color& clearColor);

    // Set attribute data.
    inline void setPositionIndex(int positionIndex, int numPositionsTotal) {
        this->positionIndex = positionIndex;
        this->numPositionsTotal = numPositionsTotal;
    }
    inline void setAttributeDisplayName(const std::string& attributeDisplayName) {
        this->attributeDisplayName = attributeDisplayName;
    }
    inline void setAttributeMinValue(float attributeMinValue) { this->attributeMinValue = attributeMinValue; }
    inline void setAttributeMaxValue(float attributeMaxValue) { this->attributeMaxValue = attributeMaxValue; }
    inline void setTransferFunctionColorMap(const std::vector<sgl::Color16>& transferFunctionColorMap) {
        this->transferFunctionColorMap = transferFunctionColorMap;
    }

    static void setFontScaleStandard(float fontScale) {
        ColorLegendWidget::fontScaleResetValue = fontScale;
    }
    static void setFontScale(float fontScale) {
        ColorLegendWidget::fontScale = fontScale;
    }
    static void resetStandardSize() {
        ColorLegendWidget::regionHeight = regionHeightStandard;
        ColorLegendWidget::fontScale = fontScaleResetValue;
        ColorLegendWidget::textRegionWidth = textRegionWidthStandard;
    }

private:
    int positionIndex = 0; ///< When placing many widgets next to each other.
    int numPositionsTotal = 1; ///< When placing many widgets next to each other.
    float attributeMinValue = 0.0f;
    float attributeMaxValue = 1.0f;
    std::string attributeDisplayName = "Vorticity";
    std::vector<sgl::Color16> transferFunctionColorMap; ///< Colors in sRGB color space.
    sgl::Color clearColor = sgl::Color(255, 255, 255);
    sgl::Color textColor = sgl::Color(0, 0, 0);

    // Internal UI data.
    bool showWindow = true;
    static float regionHeightStandard;
    static float regionHeight;
    static const float fontScaleStandard;
    static float fontScaleResetValue;
    static float fontScale;
    static float textRegionWidthStandard;
    static float textRegionWidth;
};

}

#endif //SGL_COLORLEGENDWIDGET_HPP
