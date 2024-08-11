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

#ifndef SGL_MULTIVARTRANSFERFUNCTIONWINDOW_HPP
#define SGL_MULTIVARTRANSFERFUNCTIONWINDOW_HPP

#include <utility>

#include <Utils/File/PathWatch.hpp>
#include <Utils/SciVis/ScalarDataFormat.hpp>
#include <ImGui/Widgets/TransferFunctionWindow.hpp>
#ifdef SUPPORT_OPENGL
#include <Graphics/Buffers/GeometryBuffer.hpp>
#include <Graphics/Texture/Texture.hpp>
#endif
#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Buffers/Buffer.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#endif

#ifdef SUPPORT_TINYXML2
namespace tinyxml2 {
class XMLDocument;
class XMLPrinter;
}
#endif

namespace sgl {

class MultiVarTransferFunctionWindow;

// Data for one variable.
class DLL_OBJECT GuiVarData {
    friend class MultiVarTransferFunctionWindow;

public:
    /**
     * @param window The @see MultiVarTransferFunctionWindow parent instance.
     * @param tfPresetFile The preset transfer function file.
     * @param transferFunctionMap_sRGB The memory for storing the sRGB transfer function map.
     * @param transferFunctionMap_linearRGB The memory for storing the linear RGB transfer function map.
     */
    GuiVarData(
            MultiVarTransferFunctionWindow *window, const std::string &tfPresetFile,
            sgl::Color16 *transferFunctionMap_sRGB,
            sgl::Color16 *transferFunctionMap_linearRGB);

    bool saveTfToFile(const std::string &filename);
    bool loadTfFromFile(const std::string &filename);
    bool loadTfFromXmlString(const std::string &xmlString);

    std::string serializeXmlString();
    bool deserializeXmlString(const std::string &xmlString);
    bool getIsSelectedRangeFixed();
    void setIsSelectedRangeFixed(bool _isSelectedRangeFixed);

    void setAttributeName(int varIdx, const std::string& name);
    void setAttributeValues(const std::vector<float>& _attributes);
    void setAttributeValues(const std::vector<float>& _attributes, float minAttribute, float maxAttribute);

    bool renderGui();

    inline const std::string &getSaveFileString() { return saveFileString; }

    // Get data range.
    [[nodiscard]] inline float getDataRangeMin() const { return dataRange.x; }
    [[nodiscard]] inline float getDataRangeMax() const { return dataRange.y; }
    [[nodiscard]] inline const glm::vec2 &getDataRange() const { return dataRange; }
    [[nodiscard]] inline float getSelectedRangeMin() const { return selectedRange.x; }
    [[nodiscard]] inline float getSelectedRangeMax() const { return selectedRange.y; }
    [[nodiscard]] inline const glm::vec2 &getSelectedRange() const { return selectedRange; }

private:
#ifdef SUPPORT_TINYXML2
    void writeToXml(tinyxml2::XMLPrinter& printer);
    bool readFromXml(tinyxml2::XMLDocument& doc);
#endif

    void computeHistogram();
    void renderFileDialog();
    void renderOpacityGraph();
    void renderColorBar();

    // Drag-and-drop functions
    void onOpacityGraphClick();
    void onColorBarClick();
    void dragPoint();
    bool selectNearestOpacityPoint(int &currentSelectionIndex, const glm::vec2 &mousePosWidget);
    bool selectNearestColorPoint(int &currentSelectionIndex, const glm::vec2 &mousePosWidget);

    MultiVarTransferFunctionWindow *window;

    std::string attributeName;
    int varIdx = 0;
    int histogramResolution = 64;
    std::vector<float> histogram;
    glm::vec2 dataRange = glm::vec2(0.0f);
    glm::vec2 selectedRange = glm::vec2(0.0f);
    std::vector<float> attributes;
    bool isEmpty = true;
    bool recomputeMinMax = true;
    bool isSelectedRangeFixed = false;

    // Drag-and-drop data
    sgl::SelectedPointType selectedPointType = sgl::SELECTED_POINT_TYPE_NONE;
    bool dragging = false;
    bool mouseReleased = false;
    int currentSelectionIndex = 0;
    sgl::AABB2 opacityGraphBox, colorBarBox;
    glm::vec2 oldMousePosWidget;
    float opacitySelection = 1.0f;
    ImVec4 colorSelection = static_cast<ImVec4>(ImColor(255, 255, 255, 255));
    bool reRender = false;

    std::string saveFileString = "Standard.xml";
    int selectedFileIndex = -1;

    void rebuildTransferFunctionMap();
    void rebuildTransferFunctionMapLocal();
    void rebuildTransferFunctionMap_sRGB();
    void rebuildTransferFunctionMap_LinearRGB();

