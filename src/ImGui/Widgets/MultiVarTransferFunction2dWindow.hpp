/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Christoph Neuhauser
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

#ifndef SGL_MULTIVARTRANSFERFUNCTION2DWINDOW_HPP
#define SGL_MULTIVARTRANSFERFUNCTION2DWINDOW_HPP

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

namespace tinyxml2 {
class XMLDocument;
class XMLPrinter;
}

namespace sgl {

class MultiVarTransferFunction2dWindow;

struct DLL_OBJECT Tf2dShape {
    std::vector<glm::vec2> controlPoints;

};

// Data for one variable.
class DLL_OBJECT GuiVarData2d {
    friend class MultiVarTransferFunction2dWindow;

public:
    /**
     * @param window The @see MultiVarTransferFunction2dWindow parent instance.
     * @param tfPresetFile The preset transfer function file.
     * @param transferFunctionMap_sRGB The memory for storing the sRGB transfer function map.
     * @param transferFunctionMap_linearRGB The memory for storing the linear RGB transfer function map.
     */
    GuiVarData2d(
            MultiVarTransferFunction2dWindow *window, const std::string &tfPresetFile,
            sgl::Color16 *transferFunctionMap_sRGB,
            sgl::Color16 *transferFunctionMap_linearRGB);

    bool saveTfToFile(const std::string &filename);
    bool loadTfFromFile(const std::string &filename);

    std::string serializeXmlString();
    bool deserializeXmlString(const std::string &xmlString);
    bool getIsSelectedRangeFixed();
    void setIsSelectedRangeFixed(bool _isSelectedRangeFixed);

    void setAttributeName(int varIdx, const std::string& name);
    void setAttributeValues(const std::vector<float>& _attributesScalar, const std::vector<float>& _attributesGradient);
    void setAttributeValues(
            const std::vector<float>& _attributesScalar, const std::vector<float>& _attributesGradient,
            float minAttributeScalar, float maxAttributeScalar, float minAttributeGradient, float maxAttributeGradient);

    bool renderGui();

    inline const std::string &getSaveFileString() { return saveFileString; }

    // Get data range.
    [[nodiscard]] inline float getDataRangeMin() const { return dataRangesScalar.x; }
    [[nodiscard]] inline float getDataRangeMax() const { return dataRangesScalar.y; }
    [[nodiscard]] inline const glm::vec2 &getDataRange() const { return dataRangesScalar; }
    [[nodiscard]] inline float getSelectedRangeMin() const { return selectedRangeScalar.x; }
    [[nodiscard]] inline float getSelectedRangeMax() const { return selectedRangeScalar.y; }
    [[nodiscard]] inline const glm::vec2 &getSelectedRange() const { return selectedRangeScalar; }

private:
    void writeToXml(tinyxml2::XMLPrinter& printer);
    bool readFromXml(tinyxml2::XMLDocument& doc);

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

    MultiVarTransferFunction2dWindow *window;

    std::string attributeName;
    int varIdx = 0;
    int histogramResolution = 64;
    std::vector<float> histogram;
    glm::vec2 dataRangesScalar = glm::vec2(0.0f);
    glm::vec2 selectedRangeScalar = glm::vec2(0.0f);
    glm::vec2 dataRangesGradient = glm::vec2(0.0f);
    glm::vec2 selectedRangeGradient = glm::vec2(0.0f);
    std::vector<float> attributesScalar, attributesGradient;
    bool isEmpty = true;
    bool isSelectedRangeFixed = false;

    // Drag-and-drop data
    sgl::SelectedPointType selectedPointType = sgl::SELECTED_POINT_TYPE_NONE;
    bool dragging = false;
    bool mouseReleased = false;
    int currentSelectionIndex = 0;
    sgl::AABB2 opacityGraphBox, colorBarBox;
    glm::vec2 oldMousePosWidget;
    float opacitySelection = 1.0f;
    ImVec4 colorSelection = ImColor(255, 255, 255, 255);
    bool reRender = false;

    std::string saveFileString = "Standard.xml";
    int selectedFileIndex = -1;

    void rebuildTransferFunctionMap();
    void rebuildTransferFunctionMapLocal();
    void rebuildTransferFunctionMap_sRGB();
    void rebuildTransferFunctionMap_LinearRGB();

    // Pointer (with offset) to data stored by @see MultiVarTransferFunction2dWindow.
    sgl::Color16 *transferFunctionMap_sRGB = nullptr;
    sgl::Color16 *transferFunctionMap_linearRGB = nullptr;

