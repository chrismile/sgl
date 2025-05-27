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

#include <iostream>
#include <cmath>

#ifdef __linux__
#include <sys/inotify.h>
#include <poll.h>
#endif

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/fallback/vec.hpp>
#endif

#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/Parallel/Reduction.hpp>
#include <Utils/Parallel/Histogram.hpp>
#include <Math/Math.hpp>
#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#include <memory>
#include <Graphics/Renderer.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#endif
#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Buffers/Buffer.hpp>
#endif

#ifndef DISABLE_IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_custom.h>
#include <ImGui/imgui_stdlib.h>
#endif

#include "MultiVarTransferFunctionWindow.hpp"

#ifdef SUPPORT_TINYXML2
#include <Utils/XML.hpp>
using namespace tinyxml2;
#endif

namespace sgl {

const size_t TRANSFER_FUNCTION_TEXTURE_SIZE = 256;

GuiVarData::GuiVarData(
        MultiVarTransferFunctionWindow* window, const std::string& tfPresetFile,
        sgl::Color16* transferFunctionMap_sRGB,
        sgl::Color16* transferFunctionMap_linearRGB) {
    this->window = window;
    this->transferFunctionMap_sRGB = transferFunctionMap_sRGB;
    this->transferFunctionMap_linearRGB = transferFunctionMap_linearRGB;

    std::string tfFileName = window->saveDirectory + tfPresetFile;
    const std::string stdFileName = window->saveDirectory + "Standard.xml";
    if (tfFileName.empty() || !sgl::FileUtils::get()->exists(tfFileName) || sgl::FileUtils::get()->isDirectory(tfFileName)) {
        tfFileName = stdFileName;
    }
#ifdef SUPPORT_TINYXML2
    if (sgl::FileUtils::get()->exists(tfFileName) && !sgl::FileUtils::get()->isDirectory(tfFileName)) {
        loadTfFromFile(tfFileName);
    } else {
#endif
        colorPoints = {
                sgl::ColorPoint_sRGB(sgl::Color(59, 76, 192), 0.0f),
                sgl::ColorPoint_sRGB(sgl::Color(144, 178, 254), 0.25f),
                sgl::ColorPoint_sRGB(sgl::Color(220, 220, 220), 0.5f),
                sgl::ColorPoint_sRGB(sgl::Color(245, 156, 125), 0.75f),
                sgl::ColorPoint_sRGB(sgl::Color(180, 4, 38), 1.0f)
        };
        opacityPoints = { sgl::OpacityPoint(1.0f, 0.0f), sgl::OpacityPoint(1.0f, 1.0f) };
#ifdef SUPPORT_TINYXML2
    }
#endif
}

#ifdef SUPPORT_TINYXML2
void GuiVarData::writeToXml(XMLPrinter& printer) {
    printer.OpenElement("TransferFunction");
    printer.PushAttribute("colorspace", "sRGB"); // Currently only sRGB supported for points
    printer.PushAttribute("interpolation_colorspace", sgl::COLOR_SPACE_NAMES[interpolationColorSpace]);

    printer.OpenElement("OpacityPoints");
    // Traverse all opacity points
    for (size_t i = 0; i < opacityPoints.size(); i++) {
        printer.OpenElement("OpacityPoint");
        printer.PushAttribute("position", opacityPoints.at(i).position);
        printer.PushAttribute("opacity", opacityPoints.at(i).opacity);
        printer.CloseElement();
    }
    printer.CloseElement();

    printer.OpenElement("ColorPoints");
    printer.PushAttribute("color_data", sgl::COLOR_DATA_MODE_NAMES[int(sgl::COLOR_DATA_MODE_UNSIGNED_SHORT)]);
    // Traverse all color points
    for (size_t i = 0; i < colorPoints.size(); i++) {
        printer.OpenElement("ColorPoint");
        printer.PushAttribute("position", colorPoints.at(i).position);
        printer.PushAttribute("r", (int)colorPoints.at(i).color.getR());
        printer.PushAttribute("g", (int)colorPoints.at(i).color.getG());
        printer.PushAttribute("b", (int)colorPoints.at(i).color.getB());
        printer.CloseElement();
    }
    printer.CloseElement();

    printer.CloseElement();
}
#endif

bool GuiVarData::saveTfToFile(const std::string& filename) {
#ifdef SUPPORT_TINYXML2

#ifdef _MSC_VER
    FILE* file = nullptr;
    errno_t errorCode = fopen_s(&file, filename.c_str(), "w");
    if (errorCode != 0) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in GuiVarData::saveTfToFile: Couldn't create file \"" + filename + "\"!");
        return false;
    }
#else
    FILE* file = fopen(filename.c_str(), "w");
#endif
    if (file == nullptr) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in GuiVarData::saveTfToFile: Couldn't create file \"" + filename + "\"!");
        return false;
    }

    XMLPrinter printer(file);
    writeToXml(printer);

    fclose(file);
    return true;

#else
    sgl::Logfile::get()->writeError(
            std::string() + "Error in GuiVarData::saveTfToFile: TinyXML2 support is disabled.");
    return false;
#endif
}

std::string GuiVarData::serializeXmlString() {
#ifdef SUPPORT_TINYXML2
    XMLPrinter printer;
    writeToXml(printer);
    return std::string(printer.CStr(), printer.CStrSize());
#else
    sgl::Logfile::get()->writeError(
            std::string() + "Error in GuiVarData::serializeXmlString: TinyXML2 support is disabled.");
    return "";
#endif
}

#ifdef SUPPORT_TINYXML2
bool GuiVarData::readFromXml(XMLDocument& doc) {
    XMLElement* tfNode = doc.FirstChildElement("TransferFunction");
    if (tfNode == nullptr) {
        sgl::Logfile::get()->writeError("Error in GuiVarData::readFromXml: No \"TransferFunction\" node found.");
        return false;
    }

    interpolationColorSpace = sgl::COLOR_SPACE_SRGB; // Standard
    const char* interpolationColorSpaceName = tfNode->Attribute("interpolation_colorspace");
    if (interpolationColorSpaceName != nullptr) {
        for (int i = 0; i < 2; i++) {
            if (strcmp(interpolationColorSpaceName, sgl::COLOR_SPACE_NAMES[i]) == 0) {
                interpolationColorSpace = (sgl::ColorSpace)i;
            }
        }
    }

    colorPoints.clear();
    opacityPoints.clear();

    // Traverse all opacity points
    auto opacityPointsNode = tfNode->FirstChildElement("OpacityPoints");
    if (opacityPointsNode != nullptr) {
        for (sgl::XMLIterator it(opacityPointsNode, sgl::XMLNameFilter("OpacityPoint")); it.isValid(); ++it) {
            XMLElement* childElement = *it;
            float position = childElement->FloatAttribute("position");
            float opacity = sgl::clamp(childElement->FloatAttribute("opacity"), 0.0f, 1.0f);
            opacityPoints.emplace_back(opacity, position);
        }
    }

    // Traverse all color points
    auto colorPointsNode = tfNode->FirstChildElement("ColorPoints");
    if (colorPointsNode != nullptr) {
        sgl::ColorDataMode colorDataMode = sgl::COLOR_DATA_MODE_UNSIGNED_BYTE;
        const char* colorDataModeName = colorPointsNode->Attribute("color_data");
        if (colorDataModeName != nullptr) {
            colorDataMode = sgl::parseColorDataModeName(colorDataModeName);
        }
        for (sgl::XMLIterator it(colorPointsNode, sgl::XMLNameFilter("ColorPoint")); it.isValid(); ++it) {
            XMLElement* childElement = *it;
            sgl::Color16 color;
            float position = childElement->FloatAttribute("position");
            if (colorDataMode == sgl::COLOR_DATA_MODE_UNSIGNED_BYTE) {
                int red = sgl::clamp(childElement->IntAttribute("r"), 0, 255);
                int green = sgl::clamp(childElement->IntAttribute("g"), 0, 255);
                int blue = sgl::clamp(childElement->IntAttribute("b"), 0, 255);
                color = sgl::Color(red, green, blue);
            } else if (colorDataMode == sgl::COLOR_DATA_MODE_UNSIGNED_SHORT) {
                int red = sgl::clamp(childElement->IntAttribute("r"), 0, 65535);
                int green = sgl::clamp(childElement->IntAttribute("g"), 0, 65535);
                int blue = sgl::clamp(childElement->IntAttribute("b"), 0, 65535);
                color = sgl::Color16(red, green, blue);
            } else if (colorDataMode == sgl::COLOR_DATA_MODE_FLOAT_NORMALIZED) {
                float red = sgl::clamp(childElement->FloatAttribute("r"), 0.0f, 1.0f);
                float green = sgl::clamp(childElement->FloatAttribute("g"), 0.0f, 1.0f);
                float blue = sgl::clamp(childElement->FloatAttribute("b"), 0.0f, 1.0f);
                color = sgl::Color16(glm::vec3(red, green, blue));
            } else if (colorDataMode == sgl::COLOR_DATA_MODE_FLOAT_255) {
                float red = sgl::clamp(childElement->FloatAttribute("r"), 0.0f, 255.0f) / 255.0f;
                float green = sgl::clamp(childElement->FloatAttribute("g"), 0.0f, 255.0f) / 255.0f;
                float blue = sgl::clamp(childElement->FloatAttribute("b"), 0.0f, 255.0f) / 255.0f;
                color = sgl::Color16(glm::vec3(red, green, blue));
            } else if (colorDataMode == sgl::COLOR_DATA_MODE_FLOAT_100) {
                float red = sgl::clamp(childElement->FloatAttribute("r"), 0.0f, 100.0f) / 100.0f;
                float green = sgl::clamp(childElement->FloatAttribute("g"), 0.0f, 100.0f) / 100.0f;
                float blue = sgl::clamp(childElement->FloatAttribute("b"), 0.0f, 100.0f) / 100.0f;
                color = sgl::Color16(glm::vec3(red, green, blue));
            }
            colorPoints.emplace_back(color, position);
        }
    }

    selectedPointType = sgl::SELECTED_POINT_TYPE_NONE;
    rebuildTransferFunctionMap();
    return true;
}
#endif