    // Pointer (with offset) to data stored by @see MultiVarTransferFunctionWindow.
    sgl::Color16 *transferFunctionMap_sRGB = nullptr;
    sgl::Color16 *transferFunctionMap_linearRGB = nullptr;

    sgl::ColorSpace interpolationColorSpace = sgl::COLOR_SPACE_LINEAR_RGB;
    std::vector<sgl::OpacityPoint> opacityPoints;
    std::vector<sgl::ColorPoint_sRGB> colorPoints;
    std::vector<sgl::ColorPoint_LinearRGB> colorPoints_LinearRGB;
};

class DLL_OBJECT MultiVarTransferFunctionWindow {
    friend class GuiVarData;

public:
    /**
     * @param saveDirectoryPrefix A prefix directory attached to the file names (e.g., "stress", "multivar").
     * @param tfPresetFiles A list of preset transfer function files. If more variables are given than preset files,
     * the files are repeated.
     */
    explicit MultiVarTransferFunctionWindow(
            const std::string &saveDirectoryPrefix,
            const std::vector<std::string> &tfPresetFiles = {});

    /**
     * Assumes no prefix directory should be used.
     * @param tfPresetFiles A list of preset transfer function files. If more variables are given than preset files,
     * the files are repeated.
     */
    explicit MultiVarTransferFunctionWindow(const std::vector<std::string> &tfPresetFiles = {});

    ~MultiVarTransferFunctionWindow();

    // Multi-var functions.
    void setAttributesValues(
            const std::vector<std::string>& names,
            const std::vector<std::vector<float>>& allAttributes,
            size_t defaultVarIndex = 0);

    /*
     * Secondary interface, where attribute data is not supplied through @see setAttributesValues, but when used
     * through a callback. 'fmt' is the format of the entries. 'attributes' and 'fmt' may be a null pointer.
     */
    using RequestAttributeValuesCallback = std::function<void(
            int varIdx, const void** attributes, ScalarDataFormat* fmt, size_t& numAttributes, float& minVal, float& maxVal)>;
    inline void setRequestAttributeValuesCallback(RequestAttributeValuesCallback callback) {
        requestAttributeValuesCallback = std::move(callback);
    }
    void setAttributeNames(const std::vector<std::string>& names, size_t defaultVarIndex = 0);
    void setAttributeDataDirty(int varIdx);
    void loadAttributeDataIfEmpty(int varIdx);
    void updateAttributeName(int varIdx, const std::string& attributeName);
    void removeAttribute(int varIdx);
    void addAttributeName(const std::string& name);
    bool getIsSelectedRangeFixed(int varIdx);
    void setIsSelectedRangeFixed(int varIdx, bool _isSelectedRangeFixed);

    /*
     * Some programs may support computing the histogram themselves (e.g., on the GPU).
     * The following callback (optional) should return 'true' if the histogram is calculated externally.
     */
    using RequestHistogramCallback = std::function<bool(
            int varIdx, int histSize, std::vector<float>& histogram, float& selectedRangeMin, float& selectedRangeMax,
            float& dataRangeMin, float& dataRangeMax, bool recomputeMinMax, bool isSelectedRangeFixed)>;
    inline void setRequestHistogramCallback(RequestHistogramCallback callback) {
        requestHistogramCallback = std::move(callback);
    }

    bool loadFunctionFromFile(int varIdx, const std::string& filename);
    bool loadFunctionFromXmlString(int varIdx, const std::string& xmlString);
    bool loadFromTfNameList(const std::vector<std::string> &tfNames);
    std::string serializeXmlString(int varIdx);
    bool deserializeXmlString(int varIdx, const std::string &xmlString);

    bool renderGui();
    void update(float dt);

    [[nodiscard]] inline bool getShowWindow() const { return showWindow; }
    [[nodiscard]] inline bool& getShowWindow() { return showWindow; }
    inline void setShowWindow(bool show) { showWindow = show; }
    void setClearColor(const sgl::Color &_clearColor);
    void setUseLinearRGB(bool _useLinearRGB);

    // 1D array texture, with one 1D color (RGBA) texture slice per variable.
#ifdef SUPPORT_OPENGL
    sgl::TexturePtr& getTransferFunctionMapTexture();
#endif
#ifdef SUPPORT_VULKAN
    sgl::vk::TexturePtr &getTransferFunctionMapTextureVulkan();
#endif