    // TODO
    sgl::ColorSpace interpolationColorSpace = sgl::COLOR_SPACE_LINEAR_RGB;
    std::vector<Tf2dShape> shapes;
};

class DLL_OBJECT MultiVarTransferFunction2dWindow {
    friend class GuiVarData2d;

public:
    /**
     * @param saveDirectoryPrefix A prefix directory attached to the file names (e.g., "stress", "multivar").
     * @param tfPresetFiles A list of preset transfer function files. If more variables are given than preset files,
     * the files are repeated.
     */
    explicit MultiVarTransferFunction2dWindow(
            const std::string &saveDirectoryPrefix,
            const std::vector<std::string> &tfPresetFiles = {});

    /**
     * Assumes no prefix directory should be used.
     * @param tfPresetFiles A list of preset transfer function files. If more variables are given than preset files,
     * the files are repeated.
     */
    explicit MultiVarTransferFunction2dWindow(const std::vector<std::string> &tfPresetFiles = {});

    ~MultiVarTransferFunction2dWindow();

    // Multi-var functions.
    void setAttributesValues(
            const std::vector<std::string>& names,
            const std::vector<std::vector<float>>& allAttributes);

    /*
     * Secondary interface, where attribute data is not supplied through @see setAttributesValues, but when used
     * through a callback. 'fmt' is the format of the entries. 'attributes' and 'fmt' may be a null pointer.
     */
    using RequestAttributeValuesCallback = std::function<void(
            int varIdx, const void** attributesScalar, const void** attributesGradient,
            ScalarDataFormat* fmtScalar, ScalarDataFormat* fmtGradient,
            size_t& numAttributes, float& minValScalar, float& maxValScalar,
            float& minValGradient, float& maxValGradient)>;
    inline void setRequestAttributeValuesCallback(RequestAttributeValuesCallback callback) {
        requestAttributeValuesCallback = std::move(callback);
    }
    void setAttributeNames(const std::vector<std::string>& names);
    void setAttributeDataDirty(int varIdx);
    void loadAttributeDataIfEmpty(int varIdx);
    void updateAttributeName(int varIdx, const std::string& attributeName);
    void removeAttribute(int varIdx);
    void addAttributeName(const std::string& name);
    bool getIsSelectedRangeFixed(int varIdx);
    void setIsSelectedRangeFixed(int varIdx, bool _isSelectedRangeFixed);

    //bool saveCurrentVarTfToFile(const std::string& filename);
    //bool loadTfFromFile(int varIdx, const std::string& filename);
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
    [[nodiscard]] inline float getDataRangeMin(int varIdx) const { return guiVarData2d.at(varIdx).dataRangesScalar.x; }
    [[nodiscard]] inline float getDataRangeMax(int varIdx) const { return guiVarData2d.at(varIdx).dataRangesScalar.y; }
    [[nodiscard]] inline const glm::vec2& getDataRange(int varIdx) const { return guiVarData2d.at(varIdx).dataRangesScalar; }
    [[nodiscard]] inline float getSelectedRangeMin(int varIdx) const { return guiVarData2d.at(varIdx).selectedRangeScalar.x; }
    [[nodiscard]] inline float getSelectedRangeMax(int varIdx) const { return guiVarData2d.at(varIdx).selectedRangeScalar.y; }
    [[nodiscard]] inline const glm::vec2& getSelectedRange(int varIdx) const {
        return guiVarData2d.at(varIdx).selectedRangeScalar;
    }
    [[nodiscard]] inline std::pair<float, float> getSelectedRangePair(int varIdx) {
        auto& varData = guiVarData2d.at(varIdx);
        if (requestAttributeValuesCallback && varData.isEmpty) {
            loadAttributeDataIfEmpty(varIdx);
        }
        const glm::vec2 selectedRange = varData.selectedRangeScalar;
        return std::make_pair(selectedRange.x, selectedRange.y);
    }

    inline void setSelectedRange(int varIdx, const glm::vec2 &range) {
        guiVarData2d.at(varIdx).selectedRangeScalar = range;
        if (!guiVarData2d.at(varIdx).isEmpty) {
            guiVarData2d.at(varIdx).computeHistogram();
        }
        guiVarData2d.at(varIdx).window->rebuildRangeSsbo();
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
    std::vector<GuiVarData2d> guiVarData2d;
    std::vector<bool> dirtyIndices;
    size_t selectedVarIndex = 0;
    GuiVarData2d* currVarData = nullptr;
    bool useAttributeArrays = false;

    // Secondary, on-request loading interface.
    RequestAttributeValuesCallback requestAttributeValuesCallback{};

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

#endif //SGL_MULTIVARTRANSFERFUNCTION2DWINDOW_HPP
