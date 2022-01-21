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

#include <cmath>
#include <glm/glm.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_custom.h>
#include <ImGui/imgui_stdlib.h>

#include <Utils/AppSettings.hpp>
#include <Utils/XML.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
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
#include "TransferFunctionWindow.hpp"

using namespace tinyxml2;

namespace sgl {

ColorDataMode parseColorDataModeName(const std::string& dataModeName) {
    for (size_t i = 0; i < NUM_COLOR_DATA_MODES; i++) {
        if (dataModeName == COLOR_DATA_MODE_NAMES[i]) {
            return ColorDataMode(i);
        }
    }
    return COLOR_DATA_MODE_FLOAT_255;
}


const size_t TRANSFER_FUNCTION_TEXTURE_SIZE = 256;

TransferFunctionWindow::TransferFunctionWindow() {
    colorPoints = {
            ColorPoint_sRGB(sgl::Color(59, 76, 192), 0.0f),
            ColorPoint_sRGB(sgl::Color(144, 178, 254), 0.25f),
            ColorPoint_sRGB(sgl::Color(220, 220, 220), 0.5f),
            ColorPoint_sRGB(sgl::Color(245, 156, 125), 0.75f),
            ColorPoint_sRGB(sgl::Color(180, 4, 38), 1.0f)
    };
    interpolationColorSpace = COLOR_SPACE_LINEAR_RGB;
    opacityPoints = { OpacityPoint(1.0f, 0.0f), OpacityPoint(1.0f, 1.0f) };
    transferFunctionMap_sRGB.resize(TRANSFER_FUNCTION_TEXTURE_SIZE);
    transferFunctionMap_linearRGB.resize(TRANSFER_FUNCTION_TEXTURE_SIZE);
#ifdef SUPPORT_OPENGL
    if (AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        tfMapTextureSettings.type = sgl::TEXTURE_1D;
        tfMapTextureSettings.internalFormat = GL_RGBA16;
        tfMapTexture = sgl::TextureManager->createEmptyTexture(TRANSFER_FUNCTION_TEXTURE_SIZE, tfMapTextureSettings);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (AppSettings::get()->getPrimaryDevice()) {
        tfMapImageSettingsVulkan.imageType = VK_IMAGE_TYPE_1D;
        tfMapImageSettingsVulkan.format = VK_FORMAT_R16G16B16A16_UNORM;
        tfMapImageSettingsVulkan.width = TRANSFER_FUNCTION_TEXTURE_SIZE;
        tfMapTextureVulkan = std::make_shared<sgl::vk::Texture>(
                AppSettings::get()->getPrimaryDevice(), tfMapImageSettingsVulkan);
        minMaxUboVulkan = std::make_shared<sgl::vk::Buffer>(
                sgl::AppSettings::get()->getPrimaryDevice(), sizeof(glm::vec2),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
    }
#endif

    saveDirectory = sgl::AppSettings::get()->getDataDirectory() + "TransferFunctions/";
    directoryContentWatch.setPath(saveDirectory, true);
    directoryContentWatch.initialize();

    updateAvailableFiles();
    rebuildTransferFunctionMap();
    rebuildRangeUbo();

    if (sgl::FileUtils::get()->exists(saveDirectory + "Standard.xml")) {
        loadFunctionFromFile(saveDirectory + "Standard.xml");
    }
}

bool TransferFunctionWindow::saveFunctionToFile(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "w");
    if (file == nullptr) {
        sgl::Logfile::get()->writeError(std::string()
                + "ERROR: TransferFunctionWindow::saveFunctionToFile: Couldn't create file \"" + filename + "\"!");
        return false;
    }

    XMLPrinter printer(file);
    printer.OpenElement("TransferFunction");
    printer.PushAttribute("colorspace", "sRGB"); // Currently only sRGB supported for points
    printer.PushAttribute("interpolation_colorspace", COLOR_SPACE_NAMES[interpolationColorSpace]);

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
    printer.PushAttribute("color_data", COLOR_DATA_MODE_NAMES[int(COLOR_DATA_MODE_UNSIGNED_SHORT)]);
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

    fclose(file);
    return true;
}

bool TransferFunctionWindow::loadFunctionFromFile(const std::string& filename) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != 0) {
        sgl::Logfile::get()->writeError(std::string()
                + "TransferFunctionWindow::loadFunctionFromFile: Couldn't open file \"" + filename + "\"!");
        return false;
    }
    XMLElement* tfNode = doc.FirstChildElement("TransferFunction");
    if (tfNode == nullptr) {
        sgl::Logfile::get()->writeError("TransferFunctionWindow::loadFunctionFromFile: No \"TransferFunction\" node found!");
        return false;
    }

