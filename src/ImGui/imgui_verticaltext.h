/*
 * This file contains adapted code from the ImGui library. The ImGui library is published under the MIT License.
 * All changes are published under the BSD 2-Clause License (see below).
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
 * Changes
 *
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

#ifndef SGL_IMGUI_VERTICALTEXT_H
#define SGL_IMGUI_VERTICALTEXT_H

#include "imgui.h"

namespace ImGui {

IMGUI_API ImVec2 CalcTextSizeVertical(
        const char* text, const char* text_end = NULL, bool hide_text_after_double_hash = false,
        float wrap_height = -1.0f);

IMGUI_API void AddTextVertical(
        ImDrawList* draw_list, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL,
        bool orientation_ccw = false);

IMGUI_API void AddTextVertical(
        ImDrawList* draw_list, ImFont* font, float font_size, const ImVec2& pos, ImU32 col,
        const char* text_begin, const char* text_end = NULL, float wrap_height = 0.0f,
        const ImVec4* cpu_fine_clip_rect = NULL, bool orientation_ccw = false);

IMGUI_API void RenderTextVertical(
        ImDrawList* draw_list, ImFont* font, float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect,
        const char* text_begin, const char* text_end, float wrap_height = 0.0f, bool cpu_fine_clip = false,
        bool orientation_ccw = false);

}

#endif //SGL_IMGUI_VERTICALTEXT_H
