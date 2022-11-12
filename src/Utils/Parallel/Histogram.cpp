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

#include <atomic>
#include <algorithm>
#include <cmath>

#ifdef USE_TBB
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#endif

#include "Reduction.hpp"
#include "Histogram.hpp"

namespace sgl {

void computeHistogram(
        std::vector<float>& histogram, int histogramResolution,
        const float* values, size_t numValues, float minVal, float maxVal) {
    std::vector<std::atomic<int>> histogramAtomic(histogramResolution);
    for (int histIdx = 0; histIdx < histogramResolution; histIdx++) {
        histogramAtomic.at(histIdx) = 0;
    }

#ifdef USE_TBB
    tbb::parallel_for(tbb::blocked_range<size_t>(0, numValues), [&](auto const& r) {
        for (auto valIdx = r.begin(); valIdx != r.end(); valIdx++) {
#else
#if _OPENMP >= 201107
    #pragma omp parallel for shared(values, numValues, minVal, maxVal, histogramResolution, histogramAtomic) default(none)
#endif
    for (size_t valIdx = 0; valIdx < numValues; valIdx++) {
#endif
        float value = values[valIdx];
        if (std::isnan(value)) {
            continue;
        }
        int histIdx = std::clamp(
                static_cast<int>((value - minVal) / (maxVal - minVal) * static_cast<float>(histogramResolution)),
                0, histogramResolution - 1);
        histogramAtomic.at(histIdx)++;
    }
#ifdef USE_TBB
    });
#endif

    histogram.resize(histogramResolution);
    for (int histIdx = 0; histIdx < histogramResolution; histIdx++) {
        histogram.at(histIdx) = float(histogramAtomic.at(histIdx));
    }

    // Normalize values of histogram.
#ifdef USE_TBB
    float histogramMax = tbb::parallel_reduce(
        tbb::blocked_range<int>(0, histogramResolution), 0.0f,
        [&](tbb::blocked_range<int> const& r, float histogramMax) {
            for (auto histIdx = r.begin(); histIdx != r.end(); histIdx++) {
#else
    float histogramMax = 0.0f;
#if _OPENMP >= 201107
    #pragma omp parallel for shared(histogram, histogramResolution) reduction(max: histogramMax) default(none)
#endif
    for (int histIdx = 0; histIdx < histogramResolution; histIdx++) {
#endif
        histogramMax = std::max(histogramMax, histogram.at(histIdx));
    }
#ifdef USE_TBB
            return histogramMax;
        }, sgl::max_predicate());
#endif

#ifdef USE_TBB
    tbb::parallel_for(tbb::blocked_range<int>(0, histogramResolution), [&](auto const& r) {
        for (auto histIdx = r.begin(); histIdx != r.end(); histIdx++) {
#else
#if _OPENMP >= 201107
    #pragma omp parallel for shared(histogram, histogramResolution, histogramMax) default(none)
#endif
    for (int histIdx = 0; histIdx < histogramResolution; histIdx++) {
#endif
        histogram.at(histIdx) /= histogramMax;
    }
#ifdef USE_TBB
    });
#endif
}

void computeHistogram(
        std::vector<float>& histogram, int histogramResolution,
        const float* values, size_t numValues) {
    auto [minVal, maxVal] = sgl::reduceFloatArrayMinMax(values, numValues);
    computeHistogram(histogram, histogramResolution, values, numValues, minVal, maxVal);
}

}