    interpolationColorSpace = COLOR_SPACE_SRGB; // Standard
    const char* interpolationColorSpaceName = tfNode->Attribute("colorspace_interpolation");
    if (interpolationColorSpaceName != nullptr) {
        for (int i = 0; i < 2; i++) {
            if (strcmp(interpolationColorSpaceName, COLOR_SPACE_NAMES[interpolationColorSpace]) == 0) {
                interpolationColorSpace = (ColorSpace)i;
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
        ColorDataMode colorDataMode = COLOR_DATA_MODE_UNSIGNED_BYTE;
        const char* colorDataModeName = colorPointsNode->Attribute("color_data");
        if (colorDataModeName != nullptr) {
            colorDataMode = parseColorDataModeName(colorDataModeName);
        }
        for (sgl::XMLIterator it(colorPointsNode, sgl::XMLNameFilter("ColorPoint")); it.isValid(); ++it) {
            XMLElement* childElement = *it;
            sgl::Color16 color;
            float position = childElement->FloatAttribute("position");
            if (colorDataMode == COLOR_DATA_MODE_UNSIGNED_BYTE) {
                int red = sgl::clamp(childElement->IntAttribute("r"), 0, 255);
                int green = sgl::clamp(childElement->IntAttribute("g"), 0, 255);
                int blue = sgl::clamp(childElement->IntAttribute("b"), 0, 255);
                color = sgl::Color(red, green, blue);
            } else if (colorDataMode == COLOR_DATA_MODE_UNSIGNED_SHORT) {
                int red = sgl::clamp(childElement->IntAttribute("r"), 0, 65535);
                int green = sgl::clamp(childElement->IntAttribute("g"), 0, 65535);
                int blue = sgl::clamp(childElement->IntAttribute("b"), 0, 65535);
                color = sgl::Color16(red, green, blue);
            } else if (colorDataMode == COLOR_DATA_MODE_FLOAT_NORMALIZED) {
                float red = sgl::clamp(childElement->FloatAttribute("r"), 0.0f, 1.0f);
                float green = sgl::clamp(childElement->FloatAttribute("g"), 0.0f, 1.0f);
                float blue = sgl::clamp(childElement->FloatAttribute("b"), 0.0f, 1.0f);
                color = sgl::Color16(glm::vec3(red, green, blue));
            } else if (colorDataMode == COLOR_DATA_MODE_FLOAT_255) {
                float red = sgl::clamp(childElement->FloatAttribute("r"), 0.0f, 255.0f) / 255.0f;
                float green = sgl::clamp(childElement->FloatAttribute("g"), 0.0f, 255.0f) / 255.0f;
                float blue = sgl::clamp(childElement->FloatAttribute("b"), 0.0f, 255.0f) / 255.0f;
                color = sgl::Color16(glm::vec3(red, green, blue));
            }
            colorPoints.emplace_back(color, position);
        }
    }

    selectedPointType = SELECTED_POINT_TYPE_NONE;
    rebuildTransferFunctionMap();
    return true;
}

void TransferFunctionWindow::updateAvailableFiles() {
    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectory);
    std::vector<std::string> availableFilesAll = sgl::FileUtils::get()->getFilesInDirectoryVector(saveDirectory);
    availableFiles.clear();
    availableFiles.reserve(availableFilesAll.size());

    for (const std::string& filename : availableFilesAll) {
        if (sgl::FileUtils::get()->hasExtension(filename.c_str(), ".xml")) {
            availableFiles.push_back(filename);
        }
    }
    //std::sort(availableFiles.begin(), availableFiles.end());
    sgl::FileUtils::get()->sortPathStrings(availableFiles);

    // Update currently selected filename
    for (size_t i = 0; i < availableFiles.size(); i++) {
        availableFiles.at(i) = sgl::FileUtils::get()->getPureFilename(availableFiles.at(i));
        if (availableFiles.at(i) == saveFileString) {
            selectedFileIndex = (int)i;
        }
    }
}


void TransferFunctionWindow::setClearColor(const sgl::Color& clearColor) {
    this->clearColor = clearColor;
}

void TransferFunctionWindow::setHistogram(const std::vector<int>& occurences) {
    histogramResolution = static_cast<int>(occurences.size());
    histogram.clear();
    histogram.resize(histogramResolution);
    for (size_t i = 0; i < occurences.size(); i++) {
        histogram.at(i) = float(occurences.at(i));
    }

    float maxNum = 1.0f;
    for (float num : histogram) {
        maxNum = std::max(num, maxNum);
    }

    for (float& num : histogram) {
        num /= maxNum;
    }
}

void TransferFunctionWindow::computeHistogram(const std::vector<float>& attributes) {
    this->attributes = attributes;
    float minAttr = std::numeric_limits<float>::max();
    float maxAttr = std::numeric_limits<float>::lowest();
#if _OPENMP >= 201107
    #pragma omp parallel for reduction(min: minAttr) reduction(max: maxAttr) shared(attributes) default(none)
#endif
    for (size_t i = 0; i < attributes.size(); i++) {
        float value = attributes.at(i);
        minAttr = std::min(minAttr, value);
        maxAttr = std::max(maxAttr, value);
    }
    this->dataRange = glm::vec2(minAttr, maxAttr);
    this->selectedRange = glm::vec2(minAttr, maxAttr);
    recomputeHistogram();
    rebuildRangeUbo();
}

void TransferFunctionWindow::computeHistogram(const std::vector<float>& attributes, float minAttr, float maxAttr) {
    this->attributes = attributes;
    this->dataRange = glm::vec2(minAttr, maxAttr);
    this->selectedRange = glm::vec2(minAttr, maxAttr);
    recomputeHistogram();
    rebuildRangeUbo();
}


void TransferFunctionWindow::recomputeHistogram() {
    histogram.clear();
    histogram.resize(histogramResolution);

    for (float attr : attributes) {
        int index = glm::clamp(
                static_cast<int>((attr - selectedRange.x) / (selectedRange.y - selectedRange.x) * histogramResolution),
                0, histogramResolution - 1);
        histogram.at(index) += 1;
    }

    float maxNum = 1.0f;
    for (float num : histogram) {
        maxNum = std::max(num, maxNum);
    }

    for (float& num : histogram) {
        num /= maxNum;
    }
}


glm::vec4 TransferFunctionWindow::getLinearRGBColorAtAttribute(float attribute) {
    int idx = glm::clamp((int)std::round(attribute*int(TRANSFER_FUNCTION_TEXTURE_SIZE-1)),
                         0, (int)(TRANSFER_FUNCTION_TEXTURE_SIZE-1));
    // Alpha always linear, doesn't matter which map we take
    return transferFunctionMap_linearRGB[idx].getFloatColorRGBA();
}

float TransferFunctionWindow::getOpacityAtAttribute(float attribute) {
    int idx = glm::clamp((int)std::round(attribute*int(TRANSFER_FUNCTION_TEXTURE_SIZE-1)),
                         0, (int)(TRANSFER_FUNCTION_TEXTURE_SIZE-1));
    // Alpha always linear, doesn't matter which map we take
    return transferFunctionMap_sRGB[idx].getFloatA();
}


bool TransferFunctionWindow::renderGui() {
    if (showTransferFunctionWindow) {
        sgl::ImGuiWrapper::get()->setNextWindowStandardPosSize(
                standardPositionX, standardPositionY, standardWidth, standardHeight);
        if (!ImGui::Begin("Transfer Function", &showTransferFunctionWindow)) {
            // Window collapsed
            ImGui::End();
            return false;
        }

        renderOpacityGraph();
        renderColorBar();

        if (selectedPointType == SELECTED_POINT_TYPE_OPACITY) {
            if (ImGui::DragFloat("Opacity", &opacitySelection, 0.001f, 0.0f, 1.0f)) {
                opacityPoints.at(currentSelectionIndex).opacity = opacitySelection;
                rebuildTransferFunctionMap();
                reRender = true;
            }
        } else if (selectedPointType == SELECTED_POINT_TYPE_COLOR) {
            if (ImGui::ColorEdit3("Color", (float*)&colorSelection)) {
                colorPoints.at(currentSelectionIndex).color = sgl::color16FromFloat(
                        colorSelection.x, colorSelection.y, colorSelection.z, colorSelection.w);
                rebuildTransferFunctionMap();
                reRender = true;
            }
        }

        if (ImGui::Combo("Color Space", (int*)&interpolationColorSpace,
                COLOR_SPACE_NAMES, IM_ARRAYSIZE(COLOR_SPACE_NAMES))) {
            rebuildTransferFunctionMap();
            reRender = true;
        }

        if (ImGui::SliderFloat2("Range", &selectedRange.x, dataRange.x, dataRange.y)) {
            recomputeHistogram();
            rebuildRangeUbo();
            reRender = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            selectedRange = dataRange;
            recomputeHistogram();
            rebuildRangeUbo();
            reRender = true;
        }

        if (ImGui::SliderInt("Histogram Res.", &histogramResolution, 1, 256)) {
            recomputeHistogram();
        }

        renderFileDialog();

        ImGui::End();

        if (reRender) {
            reRender = false;
            return true;
        }
    }

    return false;
}

void TransferFunctionWindow::renderFileDialog() {
    // Load file data
    if (ImGui::ListBox("##availablefiles",& selectedFileIndex, [this](void* data, int idx, const char** out_text) -> bool {
        *out_text = availableFiles.at(idx).c_str();
        return true;
    }, nullptr, int(availableFiles.size()), 4)) {
        saveFileString = availableFiles.at(selectedFileIndex);
    } ImVec2 cursorPosEnd = ImGui::GetCursorPos(); ImGui::SameLine();

    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::Text("Available files"); ImGui::SameLine(); ImGui::SetCursorPos(cursorPos + ImVec2(0.0f, 42.0f));
    if (ImGui::Button("Load file") && selectedFileIndex >= 0) {
        loadFunctionFromFile(saveDirectory + availableFiles.at(selectedFileIndex));
        reRender = true;
    } ImGui::SetCursorPos(cursorPosEnd);

    // Save file data
    ImGui::InputText("##savefilelabel", &saveFileString); ImGui::SameLine();
    if (ImGui::Button("Save file")) {
        saveFunctionToFile(saveDirectory + saveFileString);
        updateAvailableFiles();
    }
}

void TransferFunctionWindow::renderOpacityGraph() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();
    float regionWidth = ImGui::GetContentRegionAvail().x;
    float graphHeight = 300 * scaleFactor / 1.875f;
    float border = 2.0f * scaleFactor;
    float areaWidth = regionWidth - 2.0f * border;
    float areaHeight = graphHeight - 2.0f * border;
    opacityGraphBox.min = glm::vec2(
            ImGui::GetCursorScreenPos().x + border, ImGui::GetCursorScreenPos().y + border);
    opacityGraphBox.max = opacityGraphBox.min + glm::vec2(areaWidth, areaHeight);

