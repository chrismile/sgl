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

#ifndef SGL_GEOMETRY_FALLBACK_MAT_HPP
#define SGL_GEOMETRY_FALLBACK_MAT_HPP

// Drop-in replacement for glm.

#include <stdexcept>
#include "vec3.hpp"
#include "vec4.hpp"

namespace glm {

class mat4;

class DLL_OBJECT mat3 {
public:
    explicit mat3() : value{vec3(0), vec3(0), vec3(0)} {}
    explicit mat3(float val) : value{vec3(val, 0, 0), vec3(0, val, 0), vec3(0, 0, val)} {}
    explicit mat3(mat4 const& m);
    mat3(
            float x0, float y0, float z0,
            float x1, float y1, float z1,
            float x2, float y2, float z2)
            : value{vec3(x0, y0, z0), vec3(x1, y1, z1), vec3(x2, y2, z2)} {}
    mat3(vec3 const& v0, vec3 const& v1, vec3 const& v2) : value{v0, v1, v2} {}

    vec3 value[3];
    constexpr vec3& operator[](int i) {
        return value[i];
    }
    constexpr vec3 const& operator[](int i) const {
        return value[i];
    }
};
inline float const* value_ptr(mat3 const& m) {
    return &m.value[0].x;
}
inline vec3 operator*(mat3 const& m, vec3 const& v) {
    return vec3(
            m.value[0].x * v.x + m.value[1].x * v.y + m.value[2].x * v.z,
            m.value[0].y * v.x + m.value[1].y * v.y + m.value[2].y * v.z,
            m.value[0].z * v.x + m.value[1].z * v.y + m.value[2].z * v.z);
}
inline mat3 operator*(mat3 const& m0, mat3 const& m1) {
    auto result = mat3();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i][j] = m0[0][j] * m1[i][0] + m0[1][j] * m1[i][1] + m0[2][j] * m1[i][2];
        }
    }
    return result;
}
inline mat3 transpose(mat3 const& m) {
    return mat3(
            m[0][0], m[1][0], m[2][0],
            m[0][1], m[1][1], m[2][1],
            m[0][2], m[1][2], m[2][2]);
}
inline mat3 inverse(mat3 const& m) {
    auto result = mat3();
    const float c0 = m[1][1] * m[2][2] - m[2][1] * m[1][2];
    const float c1 = m[1][2] * m[2][0] - m[2][2] * m[1][0];
    const float c2 = m[1][0] * m[2][1] - m[2][0] * m[1][1];
    const float inverseDeterminant = 1.0f / (m[0][0] * c0 + m[0][1] * c1 + m[0][2] * c2);
    result[0][0] = c0 * inverseDeterminant;
    result[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * inverseDeterminant;
    result[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * inverseDeterminant;
    result[1][0] = c1 * inverseDeterminant;
    result[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * inverseDeterminant;
    result[1][2] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * inverseDeterminant;
    result[2][0] = c2 * inverseDeterminant;
    result[2][1] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * inverseDeterminant;
    result[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * inverseDeterminant;
    return result;
}

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
inline float const* value_ptr(mat4 const& m) {
    return &m.value[0].x;
}
inline vec4 operator*(mat4 const& m, vec4 const& v) {
    return vec4(
            m.value[0].x * v.x + m.value[1].x * v.y + m.value[2].x * v.z + m.value[3].x * v.w,
            m.value[0].y * v.x + m.value[1].y * v.y + m.value[2].y * v.z + m.value[3].y * v.w,
            m.value[0].z * v.x + m.value[1].z * v.y + m.value[2].z * v.z + m.value[3].z * v.w,
            m.value[0].w * v.x + m.value[1].w * v.y + m.value[2].w * v.z + m.value[3].w * v.w);
}
inline mat4 operator*(mat4 const& m0, mat4 const& m1) {
    auto result = mat4();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i][j] = m0[0][j] * m1[i][0] + m0[1][j] * m1[i][1] + m0[2][j] * m1[i][2] + m0[3][j] * m1[i][3];
        }
    }
    return result;
}
inline mat4 transpose(mat4 const& m) {
    return mat4(
            m[0][0], m[1][0], m[2][0], m[3][0],
            m[0][1], m[1][1], m[2][1], m[3][1],
            m[0][2], m[1][2], m[2][2], m[3][2],
            m[0][3], m[1][3], m[2][3], m[3][3]);
}
inline mat4 inverse(mat4 const& m) {
    throw std::runtime_error(
            "Fatal error: Matrix inversion is unimplemented. Please compile with GLM support enabled.");
    return mat4();
}

inline mat3::mat3(mat4 const& m) : value{
    vec3(m[0][0], m[0][1], m[0][2]), vec3(m[1][0], m[1][1], m[1][2]), vec3(m[2][0], m[2][1], m[2][2])} {}

}

#endif //SGL_GEOMETRY_FALLBACK_MAT_HPP