bool GuiVarData::loadTfFromFile(const std::string& filename) {
#ifdef SUPPORT_TINYXML2
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != 0) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in GuiVarData::loadTfFromFile: Couldn't open file \""
                + filename + "\".");
        return false;
    }
    return readFromXml(doc);
#else
    sgl::Logfile::get()->writeError(
            std::string() + "Error in GuiVarData::loadTfFromFile: TinyXML2 support is disabled.");
    return false;
#endif
}

bool GuiVarData::loadTfFromXmlString(const std::string &xmlString) {
#ifdef SUPPORT_TINYXML2
    XMLDocument doc;
    if (doc.Parse(xmlString.c_str(), xmlString.size()) != 0) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in GuiVarData::loadTfFromXmlString: Error encountered while parsing data.");
        return false;
    }
    return readFromXml(doc);
#else
    sgl::Logfile::get()->writeError(
            std::string() + "Error in GuiVarData::loadTfFromXmlString: TinyXML2 support is disabled.");
    return false;
#endif
}

bool GuiVarData::deserializeXmlString(const std::string& xmlString) {
#ifdef SUPPORT_TINYXML2
    XMLDocument doc;
    if (doc.Parse(xmlString.c_str(), xmlString.size()) != 0) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in GuiVarData::deserializeXmlString: Couldn't parse passed string.");
        return false;
    }
    return readFromXml(doc);
#else
    sgl::Logfile::get()->writeError(
            std::string() + "Error in GuiVarData::deserializeXmlString: TinyXML2 support is disabled.");
    return false;
#endif
}

void GuiVarData::setAttributeName(int _varIdx, const std::string& name) {
    varIdx = _varIdx;
    attributeName = name;
}

void GuiVarData::setAttributeValues(const std::vector<float>& _attributes) {
    auto [minAttr, maxAttr] = sgl::reduceFloatArrayMinMax(_attributes);
    setAttributeValues(_attributes, minAttr, maxAttr);
}

void GuiVarData::setAttributeValues(const std::vector<float>& _attributes, float minAttribute, float maxAttribute) {
    this->attributes = _attributes;
    this->dataRange = glm::vec2(minAttribute, maxAttribute);
    this->selectedRange = glm::vec2(minAttribute, maxAttribute);
    this->isEmpty = false;
    computeHistogram();
}

void GuiVarData::computeHistogram() {
    if (window->requestHistogramCallback && window->requestHistogramCallback(
            varIdx, histogramResolution, histogram,
            selectedRange.x, selectedRange.y, dataRange.x, dataRange.y,
            recomputeMinMax, isSelectedRangeFixed)) {
        recomputeMinMax = false;
        return;
    }
    if (recomputeMinMax && window->requestAttributeValuesCallback) {
        size_t numAttributes = 0;
        float minVal = std::numeric_limits<float>::max();
        float maxVal = std::numeric_limits<float>::lowest();
        window->requestAttributeValuesCallback(varIdx, nullptr, nullptr, numAttributes, minVal, maxVal);
        dataRange.x = minVal;
        dataRange.y = maxVal;
        if (!isSelectedRangeFixed) {
            selectedRange = dataRange;
        }
        recomputeMinMax = false;
    }
    if (window->requestAttributeValuesCallback) {
        const void* attributesPtr = nullptr;
        ScalarDataFormat fmt = ScalarDataFormat::FLOAT;
        size_t numAttributes = 0;
        float minVal, maxVal;
        window->requestAttributeValuesCallback(
                varIdx, &attributesPtr, &fmt, numAttributes, minVal, maxVal);
        if (fmt == ScalarDataFormat::FLOAT) {
            sgl::computeHistogram(
                    histogram, histogramResolution, static_cast<const float*>(attributesPtr), numAttributes,
                    selectedRange.x, selectedRange.y);
        } else if (fmt == ScalarDataFormat::BYTE) {
            sgl::computeHistogramUnormByte(
                    histogram, histogramResolution, static_cast<const uint8_t*>(attributesPtr), numAttributes,
                    selectedRange.x, selectedRange.y);
        } else if (fmt == ScalarDataFormat::SHORT) {
            sgl::computeHistogramUnormShort(
                    histogram, histogramResolution, static_cast<const uint16_t*>(attributesPtr), numAttributes,
                    selectedRange.x, selectedRange.y);
        } else if (fmt == ScalarDataFormat::FLOAT16) {
            sgl::Logfile::get()->throwError("Error in GuiVarData::computeHistogram: FLOAT16 is not yet supported.");
        } else {
            sgl::Logfile::get()->throwError(
                    "Error in GuiVarData::computeHistogram: Invalid number of bytes per component.");
        }
    } else {
        sgl::computeHistogram(
                histogram, histogramResolution, attributes.data(), attributes.size(),
                selectedRange.x, selectedRange.y);
    }

    /*float histogramsMax = 0;
    histogram = std::vector<float>(histogramResolution, 0.0f);

    if (window->requestAttributeValuesCallback) {
        std::shared_ptr<float[]> attributesSharedPtr;
        size_t numAttributes = 0;
        float minVal, maxVal;
        window->requestAttributeValuesCallback(varIdx, attributesSharedPtr, numAttributes, minVal, maxVal);
        float* attributesPtr = attributesSharedPtr.get();
        for (size_t idx = 0; idx < numAttributes; idx++) {
            float value = attributesPtr[idx];
            if (std::isnan(value)) {
                continue;
            }
            int32_t index = glm::clamp(
                    static_cast<int>((value - selectedRange.x) / (selectedRange.y - selectedRange.x)
                                     * static_cast<float>(histogramResolution)), 0, histogramResolution - 1);
            histogram.at(index)++;
        }
    } else {
        for (const float& value : attributes) {
            int32_t index = glm::clamp(
                    static_cast<int>((value - selectedRange.x) / (selectedRange.y - selectedRange.x)
                                     * static_cast<float>(histogramResolution)), 0, histogramResolution - 1);
            histogram.at(index)++;
        }
    }

    // Normalize values of histogram.
    for (const float& binNumber : histogram) {
        histogramsMax = std::max(histogramsMax, binNumber);
    }

    for (float& numBin : histogram) {
        numBin /= histogramsMax;
    }*/
}