    ImColor backgroundColor(clearColor.getFloatR(), clearColor.getFloatG(), clearColor.getFloatB());
    ImColor borderColor(1.0f - clearColor.getFloatR(), 1.0f - clearColor.getFloatG(), 1.0f - clearColor.getFloatB());

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
        ImVec2 centerPt = ImVec2(startPos.x + border + opacityPoints.at(i).position * areaWidth,
                startPos.y + border + (1.0f - opacityPoints.at(i).opacity) * areaHeight);
        float radius = 4*scaleFactor;
        if (selectedPointType == SELECTED_POINT_TYPE_OPACITY && i == currentSelectionIndex) {
            radius = 6*scaleFactor;
        }
        drawList->AddCircleFilled(centerPt, radius, backgroundColor, 24);
        drawList->AddCircle(centerPt, radius, borderColor, 24, 1.5f);
    }
}

void TransferFunctionWindow::renderColorBar() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();
    float regionWidth = ImGui::GetContentRegionAvail().x - 2;
    float barHeight = 30 * scaleFactor / 1.875f;
    colorBarBox.min = glm::vec2(ImGui::GetCursorScreenPos().x + 1, ImGui::GetCursorScreenPos().y + 1);
    colorBarBox.max = colorBarBox.min + glm::vec2(regionWidth - 2, barHeight - 2);

    // Draw bar
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 pos = ImVec2(startPos.x + 1, startPos.y + 1);
    for (size_t i = 0; i < TRANSFER_FUNCTION_TEXTURE_SIZE; i++) {
        sgl::Color16 color = transferFunctionMap_sRGB[i];
        ImU32 colorImgui = ImColor(color.getFloatR(), color.getFloatG(), color.getFloatB());
        drawList->AddLine(
                ImVec2(pos.x, pos.y), ImVec2(pos.x, pos.y + barHeight), colorImgui,
                2.0f * regionWidth / 255.0f);
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
        if (selectedPointType == SELECTED_POINT_TYPE_COLOR && i == currentSelectionIndex) {
            radius = 6*scaleFactor;
        }
        drawList->AddCircleFilled(centerPt, radius, colorImgui, 24);
        drawList->AddCircle(centerPt, radius, colorInvertedImgui, 24);
    }

    if (ImGui::ClickArea("##bararea", ImVec2(regionWidth + 2, barHeight), mouseReleased)) {
        onColorBarClick();
    }
}


