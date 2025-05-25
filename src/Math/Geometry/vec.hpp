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

template<class T> class DLL_OBJECT tvec2 {
public:
    tvec2() : x(0), y(0) {}
    tvec2(T x, T y) : x(x), y(y) {}
    explicit tvec2(T val) : x(val), y(val) {}
    template<class Tp> tvec2(tvec2<Tp> const& val) : x(T(val.x)), y(T(val.y)) {}
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
    tvec2<T>& operator-() {
        x = -x;
        y = -y;
        return *this;
    }
    tvec2<T>& operator+=(tvec2<T> const& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    tvec2<T>& operator-=(tvec2<T> const& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    tvec2<T> operator-() const {
        return tvec2<T>(-x, -y);
    }
    tvec2<T>& operator*=(T rhs) {
        x *= rhs;
        y *= rhs;
        return *this;
    }
    tvec2<T>& operator/=(T rhs) {
        x /= rhs;
        y /= rhs;
        return *this;
    }
};
template<class T> bool operator==(tvec2<T> const& lhs, tvec2<T> const& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}
template<class T> bool operator!=(tvec2<T> const& lhs, tvec2<T> const& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
}
template<class T> tvec2<T> operator+(tvec2<T> const& lhs, T scalar) {
    return tvec2<T>(lhs.x + scalar, lhs.y + scalar);
}
template<class T> tvec2<T> operator-(tvec2<T> const& lhs, T scalar) {
    return tvec2<T>(lhs.x - scalar, lhs.y - scalar);
}
template<class T> tvec2<T> operator+(tvec2<T> const& lhs, tvec2<T> const& rhs) {
    return tvec2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}
template<class T> tvec2<T> operator-(tvec2<T> const& lhs, tvec2<T> const& rhs) {
    return tvec2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}
template<class T> tvec2<T> operator*(tvec2<T> const& lhs, tvec2<T> const& rhs) {
    return tvec2<T>(lhs.x * rhs.x, lhs.y * rhs.y);
}
template<class T> tvec2<T> operator/(tvec2<T> const& lhs, tvec2<T> const& rhs) {
    return tvec2<T>(lhs.x / rhs.x, lhs.y / rhs.y);
}
template<class T> tvec2<T> operator*(tvec2<T> const& v, T scalar) {
    return tvec2<T>(v.x * scalar, v.y * scalar);
}
template<class T> tvec2<T> operator*(T scalar, tvec2<T> const& v) {
    return tvec2<T>(scalar * v.x, scalar * v.y);
}
template<class T> tvec2<T> operator/(tvec2<T> const& v, T scalar) {
    return tvec2<T>(v.x / scalar, v.y / scalar);
}
template<class T> tvec2<T> operator/(T scalar, tvec2<T> const& v) {
    return tvec2<T>(scalar / v.x, scalar / v.y);
}
template<class T> T const* value_ptr(tvec2<T> const& v) {
    return &v.x;
}
template<class T> constexpr tvec2<T> min(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<T>(min(v0.x, v1.x), min(v0.y, v1.y), min(v0.z, v1.z), min(v0.w, v1.w));
}
template<class T> constexpr tvec2<T> max(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<T>(max(v0.x, v1.x), max(v0.y, v1.y), max(v0.z, v1.z), max(v0.w, v1.w));
}
template<class T> T dot(tvec2<T> const& v0, tvec2<T> const& v1) {
    return v0.x * v1.x + v0.y * v1.y;
}
template<class T> T length(tvec2<T> const& v) {
    return std::sqrt(dot(v, v));
}
template<class T> T distance(tvec2<T> const& v0, tvec2<T> const& v1) {
    return length(v0 - v1);
}
template<class T> tvec2<T> normalize(tvec2<T> const& v) {
    return v / length(v);
}
template<class T> tvec2<T> clamp(tvec2<T> const& v, const T& lo, const T& hi) {
    return tvec2<T>(clamp(v.x, lo, hi), clamp(v.y, lo, hi));
}
template<class T> tvec2<T> clamp(tvec2<T> const& v, tvec2<T> const& lo, tvec2<T> const& hi) {
    return tvec2<T>(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y));
}
template<class T> tvec2<T> pow(tvec2<T> const& v, const T& x) {
    return tvec2<T>(pow(v.x, x), pow(v.y, x));
}
template<class T> tvec2<T> pow(tvec2<T> const& v, tvec2<T> const& x) {
    return tvec2<T>(pow(v.x, x.x), pow(v.y, x.y));
}
template<class T, class Ta> tvec2<T> mix(const tvec2<T>& x, const tvec2<T>& y, const Ta& a) {
    return x * (T(1) - a) + y * a;
}
template<class T, class Ta> tvec2<T> mix(const tvec2<T>& x, const tvec2<T>& y, const tvec2<Ta>& a) {
    return tvec2<T>(mix(x.x, y.x, a.x), mix(x.y, y.y, a.y));
}
template<class T> tvec2<bool> equal(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<bool>(v0.x == v1.x, v0.y == v1.y);
}
template<class T> tvec2<bool> notEqual(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<bool>(v0.x != v1.x, v0.y != v1.y);
}
template<class T> tvec2<bool> lessThan(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<bool>(v0.x < v1.x, v0.y < v1.y);
}
template<class T> tvec2<bool> lessThanEqual(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<bool>(v0.x <= v1.x, v0.y <= v1.y);
}
template<class T> tvec2<bool> greaterThan(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<bool>(v0.x < v1.x, v0.y < v1.y);
}
template<class T> tvec2<bool> greaterThanEqual(tvec2<T> const& v0, tvec2<T> const& v1) {
    return tvec2<bool>(v0.x <= v1.x, v0.y <= v1.y);
}
typedef tvec2<float> vec2;
typedef tvec2<double> dvec2;
typedef tvec2<int> ivec2;
typedef tvec2<unsigned int> uvec2;
typedef tvec2<bool> bvec2;

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
template<class T> tvec2<T> clamp(tvec3<T> const& v, const T& lo, const T& hi) {
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
    return tvec2<T>(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi));
}
template<class T> tvec4<T> clamp(tvec4<T> const& v, tvec4<T> const& lo, tvec4<T> const& hi) {
    return tvec4<T>(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w));
}
template<class T> tvec4<T> pow(tvec4<T> const& v, const T& x) {
    return tvec4<T>(pow(v.x, x), pow(v.y, x), pow(v.z, x), pow(v.w, x));
}
template<class T> tvec4<T> pow(tvec4<T> const& v, tvec4<T> const& x) {
    return tvec3<T>(pow(v.x, x.x), pow(v.y, x.y), pow(v.z, x.z), pow(v.w, x.w));
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

template<class T> tvec3<T>::tvec3(tvec4<T> const& v4) : x(v4.x), y(v4.y), z(v4.z) {}


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
    throw std::runtime_error(
            "Fatal error: Matrix-matrix multiplication is unimplemented. Please compile with GLM support enabled.");
    return mat3();
}
inline mat3 transpose(mat3 const& m) {
    return mat3(
            m[0][0], m[1][0], m[2][0],
            m[0][1], m[1][1], m[2][1],
            m[0][2], m[1][2], m[2][2]);
}
inline mat3 inverse(mat3 const& m) {
    throw std::runtime_error(
            "Fatal error: Matrix inversion is unimplemented. Please compile with GLM support enabled.");
    return mat3();
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
    throw std::runtime_error(
            "Fatal error: Matrix-matrix multiplication is unimplemented. Please compile with GLM support enabled.");
    return mat4();
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
        static_assert(false, "Unsupported type passed to identity<T>().");
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

#endif //SGL_VEC_HPP