// For OpenGL: Has TRANSFER_FUNCTION_TEXTURE_SIZE entries.
// Get mapped color for normalized attribute by accessing entry at "attr*255".
void GuiVarData::rebuildTransferFunctionMap() {
    rebuildTransferFunctionMapLocal();
    window->rebuildTransferFunctionMap();
}

void GuiVarData::rebuildTransferFunctionMapLocal() {
    // Create linear RGB color points
    colorPoints_LinearRGB.clear();
    for (sgl::ColorPoint_sRGB& colorPoint : colorPoints) {
        glm::vec3 linearRGBColor = sgl::TransferFunctionWindow::sRGBToLinearRGB(colorPoint.color.getFloatColorRGB());
        colorPoints_LinearRGB.push_back(sgl::ColorPoint_LinearRGB(linearRGBColor, colorPoint.position));
    }

    if (interpolationColorSpace == sgl::COLOR_SPACE_LINEAR_RGB) {
        rebuildTransferFunctionMap_LinearRGB();
    } else {
        rebuildTransferFunctionMap_sRGB();
    }
}

// For OpenGL: Has 256 entries. Get mapped color for normalized attribute by accessing entry at "attr*255".
void GuiVarData::rebuildTransferFunctionMap_LinearRGB() {
    int colorPointsIdx = 0;
    int opacityPointsIdx = 0;
    for (size_t i = 0; i < TRANSFER_FUNCTION_TEXTURE_SIZE; i++) {
        glm::vec3 linearRGBColorAtIdx;
        float opacityAtIdx;
        float currentPosition = static_cast<float>(i) / float(TRANSFER_FUNCTION_TEXTURE_SIZE-1);

        // colorPoints.at(colorPointsIdx) should be to the right of/equal to currentPosition
        while (colorPoints_LinearRGB.at(colorPointsIdx).position < currentPosition) {
            colorPointsIdx++;
        }
        while (opacityPoints.at(opacityPointsIdx).position < currentPosition) {
            opacityPointsIdx++;
        }

        // Now compute the color...
        if (colorPoints_LinearRGB.at(colorPointsIdx).position == currentPosition) {
            linearRGBColorAtIdx = colorPoints_LinearRGB.at(colorPointsIdx).color;
        } else {
            glm::vec3 color0 = colorPoints_LinearRGB.at(colorPointsIdx-1).color;
            glm::vec3 color1 = colorPoints_LinearRGB.at(colorPointsIdx).color;
            float pos0 = colorPoints_LinearRGB.at(colorPointsIdx-1).position;
            float pos1 = colorPoints_LinearRGB.at(colorPointsIdx).position;
            float factor = 1.0f - (pos1 - currentPosition) / (pos1 - pos0);
            linearRGBColorAtIdx = glm::mix(color0, color1, factor);
        }

        // ... and the opacity.
        if (opacityPoints.at(opacityPointsIdx).position == currentPosition) {
            opacityAtIdx = opacityPoints.at(opacityPointsIdx).opacity;
        } else {
            float opacity0 = opacityPoints.at(opacityPointsIdx-1).opacity;
            float opacity1 = opacityPoints.at(opacityPointsIdx).opacity;
            float pos0 = opacityPoints.at(opacityPointsIdx-1).position;
            float pos1 = opacityPoints.at(opacityPointsIdx).position;
            float factor = 1.0f - (pos1 - currentPosition) / (pos1 - pos0);
            opacityAtIdx = sgl::interpolateLinear(opacity0, opacity1, factor);
        }

        transferFunctionMap_linearRGB[i] = sgl::Color16(glm::vec4(linearRGBColorAtIdx, opacityAtIdx));
        transferFunctionMap_sRGB[i] = sgl::Color16(glm::vec4(
                sgl::TransferFunctionWindow::linearRGBTosRGB(linearRGBColorAtIdx), opacityAtIdx));
    }
}

// For OpenGL: Has 256 entries. Get mapped color for normalized attribute by accessing entry at "attr*255".
void GuiVarData::rebuildTransferFunctionMap_sRGB() {
    int colorPointsIdx = 0;
    int opacityPointsIdx = 0;
    for (size_t i = 0; i < TRANSFER_FUNCTION_TEXTURE_SIZE; i++) {
        glm::vec3 sRGBColorAtIdx;
        float opacityAtIdx;
        float currentPosition = static_cast<float>(i) / float(TRANSFER_FUNCTION_TEXTURE_SIZE-1);

        // colorPoints.at(colorPointsIdx) should be to the right of/equal to currentPosition
        while (colorPoints.at(colorPointsIdx).position < currentPosition) {
            colorPointsIdx++;
        }
        while (opacityPoints.at(opacityPointsIdx).position < currentPosition) {
            opacityPointsIdx++;
        }

        // Now compute the color...
        if (colorPoints.at(colorPointsIdx).position == currentPosition) {
            sRGBColorAtIdx = colorPoints.at(colorPointsIdx).color.getFloatColorRGB();
        } else {
            glm::vec3 color0 = colorPoints.at(colorPointsIdx-1).color.getFloatColorRGB();
            glm::vec3 color1 = colorPoints.at(colorPointsIdx).color.getFloatColorRGB();
            float pos0 = colorPoints.at(colorPointsIdx-1).position;
            float pos1 = colorPoints.at(colorPointsIdx).position;
            float factor = 1.0f - (pos1 - currentPosition) / (pos1 - pos0);
            sRGBColorAtIdx = glm::mix(color0, color1, factor);
        }

        // ... and the opacity.
        if (opacityPoints.at(opacityPointsIdx).position == currentPosition) {
            opacityAtIdx = opacityPoints.at(opacityPointsIdx).opacity;
        } else {
            float opacity0 = opacityPoints.at(opacityPointsIdx-1).opacity;
            float opacity1 = opacityPoints.at(opacityPointsIdx).opacity;
            float pos0 = opacityPoints.at(opacityPointsIdx-1).position;
            float pos1 = opacityPoints.at(opacityPointsIdx).position;
            float factor = 1.0f - (pos1 - currentPosition) / (pos1 - pos0);
            opacityAtIdx = sgl::interpolateLinear(opacity0, opacity1, factor);
        }

        transferFunctionMap_linearRGB[i] = sgl::Color16(glm::vec4(
                sgl::TransferFunctionWindow::sRGBToLinearRGB(sRGBColorAtIdx), opacityAtIdx));
        transferFunctionMap_sRGB[i] = sgl::Color16(glm::vec4(sRGBColorAtIdx, opacityAtIdx));
    }
}

bool GuiVarData::getIsSelectedRangeFixed() {
    return isSelectedRangeFixed;
}

void GuiVarData::setIsSelectedRangeFixed(bool _isSelectedRangeFixed) {
    isSelectedRangeFixed = _isSelectedRangeFixed;
}

bool GuiVarData::renderGui() {
#ifndef DISABLE_IMGUI
    renderOpacityGraph();
    renderColorBar();

    if (selectedPointType == sgl::SELECTED_POINT_TYPE_OPACITY) {
        if (ImGui::DragFloat("Opacity", &opacitySelection, 0.001f, 0.0f, 1.0f)) {
            opacityPoints.at(currentSelectionIndex).opacity = opacitySelection;
            rebuildTransferFunctionMap();
            reRender = true;
        }
    } else if (selectedPointType == sgl::SELECTED_POINT_TYPE_COLOR) {
        if (ImGui::ColorEdit3("Color", (float*)&colorSelection)) {
            colorPoints.at(currentSelectionIndex).color = sgl::color16FromFloat(
                    colorSelection.x, colorSelection.y, colorSelection.z, colorSelection.w);
            rebuildTransferFunctionMap();
            reRender = true;
        }
    }

    if (ImGui::Combo(
            "Color Space", (int*)&interpolationColorSpace,
            sgl::COLOR_SPACE_NAMES, IM_ARRAYSIZE(sgl::COLOR_SPACE_NAMES))) {
        rebuildTransferFunctionMap();
        reRender = true;
    }

    if (ImGui::SliderFloat2("Range", &selectedRange.x, dataRange.x, dataRange.y)) {
        computeHistogram();
        window->rebuildRangeSsbo();
        reRender = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        selectedRange = dataRange;
        computeHistogram();
        window->rebuildRangeSsbo();
        reRender = true;
    }
    if (window->requestAttributeValuesCallback) {
        ImGui::SameLine();
        ImGui::Checkbox("Fix", &isSelectedRangeFixed);
    }

    if (ImGui::SliderInt("Histogram Res.", &histogramResolution, 1, 256)) {
        computeHistogram();
    }

    renderFileDialog();

    if (reRender) {
        reRender = false;
        return true;
    }
#endif

    return false;
}