// For OpenGL: Has 256 entries. Get mapped color for normalized attribute by accessing entry at "attr*255".
/*std::vector<sgl::Color> TransferFunctionWindow::getTransferFunctionMap() {
    return transferFunctionMap;
}*/

#ifdef SUPPORT_OPENGL
sgl::TexturePtr& TransferFunctionWindow::getTransferFunctionMapTexture() {
    return tfMapTexture;
}
#endif
#ifdef SUPPORT_VULKAN
sgl::vk::TexturePtr& TransferFunctionWindow::getTransferFunctionMapTextureVulkan() {
    return tfMapTextureVulkan;
}
#endif

bool TransferFunctionWindow::getTransferFunctionMapRebuilt() {
    if (transferFunctionMapRebuilt) {
        // Reset the flag
        transferFunctionMapRebuilt = false;
        return true;
    }
    return false;
}

void TransferFunctionWindow::rebuildRangeUbo() {
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice()) {
        minMaxUboVulkan->uploadData(sizeof(glm::vec2), &selectedRange.x);
    }
#endif
}

// For OpenGL: Has 256 entries. Get mapped color for normalized attribute by accessing entry at "attr*255".
void TransferFunctionWindow::rebuildTransferFunctionMap() {
    // Create linear RGB color points
    colorPoints_LinearRGB.clear();
    for (ColorPoint_sRGB& colorPoint : colorPoints) {
        glm::vec3 linearRGBColor = sRGBToLinearRGB(colorPoint.color.getFloatColorRGB());
        colorPoints_LinearRGB.push_back(ColorPoint_LinearRGB(linearRGBColor, colorPoint.position));
    }

    if (interpolationColorSpace == COLOR_SPACE_LINEAR_RGB) {
        rebuildTransferFunctionMap_LinearRGB();
    } else {
        rebuildTransferFunctionMap_sRGB();
    }

#ifdef SUPPORT_OPENGL
    if (AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        sgl::PixelFormat pixelFormat;
        pixelFormat.pixelType = GL_UNSIGNED_SHORT;
        if (useLinearRGB) {
            tfMapTexture->uploadPixelData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE, transferFunctionMap_linearRGB.data(), pixelFormat);
        } else {
            tfMapTexture->uploadPixelData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE, transferFunctionMap_sRGB.data(), pixelFormat);
        }
    }
