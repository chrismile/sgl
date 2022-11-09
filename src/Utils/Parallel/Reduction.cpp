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

#include <limits>
#include <cstddef>

#ifdef USE_TBB
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#endif

#include <Math/Geometry/AABB3.hpp>
#include "Reduction.hpp"

namespace sgl {

std::pair<float, float> reduceFloatArrayMinMax(
        const std::vector<float>& floatValues, std::pair<float, float> init) {
#ifdef USE_TBB

    return tbb::parallel_reduce(
            tbb::blocked_range<size_t>(0, floatValues.size()), init,
            [&floatValues](tbb::blocked_range<size_t> const& r, std::pair<float, float> init) {
                for (auto i = r.begin(); i != r.end(); i++) {
                    float value = floatValues.at(i);
                    init.first = std::min(init.first, value);
                    init.second = std::max(init.second, value);
                }
                return init;
            }, &reductionFunctionFloatMinMax);

#else

    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();
    size_t n = floatValues.size();
#if _OPENMP >= 201107
    #pragma omp parallel for shared(floatValues, n) reduction(min: minValue) reduction(max: maxValue) \
    default(none)
#endif
    for (size_t i = 0; i < n; i++) {
        minValue = std::min(minValue, floatValues.at(i));
        maxValue = std::max(maxValue, floatValues.at(i));
    }
    return std::make_pair(minValue, maxValue);

#endif
}

std::pair<float, float> reduceFloatArrayMinMax(
        const float* floatValues, size_t N, std::pair<float, float> init) {
#ifdef USE_TBB

    return tbb::parallel_reduce(
            tbb::blocked_range<size_t>(0, N), init,
            [&floatValues](tbb::blocked_range<size_t> const& r, std::pair<float, float> init) {
                for (auto i = r.begin(); i != r.end(); i++) {
                    float value = floatValues[i];
                    init.first = std::min(init.first, value);
                    init.second = std::max(init.second, value);
                }
                return init;
            }, &reductionFunctionFloatMinMax);

#else

    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();
#if _OPENMP >= 201107
    #pragma omp parallel for shared(floatValues, N) reduction(min: minValue) reduction(max: maxValue) \
    default(none)
#endif
    for (size_t i = 0; i < N; i++) {
        minValue = std::min(minValue, floatValues[i]);
        maxValue = std::max(maxValue, floatValues[i]);
    }
    return std::make_pair(minValue, maxValue);

#endif
}

sgl::AABB3 reduceVec3ArrayAabb(const std::vector<glm::vec3>& positions) {
#ifdef USE_TBB

    return tbb::parallel_reduce(
            tbb::blocked_range<size_t>(0, positions.size()), sgl::AABB3(),
            [&positions](tbb::blocked_range<size_t> const& r, sgl::AABB3 init) {
                for (auto i = r.begin(); i != r.end(); i++) {
                    const glm::vec3& pt = positions.at(i);
                    init.min.x = std::min(init.min.x, pt.x);
                    init.min.y = std::min(init.min.y, pt.y);
                    init.min.z = std::min(init.min.z, pt.z);
                    init.max.x = std::max(init.max.x, pt.x);
                    init.max.y = std::max(init.max.y, pt.y);
                    init.max.z = std::max(init.max.z, pt.z);
                }
                return init;
            },
            [&](sgl::AABB3 lhs, sgl::AABB3 rhs) -> sgl::AABB3 {
                lhs.combine(rhs);
                return lhs;
            });

#else

    float minX, minY, minZ, maxX, maxY, maxZ;
    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = std::numeric_limits<float>::lowest();
    size_t n = positions.size();
#if _OPENMP >= 201107
    #pragma omp parallel for shared(positions, n) default(none) reduction(min: minX) reduction(min: minY) \
    reduction(min: minZ) reduction(max: maxX) reduction(max: maxY) reduction(max: maxZ)
#endif
    for (size_t i = 0; i < n; i++) {
        const glm::vec3& pt = positions.at(i);
        minX = std::min(minX, pt.x);
        minY = std::min(minY, pt.y);
        minZ = std::min(minZ, pt.z);
        maxX = std::max(maxX, pt.x);
        maxY = std::max(maxY, pt.y);
        maxZ = std::max(maxZ, pt.z);
    }
    sgl::AABB3 aabb;
    aabb.min = glm::vec3(minX, minY, minZ);
    aabb.max = glm::vec3(maxX, maxY, maxZ);
    return aabb;

#endif
}

std::pair<float, float> reductionFunctionFloatMinMax(
        std::pair<float, float> lhs, std::pair<float, float> rhs) {
    return { std::min(lhs.first, rhs.first), std::max(lhs.second, rhs.second) };
}

}
