/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Christoph Neuhauser
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

#ifndef SGL_TRANSFERFUNCTIONWINDOW_HPP
#define SGL_TRANSFERFUNCTIONWINDOW_HPP

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <Math/Geometry/AABB2.hpp>
#include <Utils/File/PathWatch.hpp>
#include <Graphics/Color.hpp>
#ifdef SUPPORT_OPENGL
#include <Graphics/Texture/Texture.hpp>
#endif
#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Image/Image.hpp>
#endif
#ifndef DISABLE_IMGUI
#include <ImGui/ImGuiWrapper.hpp>
#endif

namespace sgl {

enum ColorSpace {
    COLOR_SPACE_SRGB, COLOR_SPACE_LINEAR_RGB
};
const char* const COLOR_SPACE_NAMES[] {
    "sRGB", "Linear RGB"
};

/**
 * A color point stores sRGB color values.
 */
struct DLL_OBJECT ColorPoint_sRGB {
    ColorPoint_sRGB(const sgl::Color16& color, float position) : color(color), position(position) {}
    sgl::Color16 color;
    float position;
};

struct DLL_OBJECT ColorPoint_LinearRGB {
    ColorPoint_LinearRGB(const glm::vec3& color, float position) : color(color), position(position) {}
    glm::vec3 color;
    float position;
};

struct DLL_OBJECT OpacityPoint {
    OpacityPoint(float opacity, float position) : opacity(opacity), position(position) {}
    float opacity;
    float position;
};

enum SelectedPointType {
    SELECTED_POINT_TYPE_NONE, SELECTED_POINT_TYPE_OPACITY, SELECTED_POINT_TYPE_COLOR
};

enum ColorDataMode {
    // 0 - 255
    COLOR_DATA_MODE_UNSIGNED_BYTE,
    // 0 - 65535
    COLOR_DATA_MODE_UNSIGNED_SHORT,
    // 0.0 - 1.0
    COLOR_DATA_MODE_FLOAT_NORMALIZED,
    // 0.0 - 255.0
    COLOR_DATA_MODE_FLOAT_255
};

const char* const COLOR_DATA_MODE_NAMES[] = {
        "ubyte", "ushort", "float", "float_255"
};
const int NUM_COLOR_DATA_MODES = ((int)(sizeof(COLOR_DATA_MODE_NAMES) / sizeof(*COLOR_DATA_MODE_NAMES)));

DLL_OBJECT ColorDataMode parseColorDataModeName(const std::string& dataModeName);

/**
 * Stores color and opacity points and renders the GUI.
 */
class DLL_OBJECT TransferFunctionWindow
{
public:
    TransferFunctionWindow();

    inline const std::string& getSaveDirectory() { return saveDirectory; }
    bool saveFunctionToFile(const std::string& filename);
    bool loadFunctionFromFile(const std::string& filename);
    void updateAvailableFiles();

    bool renderGui();
    void update(float dt);

    [[nodiscard]] inline bool getShowWindow() const { return showTransferFunctionWindow; }
    [[nodiscard]] inline bool& getShowWindow() { return showTransferFunctionWindow; }
    inline void setShowWindow(bool show) { showTransferFunctionWindow = show; }
    void setClearColor(const sgl::Color& clearColor);
    void computeHistogram(const std::vector<float>& attributes);
    void computeHistogram(const std::vector<float>& attributes, float minAttr, float maxAttr);
    void setUseLinearRGB(bool useLinearRGB);

    // For querying transfer function in application
    glm::vec4 getLinearRGBColorAtAttribute(float attribute); // attribute: Between 0 and 1
    float getOpacityAtAttribute(float attribute); // attribute: Between 0 and 1
    const std::vector<sgl::Color16>& getTransferFunctionMap_sRGB() { return transferFunctionMap_sRGB; }

    // For OpenGL: Has 256 entries. Get mapped color for normalized attribute by accessing entry at "attr*255".
#ifdef SUPPORT_OPENGL
    sgl::TexturePtr& getTransferFunctionMapTexture();
#endif
#ifdef SUPPORT_VULKAN
    sgl::vk::TexturePtr& getTransferFunctionMapTextureVulkan();
#endif
    bool getTransferFunctionMapRebuilt();

#ifdef SUPPORT_VULKAN
    /// Returns the Data range uniform buffer object.
    inline sgl::vk::BufferPtr& getMinMaxUboVulkan() { return minMaxUboVulkan; }
#endif

    // For ray tracing interface
    inline const std::vector<OpacityPoint>& getOpacityPoints() { return opacityPoints; }
    inline const std::vector<ColorPoint_sRGB>& getColorPoints_sRGB() { return colorPoints; }
    inline const std::vector<ColorPoint_LinearRGB>& getColorPoints_LinearRGB() { return colorPoints_LinearRGB; }

