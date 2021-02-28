//
// Created by christoph on 10.10.18.
//

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include "imgui_custom.h"

bool ImGui::ListBox(const char* label, int* current_item, std::function<bool(void*, int, const char**)> items_getter, void* data, int items_count, int height_in_items)
{
    if (!ListBoxHeader(label, items_count, height_in_items))
        return false;

    // Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const bool item_selected = (i == *current_item);
            const char* item_text;
            if (!items_getter(data, i, &item_text))
                item_text = "*Unknown item*";

            PushID(i);
            if (Selectable(item_text, item_selected))
            {
                *current_item = i;
                value_changed = true;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
    ListBoxFooter();
    if (value_changed)
        MarkItemEdited(g.CurrentWindow->DC.LastItemId);

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
    float offset = std::sin(g.Time / EULER_CONSTANT * 2.0f * speed) * 1.0f + 2.0f;
    float angleStart = g.Time * speed - offset / 2.0f;
    float angleEnd = g.Time * speed + offset / 2.0f;
    draw_list->PathArcTo(center, radius, angleStart, angleEnd, 12);
    draw_list->PathStroke(ImGui::ColorConvertFloat4ToU32(color), false, thickness);
}