#endif
#ifdef SUPPORT_VULKAN
    if (AppSettings::get()->getPrimaryDevice()) {
        if (useLinearRGB) {
            tfMapTextureVulkan->getImage()->uploadData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE * 8, transferFunctionMap_linearRGB.data());
        } else {
            tfMapTextureVulkan->getImage()->uploadData(
                    TRANSFER_FUNCTION_TEXTURE_SIZE * 8, transferFunctionMap_sRGB.data());
        }
    }
#endif

   transferFunctionMapRebuilt = true;
}

// For OpenGL: Has 256 entries. Get mapped color for normalized attribute by accessing entry at "attr*255".
void TransferFunctionWindow::rebuildTransferFunctionMap_LinearRGB() {
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

        transferFunctionMap_linearRGB.at(i) = sgl::Color16(glm::vec4(linearRGBColorAtIdx, opacityAtIdx));
        transferFunctionMap_sRGB.at(i) = sgl::Color16(
                glm::vec4(linearRGBTosRGB(linearRGBColorAtIdx), opacityAtIdx));
    }
}

// For OpenGL: Has 256 entries. Get mapped color for normalized attribute by accessing entry at "attr*255".
void TransferFunctionWindow::rebuildTransferFunctionMap_sRGB() {
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

        transferFunctionMap_linearRGB.at(i) = sgl::Color16(
                glm::vec4(sRGBToLinearRGB(sRGBColorAtIdx), opacityAtIdx));
        transferFunctionMap_sRGB.at(i) = sgl::Color16(
                glm::vec4(sRGBColorAtIdx, opacityAtIdx));
    }
}