void GuiVarData::renderFileDialog() {
#ifndef DISABLE_IMGUI
    // Load file data
    if (ImGui::ListBox("##availablefiles", &selectedFileIndex, [this](void* data, int idx, const char** out_text) -> bool {
        *out_text = window->availableFiles.at(idx).c_str();
        return true;
    }, nullptr, int(window->availableFiles.size()), 4)) {
        saveFileString = window->availableFiles.at(selectedFileIndex);
    } ImVec2 cursorPosEnd = ImGui::GetCursorPos(); ImGui::SameLine();

    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::Text("Available files"); ImGui::SameLine(); ImGui::SetCursorPos(cursorPos + ImVec2(0.0f, 42.0f));
    if (ImGui::Button("Load file") && selectedFileIndex >= 0) {
        loadTfFromFile(window->saveDirectory + window->availableFiles.at(selectedFileIndex));
        reRender = true;
    } ImGui::SetCursorPos(cursorPosEnd);

    // Save file data
    ImGui::InputText("##savefilelabel", &saveFileString); ImGui::SameLine();
    if (ImGui::Button("Save file")) {
        saveTfToFile(window->saveDirectory + saveFileString);
        window->updateAvailableFiles();
    }
#endif
}

void GuiVarData::renderOpacityGraph() {
#ifndef DISABLE_IMGUI
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();
    float regionWidth = ImGui::GetContentRegionAvail().x;
    float graphHeight = 300 * scaleFactor / 1.875f;
    float border = 2*scaleFactor;
    float areaWidth = regionWidth - 2.0f*border;
    float areaHeight = graphHeight - 2.0f*border;
    opacityGraphBox.min = glm::vec2(ImGui::GetCursorScreenPos().x + border, ImGui::GetCursorScreenPos().y + border);
    opacityGraphBox.max = opacityGraphBox.min + glm::vec2(areaWidth, areaHeight);

    sgl::Color& clearColor = window->clearColor;
    ImColor backgroundColor(clearColor.getFloatR(), clearColor.getFloatG(), clearColor.getFloatB());
    ImColor borderColor(
            1.0f - clearColor.getFloatR(), 1.0f - clearColor.getFloatG(), 1.0f - clearColor.getFloatB());

    // First render the graph box
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 cursorPosHistogram = ImGui::GetCursorPos();
    drawList->AddRectFilled(
            ImVec2(startPos.x, startPos.y),
            ImVec2(startPos.x + regionWidth, startPos.y + graphHeight),
            borderColor, ImGui::GetStyle().FrameRounding);
    drawList->AddRectFilled(
            ImVec2(startPos.x + border, startPos.y + border),
            ImVec2(startPos.x + regionWidth - border, startPos.y + graphHeight - border),
            backgroundColor, ImGui::GetStyle().FrameRounding);

    if (ImGui::ClickArea("##grapharea", ImVec2(regionWidth, graphHeight + 2), mouseReleased)) {
        onOpacityGraphClick();
    }
    //ImGui::SetItemAllowOverlap();
    ImGui::SetCursorPos(cursorPosHistogram + ImVec2(border, border));

    ImVec2 oldPadding = ImGui::GetStyle().FramePadding;
    ImGui::GetStyle().FramePadding = ImVec2(1, 1);
    ImGui::PlotHistogram(
            "##histogram", histogram.data(), int(histogram.size()), 0,
            nullptr, 0.0f, 1.0f,
            ImVec2(regionWidth - border * 2, graphHeight - border * 2));
    ImGui::GetStyle().FramePadding = oldPadding;

    // Then render the graph itself
    for (int i = 0; i < (int)opacityPoints.size()-1; i++) {
        float positionX0 = opacityPoints.at(i).position * areaWidth + border;
        float positionX1 = opacityPoints.at(i+1).position * areaWidth + border;
        float positionY0 = (1.0f - opacityPoints.at(i).opacity) * areaHeight + border;
        float positionY1 = (1.0f - opacityPoints.at(i+1).opacity) * areaHeight + border;
        drawList->AddLine(
                ImVec2(startPos.x + positionX0, startPos.y + positionY0),
                ImVec2(startPos.x + positionX1, startPos.y + positionY1),
                borderColor, 1.5f * scaleFactor);
    }

    // Finally, render the points
    for (int i = 0; i < (int)opacityPoints.size(); i++) {
        ImVec2 centerPt = ImVec2(
                startPos.x + border + opacityPoints.at(i).position * areaWidth,
                startPos.y + border + (1.0f - opacityPoints.at(i).opacity) * areaHeight);
        float radius = 4*scaleFactor;
        if (selectedPointType == sgl::SELECTED_POINT_TYPE_OPACITY && i == currentSelectionIndex) {
            radius = 6*scaleFactor;
        }
        drawList->AddCircleFilled(centerPt, radius, backgroundColor, 24);
        drawList->AddCircle(centerPt, radius, borderColor, 24, 1.5f);
    }
#endif
}

void GuiVarData::renderColorBar() {
#ifndef DISABLE_IMGUI
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();
    float regionWidth = ImGui::GetContentRegionAvail().x;
    float barHeight = 30 * scaleFactor / 1.875f;
    colorBarBox.min = glm::vec2(ImGui::GetCursorScreenPos().x + 1, ImGui::GetCursorScreenPos().y + 1);
    colorBarBox.max = colorBarBox.min + glm::vec2(regionWidth - 2, barHeight - 2);

    // Draw bar
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 pos = ImVec2(startPos.x + 1, startPos.y + 1);
    for (size_t i = 0; i < TRANSFER_FUNCTION_TEXTURE_SIZE; i++) {
        sgl::Color16 color = transferFunctionMap_sRGB[i];
        ImU32 colorImgui = ImColor(color.getFloatR(), color.getFloatG(), color.getFloatB());
        drawList->AddLine(ImVec2(pos.x, pos.y), ImVec2(pos.x, pos.y + barHeight), colorImgui, 2.0f * regionWidth / 255.0f);
        pos.x += regionWidth / 255.0f;
    }

    // Draw points
    pos = ImVec2(startPos.x + 2, startPos.y + 2);
    for (int i = 0; i < (int)colorPoints.size(); i++) {
        sgl::Color16 color = colorPoints.at(i).color;
        ImU32 colorImgui = ImColor(color.getFloatR(), color.getFloatG(), color.getFloatB());
        ImU32 colorInvertedImgui = ImColor(1.0f - color.getFloatR(), 1.0f - color.getFloatG(), 1.0f - color.getFloatB());
        ImVec2 centerPt = ImVec2(pos.x + colorPoints.at(i).position * regionWidth, pos.y + barHeight/2);
        float radius = 4*scaleFactor;
        if (selectedPointType == sgl::SELECTED_POINT_TYPE_COLOR && i == currentSelectionIndex) {
            radius = 6*scaleFactor;
        }
        drawList->AddCircleFilled(centerPt, radius, colorImgui, 24);
        drawList->AddCircle(centerPt, radius, colorInvertedImgui, 24);
    }

    if (ImGui::ClickArea("##bararea", ImVec2(regionWidth + 2, barHeight), mouseReleased)) {
        onColorBarClick();
    }
#endif
}

