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

#ifndef SGL_GEOMETRY_FALLBACK_UTIL_HPP
#define SGL_GEOMETRY_FALLBACK_UTIL_HPP

// Drop-in replacement for glm.

#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace glm {

template<class T> constexpr T abs(const T& x) {
    return x >= T(0) ? x : -x;
}
template<class T> constexpr T min(const T& val0, const T& val1) {
    return val0 < val1 ? val0 : val1;
}
template<class T> constexpr T max(const T& val0, const T& val1) {
    return val0 > val1 ? val0 : val1;
}
template<class T> constexpr T clamp(const T& v, const T& lo, const T& hi) {
    return std::clamp(v, lo, hi);
}
template<class T, class Ta> constexpr T mix(const T& x, const T& y, const Ta& a) {
    return x * (T(1) - a) + y * a;
}
template<class T> T round(const T& x) {
    return std::round(x); //< constexpr since C++23
}
template<class T> T floor(const T& x) {
    return std::floor(x); //< constexpr since C++23
}
template<class T> T ceil(const T& x) {
    return std::ceil(x); //< constexpr since C++23
}
template<class T> T sin(const T& x) {
    return std::sin(x); //< constexpr since C++23
}
template<class T> T cos(const T& x) {
    return std::cos(x); //< constexpr since C++23
}
template<class T> T tan(const T& x) {
    return std::tan(x); //< constexpr since C++23
}
template<class T> T asin(const T& x) {
    return std::asin(x); //< constexpr since C++23
}
template<class T> T acos(const T& x) {
    return std::acos(x); //< constexpr since C++23
}
template<class T> T atan(const T& x) {
    return std::atan(x); //< constexpr since C++23
}
template<class T> T exp(const T& x) {
    return std::exp(x); //< constexpr since C++23
}
template<class T> T pow(const T& x, const T& y) {
    return std::pow(x, y); //< constexpr since C++23
}
template<class T> T sqrt(const T& x) {
    return std::sqrt(x); //< constexpr since C++23
}

}

#endif //SGL_GEOMETRY_FALLBACK_UTIL_HPP
