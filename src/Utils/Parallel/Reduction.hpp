/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Christoph Neuhauser
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
#include <glm/vec3.hpp>

namespace sgl {

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

/*
 * Functions for the parallel min-max reduction of a vec3 array.
 */
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
};
struct max_predicate {
    template<class T, class U>
    constexpr decltype(auto) operator()(T&& t, U&& u) const{
        return t < u ? std::forward<U>(u) : std::forward<T>(t);
    }
};

}

#endif //SGL_REDUCTION_HPP
