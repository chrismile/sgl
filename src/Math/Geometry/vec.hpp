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

#ifndef SGL_VEC_HPP
#define SGL_VEC_HPP

// Drop-in replacement for glm.

namespace glm {

template<class T> class DLL_OBJECT tvec2 {
public:
    tvec2() : x(0), y(0) {}
    tvec2(T x, T y) : x(x), y(y) {}
    explicit tvec2(T val) : x(val), y(val) {}
    union { T x, r; };
    union { T y, g; };
    constexpr T& operator[](int i) {
        switch(i) {
            default:
            case 0:
                return x;
            case 1:
                return y;
        }
    }
    constexpr T const& operator[](int i) const {
        switch(i) {
            default:
            case 0:
                return x;
            case 1:
                return y;
        }
    }
};
typedef tvec2<float> vec2;
typedef tvec2<double> dvec2;
typedef tvec2<int> ivec2;
typedef tvec2<unsigned int> uvec2;

template<class T> class DLL_OBJECT tvec3 {
public:
    tvec3() : x(0), y(0), z(0) {}
    tvec3(T x, T y, T z) : x(x), y(y), z(z) {}
    explicit tvec3(T val) : x(val), y(val), z(val) {}
    union { T x, r; };
    union { T y, g; };
    union { T z, b; };
    constexpr T& operator[](int i) {
        switch(i) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
        }
    }
    constexpr T const& operator[](int i) const {
        switch(i) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
        }
    }
};
typedef tvec3<float> vec3;
typedef tvec3<double> dvec3;
typedef tvec3<int> ivec3;
typedef tvec3<unsigned int> uvec3;

template<class T> class DLL_OBJECT tvec4 {
public:
    tvec4() : x(0), y(0), z(0), w(0) {}
    tvec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    explicit tvec4(T val) : x(val), y(val), z(val), w(val) {}
    union { T x, r; };
    union { T y, g; };
    union { T z, b; };
    union { T w, a; };
    constexpr T& operator[](int i) {
        switch(i) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            case 3:
                return w;
        }
    }
    constexpr T const& operator[](int i) const {
        switch(i) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            case 3:
                return w;
        }
    }
};
typedef tvec4<float> vec4;
typedef tvec4<double> dvec4;
typedef tvec4<int> ivec4;
typedef tvec4<unsigned int> uvec4;

class DLL_OBJECT mat4 {
public:
    explicit mat4() : value{vec4(0), vec4(0), vec4(0), vec4(0)} {}
    explicit mat4(float val) : value{vec4(val, 0, 0, 0), vec4(0, val, 0, 0), vec4(0, 0, val, 0), vec4(0, 0, 0, val)} {}
    mat4(
            float x0, float y0, float z0, float w0,
            float x1, float y1, float z1, float w1,
            float x2, float y2, float z2, float w2,
            float x3, float y3, float z3, float w3)
            : value{vec4(x0, y0, z0, w0), vec4(x1, y1, z1, w1), vec4(x2, y2, z2, w2), vec4(x3, y3, z3, w3)} {}
    mat4(vec4 const& v0, vec4 const& v1, vec4 const& v2, vec4 const& v3) : value{v0, v1, v2, v3} {}

    vec4 value[4];
    constexpr vec4& operator[](int i) {
        return value[i];
    }
    constexpr vec4 const& operator[](int i) const {
        return value[i];
    }
};

inline mat4 transpose(mat4 const& m) {
    return mat4(
            m[0][0], m[1][0], m[2][0], m[3][0],
            m[0][1], m[1][1], m[2][1], m[3][1],
            m[0][2], m[1][2], m[2][2], m[3][2],
            m[0][3], m[1][3], m[2][3], m[3][3]);
}

}

#endif //SGL_VEC_HPP
