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

#ifndef SGL_PROPERTYEDITOR_HPP
#define SGL_PROPERTYEDITOR_HPP

#include <string>
#include <utility>
#include "../imgui.h"

namespace ImGui {
enum class EditMode : unsigned int;
}

namespace sgl {

class DLL_OBJECT PropertyEditor {
public:
    PropertyEditor(std::string name, bool& show) : windowName(std::move(name)), showPropertyEditor(show) {
        tableName = windowName + " Table";
    }

    inline void setInitWidthValues(float width) { initWidthValues = width; }

    bool begin();
    void end();

    bool beginTable();
    void endTable();

    bool beginNode(const std::string& nodeText);
    void endNode();

    void addText(const std::string& nodeText, const std::string& value);

    bool addSliderInt(
            const std::string& name, int* value, int minVal, int maxVal,
            const char* format = "%d", ImGuiSliderFlags flags = 0);
    bool addSliderIntPowerOfTwo(
            const std::string& name, int* value, int minVal, int maxVal,
            const char* format = "%d", ImGuiSliderFlags flags = 0);
    bool addSliderFloat(
            const std::string& name, float* value, float minVal, float maxVal,
            const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    bool addSliderFloat3(
            const std::string& name, float* value, float minVal, float maxVal,
            const char* format = "%.3f", ImGuiSliderFlags flags = 0);

    ImGui::EditMode addSliderFloatEdit(
            const std::string& name, float* value, float minVal, float maxVal,
            const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    ImGui::EditMode addSliderFloat2Edit(
            const std::string& name, float* value, float minVal, float maxVal,
            const char* format = "%.3f", ImGuiSliderFlags flags = 0);

    bool addColorEdit3(const std::string& label, float col[3], ImGuiColorEditFlags flags = 0);
    bool addColorEdit4(const std::string& label, float col[4], ImGuiColorEditFlags flags = 0);

    bool addCheckbox(const std::string& name, bool* value);
    bool addButton(const std::string& labelText, const std::string& buttonText);
    bool addInputAction(const std::string& name, std::string* text);

    bool addCombo(
            const std::string& label, int* current_item, const char* const items[], int items_count,
            int popup_max_height_in_items = -1);
    bool addCombo(
            const std::string& label, int* current_item, const std::string* items, int items_count,
            int popup_max_height_in_items = -1);
    bool addBeginCombo(const std::string& label, const std::string& preview_value, ImGuiComboFlags flags = 0);
    void addEndCombo();

    void addCustomWidgets(const std::string& label);

private:
    std::string windowName;
    std::string tableName;
    bool& showPropertyEditor;
    bool windowWasOpened = true;
    float initWidthValues = 240.0f;
};

}

#endif //SGL_PROPERTYEDITOR_HPP
