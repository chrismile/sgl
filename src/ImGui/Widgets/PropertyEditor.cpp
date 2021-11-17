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

#include <ImGui/ImGuiWrapper.hpp>
#include <ImGui/imgui_custom.h>
#include "PropertyEditor.hpp"

namespace sgl {

static const ImGuiTableFlags tableFlags =
        ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
static const ImGuiTreeNodeFlags treeNodeFlagsLeaf =
        ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen
        | ImGuiTreeNodeFlags_SpanFullWidth;

bool PropertyEditor::begin() {
    if (ImGui::Begin(windowName.c_str(), &showPropertyEditor)) {
        windowWasOpened = true;
    } else  {
        windowWasOpened = false;
    }
    return windowWasOpened;
}

void PropertyEditor::end() {
    ImGui::End();
}

bool PropertyEditor::beginTable() {
    if (windowWasOpened) {
        bool tableOpen = ImGui::BeginTable(tableName.c_str(), 2, tableFlags);

        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, initWidthValues);
        ImGui::TableHeadersRow();

        return tableOpen;
    } else  {
        return false;
    }
}

void PropertyEditor::endTable() {
    if (windowWasOpened) {
        ImGui::EndTable();
    }
}


bool PropertyEditor::beginNode(const std::string& nodeText) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    bool open = ImGui::TreeNodeEx(nodeText.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
    ImGui::TableNextColumn();
    ImGui::TextDisabled("--");
    return open;
}

void PropertyEditor::endNode() {
    ImGui::TreePop();
}


void PropertyEditor::addText(const std::string& nodeText, const std::string& value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(nodeText.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::TextUnformatted(value.c_str());
}


bool PropertyEditor::addSliderInt(
        const std::string& name, int* value, int minVal, int maxVal,
        const char* format, ImGuiSliderFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderInt(internalId.c_str(), value, minVal, maxVal, format, flags);
}

bool PropertyEditor::addSliderIntPowerOfTwo(
        const std::string& name, int* value, int minVal, int maxVal,
        const char* format, ImGuiSliderFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderIntPowerOfTwo(internalId.c_str(), value, minVal, maxVal, format, flags);
}

bool PropertyEditor::addSliderFloat(
        const std::string& name, float* value, float minVal, float maxVal,
        const char* format, ImGuiSliderFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderFloat(internalId.c_str(), value, minVal, maxVal, format, flags);
}

bool PropertyEditor::addSliderFloat3(
        const std::string& name, float* value, float minVal, float maxVal,
        const char* format, ImGuiSliderFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderFloat3(internalId.c_str(), value, minVal, maxVal, format, flags);
}


ImGui::EditMode PropertyEditor::addSliderFloatEdit(
        const std::string& name, float* value, float minVal, float maxVal,
        const char* format, ImGuiSliderFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderFloatEdit(internalId.c_str(), value, minVal, maxVal, format, flags);
}

ImGui::EditMode PropertyEditor::addSliderFloat2Edit(
        const std::string& name, float* value, float minVal, float maxVal,
        const char* format, ImGuiSliderFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderFloat2Edit(internalId.c_str(), value, minVal, maxVal, format, flags);
}


bool PropertyEditor::addColorEdit3(const std::string& label, float col[3], ImGuiColorEditFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(label.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + label;
    return ImGui::ColorEdit3(internalId.c_str(), col, flags);
}

bool PropertyEditor::addColorEdit4(const std::string& label, float col[4], ImGuiColorEditFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(label.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + label;
    return ImGui::ColorEdit4(internalId.c_str(), col, flags);
}


bool PropertyEditor::addCheckbox(const std::string& name, bool* value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::Checkbox(internalId.c_str(), value);
}

bool PropertyEditor::addButton(const std::string& labelText, const std::string& buttonText) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(labelText.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    return ImGui::Button(buttonText.c_str());
}

bool PropertyEditor::addInputAction(const std::string& name, std::string* text) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    bool clicked = ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::Button(internalId.c_str()) || clicked;
}

bool PropertyEditor::addCombo(
        const std::string& label, int* current_item, const char* const items[], int items_count,
        int popup_max_height_in_items) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(label.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + label;
    return ImGui::Combo(internalId.c_str(), current_item, items, items_count, popup_max_height_in_items);
}

bool PropertyEditor::addCombo(
        const std::string& label, int* current_item, const std::string* items, int items_count,
        int popup_max_height_in_items) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(label.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + label;
    return ImGui::Combo(internalId.c_str(), current_item, items, items_count, popup_max_height_in_items);
}

bool PropertyEditor::addBeginCombo(const std::string& label, const std::string& preview_value, ImGuiComboFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(label.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + label;
    return ImGui::BeginCombo(internalId.c_str(), preview_value.c_str(), flags);
}

void PropertyEditor::addEndCombo() {
    ImGui::EndCombo();
}

void PropertyEditor::addCustomWidgets(const std::string& label) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(label.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
}

}
