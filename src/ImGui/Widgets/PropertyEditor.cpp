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
        bool tableOpen = ImGui::BeginTable(tableName.c_str(), 2, tableFlags);

        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 240.0f);
        ImGui::TableHeadersRow();

        return tableOpen;
    } else  {
        windowWasOpened = false;
        return false;
    }
}

void PropertyEditor::end() {
    if (windowWasOpened) {
        ImGui::EndTable();
    }
    ImGui::End();
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


bool PropertyEditor::addSliderInt(const std::string& name, int* value, int minVal, int maxVal) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderInt(internalId.c_str(), value, minVal, maxVal);
}

bool PropertyEditor::addSliderFloat(const std::string& name, float* value, float minVal, float maxVal) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderFloat(internalId.c_str(), value, minVal, maxVal);
}

bool PropertyEditor::addSliderFloat3(const std::string& name, float* value, float minVal, float maxVal) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::SliderFloat3(internalId.c_str(), value, minVal, maxVal);
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

bool PropertyEditor::addInputAction(const std::string& name, std::string* text) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    bool clicked = ImGui::TreeNodeEx(name.c_str(), treeNodeFlagsLeaf);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    std::string internalId = "##" + name;
    return ImGui::Button(internalId.c_str());
}

}