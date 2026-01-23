/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#ifndef SGL_FORMAT_HPP
#define SGL_FORMAT_HPP

#include <string>
#include <string_view>
#include <vector>

namespace sgl {

/**
 * Fallback for C++20 function std::format (or when the format string is not a constexpr).
 * "{}" is replaced consecutively with the passed arguments.
 *
 * The "relaxed" version will not complain when '{' or '}' is used without alone.
 * That is useful, e.g., when parsing C/C++/GLSL/HLSL code (as long as the empty expression "{}" is not used).
 *
 * The "positional" version used "$idx" instead of "{}" (e.g., "$0", "$1", ...).
 */

DLL_OBJECT std::string formatStringList(
        const std::string_view& formatString, std::initializer_list<std::string> argsList);
DLL_OBJECT std::string formatStringListRelaxed(
        const std::string_view& formatString, std::initializer_list<std::string> argsList);
DLL_OBJECT std::string formatStringListPositional(
        const std::string_view& formatString, std::vector<std::string> argsList);

template<class T>
std::string to_string(T val) {
    return std::to_string(val);
}
inline std::string to_string(const char* val) {
    return { val };
}
inline std::string to_string(const std::string& val) {
    return val;
}

#if __cplusplus >= 201703L
template<class... T>
std::string formatString(const std::string_view& formatString, T... args) {
    return formatStringList(formatString, {to_string(args)...});
}

template<class... T>
std::string formatStringRelaxed(const std::string_view& formatString, T... args) {
    return formatStringListRelaxed(formatString, {to_string(args)...});
}

template<class... T>
std::string formatStringPositional(const std::string_view& formatString, T... args) {
    return formatStringListPositional(formatString, {to_string(args)...});
}
#endif

}

#endif //SGL_FORMAT_HPP