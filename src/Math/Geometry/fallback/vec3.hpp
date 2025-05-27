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

#ifndef SGL_GEOMETRY_FALLBACK_VEC3_HPP
#define SGL_GEOMETRY_FALLBACK_VEC3_HPP

// Drop-in replacement for glm.

#include <cmath>
#include "util.hpp"

namespace glm {

template<typename T> class tvec4;

template<class T> class DLL_OBJECT tvec3 {
public:
    tvec3() : x(0), y(0), z(0) {}
    tvec3(T x, T y, T z) : x(x), y(y), z(z) {}
    tvec3(tvec4<T> const& v4);
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
    tvec3<T>& operator-() {
        x = -x;
        y = -y;
        z = -z;
        return *this;
    }
    tvec3<T> operator-() const {
        return tvec3<T>(-x, -y, -z);
    }
    tvec3<T>& operator+=(tvec3<T> const& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    tvec3<T>& operator-=(tvec3<T> const& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
    tvec3<T>& operator*=(T rhs) {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }
    tvec3<T>& operator/=(T rhs) {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        return *this;
    }
};
template<class T> bool operator==(tvec3<T> const& lhs, tvec3<T> const& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}
template<class T> bool operator!=(tvec3<T> const& lhs, tvec3<T> const& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}
template<class T> tvec3<T> operator+(tvec3<T> const& lhs, T scalar) {
    return tvec3<T>(lhs.x + scalar, lhs.y + scalar, lhs.z + scalar);
}
template<class T> tvec3<T> operator-(tvec3<T> const& lhs, T scalar) {
    return tvec3<T>(lhs.x - scalar, lhs.y - scalar, lhs.z - scalar);
}
template<class T> tvec3<T> operator+(tvec3<T> const& lhs, tvec3<T> const& rhs) {
    return tvec3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z + rhs.z);
}
template<class T> tvec3<T> operator-(tvec3<T> const& lhs, tvec3<T> const& rhs) {
    return tvec3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}
template<class T> tvec3<T> operator*(tvec3<T> const& lhs, tvec3<T> const& rhs) {
    return tvec3<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}
template<class T> tvec3<T> operator/(tvec3<T> const& lhs, tvec3<T> const& rhs) {
    return tvec3<T>(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
}
template<class T> tvec3<T> operator*(tvec3<T> const& v, T scalar) {
    return tvec3<T>(v.x * scalar, v.y * scalar, v.z * scalar);
}
template<class T> tvec3<T> operator*(T scalar, tvec3<T> const& v) {
    return tvec3<T>(scalar * v.x, scalar * v.y, scalar * v.z);
}
template<class T> tvec3<T> operator/(tvec3<T> const& v, T scalar) {
    return tvec3<T>(v.x / scalar, v.y / scalar, v.z / scalar);
}
template<class T> tvec3<T> operator/(T scalar, tvec3<T> const& v) {
    return tvec3<T>(scalar / v.x, scalar / v.y, scalar / v.z);
}
template<class T> T const* value_ptr(tvec3<T> const& v) {
    return &v.x;
}
template<class T> constexpr tvec3<T> min(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<T>(min(v0.x, v1.x), min(v0.y, v1.y), min(v0.z, v1.z));
}
template<class T> constexpr tvec3<T> max(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<T>(max(v0.x, v1.x), max(v0.y, v1.y), max(v0.z, v1.z));
}
template<class T> tvec3<T> cross(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<T>(
            v0.y * v1.z - v0.z * v1.y,
            v0.z * v1.x - v0.x * v1.z,
            v0.x * v1.y - v0.y * v1.x);
}
template<class T> T dot(tvec3<T> const& v0, tvec3<T> const& v1) {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}
template<class T> T length(tvec3<T> const& v) {
    return std::sqrt(dot(v, v));
}
template<class T> T distance(tvec3<T> const& v0, tvec3<T> const& v1) {
    return length(v0 - v1);
}
template<class T> tvec3<T> normalize(tvec3<T> const& v) {
    return v / length(v);
}
template<class T> tvec3<T> clamp(tvec3<T> const& v, const T& lo, const T& hi) {
    return tvec3<T>(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi));
}
template<class T> tvec3<T> clamp(tvec3<T> const& v, tvec3<T> const& lo, tvec3<T> const& hi) {
    return tvec3<T>(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z));
}
template<class T> tvec3<T> pow(tvec3<T> const& v, const T& x) {
    return tvec3<T>(pow(v.x, x), pow(v.y, x), pow(v.z, x));
}
template<class T> tvec3<T> pow(tvec3<T> const& v, tvec3<T> const& x) {
    return tvec3<T>(pow(v.x, x.x), pow(v.y, x.y), pow(v.z, x.z));
}
template<class T, class Ta> tvec3<T> mix(const tvec3<T>& x, const tvec3<T>& y, const Ta& a) {
    return x * (T(1) - a) + y * a;
}
template<class T, class Ta> tvec3<T> mix(const tvec3<T>& x, const tvec3<T>& y, const tvec3<Ta>& a) {
    return tvec3<T>(mix(x.x, y.x, a.x), mix(x.y, y.y, a.y), mix(x.z, y.z, a.z));
}
template<class T> tvec3<bool> equal(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<bool>(v0.x == v1.x, v0.y == v1.y, v0.z == v1.z);
}
template<class T> tvec3<bool> notEqual(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<bool>(v0.x != v1.x, v0.y != v1.y, v0.z != v1.z);
}
template<class T> tvec3<bool> lessThan(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<bool>(v0.x < v1.x, v0.y < v1.y, v0.z < v1.z);
}
template<class T> tvec3<bool> lessThanEqual(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<bool>(v0.x <= v1.x, v0.y <= v1.y, v0.z <= v1.z);
}
template<class T> tvec3<bool> greaterThan(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<bool>(v0.x < v1.x, v0.y < v1.y, v0.z < v1.z);
}
template<class T> tvec3<bool> greaterThanEqual(tvec3<T> const& v0, tvec3<T> const& v1) {
    return tvec3<bool>(v0.x <= v1.x, v0.y <= v1.y, v0.z <= v1.z);
}
typedef tvec3<float> vec3;
typedef tvec3<double> dvec3;
typedef tvec3<int> ivec3;
typedef tvec3<unsigned int> uvec3;
typedef tvec3<bool> bvec3;

template<class T> tvec3<T>::tvec3(tvec4<T> const& v4) : x(v4.x), y(v4.y), z(v4.z) {}

}

#endif //SGL_GEOMETRY_FALLBACK_VEC3_HPP