    // Get data range.
    [[nodiscard]] inline float getDataRangeMin() const { return dataRange.x; }
    [[nodiscard]] inline float getDataRangeMax() const { return dataRange.y; }
    [[nodiscard]] inline const glm::vec2& getDataRange() const { return dataRange; }
    [[nodiscard]] inline float getSelectedRangeMin() const { return selectedRange.x; }
    [[nodiscard]] inline float getSelectedRangeMax() const {
        // Use epsilon to avoid division by NaN.
        return selectedRange.y + (selectedRange.x == selectedRange.y ? 1e-4f : 0.0f);
    }
    [[nodiscard]] inline const glm::vec2& getSelectedRange() const { return selectedRange; }
    inline void setSelectedRange(const glm::vec2& _selectedRange) {
        this->selectedRange = _selectedRange;
        recomputeHistogram();
        rebuildRangeUbo();
    }

    // sRGB and linear RGB conversion
    static glm::vec3 sRGBToLinearRGB(const glm::vec3& color_LinearRGB);
    static glm::vec3 linearRGBTosRGB(const glm::vec3& color_sRGB);
    static std::vector<sgl::Color16> createColorMapFromPoints(
            const std::vector<OpacityPoint>& opacityPoints,
            const std::vector<sgl::ColorPoint_sRGB>& colorPoints,
            size_t textureResolution = 256, ColorSpace interpolationColorSpace = COLOR_SPACE_LINEAR_RGB,
            bool outputUseLinearRGB = false);

    inline void setStandardWindowSize(int width, int height) { standardWidth = width; standardHeight = height; }
    inline void setStandardWindowPosition(int x, int y) { standardPositionX = x; standardPositionY = y; }

private:
    void renderFileDialog();
    void renderOpacityGraph();
    void renderColorBar();

    // Drag-and-drop functions
    void onOpacityGraphClick();
    void onColorBarClick();
    void dragPoint();
    bool selectNearestOpacityPoint(int& currentSelectionIndex, const glm::vec2& mousePosWidget);
    bool selectNearestColorPoint(int& currentSelectionIndex, const glm::vec2& mousePosWidget);

    // Histogram data.
    void setHistogram(const std::vector<int>& occurences);
    void recomputeHistogram();
    int histogramResolution = 64;
    std::vector<float> histogram;
    glm::vec2 dataRange = glm::vec2(0.0f);
    glm::vec2 selectedRange = glm::vec2(0.0f);
    std::vector<float> attributes;

    // Drag-and-drop data
    SelectedPointType selectedPointType = SELECTED_POINT_TYPE_NONE;
    bool dragging = false;
    bool mouseReleased = false;
    int currentSelectionIndex = 0;
    sgl::AABB2 opacityGraphBox, colorBarBox;
    glm::vec2 oldMousePosWidget;

    // GUI
    bool reRender = false;
    bool showTransferFunctionWindow = true;
    int standardWidth = 612;
    int standardHeight = 774;
    int standardPositionX = 0;
    int standardPositionY = 1334;
#ifndef DISABLE_IMGUI
    float opacitySelection = 1.0f;
    ImVec4 colorSelection = static_cast<ImVec4>(ImColor(255, 255, 255, 255));
#endif
    sgl::Color clearColor;
    ColorSpace interpolationColorSpace;

    // Transfer function directory watch.
    sgl::PathWatch directoryContentWatch;

    std::string saveDirectory;
    std::string saveFileString = "Standard.xml";
    std::vector<std::string> availableFiles;
    int selectedFileIndex = -1;
    void rebuildTransferFunctionMap();
    static void rebuildTransferFunctionMap_sRGB(
            const std::vector<OpacityPoint>& opacityPoints,
            const std::vector<ColorPoint_sRGB>& colorPoints,
            size_t textureResolution,
            std::vector<sgl::Color16>& transferFunctionMap_sRGB,
            std::vector<sgl::Color16>& transferFunctionMap_linearRGB);
    static void rebuildTransferFunctionMap_LinearRGB(
            const std::vector<OpacityPoint>& opacityPoints,
            const std::vector<ColorPoint_LinearRGB>& colorPoints_LinearRGB,
            size_t textureResolution,
            std::vector<sgl::Color16>& transferFunctionMap_sRGB,
            std::vector<sgl::Color16>& transferFunctionMap_linearRGB);
    void rebuildRangeUbo();
    std::vector<sgl::Color16> transferFunctionMap_sRGB;
    std::vector<sgl::Color16> transferFunctionMap_linearRGB;
#ifdef SUPPORT_OPENGL
    sgl::TexturePtr tfMapTexture;
    sgl::TextureSettings tfMapTextureSettings;
#endif
#ifdef SUPPORT_VULKAN
    sgl::vk::TexturePtr tfMapTextureVulkan;
    sgl::vk::ImageSettings tfMapImageSettingsVulkan;
#endif

#ifdef SUPPORT_VULKAN
    // Data range uniform buffer object (storing float min, float max).
    sgl::vk::BufferPtr minMaxUboVulkan;
#endif

    std::vector<OpacityPoint> opacityPoints;
    std::vector<ColorPoint_sRGB> colorPoints;
    std::vector<ColorPoint_LinearRGB> colorPoints_LinearRGB;
    bool useLinearRGB = true;
    bool transferFunctionMapRebuilt = true;
};

}

#endif //SGL_TRANSFERFUNCTIONWINDOW_HPP
