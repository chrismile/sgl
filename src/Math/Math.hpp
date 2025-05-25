/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

#ifndef SRC_MATH_MATH_HPP_
#define SRC_MATH_MATH_HPP_

#include <cmath>
#include <cstdint>
#ifdef USE_GLM
#include <glm/fwd.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

#if _cplusplus >= 201907L
#include <bit>
#endif

/// Collection of math utility functions
namespace sgl {

const float PI = 3.1415926535897932f;
const float TWO_PI = PI * 2.0f;
const float HALF_PI = PI / 2.0f;

template <typename T> inline T abs(T a) { return a > T(0) ? a : -a; }
inline bool floatEquals(float a, float b) { return sgl::abs(a - b) < 0.0001f; }
inline bool floatEquals(float a, float b, float dt) { return sgl::abs(a - b) < dt; }
inline bool floatLess(float a, float b) { return a < b + 0.0001f; }
inline bool floatLess(float a, float b, float dt) { return a < b + dt; }
template <typename T> inline T clamp(T val, T min, T max) { return val < min ? min : (val > max ? max : val); }
template <typename T> inline float ceil(T val) { return std::ceil(val); }
template <typename T> inline float exp(T val) { return std::exp(val); }
template <typename T> inline float sqrt(T val) { return std::sqrt(val); }
template <typename T> inline float sqr(T val) { return val * val; }
inline float pow(float val, int n) { float p = val; for (int i = 1; i < n; ++i) { p *= val; } return p; }
inline float min(float v1, float v2) { return v1 < v2 ? v1 : v2; }
inline float max(float v1, float v2) { return v1 > v2 ? v1 : v2; }
inline int min(int v1, int v2) { return v1 < v2 ? v1 : v2; }
inline int max(int v1, int v2) { return v1 > v2 ? v1 : v2; }
inline int sign(int v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }
inline int sign(float v) { return v > 0.0001f ? 1 : v < -0.0001f ? -1 : 0; }

inline float sin(float val) { return std::sin(val); }
inline float cos(float val) { return std::cos(val); }
inline float tan(float val) { return std::tan(val); }
inline float asin(float val) { return std::asin(val); }
inline float acos(float val) { return std::acos(val); }
inline float atan(float val) { return std::atan(val); }
inline float atan2(float y, float x) { return std::atan2(y, x); }
inline float degToRad(float val) { return val / 180.0f * PI; }
inline float radToDeg(float val) { return val * 180.0f / PI; }

inline bool isPowerOfTwo(int x) { return (x != 0) && ((x & (x - 1)) == 0); }
inline int nextPowerOfTwo(int x) { --x; x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16; return x+1; }
inline int lastPowerOfTwo(int x) { return nextPowerOfTwo(x/2+1); }

inline int iceil(int x, int y) { return (x - 1) / y + 1; }
// Avoids going into negative for x == 0 and overflow.
inline uint32_t uiceil(uint32_t x, uint32_t y) { return x > 0 ? (x - 1) / y + 1 : 0; }
inline uint64_t ulceil(uint64_t x, uint64_t y) { return x > 0 ? (x - 1) / y + 1 : 0; }
inline size_t sizeceil(size_t x, size_t y) { return x > 0 ? (x - 1) / y + 1 : 0; }

// Fast integer square root, i.e., floor(sqrt(s)).
DLL_OBJECT uint32_t uisqrt(uint32_t s);

DLL_OBJECT uint32_t convertBitRepresentationFloatToUint32(float val);

inline int nextMultiple(int num, int multiple) {
    int remainder = num % multiple;
    if (remainder == 0) {
        return num;
    }
    return num + multiple - remainder;
}

inline int intlog2(int x) {
    int log2x = 0;
    while ((x >>= 1) != 0) {
        ++log2x;
    }
    return log2x;
}

inline int floorDiv(int a, int b) {
    int div = a / b;
    if (a < 0 && a % b != 0) {
        div -= 1;
    }
    return div;
}

inline int floorMod(int a, int b) {
    int div = floorDiv(a, b);
    return a - b * div;
}

inline int floorDiv(float a, float b) {
    int div = int(a / b);
    if (a < 0 && std::fmod(a, b) != 0.0f) {
        div -= 1;
    }
    return div;
}

inline float floorMod(float a, float b) {
    int div = floorDiv(a, b);
    return a - b * float(div);
}

inline int ceilDiv(int a, int b) {
    int div = a/b;
    if (a > 0 && a % b != 0) {
        div += 1;
    }
    return div;
}


/**
 * Returns the number of bits set in the passed 32-bit unsigned integer number.
 * For more details see:
 * https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
 */
inline uint32_t getNumberOfBitsSet(uint32_t number) {
#if _cplusplus >= 201907L
    return std::popcount(number);
#elif defined(__GNUC__)
    return __builtin_popcount(number);
#else
    number = number - ((number >> 1) & 0x55555555);
    number = (number & 0x33333333) + ((number >> 2) & 0x33333333);
    number = (number + (number >> 4)) & 0x0F0F0F0F;
    return (number * 0x01010101) >> 24;
#endif
}


/// Interpolation
template <typename T> T interpolateLinear(const T &val1, const T &val2, float factor)
{
    return val1 + factor * (val2 - val1);
}

template <typename T> T interpolateHermite(const T &val1, const T &tangent1, const T &val2, const T &tangent2, float factor)
{
    T A(val1 * 2.0f - val2 * 2.0f + tangent1 + tangent2);
    T B(val2 * 3.0f - val1 * 3.0f - tangent1 * 2.0f - tangent2);
    return A * factor * factor * factor + B * factor * factor + tangent1 * factor + val1;
}

template <typename T> T interpolateBilinear(const T &a, const T &b, const T &c, const T &d, float factorx, float factory)
{
    T p(a + factorx * (b - a));
    T q(c + factorx * (d - c));
    return p + factory * (q - p);
}

DLL_OBJECT float vectorAngle(const glm::vec2 &u, const glm::vec2& v);

}

/*! SRC_MATH_MATH_HPP_ */
#endif