void GuiVarData::onOpacityGraphClick() {
#ifndef DISABLE_IMGUI
    glm::vec2 mousePosWidget = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y) - opacityGraphBox.min;

    glm::vec2 normalizedPosition = mousePosWidget / opacityGraphBox.getDimensions();
    normalizedPosition.y = 1.0f - normalizedPosition.y;
    normalizedPosition = glm::clamp(normalizedPosition, glm::vec2(0), glm::vec2(1));
    dragging = false;

    if (selectNearestOpacityPoint(currentSelectionIndex, mousePosWidget)) {
        // A) Point near to normalized position
        if (ImGui::GetIO().MouseClicked[0]) {
            // A.1 Left clicked? Select/drag-and-drop
            opacitySelection = opacityPoints.at(currentSelectionIndex).opacity;
            selectedPointType = sgl::SELECTED_POINT_TYPE_OPACITY;
            dragging = true;
        } else if (ImGui::GetIO().MouseClicked[1] && currentSelectionIndex != 0
                   && currentSelectionIndex != int(opacityPoints.size())-1) {
            // A.2 Middle clicked? Delete point
            opacityPoints.erase(opacityPoints.begin() + currentSelectionIndex);
            selectedPointType = sgl::SELECTED_POINT_TYPE_NONE;
            reRender = true;
        }
    } else {
        // B) If no point near and left clicked: Create new point at position
        if (ImGui::GetIO().MouseClicked[0]) {
            // Compute insert position for new point
            int insertPosition = 0;
            for (insertPosition = 0; insertPosition < (int)opacityPoints.size(); insertPosition++) {
                if (normalizedPosition.x < opacityPoints.at(insertPosition).position
                    || insertPosition == int(opacityPoints.size())-1) {
                    break;
                }
            }

            // Add new opacity point
            glm::vec2 newPosition = normalizedPosition;
            float newOpacity = newPosition.y;
            opacityPoints.insert(opacityPoints.begin() + insertPosition, sgl::OpacityPoint(newOpacity, newPosition.x));
            currentSelectionIndex = insertPosition;
            opacitySelection = opacityPoints.at(currentSelectionIndex).opacity;
            selectedPointType = sgl::SELECTED_POINT_TYPE_OPACITY;
            dragging = true;
            reRender = true;
        }
    }

    rebuildTransferFunctionMap();
#endif
}

void GuiVarData::onColorBarClick() {
#ifndef DISABLE_IMGUI
    glm::vec2 mousePosWidget = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y) - colorBarBox.min;
    float normalizedPosition = mousePosWidget.x / colorBarBox.getWidth();
    dragging = false;

    if (selectNearestColorPoint(currentSelectionIndex, mousePosWidget)) {
        // A) Point near to normalized position
        if (ImGui::GetIO().MouseClicked[0]) {
            // A.1 Left clicked? Select/drag-and-drop
            sgl::Color16& color16 = colorPoints.at(currentSelectionIndex).color;
            colorSelection = ImColor(color16.getFloatR(), color16.getFloatG(), color16.getFloatB());
            selectedPointType = sgl::SELECTED_POINT_TYPE_COLOR;
            if (currentSelectionIndex != 0 && currentSelectionIndex != int(colorPoints.size())-1) {
                dragging = true;
            }
        } else if (ImGui::GetIO().MouseClicked[1] && currentSelectionIndex != 0
                   && currentSelectionIndex != int(colorPoints.size())-1) {
            // A.2 Middle clicked? Delete point
            colorPoints.erase(colorPoints.begin() + currentSelectionIndex);
            colorPoints_LinearRGB.erase(colorPoints_LinearRGB.begin() + currentSelectionIndex);
            selectedPointType = sgl::SELECTED_POINT_TYPE_NONE;
            reRender = true;
        }
    } else {
        // B) If no point near and left clicked: Create new point at position
        if (ImGui::GetIO().MouseClicked[0]) {
            // Compute insert position for new point
            int insertPosition = 0;
            for (insertPosition = 0; insertPosition < (int)colorPoints.size(); insertPosition++) {
                if (normalizedPosition < colorPoints.at(insertPosition).position
                    || insertPosition == int(colorPoints.size())-1) {
                    break;
                }
            }

            // Add new color point
            float newPosition = normalizedPosition;
            if (interpolationColorSpace == sgl::COLOR_SPACE_LINEAR_RGB) {
                // Linear RGB interplation
                glm::vec3 newColor_linearRGB = glm::mix(
                        colorPoints_LinearRGB.at(insertPosition-1).color,
                        colorPoints_LinearRGB.at(insertPosition).color,
                        1.0f - (colorPoints_LinearRGB.at(insertPosition).position - newPosition)
                              / (colorPoints_LinearRGB.at(insertPosition).position
                                 - colorPoints_LinearRGB.at(insertPosition-1).position));
                sgl::Color16 newColorsRGB(sgl::TransferFunctionWindow::linearRGBTosRGB(newColor_linearRGB));
                colorPoints_LinearRGB.insert(
                        colorPoints_LinearRGB.begin() + insertPosition,
                        sgl::ColorPoint_LinearRGB(newColor_linearRGB, newPosition));
                colorPoints.insert(
                        colorPoints.begin() + insertPosition,
                        sgl::ColorPoint_sRGB(newColorsRGB, newPosition));
            } else {
                // sRGB interpolation
                sgl::Color16 newColor = sgl::color16Lerp(
                        colorPoints.at(insertPosition-1).color,
                        colorPoints.at(insertPosition).color,
                        1.0f - (colorPoints.at(insertPosition).position - newPosition)
                              / (colorPoints.at(insertPosition).position - colorPoints.at(insertPosition-1).position));
                colorPoints.insert(
                        colorPoints.begin() + insertPosition,
                        sgl::ColorPoint_sRGB(newColor, newPosition));
                // colorPoints_LinearRGB computed in @ref rebuildTransferFunctionMap
            }
            currentSelectionIndex = insertPosition;
            sgl::Color16& color16 = colorPoints.at(currentSelectionIndex).color;
            colorSelection = ImColor(color16.getFloatR(), color16.getFloatG(), color16.getFloatB());
            selectedPointType = sgl::SELECTED_POINT_TYPE_COLOR;
            reRender = true;
        }
    }

    rebuildTransferFunctionMap();
#endif
}

void GuiVarData::dragPoint() {
#ifndef DISABLE_IMGUI
    if (mouseReleased) {
        dragging = false;
    }

    glm::vec2 mousePosWidget = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y) - opacityGraphBox.min;
    if (!dragging || mousePosWidget == oldMousePosWidget) {
        oldMousePosWidget = mousePosWidget;
        return;
    }
    oldMousePosWidget = mousePosWidget;

    if (selectedPointType == sgl::SELECTED_POINT_TYPE_OPACITY) {
        glm::vec2 normalizedPosition = mousePosWidget / opacityGraphBox.getDimensions();
        normalizedPosition.y = 1.0f - normalizedPosition.y;
        normalizedPosition = glm::clamp(normalizedPosition, 0.0f, 1.0f);
        if (currentSelectionIndex == 0) {
            normalizedPosition.x = 0.0f;
        }
        if (currentSelectionIndex == int(opacityPoints.size()) - 1) {
            normalizedPosition.x = 1.0f;
        }
        // Clip to neighbors!
        if (currentSelectionIndex != 0
                && normalizedPosition.x < opacityPoints.at(currentSelectionIndex - 1).position) {
            normalizedPosition.x = opacityPoints.at(currentSelectionIndex - 1).position;
        }
        if (currentSelectionIndex != int(opacityPoints.size()) - 1
                && normalizedPosition.x > opacityPoints.at(currentSelectionIndex + 1).position) {
            normalizedPosition.x = opacityPoints.at(currentSelectionIndex + 1).position;
        }
        opacityPoints.at(currentSelectionIndex).position = normalizedPosition.x;
        opacityPoints.at(currentSelectionIndex).opacity = normalizedPosition.y;
        opacitySelection = opacityPoints.at(currentSelectionIndex).opacity;
    }

    if (selectedPointType == sgl::SELECTED_POINT_TYPE_COLOR) {
        float normalizedPosition = mousePosWidget.x / colorBarBox.getWidth();
        normalizedPosition = glm::clamp(normalizedPosition, 0.0f, 1.0f);
        // Clip to neighbors!
        if (currentSelectionIndex != 0
                && normalizedPosition < colorPoints.at(currentSelectionIndex - 1).position) {
            normalizedPosition = colorPoints.at(currentSelectionIndex - 1).position;
        }
        if (currentSelectionIndex != int(colorPoints.size()) - 1
                && normalizedPosition > colorPoints.at(currentSelectionIndex + 1).position) {
            normalizedPosition = colorPoints.at(currentSelectionIndex + 1).position;
        }
        colorPoints.at(currentSelectionIndex).position = normalizedPosition;
    }

    rebuildTransferFunctionMap();
    reRender = true;
