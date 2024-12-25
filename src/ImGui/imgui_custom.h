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

#ifndef SGL_IMGUI_CUSTOM_HPP
#define SGL_IMGUI_CUSTOM_HPP

#include <string>
#include <functional>

#include "imgui.h"

namespace ImGui
{

// Text Utilities
IMGUI_API ImVec2 CalcTextSize(ImFont* font, float font_size, const char* text, const char* text_end = NULL, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);

/*
 * - No change: The same as SliderXXX returning false.
 * - Live edit: The user is editing the slider value (either by dragging or by text input).
 * - Input finished: The user has finished the input (e.g., by releasing the mouse button, pressing enter or the element
 * losing focus).
 */
enum class EditMode : unsigned int {
    NO_CHANGE, LIVE_EDIT, INPUT_FINISHED
};

IMGUI_API bool ListBox(const char* label, int* current_item, std::function<bool(void*, int, const char**)> items_getter, void* data, int items_count, int height_in_items = -1);
IMGUI_API bool Combo(const char* label, int* current_item, const std::string* items, int items_count, int popup_max_height_in_items = -1);
IMGUI_API bool ClickArea(const char *str_id, const ImVec2 &size_arg, bool &mouseReleased);
IMGUI_API void ProgressSpinner(const char* str_id, float radius, float thickness, float speed, const ImVec4& color);

IMGUI_API bool SliderFloatActive(const char* label, float* v, float v_min, float v_max, bool is_active, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderIntActive(const char* label, int* v, int v_min, int v_max, bool is_active, const char* format = "%d");
IMGUI_API bool SliderScalarActive(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, bool is_active, const char* format = NULL, ImGuiSliderFlags flags = 0);

IMGUI_API bool SliderIntPowerOfTwo(const char* label, int* v, int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderIntNPowerOfTwo(const char* label, int* v, int components, int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderInt2PowerOfTwo(const char* label, int v[2], int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderInt3PowerOfTwo(const char* label, int v[3], int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);

/*
 * Slider for double precision floating point numbers.
 */
IMGUI_API bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

/*
 * Only changes the returns true/changes the value v if the editing has finished.
 */
IMGUI_API bool SliderFloatNoLiveEdit(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display.
IMGUI_API bool SliderFloat2NoLiveEdit(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderFloat3NoLiveEdit(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderFloat4NoLiveEdit(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderAngleNoLiveEdit(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderIntNoLiveEdit(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderInt2NoLiveEdit(const char* label, int v[2], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderInt3NoLiveEdit(const char* label, int v[3], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderInt4NoLiveEdit(const char* label, int v[4], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderScalarNoLiveEdit(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderScalarNNoLiveEdit(const char* label, ImGuiDataType data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API bool VSliderFloatNoLiveEdit(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool VSliderIntNoLiveEdit(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool VSliderScalarNoLiveEdit(const char* label, const ImVec2& size, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = NULL, ImGuiSliderFlags flags = 0);

/*
 * Sliders with three separate edit modes.
 * - No change: The same as SliderXXX returning false.
 * - Live edit: The user is editing the slider value (either by dragging or by text input).
 * - Input finished: The user has finished the input (e.g., by releasing the mouse button, pressing enter or the element
 * losing focus).
 */
IMGUI_API EditMode SliderFloatEdit(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display.
IMGUI_API EditMode SliderFloat2Edit(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderFloat3Edit(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderFloat4Edit(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderAngleEdit(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderIntEdit(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderInt2Edit(const char* label, int v[2], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderInt3Edit(const char* label, int v[3], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderInt4Edit(const char* label, int v[4], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderScalarEdit(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderScalarNEdit(const char* label, ImGuiDataType data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API EditMode VSliderFloatEdit(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode VSliderIntEdit(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API EditMode VSliderScalarEdit(const char* label, const ImVec2& size, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderIntPowerOfTwoEdit(const char* label, int* v, int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderInt2PowerOfTwoEdit(const char* label, int v[2], int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);
IMGUI_API EditMode SliderInt3PowerOfTwoEdit(const char* label, int v[3], int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);

IMGUI_API void HelpMarker(const char* label);

}

#endif //SGL_IMGUI_CUSTOM_HPP
