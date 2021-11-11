/*
 * This code contains code from ImGui. ImGui is covered by the MIT license, and
 * the changes are covered by the BSD 2-Clause License.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2020 Omar Cornut
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
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

#include <cmath>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include "imgui_custom.h"

#include <Math/Math.hpp>

bool ImGui::ListBox(const char* label, int* current_item, std::function<bool(void*, int, const char**)> items_getter, void* data, int items_count, int height_in_items)
{
    ImGuiContext& g = *GImGui;

    // Calculate size from "height_in_items"
    if (height_in_items < 0)
        height_in_items = ImMin(items_count, 7);
    float height_in_items_f = height_in_items + 0.25f;
    ImVec2 size(0.0f, ImFloor(GetTextLineHeightWithSpacing() * height_in_items_f + g.Style.FramePadding.y * 2.0f));

    if (!BeginListBox(label, size))
        return false;

    // Assume all items have even height (= 1 line of text). If you need items of different height,
    // you can create a custom version of ListBox() in your code without using the clipper.
    bool value_changed = false;
    ImGuiListClipper clipper;
    clipper.Begin(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const char* item_text;
            if (!items_getter(data, i, &item_text))
                item_text = "*Unknown item*";

            PushID(i);
            const bool item_selected = (i == *current_item);
            if (Selectable(item_text, item_selected))
            {
                *current_item = i;
                value_changed = true;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
    EndListBox();

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

// Getter for the old Combo() API: const char*[]
static bool Items_ArrayGetterString(void* data, int idx, const char** out_text)
{
    const std::string* items = (const std::string*)data;
    if (out_text)
        *out_text = items[idx].c_str();
    return true;
}

// Combo box helper allowing to pass an array of strings.
bool ImGui::Combo(const char* label, int* current_item, const std::string* items, int items_count, int height_in_items)
{
    const bool value_changed = Combo(label, current_item, Items_ArrayGetterString, (void*)items, items_count, height_in_items);
    return value_changed;
}

// Code adapted from ImGui's InvisibleButton
bool ImGui::ClickArea(const char *str_id, const ImVec2 &size_arg, bool &mouseReleased)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(str_id);
    ImVec2 size = ImGui::CalcItemSize(size_arg, 0.0f, 0.0f);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = ImGui::ItemHoverable(bb, id);
    bool clicked = ImGui::GetIO().MouseClicked[0] || ImGui::GetIO().MouseClicked[1] || ImGui::GetIO().MouseClicked[2];
    mouseReleased = ImGui::GetIO().MouseReleased[0];

    return hovered && clicked;
}

void ImGui::ProgressSpinner(const char* str_id, float radius, float thickness, float speed, const ImVec4& color)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    float totalRadius = radius > 0.0f ? radius + thickness : -1.0f;
    ImVec2 size(totalRadius, totalRadius);
    if (radius < 0.0f) {
        size = ImVec2(g.FontSize + style.FramePadding.x*2.0f, g.FontSize + style.FramePadding.y*2.0f);
        radius = g.FontSize * 0.5f;
        thickness = g.FontSize * 0.2f;
    }

    ImRect bb(pos, pos + size);
    ImVec2 center = bb.GetCenter();
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, 0))
        return;

    constexpr float EULER_CONSTANT = 2.71828182845904523536f;

    ImDrawList* draw_list = window->DrawList;
    float offset = float(std::sin(g.Time / EULER_CONSTANT * 2.0f * speed)) * 1.0f + 2.0f;
    float angleStart = float(g.Time * speed - offset / 2.0f);
    float angleEnd = float(g.Time * speed + offset / 2.0f);
    draw_list->PathArcTo(center, radius, angleStart, angleEnd, 12);
    draw_list->PathStroke(ImGui::ColorConvertFloat4ToU32(color), false, thickness);
}



// --- Sliders that can be enabled/disabled ---

// Even though we changed the compile-time default, we expect users to have carried %f around, which would break the display of DragInt() calls.
// To honor backward compatibility we are rewriting the format string, unless IMGUI_DISABLE_OBSOLETE_FUNCTIONS is enabled. What could possibly go wrong?!
static const char* PatchFormatStringFloatToInt(const char* fmt)
{
    if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
        return "%d";
    const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
    const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
    if (fmt_end > fmt_start && fmt_end[-1] == 'f')
    {
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        if (fmt_start == fmt && fmt_end[0] == 0)
            return "%d";
        ImGuiContext& g = *GImGui;
        ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
        return g.TempBuffer;
#else
        IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
    }
    return fmt;
}

// Note: p_data, p_min and p_max are _pointers_ to a memory address holding the data. For a slider, they are all required.
// Read code of e.g. SliderFloat(), SliderInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
bool ImGui::SliderScalarActive(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, bool is_active, const char* format, ImGuiSliderFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;
    else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
        format = PatchFormatStringFloatToInt(format);

    // Tabbing or CTRL-clicking on Slider turns it into an input box
    const bool hovered = is_active && ItemHoverable(frame_bb, id);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
        const bool clicked = (hovered && g.IO.MouseClicked[0]);
        if (input_requested_by_tabbing || clicked || g.NavActivateId == id || g.NavActivateInputId == id)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            if (temp_input_allowed && (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || g.NavActivateInputId == id))
                temp_input_is_active = true;
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
        const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x) {
        ImU32 color;
        if (is_active) {
            color = GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
        } else {
            color = GetColorU32(ImGuiCol_TextDisabled);
        }
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, color, style.GrabRounding);
    }

    if (!is_active) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(GetColorU32(ImGuiCol_TextDisabled))));
    }

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if (!is_active) {
        ImGui::PopStyleColor(1);
    }

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
    return value_changed;
}

bool ImGui::SliderFloatActive(const char* label, float* v, float v_min, float v_max, bool is_active, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarActive(label, ImGuiDataType_Float, v, &v_min, &v_max, is_active, format, flags);
}

bool ImGui::SliderIntActive(const char* label, int* v, int v_min, int v_max, bool is_active, const char* format)
{
    return SliderScalarActive(label, ImGuiDataType_S32, v, &v_min, &v_max, is_active, format);
}


bool ImGui::SliderIntPowerOfTwo(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    int log_min = sgl::intlog2(v_min);
    int log_max = sgl::intlog2(v_max);
    int log_v = sgl::intlog2(*v);

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(ImGuiDataType_S32)->PrintFmt;
    else if (strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
        format = PatchFormatStringFloatToInt(format);

    // Tabbing or CTRL-clicking on Slider turns it into an input box
    const bool hovered = ItemHoverable(frame_bb, id);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
        const bool clicked = (hovered && g.IO.MouseClicked[0]);
        if (input_requested_by_tabbing || clicked || g.NavActivateId == id || g.NavActivateInputId == id)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            if (temp_input_allowed && (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || g.NavActivateInputId == id))
                temp_input_is_active = true;
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
        const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
        return TempInputScalar(
                frame_bb, id, label, ImGuiDataType_S32, v, format, is_clamp_input ? &v_min : NULL, is_clamp_input ? &v_max : NULL);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, ImGuiDataType_S32, &log_v, &log_min, &log_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), ImGuiDataType_S32, v, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);

    *v = 1 << log_v;
    return value_changed;
}



bool ImGui::SliderScalarNoLiveEdit(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    const ImGuiDataTypeInfo* data_type_info = DataTypeGetInfo(data_type);
    uint64_t tmp_data = 0;
    void* p_tmp = &tmp_data;
    memcpy(p_tmp, p_data, data_type_info->Size);

    SliderScalar(label, data_type, p_tmp,  p_min, p_max, format, flags);

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        memcpy(p_data, p_tmp, data_type_info->Size);
        return true;
    }
    return false;
}

// Add multiple sliders on 1 line for compact edition of multiple components
bool ImGui::SliderScalarNNoLiveEdit(const char* label, ImGuiDataType data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, ImGuiSliderFlags flags)
{
    SliderScalarN(label, data_type, v, components, v_min, v_max, format, flags);
    return ImGui::IsItemDeactivatedAfterEdit();
}

bool ImGui::SliderFloatNoLiveEdit(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNoLiveEdit(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderFloat2NoLiveEdit(const char* label, float v[2], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNNoLiveEdit(label, ImGuiDataType_Float, v, 2, &v_min, &v_max, format, flags);
}

bool ImGui::SliderFloat3NoLiveEdit(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNNoLiveEdit(label, ImGuiDataType_Float, v, 3, &v_min, &v_max, format, flags);
}

bool ImGui::SliderFloat4NoLiveEdit(const char* label, float v[4], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNNoLiveEdit(label, ImGuiDataType_Float, v, 4, &v_min, &v_max, format, flags);
}

bool ImGui::SliderAngleNoLiveEdit(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, ImGuiSliderFlags flags)
{
    if (format == NULL)
        format = "%.0f deg";
    float v_deg = (*v_rad) * 360.0f / (2 * IM_PI);
    bool value_changed = SliderFloatNoLiveEdit(label, &v_deg, v_degrees_min, v_degrees_max, format, flags);
    *v_rad = v_deg * (2 * IM_PI) / 360.0f;
    return value_changed;
}

bool ImGui::SliderIntNoLiveEdit(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNoLiveEdit(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderInt2NoLiveEdit(const char* label, int v[2], int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNNoLiveEdit(label, ImGuiDataType_S32, v, 2, &v_min, &v_max, format, flags);
}

bool ImGui::SliderInt3NoLiveEdit(const char* label, int v[3], int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNNoLiveEdit(label, ImGuiDataType_S32, v, 3, &v_min, &v_max, format, flags);
}

bool ImGui::SliderInt4NoLiveEdit(const char* label, int v[4], int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNNoLiveEdit(label, ImGuiDataType_S32, v, 4, &v_min, &v_max, format, flags);
}

bool ImGui::VSliderScalarNoLiveEdit(const char* label, const ImVec2& size, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    VSliderScalar(label, size, data_type, p_data, p_min, p_max, format, flags);
    return ImGui::IsItemDeactivatedAfterEdit();
}

bool ImGui::VSliderFloatNoLiveEdit(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return VSliderScalarNoLiveEdit(label, size, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool ImGui::VSliderIntNoLiveEdit(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return VSliderScalarNoLiveEdit(label, size, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}



EditMode ImGui::SliderScalarEdit(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    bool isEdited = SliderScalar(label, data_type, p_data,  p_min, p_max, format, flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        return EditMode::INPUT_FINISHED;
    } else if (isEdited) {
        return EditMode::LIVE_EDIT;
    } else {
        return EditMode::NO_CHANGE;
    }
}

// Add multiple sliders on 1 line for compact edition of multiple components
EditMode ImGui::SliderScalarNEdit(const char* label, ImGuiDataType data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, ImGuiSliderFlags flags)
{
    bool isEdited = SliderScalarN(label, data_type, v, components, v_min, v_max, format, flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        return EditMode::INPUT_FINISHED;
    } else if (isEdited) {
        return EditMode::LIVE_EDIT;
    } else {
        return EditMode::NO_CHANGE;
    }
}

EditMode ImGui::SliderFloatEdit(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarEdit(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

EditMode ImGui::SliderFloat2Edit(const char* label, float v[2], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNEdit(label, ImGuiDataType_Float, v, 2, &v_min, &v_max, format, flags);
}

EditMode ImGui::SliderFloat3Edit(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNEdit(label, ImGuiDataType_Float, v, 3, &v_min, &v_max, format, flags);
}

EditMode ImGui::SliderFloat4Edit(const char* label, float v[4], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNEdit(label, ImGuiDataType_Float, v, 4, &v_min, &v_max, format, flags);
}

EditMode ImGui::SliderAngleEdit(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, ImGuiSliderFlags flags)
{
    if (format == NULL)
        format = "%.0f deg";
    float v_deg = (*v_rad) * 360.0f / (2 * IM_PI);
    EditMode edit_mode = SliderFloatEdit(label, &v_deg, v_degrees_min, v_degrees_max, format, flags);
    *v_rad = v_deg * (2 * IM_PI) / 360.0f;
    return edit_mode;
}

EditMode ImGui::SliderIntEdit(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarEdit(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

EditMode ImGui::SliderInt2Edit(const char* label, int v[2], int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNEdit(label, ImGuiDataType_S32, v, 2, &v_min, &v_max, format, flags);
}

EditMode ImGui::SliderInt3Edit(const char* label, int v[3], int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNEdit(label, ImGuiDataType_S32, v, 3, &v_min, &v_max, format, flags);
}

EditMode ImGui::SliderInt4Edit(const char* label, int v[4], int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarNEdit(label, ImGuiDataType_S32, v, 4, &v_min, &v_max, format, flags);
}

EditMode ImGui::VSliderScalarEdit(const char* label, const ImVec2& size, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    bool isEdited = VSliderScalar(label, size, data_type, p_data, p_min, p_max, format, flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        return EditMode::INPUT_FINISHED;
    } else if (isEdited) {
        return EditMode::LIVE_EDIT;
    } else {
        return EditMode::NO_CHANGE;
    }
}

EditMode ImGui::VSliderFloatEdit(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return VSliderScalarEdit(label, size, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

EditMode ImGui::VSliderIntEdit(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return VSliderScalarEdit(label, size, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

void ImGui::HelpMarker(const char* label)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(label);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
