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

#ifndef SGL_GEOMETRY_FALLBACK_VEC2_HPP
#define SGL_GEOMETRY_FALLBACK_VEC2_HPP

// Drop-in replacement for glm.

#include <cmath>
#include "util.hpp"

namespace glm {

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

}

#endif //SGL_GEOMETRY_FALLBACK_VEC2_HPP
