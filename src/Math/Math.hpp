/*!
 * Math.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_MATH_MATH_HPP_
#define SRC_MATH_MATH_HPP_

#include <cmath>
#include <glm/fwd.hpp>

//! Collection of math utility functions
namespace sgl
{

const float PI = 3.1415926535897932f;
const float TWO_PI = PI * 2.0f;
const float HALF_PI = PI / 2.0f;

inline float abs(float a) { return a > 0 ? a : -a; }
inline float abs(int a) { return a > 0 ? a : -a; }
inline bool floatEquals(float a, float b) { return sgl::abs(a - b) < 0.0001f; }
inline bool floatEquals(float a, float b, float dt) { return sgl::abs(a - b) < dt; }
inline bool floatLess(float a, float b) { return a < b + 0.0001f; }
inline bool floatLess(float a, float b, float dt) { return a < b + dt; }
template <typename T> inline T clamp(T val, T min, T max) { return val < min ? min : (val > max ? max : val); }
inline float ceil(float val) { return std::ceil(val); }
inline float exp(float val) { return std::exp(val); }
inline float sqrt(float val) { return std::sqrt(val); }
inline float sqr(float val) { return val*val; }
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
inline float atan2(float x, float y) { return std::atan2(x, y); }
inline float degToRad(float val) { return val / 180.0f * PI; }
inline float radToDeg(float val) { return val * 180.0f / PI; }

inline bool isPowerOfTwo(int x) { return (x != 0) && ((x & (x - 1)) == 0); }
inline int nextPowerOfTwo(int x) { --x; x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16; return x+1; }
inline int lastPowerOfTwo(int x) { return nextPowerOfTwo(x/2+1); }

inline int intlog2(int x) {
    int log2x = 0;
    while ((x >>= 1) != 0) {
        ++log2x;
    }
    return log2x;
}

inline int floorDiv(int a, int b) {
    int div = a/b;
    if (a < 0 && a%b != 0) {
        div -= 1;
    }
    return div;
}

inline int floorMod(int a, int b) {
    int div = floorDiv(a, b);
    return a-b*div;
}

inline int floorDiv(float a, float b) {
    int div = a/b;
    if (a < 0 && fmod(a,b) != 0) {
        div -= 1;
    }
    return div;
}

inline float floorMod(float a, float b) {
    int div = floorDiv(a, b);
    return a-b*div;
}

inline int ceilDiv(int a, int b) {
    int div = a/b;
    if (a > 0 && a%b != 0) {
        div += 1;
    }
    return div;
}

//! Interpolation
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

float vectorAngle(const glm::vec2 &u, const glm::vec2& v);

}

/*! SRC_MATH_MATH_HPP_ */
#endif
