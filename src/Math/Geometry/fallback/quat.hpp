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

#ifndef SGL_GEOMETRY_FALLBACK_QUAT_HPP
#define SGL_GEOMETRY_FALLBACK_QUAT_HPP

// Drop-in replacement for glm.

#include <stdexcept>
#include <cmath>

namespace glm {

class DLL_OBJECT quat {
public:
    quat() : x(0), y(0), z(0), w(1) {}
    quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    explicit quat(float val) : x(val), y(val), z(val), w(val) {}
    union { float x, r; };
    union { float y, g; };
    union { float z, b; };
    union { float w, a; };
    constexpr float& operator[](int i) {
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
    constexpr float const& operator[](int i) const {
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
    inline quat& operator*=(quat const& rhs) {
        throw std::runtime_error(
                "Fatal error: Quaternion multiplication is unimplemented. Please compile with GLM support enabled.");
        return *this;
    }
};
inline quat operator+(quat const& lhs, quat const& rhs) {
    return quat(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}
inline quat operator-(quat const& lhs, quat const& rhs) {
    return quat(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z,lhs.w - rhs.w);
}
inline quat operator*(quat const& lhs, quat const& rhs) {
    throw std::runtime_error(
            "Fatal error: Quaternion multiplication is unimplemented. Please compile with GLM support enabled.");
    return quat(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}
inline quat operator/(quat const& lhs, quat const& rhs) {
    return quat(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
}
inline quat operator*(quat const& q, float scalar) {
    return quat(q.x * scalar, q.y * scalar, q.z * scalar, q.w * scalar);
}
inline quat operator*(float scalar, quat const& q) {
    return quat(scalar * q.x, scalar * q.y, scalar * q.z, scalar * q.w);
}
inline float dot(quat const& q0, quat const& q1) {
    return q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
}
inline float length(quat const& q) {
    return std::sqrt(dot(q, q));
}
inline quat slerp(quat const& q0, quat const& q1, float t) {
    throw std::runtime_error(
            "Fatal error: slerp(q0, q1, t) is unimplemented. Please compile with GLM support enabled.");
    return quat();
}

}

#endif //SGL_GEOMETRY_FALLBACK_QUAT_HPP
