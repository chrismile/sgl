/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#ifndef SGL_WEBGPU_COMMON_HPP
#define SGL_WEBGPU_COMMON_HPP

#include <string>
#include <cstring>
#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

}
#ifdef WEBGPU_LEGACY_API
inline void stdStringToWgpuView(char const*& view, const std::string& stdString) {
    if (stdString.empty()) {
        view = nullptr;
    } else {
        view = stdString.c_str();
    }
}
inline void cStringToWgpuView(char const*& view, const char* cString) {
    view = cString;
}
#else
inline void stdStringToWgpuView(WGPUStringView& view, const std::string& stdString) {
    if (stdString.empty()) {
        view.data = nullptr;
        view.length = 0;
    } else {
        view.data = stdString.c_str();
        view.length = stdString.length();
    }
}
inline void cStringToWgpuView(WGPUStringView& view, const char* cString) {
    if (!cString) {
        view.data = nullptr;
        view.length = 0;
        return;
    }
    size_t stringLength = strlen(cString);
    if (stringLength == 0) {
        view.data = nullptr;
        view.length = 0;
    } else {
        view.data = cString;
        view.length = stringLength;
    }
}
#endif

}

#endif //SGL_WEBGPU_COMMON_HPP
