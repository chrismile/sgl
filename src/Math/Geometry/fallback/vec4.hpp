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

#ifndef SGL_GEOMETRY_FALLBACK_VEC4_HPP
#define SGL_GEOMETRY_FALLBACK_VEC4_HPP

// Drop-in replacement for glm.

#include <stdexcept>
#include <cmath>
#include "util.hpp"

namespace glm {

template<typename T> class tvec3;

template<class T> class DLL_OBJECT tvec4 {
public:
    tvec4() : x(0), y(0), z(0), w(0) {}
    tvec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    tvec4(tvec3<T> const& xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
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
    tvec4<T>& operator-() {
        x = -x;
        y = -y;
        z = -z;
        w = -w;
        return *this;
    }
    tvec4<T> operator-() const {
        return tvec4<T>(-x, -y, -z, -w);
    }
    tvec4<T>& operator+=(tvec4<T> const& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }
    tvec4<T>& operator-=(tvec4<T> const& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }
    tvec4<T>& operator*=(T rhs) {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
        return *this;
    }
    tvec4<T>& operator/=(T rhs) {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        w /= rhs;
        return *this;
    }
};
template<class T> bool operator==(tvec4<T> const& lhs, tvec4<T> const& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}
template<class T> bool operator!=(tvec4<T> const& lhs, tvec4<T> const& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}
template<class T> tvec4<T> operator+(tvec4<T> const& lhs, T scalar) {
    return tvec4<T>(lhs.x + scalar, lhs.y + scalar, lhs.z + scalar, lhs.w + scalar);
}
template<class T> tvec4<T> operator-(tvec4<T> const& lhs, T scalar) {
    return tvec4<T>(lhs.x - scalar, lhs.y - scalar, lhs.z - scalar, lhs.w - scalar);
}
template<class T> tvec4<T> operator+(tvec4<T> const& lhs, tvec4<T> const& rhs) {
    return tvec4<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}
template<class T> tvec4<T> operator-(tvec4<T> const& lhs, tvec4<T> const& rhs) {
    return tvec4<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}
template<class T> tvec4<T> operator*(tvec4<T> const& lhs, tvec4<T> const& rhs) {
    return tvec4<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}
template<class T> tvec4<T> operator/(tvec4<T> const& lhs, tvec4<T> const& rhs) {
    return tvec4<T>(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
}
template<class T> tvec4<T> operator*(tvec4<T> const& v, T scalar) {
    return tvec4<T>(v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar);
}
template<class T> tvec4<T> operator*(T scalar, tvec4<T> const& v) {
    return tvec4<T>(scalar * v.x, scalar * v.y, scalar * v.z, scalar * v.w);
}
template<class T> tvec4<T> operator/(tvec4<T> const& v, T scalar) {
    return tvec4<T>(v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar);
}
template<class T> tvec4<T> operator/(T scalar, tvec4<T> const& v) {
    return tvec4<T>(scalar / v.x, scalar / v.y, scalar / v.z, scalar / v.w);
}
template<class T> T const* value_ptr(tvec4<T> const& v) {
    return &v.x;
}
template<class T> constexpr tvec4<T> min(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<T>(min(v0.x, v1.x), min(v0.y, v1.y), min(v0.z, v1.z), min(v0.w, v1.w));
}
template<class T> constexpr tvec4<T> max(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<T>(max(v0.x, v1.x), max(v0.y, v1.y), max(v0.z, v1.z), max(v0.w, v1.w));
}
template<class T> T dot(tvec4<T> const& v0, tvec4<T> const& v1) {
    return v0.x * v1.x + v0.x * v1.y + v0.x * v1.z + v0.x * v1.w;
}
template<class T> tvec4<T> clamp(tvec4<T> const& v, const T& lo, const T& hi) {
    return tvec4<T>(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi));
}
template<class T> tvec4<T> clamp(tvec4<T> const& v, tvec4<T> const& lo, tvec4<T> const& hi) {
    return tvec4<T>(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w));
}
template<class T> tvec4<T> pow(tvec4<T> const& v, const T& x) {
    return tvec4<T>(pow(v.x, x), pow(v.y, x), pow(v.z, x), pow(v.w, x));
}
template<class T> tvec4<T> pow(tvec4<T> const& v, tvec4<T> const& x) {
    return tvec4<T>(pow(v.x, x.x), pow(v.y, x.y), pow(v.z, x.z), pow(v.w, x.w));
}
template<class T, class Ta> tvec4<T> mix(const tvec4<T>& x, const tvec4<T>& y, const Ta& a) {
    return x * (T(1) - a) + y * a;
}
template<class T, class Ta> tvec4<T> mix(const tvec4<T>& x, const tvec4<T>& y, const tvec4<Ta>& a) {
    return tvec4<T>(mix(x.x, y.x, a.x), mix(x.y, y.y, a.y), mix(x.z, y.z, a.z), mix(x.w, y.w, a.w));
}
template<class T> tvec4<bool> equal(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<bool>(v0.x == v1.x, v0.y == v1.y, v0.z == v1.z, v0.w == v1.w);
}
template<class T> tvec4<bool> notEqual(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<bool>(v0.x != v1.x, v0.y != v1.y, v0.z != v1.z, v0.w != v1.w);
}
template<class T> tvec4<bool> lessThan(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<bool>(v0.x < v1.x, v0.y < v1.y, v0.z < v1.z, v0.w < v1.w);
}
template<class T> tvec4<bool> lessThanEqual(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<bool>(v0.x <= v1.x, v0.y <= v1.y, v0.z <= v1.z, v0.w <= v1.w);
}
template<class T> tvec4<bool> greaterThan(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<bool>(v0.x < v1.x, v0.y < v1.y, v0.z < v1.z, v0.w < v1.w);
}
template<class T> tvec4<bool> greaterThanEqual(tvec4<T> const& v0, tvec4<T> const& v1) {
    return tvec4<bool>(v0.x <= v1.x, v0.y <= v1.y, v0.z <= v1.z, v0.w <= v1.w);
}
typedef tvec4<float> vec4;
typedef tvec4<double> dvec4;
typedef tvec4<int> ivec4;
typedef tvec4<unsigned int> uvec4;
typedef tvec4<bool> bvec4;

}

#endif //SGL_GEOMETRY_FALLBACK_VEC4_HPP