#endif
}

bool GuiVarData::selectNearestOpacityPoint(int& currentSelectionIndex, const glm::vec2& mousePosWidget) {
#ifndef DISABLE_IMGUI
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();

    int closestPointIdx = -1;
    float closestDistance = std::numeric_limits<float>::max();
    for (int i = 0; i < (int)opacityPoints.size(); i++) {
        glm::vec2 centerPt = glm::vec2(
                opacityPoints.at(i).position * opacityGraphBox.getWidth(),
                (1.0f - opacityPoints.at(i).opacity) * opacityGraphBox.getHeight());
        float currentDistance = glm::length(centerPt - mousePosWidget);
        if (currentDistance < scaleFactor * 10.0f && currentDistance < closestDistance) {
            closestPointIdx = i;
            closestDistance = currentDistance;
        }
    }

    if (closestPointIdx >= 0) {
        currentSelectionIndex = closestPointIdx;
        return true;
    }
#endif

    return false;
}

bool GuiVarData::selectNearestColorPoint(int& currentSelectionIndex, const glm::vec2& mousePosWidget) {
#ifndef DISABLE_IMGUI
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();

    int closestPointIdx = -1;
    float closestDistance = std::numeric_limits<float>::max();
    for (int i = 0; i < (int)colorPoints.size(); i++) {
        ImVec2 centerPt = ImVec2(
                colorPoints.at(i).position * colorBarBox.getWidth(),
                colorBarBox.getHeight()/2);
        float currentDistance = glm::abs(centerPt.x - mousePosWidget.x);
        if (currentDistance < scaleFactor * 10.0f && currentDistance < closestDistance) {
            closestPointIdx = i;
            closestDistance = currentDistance;
        }
    }

    if (closestPointIdx >= 0) {
        currentSelectionIndex = closestPointIdx;
        return true;
    }
#endif

    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

MultiVarTransferFunctionWindow::MultiVarTransferFunctionWindow(
        const std::string& saveDirectoryPrefix,
        const std::vector<std::string>& tfPresetFiles) {
    parentDirectory = sgl::AppSettings::get()->getDataDirectory();
    saveDirectory = sgl::AppSettings::get()->getDataDirectory() + "TransferFunctions/";

    if (!saveDirectoryPrefix.empty()) {
        directoryName = saveDirectoryPrefix;
        parentDirectory = saveDirectory;
        saveDirectory = saveDirectory + saveDirectoryPrefix + "/";
    }
    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectory);
    this->tfPresetFiles = tfPresetFiles;

    directoryContentWatch.setPath(saveDirectory, true);
    directoryContentWatch.initialize();

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == sgl::RenderSystem::OPENGL) {
        tfMapTextureSettings.type = sgl::TEXTURE_1D_ARRAY;
        tfMapTextureSettings.internalFormat = GL_RGBA16;
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice()) {
        tfMapImageSettingsVulkan.imageType = VK_IMAGE_TYPE_1D;
        tfMapImageSettingsVulkan.format = VK_FORMAT_R16G16B16A16_UNORM;
    }
#endif

    updateAvailableFiles();
}

MultiVarTransferFunctionWindow::MultiVarTransferFunctionWindow(const std::vector<std::string>& tfPresetFiles)
        : MultiVarTransferFunctionWindow("", tfPresetFiles) {}

MultiVarTransferFunctionWindow::~MultiVarTransferFunctionWindow() = default;

void MultiVarTransferFunctionWindow::setAttributesValues(
        const std::vector<std::string>& names,
        const std::vector<std::vector<float>>& allAttributes,
        size_t defaultVarIndex) {
    assert(names.size() == allAttributes.size());
    useAttributeArrays = true;
    setAttributeNames(names, defaultVarIndex);

    for (size_t varIdx = 0; varIdx < names.size(); varIdx++) {
        GuiVarData& varData = guiVarData.at(varIdx);
        varData.setAttributeValues(allAttributes.at(varIdx));
    }

    rebuildTransferFunctionMapComplete();
    rebuildRangeSsbo();
}

void MultiVarTransferFunctionWindow::recreateTfMapTexture() {
#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == sgl::RenderSystem::OPENGL) {
        tfMapTexture = sgl::TextureManager->createEmptyTexture(
                TRANSFER_FUNCTION_TEXTURE_SIZE, int(varNames.size()), tfMapTextureSettings);
        minMaxSsbo = sgl::Renderer->createGeometryBuffer(
                varNames.size() * sizeof(glm::vec2), sgl::SHADER_STORAGE_BUFFER);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice()) {
        tfMapImageSettingsVulkan.width = TRANSFER_FUNCTION_TEXTURE_SIZE;
        tfMapImageSettingsVulkan.arrayLayers = uint32_t(varNames.size());
        tfMapTextureVulkan = std::make_shared<sgl::vk::Texture>(
                sgl::AppSettings::get()->getPrimaryDevice(), tfMapImageSettingsVulkan,
                VK_IMAGE_VIEW_TYPE_1D_ARRAY);
        minMaxSsboVulkan = std::make_shared<sgl::vk::Buffer>(
                sgl::AppSettings::get()->getPrimaryDevice(), varNames.size() * sizeof(glm::vec2),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
    }
#endif
}

void MultiVarTransferFunctionWindow::setAttributeNames(const std::vector<std::string>& names, size_t defaultVarIndex) {
    varNames = names;
    transferFunctionMap_sRGB.resize(TRANSFER_FUNCTION_TEXTURE_SIZE * names.size());
    transferFunctionMap_linearRGB.resize(TRANSFER_FUNCTION_TEXTURE_SIZE * names.size());
    selectedVarIndex = defaultVarIndex;

    if (guiVarData.size() != names.size()) {
        guiVarData.clear();
        guiVarData.reserve(names.size());
        dirtyIndices.resize(names.size());

        recreateTfMapTexture();

        minMaxData.clear();
        minMaxData.resize(names.size() * 2, 0.0f);

        for (size_t varIdx = 0; varIdx < names.size(); varIdx++) {
            guiVarData.emplace_back(
                    this, tfPresetFiles.empty() ? "" : tfPresetFiles.at(varIdx % tfPresetFiles.size()),
                    &transferFunctionMap_sRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdx),
                    &transferFunctionMap_linearRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdx)
            );
        }
    }

    for (size_t varIdx = 0; varIdx < names.size(); varIdx++) {
        GuiVarData& varData = guiVarData.at(varIdx);
        varData.setAttributeName(int(varIdx), names.at(varIdx));
    }

    if (!guiVarData.empty()) {
        currVarData = &guiVarData.at(selectedVarIndex);
        if (requestAttributeValuesCallback && currVarData->isEmpty) {
            setAttributeDataDirty(int(selectedVarIndex));
        }
    }

    if (!useAttributeArrays) {
        rebuildTransferFunctionMapComplete();
    }
    updateAvailableFiles();
}

void MultiVarTransferFunctionWindow::setAttributeDataDirty(int varIdx) {
    GuiVarData& varData = guiVarData.at(varIdx);
    varData.isEmpty = true;
    if (int(selectedVarIndex) == varIdx) {
        loadAttributeDataIfEmpty(varIdx);
    }
}