void TransferFunctionWindow::update(float dt) {
    directoryContentWatch.update([this] { this->updateAvailableFiles(); });
    dragPoint();
}

glm::vec3 TransferFunctionWindow::sRGBToLinearRGB(const glm::vec3& color_LinearRGB) {
    //float factor = 2.2f;
    //return glm::pow(color_LinearRGB, glm::vec3(factor));
    // See https://en.wikipedia.org/wiki/SRGB
    return glm::mix(glm::pow((color_LinearRGB + 0.055f) / 1.055f, glm::vec3(2.4f)),
            color_LinearRGB / 12.92f, glm::lessThanEqual(color_LinearRGB, glm::vec3(0.04045f)));
}

glm::vec3 TransferFunctionWindow::linearRGBTosRGB(const glm::vec3& color_sRGB) {
    //float factor = 1.0f / 2.2f;
    //return glm::pow(color_sRGB, glm::vec3(factor));
    // See https://en.wikipedia.org/wiki/SRGB
    return glm::mix(1.055f * glm::pow(color_sRGB, glm::vec3(1.0f / 2.4f)) - 0.055f,
            color_sRGB * 12.92f, glm::lessThanEqual(color_sRGB, glm::vec3(0.0031308f)));
}

void TransferFunctionWindow::setUseLinearRGB(bool useLinearRGB) {
    this->useLinearRGB = useLinearRGB;
    rebuildTransferFunctionMap();
}

