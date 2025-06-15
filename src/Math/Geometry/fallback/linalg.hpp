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

#ifndef SGL_GEOMETRY_FALLBACK_LINALG_HPP
#define SGL_GEOMETRY_FALLBACK_LINALG_HPP

// Drop-in replacement for glm.

#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "mat.hpp"
#include "quat.hpp"

namespace glm {

template <class...> constexpr std::false_type templated_false{};
template<class T> T identity() {
    if constexpr (std::is_same_v<T, mat3>) {
        return mat3(
                1, 0, 0,
                0, 1, 0,
                0, 0, 1);
    } else if constexpr (std::is_same_v<T, mat4>) {
        return mat4(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
    } else if constexpr (std::is_same_v<T, quat>) {
        return quat(0, 0, 0, 1);
    } else {
        static_assert(templated_false<T>, "Unsupported type passed to identity<T>().");
    }
}

inline quat quat_cast(mat3 const& m) {
    throw std::runtime_error("Fatal error: quat_cast() is unimplemented. Please compile with GLM support enabled.");
    return quat();
}
inline quat quat_cast(mat4 const& m) {
    throw std::runtime_error("Fatal error: quat_cast() is unimplemented. Please compile with GLM support enabled.");
    return quat();
}
inline mat3 mat3_cast(quat const& q) {
    throw std::runtime_error("Fatal error: quat_cast() is unimplemented. Please compile with GLM support enabled.");
    return mat3();
}
inline mat4 toMat4(quat const& q) {
    throw std::runtime_error("Fatal error: toMat4() is unimplemented. Please compile with GLM support enabled.");
    return mat4();
}
inline quat angleAxis(float angle, vec3 const& axis) {
    throw std::runtime_error(
            "Fatal error: angleAxis(angle, axis) is unimplemented. Please compile with GLM support enabled.");
    return quat();
}
inline mat4 scale(vec3 const& v) {
    return mat4(
            v.x,  0.0f, 0.0f, 0.0f,
            0.0f, v.y,  0.0f, 0.0f,
            0.0f, 0.0f, v.z,  0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}
inline mat4 rotate(float angle, vec3 const& axis) {
    throw std::runtime_error(
            "Fatal error: rotate(m, angle, axis) is unimplemented. Please compile with GLM support enabled.");
    return mat4();
}
inline mat4 rotate(mat4 const& m, float angle, vec3 const& axis) {
    throw std::runtime_error(
            "Fatal error: rotate(m, angle, axis) is unimplemented. Please compile with GLM support enabled.");
}
inline mat4 lookAt(vec3 const& v0, vec3 const& v1, vec3 const& v2) {
    throw std::runtime_error(
            "Fatal error: lookAt(v0, v1, v2) is unimplemented. Please compile with GLM support enabled.");
    return mat4();
}
inline mat4 perspective(float fovy, float aspect, float nearDist, float farDist) {
    throw std::runtime_error(
            "Fatal error: perspective(fovy, aspect, nearDist, farDist) is unimplemented. "
            "Please compile with GLM support enabled.");
    return mat4();
}

}

#endif //SGL_GEOMETRY_FALLBACK_LINALG_HPP