void MultiVarTransferFunctionWindow::loadAttributeDataIfEmpty(int varIdx) {
    GuiVarData& varData = guiVarData.at(varIdx);
    if (varData.isEmpty) {
        varData.isEmpty = false;
        varData.recomputeMinMax = true;
        varData.computeHistogram();
        varData.rebuildTransferFunctionMap();
        rebuildRangeSsbo();
    }
}

void MultiVarTransferFunctionWindow::updateAttributeName(int varIdx, const std::string& attributeName) {
    GuiVarData& varData = guiVarData.at(varIdx);
    varData.setAttributeName(varIdx, attributeName);
    varNames.at(varIdx) = attributeName;
}

void MultiVarTransferFunctionWindow::removeAttribute(int varIdxRemove) {
    size_t numVarsNew = varNames.size() - 1;
    varNames.erase(varNames.begin() + varIdxRemove);
    guiVarData.erase(guiVarData.begin() + varIdxRemove);
    minMaxData.erase(minMaxData.begin() + varIdxRemove * 2, minMaxData.begin() + varIdxRemove * 2 + 2);
    dirtyIndices.erase(dirtyIndices.begin() + varIdxRemove);

    ptrdiff_t trafoRangeBegin = varIdxRemove * ptrdiff_t(TRANSFER_FUNCTION_TEXTURE_SIZE);
    ptrdiff_t trafoRangeEnd = trafoRangeBegin + ptrdiff_t(TRANSFER_FUNCTION_TEXTURE_SIZE);
    transferFunctionMap_sRGB.erase(
            transferFunctionMap_sRGB.begin() + trafoRangeBegin,
            transferFunctionMap_sRGB.begin() + trafoRangeEnd);
    transferFunctionMap_linearRGB.erase(
            transferFunctionMap_linearRGB.begin() + trafoRangeBegin,
            transferFunctionMap_linearRGB.begin() + trafoRangeEnd);

    for (size_t varIdx = 0; varIdx < numVarsNew; varIdx++) {
        guiVarData.at(varIdx).transferFunctionMap_sRGB =
                &transferFunctionMap_sRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdx);
        guiVarData.at(varIdx).transferFunctionMap_linearRGB =
                &transferFunctionMap_linearRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdx);
        guiVarData.at(varIdx).varIdx = int(varIdx);
    }

    if (selectedVarIndex == numVarsNew) {
        selectedVarIndex--;
    }
    currVarData = &guiVarData.at(selectedVarIndex);

    recreateTfMapTexture();
    rebuildTransferFunctionMapComplete();
    rebuildRangeSsbo();
}

void MultiVarTransferFunctionWindow::addAttributeName(const std::string& name) {
    size_t varIdxNew = varNames.size();
    varNames.push_back(name);
    size_t numVarsNew = varNames.size();
    minMaxData.resize(numVarsNew * 2, 0.0f);

    transferFunctionMap_sRGB.resize(TRANSFER_FUNCTION_TEXTURE_SIZE * numVarsNew);
    transferFunctionMap_linearRGB.resize(TRANSFER_FUNCTION_TEXTURE_SIZE * numVarsNew);

    guiVarData.emplace_back(
            this, tfPresetFiles.empty() ? "" : tfPresetFiles.at(varIdxNew % tfPresetFiles.size()),
            &transferFunctionMap_sRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdxNew),
            &transferFunctionMap_linearRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdxNew));
    guiVarData.back().setAttributeName(int(guiVarData.size() - 1), name);
    for (size_t varIdx = 0; varIdx < varIdxNew; varIdx++) {
        guiVarData.at(varIdx).transferFunctionMap_sRGB =
                &transferFunctionMap_sRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdx);
        guiVarData.at(varIdx).transferFunctionMap_linearRGB =
                &transferFunctionMap_linearRGB.at(TRANSFER_FUNCTION_TEXTURE_SIZE * varIdx);
    }
    currVarData = &guiVarData.at(selectedVarIndex);
    dirtyIndices.emplace_back(true);

    recreateTfMapTexture();
    rebuildTransferFunctionMapComplete();
    rebuildRangeSsbo();
}

bool MultiVarTransferFunctionWindow::getIsSelectedRangeFixed(int varIdx) {
    return guiVarData.at(varIdx).isSelectedRangeFixed;
}

void MultiVarTransferFunctionWindow::setIsSelectedRangeFixed(int varIdx, bool _isSelectedRangeFixed) {
    guiVarData.at(varIdx).setIsSelectedRangeFixed(_isSelectedRangeFixed);
}

bool MultiVarTransferFunctionWindow::loadFunctionFromFile(int varIdx, const std::string& filename) {
    if (size_t(varIdx) >= guiVarData.size()) {
        sgl::Logfile::get()->writeError(
                "MultiVarTransferFunctionWindow::loadFunctionFromFile: varIdx >= guiVarData.size()");
        return false;
    }
    GuiVarData& varData = guiVarData.at(varIdx);
    return varData.loadTfFromFile(filename);
}

bool MultiVarTransferFunctionWindow::loadFunctionFromXmlString(int varIdx, const std::string& xmlString) {
    if (size_t(varIdx) >= guiVarData.size()) {
        sgl::Logfile::get()->writeError(
                "MultiVarTransferFunctionWindow::loadFunctionFromXmlString: varIdx >= guiVarData.size()");
        return false;
    }
    GuiVarData& varData = guiVarData.at(varIdx);
    return varData.loadTfFromXmlString(xmlString);
}


bool MultiVarTransferFunctionWindow::loadFromTfNameList(const std::vector<std::string>& tfNames) {
    if (tfNames.size() != guiVarData.size()) {
        sgl::Logfile::get()->writeError(
                "MultiVarTransferFunctionWindow::loadFromTfNameList: tfNames.size() != guiVarData.size()");
        return false;
    }

    bool succeeded = true;
    for (size_t varIdx = 0; varIdx < tfNames.size(); varIdx++) {
        GuiVarData& varData = guiVarData.at(varIdx);
        if (!varData.loadTfFromFile(saveDirectory + tfNames.at(varIdx))) {
            succeeded = false;
        }
    }
    return succeeded;
}

std::string MultiVarTransferFunctionWindow::serializeXmlString(int varIdx) {
    return guiVarData.at(varIdx).serializeXmlString();
}

bool MultiVarTransferFunctionWindow::deserializeXmlString(int varIdx, const std::string &xmlString) {
    return guiVarData.at(varIdx).deserializeXmlString(xmlString);
}

void MultiVarTransferFunctionWindow::updateAvailableFiles() {
    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectory);
    std::vector<std::string> availableFilesAll = sgl::FileUtils::get()->getFilesInDirectoryVector(saveDirectory);
    availableFiles.clear();
    availableFiles.reserve(availableFilesAll.size());

    for (const std::string& filename : availableFilesAll) {
        if (sgl::FileUtils::get()->hasExtension(filename.c_str(), ".xml")) {
            availableFiles.push_back(filename);
        }
    }
    sgl::FileUtils::get()->sortPathStrings(availableFiles);

    // Update currently selected filename
    for (size_t i = 0; i < availableFiles.size(); i++) {
        availableFiles.at(i) = sgl::FileUtils::get()->getPureFilename(availableFiles.at(i));
        for (GuiVarData& varData : guiVarData) {
            if (availableFiles.at(i) == varData.getSaveFileString()) {
                varData.selectedFileIndex = (int)i;
            }
        }
    }
}

void MultiVarTransferFunctionWindow::setClearColor(const sgl::Color& _clearColor) {
    this->clearColor = _clearColor;
}

#ifdef SUPPORT_OPENGL
sgl::TexturePtr& MultiVarTransferFunctionWindow::getTransferFunctionMapTexture() {
    return tfMapTexture;
}
#endif
#ifdef SUPPORT_VULKAN
sgl::vk::TexturePtr& MultiVarTransferFunctionWindow::getTransferFunctionMapTextureVulkan() {
    return tfMapTextureVulkan;
}
#endif

bool MultiVarTransferFunctionWindow::getTransferFunctionMapRebuilt() {
    if (transferFunctionMapRebuilt) {
        // Reset the flag
        transferFunctionMapRebuilt = false;
        return true;
    }
    return false;
}

