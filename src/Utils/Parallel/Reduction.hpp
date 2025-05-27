/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022-2024, Christoph Neuhauser
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

#ifndef SGL_REDUCTION_HPP
#define SGL_REDUCTION_HPP

#include <vector>
#include <map>

#ifdef USE_GLM
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#else
#include <Math/Geometry/fallback/vec2.hpp>
#include <Math/Geometry/fallback/vec3.hpp>
#endif

class HalfFloat;

namespace sgl {

class AABB2;
class AABB3;

/*
 * Functions for the parallel min-max reduction of a float array.
 * The first entry of the returned pair stores the minimum value, the second the maximum value.
 */
DLL_OBJECT std::pair<float, float> reduceFloatArrayMinMax(
        const std::vector<float>& floatValues, std::pair<float, float> init);
DLL_OBJECT std::pair<float, float> reduceFloatArrayMinMax(
        const float* floatValues, size_t N, std::pair<float, float> init);
inline std::pair<float, float> reduceFloatArrayMinMax(const std::vector<float>& floatValues) {
    return reduceFloatArrayMinMax(
            floatValues, std::make_pair(std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest()));
}
inline std::pair<float, float> reduceFloatArrayMinMax(const float* floatValues, size_t N) {
    return reduceFloatArrayMinMax(
            floatValues, N, std::make_pair(std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest()));
}

// For 8-bit and 16-bit UNORM data (i.e., integer values normalized to [0, 1]).
DLL_OBJECT std::pair<float, float> reduceUnormByteArrayMinMax(
        const uint8_t* values, size_t N, std::pair<float, float> init);
inline std::pair<float, float> reduceUnormByteArrayMinMax(const uint8_t* values, size_t N) {
    return reduceUnormByteArrayMinMax(
            values, N, std::make_pair(std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest()));
}
DLL_OBJECT std::pair<float, float> reduceUnormShortArrayMinMax(
        const uint16_t* values, size_t N, std::pair<float, float> init);
inline std::pair<float, float> reduceUnormShortArrayMinMax(const uint16_t* values, size_t N) {
    return reduceUnormShortArrayMinMax(
            values, N, std::make_pair(std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest()));
}

// For half float / float16 data.
DLL_OBJECT std::pair<float, float> reduceHalfFloatArrayMinMax(
        const HalfFloat* values, size_t N, std::pair<float, float> init);
inline std::pair<float, float> reduceHalfFloatArrayMinMax(const HalfFloat* values, size_t N) {
    return reduceHalfFloatArrayMinMax(
            values, N, std::make_pair(std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest()));
}

/*
 * Functions for the parallel min-max reduction of vec2 and vec3 array.
 */
DLL_OBJECT sgl::AABB2 reduceVec2ArrayAabb(const std::vector<glm::vec2>& positions);
DLL_OBJECT sgl::AABB3 reduceVec3ArrayAabb(const std::vector<glm::vec3>& positions);
DLL_OBJECT std::pair<float, float> reductionFunctionFloatMinMax(
        std::pair<float, float> lhs, std::pair<float, float> rhs);

/*
 * Predicates for the use with TBB in the minimum and maximum reduction.
 * STL only provides predicates for algebraic and logical operations, like "std::plus<>{}".
 */
struct min_predicate {
    template<class T, class U>
    constexpr decltype(auto) operator()(T&& t, U&& u) const{
        return t > u ? std::forward<U>(u) : std::forward<T>(t);
    }
    template<class T, class U>
    constexpr decltype(auto) operator()(const T& t, const U& u) const{
        return t > u ? u : t;
    }
};
struct max_predicate {
    template<class T, class U>
    constexpr decltype(auto) operator()(T&& t, U&& u) const{
        return t < u ? std::forward<U>(u) : std::forward<T>(t);
    }
    template<class T, class U>
    constexpr decltype(auto) operator()(const T& t, const U& u) const{
        return t < u ? u : t;
    }
};

struct plus_pair_predicate {
    template<class T, class U>
    constexpr decltype(auto) operator()(std::pair<T, U>&& t, std::pair<T, U>&& u) const{
        return std::make_pair(t.first + u.first, t.second + u.second);
    }
    template<class T, class U>
    constexpr decltype(auto) operator()(const std::pair<T, U>& t, const std::pair<T, U>& u) const{
        return std::make_pair(t.first + u.first, t.second + u.second);
    }
};

}

#endif //SGL_REDUCTION_HPP
