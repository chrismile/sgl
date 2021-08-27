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

namespace ImGui
{

IMGUI_API bool ListBox(const char* label, int* current_item, std::function<bool(void*, int, const char**)> items_getter, void* data, int items_count, int height_in_items = -1);
IMGUI_API bool Combo(const char* label, int* current_item, const std::string* items, int items_count, int popup_max_height_in_items = -1);
IMGUI_API bool ClickArea(const char *str_id, const ImVec2 &size_arg, bool &mouseReleased);
IMGUI_API void ProgressSpinner(const char* str_id, float radius, float thickness, float speed, const ImVec4& color);

IMGUI_API bool SliderFloatActive(const char* label, float* v, float v_min, float v_max, bool is_active, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderIntActive(const char* label, int* v, int v_min, int v_max, bool is_active, const char* format = "%d");
IMGUI_API bool SliderScalarActive(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, bool is_active, const char* format = NULL, ImGuiSliderFlags flags = 0);

IMGUI_API bool SliderIntPowerOfTwo(const char* label, int* v, int v_min, int v_max, const char* format = NULL, ImGuiSliderFlags flags = 0);

}

#endif //SGL_IMGUI_CUSTOM_HPP