void TransferFunctionWindow::onOpacityGraphClick() {
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
            selectedPointType = SELECTED_POINT_TYPE_OPACITY;
            dragging = true;
        } else if (ImGui::GetIO().MouseClicked[1] && currentSelectionIndex != 0
                && currentSelectionIndex != int(opacityPoints.size())-1) {
            // A.2 Middle clicked? Delete point
            opacityPoints.erase(opacityPoints.begin() + currentSelectionIndex);
            selectedPointType = SELECTED_POINT_TYPE_NONE;
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
            opacityPoints.insert(opacityPoints.begin() + insertPosition, OpacityPoint(newOpacity, newPosition.x));
            currentSelectionIndex = insertPosition;
            opacitySelection = opacityPoints.at(currentSelectionIndex).opacity;
            selectedPointType = SELECTED_POINT_TYPE_OPACITY;
            dragging = true;
            reRender = true;
        }
    }

    rebuildTransferFunctionMap();
}

void TransferFunctionWindow::onColorBarClick() {
    glm::vec2 mousePosWidget = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y) - colorBarBox.min;
    float normalizedPosition = mousePosWidget.x / colorBarBox.getWidth();
    dragging = false;

    if (selectNearestColorPoint(currentSelectionIndex, mousePosWidget)) {
        // A) Point near to normalized position
        if (ImGui::GetIO().MouseClicked[0]) {
            // A.1 Left clicked? Select/drag-and-drop
            sgl::Color16& color16 = colorPoints.at(currentSelectionIndex).color;
            colorSelection = ImColor(color16.getFloatR(), color16.getFloatG(), color16.getFloatB());
            selectedPointType = SELECTED_POINT_TYPE_COLOR;
            if (currentSelectionIndex != 0 && currentSelectionIndex != int(colorPoints.size())-1) {
                dragging = true;
            }
        } else if (ImGui::GetIO().MouseClicked[1] && currentSelectionIndex != 0
                   && currentSelectionIndex != int(colorPoints.size())-1) {
            // A.2 Middle clicked? Delete point
            colorPoints.erase(colorPoints.begin() + currentSelectionIndex);
            colorPoints_LinearRGB.erase(colorPoints_LinearRGB.begin() + currentSelectionIndex);
            selectedPointType = SELECTED_POINT_TYPE_NONE;
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
            if (interpolationColorSpace == COLOR_SPACE_LINEAR_RGB) {
                // Linear RGB interplation
                glm::vec3 newColor_linearRGB = glm::mix(
                        colorPoints_LinearRGB.at(insertPosition-1).color,
                        colorPoints_LinearRGB.at(insertPosition).color,
                        1.0 - (colorPoints_LinearRGB.at(insertPosition).position - newPosition)
                              / (colorPoints_LinearRGB.at(insertPosition).position
                                 - colorPoints_LinearRGB.at(insertPosition-1).position));
                sgl::Color16 newColorsRGB(linearRGBTosRGB(newColor_linearRGB));
                colorPoints_LinearRGB.insert(colorPoints_LinearRGB.begin()
                                             + insertPosition, ColorPoint_LinearRGB(newColor_linearRGB, newPosition));
                colorPoints.insert(colorPoints.begin() + insertPosition, ColorPoint_sRGB(newColorsRGB, newPosition));
            } else {
                // sRGB interpolation
                sgl::Color16 newColor = sgl::color16Lerp(
                        colorPoints.at(insertPosition-1).color,
                        colorPoints.at(insertPosition).color,
                        1.0f - (colorPoints.at(insertPosition).position - newPosition)
                              / (colorPoints.at(insertPosition).position - colorPoints.at(insertPosition-1).position));
                colorPoints.insert(colorPoints.begin() + insertPosition, ColorPoint_sRGB(newColor, newPosition));
                // colorPoints_LinearRGB computed in @ref rebuildTransferFunctionMap
            }
            currentSelectionIndex = insertPosition;
            sgl::Color16& color16 = colorPoints.at(currentSelectionIndex).color;
            colorSelection = ImColor(color16.getFloatR(), color16.getFloatG(), color16.getFloatB());
            selectedPointType = SELECTED_POINT_TYPE_COLOR;
            reRender = true;
        }
    }

    rebuildTransferFunctionMap();
}

