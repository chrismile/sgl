//
// Created by christoph on 10.10.18.
//

#ifndef SGL_IMGUI_CUSTOM_HPP
#define SGL_IMGUI_CUSTOM_HPP

#include <string>
#include <functional>

namespace ImGui
{

    IMGUI_API bool ListBox(const char* label, int* current_item, std::function<bool(void*, int, const char**)> items_getter, void* data, int items_count, int height_in_items = -1);
    IMGUI_API bool Combo(const char* label, int* current_item, const std::string* items, int items_count, int popup_max_height_in_items = -1);
    IMGUI_API bool ClickArea(const char *str_id, const ImVec2 &size_arg, bool &mouseReleased);

}
#endif //SGL_IMGUI_CUSTOM_HPP