bool MultiVarTransferFunctionWindow::getIsVariableDirty(int varIdx) {
    return dirtyIndices.at(varIdx);
}

void MultiVarTransferFunctionWindow::resetDirty() {
    for (size_t i = 0; i < dirtyIndices.size(); i++) {
        dirtyIndices.at(i) = false;
    }
}

std::vector<sgl::Color16> MultiVarTransferFunctionWindow::getTransferFunctionMap_sRGB(int varIdx) {
    return std::vector<sgl::Color16>(
            transferFunctionMap_sRGB.cbegin() + int(TRANSFER_FUNCTION_TEXTURE_SIZE) * varIdx,
            transferFunctionMap_sRGB.cbegin() + int(TRANSFER_FUNCTION_TEXTURE_SIZE) * (varIdx + 1));
}

std::vector<glm::vec4> MultiVarTransferFunctionWindow::getTransferFunctionMap_sRGBDownscaled(
        int varIdx, int numEntries) {
    std::vector<glm::vec4> colorsSubsampled;
    colorsSubsampled.reserve(numEntries);
    int idxOffset = int(TRANSFER_FUNCTION_TEXTURE_SIZE) * varIdx;
    auto Ni = float(TRANSFER_FUNCTION_TEXTURE_SIZE - 1);
    auto Nj = float(numEntries - 1);
    for (int j = 0; j < numEntries; j++) {
        float t = float(j) / Nj;
        float t0 = std::floor(t * Ni);
        float t1 = std::ceil(t * Ni);
        float f = t * Ni - t0;
        int i0 = int(t0);
        int i1 = int(t1);
        glm::vec4 c0 = transferFunctionMap_sRGB.at(idxOffset + i0).getFloatColorRGBA();
        glm::vec4 c1 = transferFunctionMap_sRGB.at(idxOffset + i1).getFloatColorRGBA();
        colorsSubsampled.push_back(glm::mix(c0, c1, f));
    }
    return colorsSubsampled;
}

std::vector<glm::vec4> MultiVarTransferFunctionWindow::getTransferFunctionMap_sRGBPremulDownscaled(
        int varIdx, int numEntries) {
    std::vector<glm::vec4> colorsSubsampled;
    colorsSubsampled.reserve(numEntries);
    int idxOffset = int(TRANSFER_FUNCTION_TEXTURE_SIZE) * varIdx;
    auto Ni = float(TRANSFER_FUNCTION_TEXTURE_SIZE - 1);
    auto Nj = float(numEntries - 1);
    for (int j = 0; j < numEntries; j++) {
        float t = float(j) / Nj;
        float t0 = std::floor(t * Ni);
        float t1 = std::ceil(t * Ni);
        float f = t * Ni - t0;
        int i0 = int(t0);
        int i1 = int(t1);
        glm::vec4 c0 = transferFunctionMap_sRGB.at(idxOffset + i0).getFloatColorRGBA();
        glm::vec4 c1 = transferFunctionMap_sRGB.at(idxOffset + i1).getFloatColorRGBA();
        c0.r *= c0.a;
        c0.g *= c0.a;
        c0.b *= c0.a;
        c1.r *= c1.a;
        c1.g *= c1.a;
        c1.b *= c1.a;
        colorsSubsampled.push_back(glm::mix(c0, c1, f));
    }
    return colorsSubsampled;
}

void MultiVarTransferFunctionWindow::setTransferFunction(
        int varIdx, const std::vector<OpacityPoint>& opacityPoints,
        const std::vector<sgl::ColorPoint_sRGB>& colorPoints,
        ColorSpace interpolationColorSpace) {
    GuiVarData& varData = guiVarData.at(varIdx);
    varData.selectedPointType = sgl::SELECTED_POINT_TYPE_NONE;
    varData.interpolationColorSpace = interpolationColorSpace;
    varData.opacityPoints = opacityPoints;
    varData.colorPoints = colorPoints;
    varData.rebuildTransferFunctionMap();
    dirtyIndices.at(varIdx) = true;
    reRender = true;
}

void MultiVarTransferFunctionWindow::update(float dt) {
    directoryContentWatch.update([this] { this->updateAvailableFiles(); });
    if (currVarData) {
        currVarData->dragPoint();
    }
}

void MultiVarTransferFunctionWindow::setUseLinearRGB(bool _useLinearRGB) {
    this->useLinearRGB = _useLinearRGB;
    rebuildTransferFunctionMapComplete();
}

void MultiVarTransferFunctionWindow::rebuildTransferFunctionMapComplete() {
    for (GuiVarData& varData : guiVarData) {
        varData.rebuildTransferFunctionMapLocal();
    }

    rebuildTransferFunctionMap();
}

void MultiVarTransferFunctionWindow::rebuildRangeSsbo() {
#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == sgl::RenderSystem::OPENGL && !minMaxSsbo) {
        return;
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice() && !minMaxSsboVulkan) {
        return;
    }
#endif

    for (size_t varIdx = 0; varIdx < guiVarData.size(); varIdx++) {
        GuiVarData& varData = guiVarData.at(varIdx);
        const glm::vec2& range = varData.getSelectedRange();
        minMaxData[varIdx * 2] = range.x;
        minMaxData[varIdx * 2 + 1] = range.y;
    }

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == sgl::RenderSystem::OPENGL) {
        minMaxSsbo->subData(0, minMaxData.size() * sizeof(float), minMaxData.data());
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice()) {
        minMaxSsboVulkan->uploadData(minMaxData.size() * sizeof(float), minMaxData.data());
    }
#endif
}

void MultiVarTransferFunctionWindow::rebuildTransferFunctionMap() {
    transferFunctionMapRebuilt = true;

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice() && !tfMapTextureVulkan) {
        return;
    }
#endif
#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == sgl::RenderSystem::OPENGL && !tfMapTexture) {
        return;
    }
#endif

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == sgl::RenderSystem::OPENGL) {
        sgl::PixelFormat pixelFormat;
        pixelFormat.pixelType = GL_UNSIGNED_SHORT;
        if (useLinearRGB) {
            tfMapTexture->uploadPixelData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE, int(varNames.size()),
                    transferFunctionMap_linearRGB.data(), pixelFormat);
        } else {
            tfMapTexture->uploadPixelData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE, int(varNames.size()),
                    transferFunctionMap_sRGB.data(), pixelFormat);
        }
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice()) {
        if (useLinearRGB) {
            tfMapTextureVulkan->getImage()->uploadData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE * uint32_t(varNames.size()) * 8,
                    transferFunctionMap_linearRGB.data());
        } else {
            tfMapTextureVulkan->getImage()->uploadData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE * uint32_t(varNames.size()) * 8,
                    transferFunctionMap_sRGB.data());
        }
    }
#endif
}

bool MultiVarTransferFunctionWindow::renderGui() {
#ifndef DISABLE_IMGUI
    sgl::ImGuiWrapper::get()->setNextWindowStandardPosSize(2, 1278, 634, 818);
    if (showWindow && !varNames.empty()) {
        if (ImGui::Begin("Multi-Var Transfer Function", &showWindow)) {
            if (ImGui::BeginCombo("Variable", varNames.at(selectedVarIndex).c_str())) {
                for (size_t i = 0; i < varNames.size(); ++i) {
                    if (ImGui::Selectable(varNames.at(i).c_str(), selectedVarIndex == i)) {
                        selectedVarIndex = i;
                        if (!guiVarData.empty()) {
                            currVarData = &guiVarData.at(selectedVarIndex);
                            if (requestAttributeValuesCallback && currVarData->isEmpty) {
                                setAttributeDataDirty(int(selectedVarIndex));
                            }
                        }
                    }
                }
                ImGui::EndCombo();
            }

            if (currVarData) {
                if (currVarData->renderGui()) {
                    dirtyIndices.at(selectedVarIndex) = true;
                    reRender = true;
                }
            }
        }
        ImGui::End();
    }

    if (reRender) {
        reRender = false;
        return true;
    }
#endif

    return false;
}

}