void TransferFunctionWindow::dragPoint() {
    if (mouseReleased) {
        dragging = false;
    }

    glm::vec2 mousePosWidget = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y) - opacityGraphBox.min;
    if (!dragging || mousePosWidget == oldMousePosWidget) {
        oldMousePosWidget = mousePosWidget;
        return;
    }
    oldMousePosWidget = mousePosWidget;

    if (selectedPointType == SELECTED_POINT_TYPE_OPACITY) {
        glm::vec2 normalizedPosition = mousePosWidget / opacityGraphBox.getDimensions();
        normalizedPosition.y = 1.0f - normalizedPosition.y;
        normalizedPosition = glm::clamp(normalizedPosition, 0.0f, 1.0f);
        if (currentSelectionIndex == 0) {
            normalizedPosition.x = 0.0f;
        }
        if (currentSelectionIndex == int(opacityPoints.size())-1) {
            normalizedPosition.x = 1.0f;
        }
        // Clip to neighbors!
        if (currentSelectionIndex != 0
                && normalizedPosition.x < opacityPoints.at(currentSelectionIndex-1).position) {
            normalizedPosition.x = opacityPoints.at(currentSelectionIndex-1).position;
        }
        if (currentSelectionIndex != int(opacityPoints.size())-1
                && normalizedPosition.x > opacityPoints.at(currentSelectionIndex+1).position) {
            normalizedPosition.x = opacityPoints.at(currentSelectionIndex+1).position;
        }
        opacityPoints.at(currentSelectionIndex).position = normalizedPosition.x;
        opacityPoints.at(currentSelectionIndex).opacity = normalizedPosition.y;
        opacitySelection = opacityPoints.at(currentSelectionIndex).opacity;
    }

    if (selectedPointType == SELECTED_POINT_TYPE_COLOR) {
        float normalizedPosition = mousePosWidget.x / colorBarBox.getWidth();
        normalizedPosition = glm::clamp(normalizedPosition, 0.0f, 1.0f);
        // Clip to neighbors!
        if (currentSelectionIndex != 0
                && normalizedPosition < colorPoints.at(currentSelectionIndex-1).position) {
            normalizedPosition = colorPoints.at(currentSelectionIndex-1).position;
        }
        if (currentSelectionIndex != int(colorPoints.size())-1
                && normalizedPosition > colorPoints.at(currentSelectionIndex+1).position) {
            normalizedPosition = colorPoints.at(currentSelectionIndex+1).position;
        }
        colorPoints.at(currentSelectionIndex).position = normalizedPosition;
    }

    rebuildTransferFunctionMap();
    reRender = true;
}

bool TransferFunctionWindow::selectNearestOpacityPoint(int& currentSelectionIndex, const glm::vec2& mousePosWidget) {
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();

    for (int i = 0; i < (int)opacityPoints.size(); i++) {
        glm::vec2 centerPt = glm::vec2(opacityPoints.at(i).position * opacityGraphBox.getWidth(),
                (1.0f - opacityPoints.at(i).opacity) * opacityGraphBox.getHeight());
        if (glm::length(centerPt - mousePosWidget) < scaleFactor * 10.0f) {
            currentSelectionIndex = i;
            return true;
        }
    }
    return false;
}

bool TransferFunctionWindow::selectNearestColorPoint(int& currentSelectionIndex, const glm::vec2& mousePosWidget) {
    float scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();

    for (int i = 0; i < (int)colorPoints.size(); i++) {
        ImVec2 centerPt = ImVec2(colorPoints.at(i).position * colorBarBox.getWidth(),
                colorBarBox.getHeight()/2);
        if (glm::abs(centerPt.x - mousePosWidget.x) < scaleFactor * 10.0f) {
            currentSelectionIndex = i;
            return true;
        }
    }
    return false;
}

}