    bool getTransferFunctionMapRebuilt();
    bool getIsVariableDirty(int varIdx);
    void resetDirty();
    std::vector<sgl::Color16> getTransferFunctionMap_sRGB(int varIdx);
    std::vector<glm::vec4> getTransferFunctionMap_sRGBDownscaled(int varIdx, int numEntries);
    std::vector<glm::vec4> getTransferFunctionMap_sRGBPremulDownscaled(int varIdx, int numEntries);

    void setTransferFunction(
            int varIdx, const std::vector<OpacityPoint>& opacityPoints,
            const std::vector<sgl::ColorPoint_sRGB>& colorPoints,
            ColorSpace interpolationColorSpace = COLOR_SPACE_LINEAR_RGB);

    // Get data range.
    [[nodiscard]] inline float getDataRangeMin(int varIdx) const { return guiVarData.at(varIdx).dataRange.x; }
    [[nodiscard]] inline float getDataRangeMax(int varIdx) const { return guiVarData.at(varIdx).dataRange.y; }
    [[nodiscard]] inline const glm::vec2& getDataRange(int varIdx) const { return guiVarData.at(varIdx).dataRange; }
    [[nodiscard]] inline float getSelectedRangeMin(int varIdx) const { return guiVarData.at(varIdx).selectedRange.x; }
    [[nodiscard]] inline float getSelectedRangeMax(int varIdx) const { return guiVarData.at(varIdx).selectedRange.y; }
    [[nodiscard]] inline const glm::vec2& getSelectedRange(int varIdx) const {
        return guiVarData.at(varIdx).selectedRange;
    }
    [[nodiscard]] inline std::pair<float, float> getSelectedRangePair(int varIdx) {
        auto& varData = guiVarData.at(varIdx);
        if (requestAttributeValuesCallback && varData.isEmpty) {
            loadAttributeDataIfEmpty(varIdx);
        }
        const glm::vec2 selectedRange = varData.selectedRange;
        return std::make_pair(selectedRange.x, selectedRange.y);
    }

    inline void setSelectedRange(int varIdx, const glm::vec2 &range) {
        guiVarData.at(varIdx).selectedRange = range;
        if (!guiVarData.at(varIdx).isEmpty) {
            guiVarData.at(varIdx).computeHistogram();
        }
        guiVarData.at(varIdx).window->rebuildRangeSsbo();
    }

    /// Returns the data range uniform buffer object.
#ifdef SUPPORT_OPENGL
    inline sgl::GeometryBufferPtr& getMinMaxSsbo() { return minMaxSsbo; }
#endif
#ifdef SUPPORT_VULKAN
    inline sgl::vk::BufferPtr& getMinMaxSsboVulkan() { return minMaxSsboVulkan; }
#endif

private:
    void updateAvailableFiles();
    void rebuildTransferFunctionMap();
    void rebuildTransferFunctionMapComplete();
    void rebuildRangeSsbo();
    void recreateTfMapTexture();

    std::vector<std::string> varNames;
    std::vector<GuiVarData> guiVarData;
    std::vector<bool> dirtyIndices;
    size_t selectedVarIndex = 0;
    GuiVarData* currVarData = nullptr;
    bool useAttributeArrays = false;

    // Secondary, on-request loading interface.
    RequestAttributeValuesCallback requestAttributeValuesCallback{};
    RequestHistogramCallback requestHistogramCallback{};

    // Data range shader storage buffer object.
#ifdef SUPPORT_OPENGL
    sgl::GeometryBufferPtr minMaxSsbo;
#endif
#ifdef SUPPORT_VULKAN
    sgl::vk::BufferPtr minMaxSsboVulkan;
#endif
    std::vector<float> minMaxData;

    // GUI
    bool showWindow = true;
    bool reRender = false;
    sgl::Color clearColor;

    // Transfer function directory watch.
    sgl::PathWatch directoryContentWatch;

    std::string directoryName = "TransferFunctions";
    std::string parentDirectory;
    std::string saveDirectory;
    std::vector<std::string> tfPresetFiles;
    std::vector<std::string> availableFiles;
#ifdef SUPPORT_OPENGL
    sgl::TexturePtr tfMapTexture;
    sgl::TextureSettings tfMapTextureSettings;
#endif
#ifdef SUPPORT_VULKAN
    sgl::vk::TexturePtr tfMapTextureVulkan;
    sgl::vk::ImageSettings tfMapImageSettingsVulkan;
#endif

    bool useLinearRGB = true;
    bool transferFunctionMapRebuilt = true;
    std::vector<sgl::Color16> transferFunctionMap_sRGB;
    std::vector<sgl::Color16> transferFunctionMap_linearRGB;
};

}

#endif //SGL_MULTIVARTRANSFERFUNCTIONWINDOW_HPP
