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

#include <Utils/File/Logfile.hpp>
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


void computeHistogramUnormByte(
        std::vector<float>& histogram, int histogramResolution,
        const uint8_t* values, size_t numValues, float minVal, float maxVal) {
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
        float value = float(values[valIdx]) / 255.0f;
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

void computeHistogramUnormByte(
        std::vector<float>& histogram, int histogramResolution,
        const uint8_t* values, size_t numValues) {
    auto [minVal, maxVal] = sgl::reduceUnormByteArrayMinMax(values, numValues);
    computeHistogramUnormByte(histogram, histogramResolution, values, numValues, minVal, maxVal);
}


void computeHistogramUnormShort(
        std::vector<float>& histogram, int histogramResolution,
        const uint16_t* values, size_t numValues, float minVal, float maxVal) {
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
        float value = float(values[valIdx]) / 65535.0f;
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

void computeHistogramUnormShort(
        std::vector<float>& histogram, int histogramResolution,
        const uint16_t* values, size_t numValues) {
    auto [minVal, maxVal] = sgl::reduceUnormShortArrayMinMax(values, numValues);
    computeHistogramUnormShort(histogram, histogramResolution, values, numValues, minVal, maxVal);
}



template<class Tx, class Ty>
void computeHistogram2dTemplated(
        std::vector<float>& histogram, int histogramResolution,
        const Tx* valuesX, const Ty* valuesY, size_t numValues,
        float minValX, float maxValX, float minValY, float maxValY) {
        int histogramResolution2d = histogramResolution * histogramResolution;
    std::vector<std::atomic<int>> histogramAtomic(histogramResolution2d);
    for (int histIdx = 0; histIdx < histogramResolution2d; histIdx++) {
        histogramAtomic.at(histIdx) = 0;
    }

#ifdef USE_TBB
    tbb::parallel_for(tbb::blocked_range<size_t>(0, numValues), [&](auto const& r) {
        for (auto valIdx = r.begin(); valIdx != r.end(); valIdx++) {
#else
#if _OPENMP >= 201107
    #pragma omp parallel for shared(valuesX, valuesY, numValues, minValX, maxValX, minValY, maxValY) \
            shared(histogramResolution, histogramAtomic) default(none)
#endif
    for (size_t valIdx = 0; valIdx < numValues; valIdx++) {
#endif
        float valueX = 0.0f, valueY = 0.0f;
        if constexpr (std::is_same<Tx, float>()) {
            valueX = valuesX[valIdx];
        } else if constexpr (std::is_same<Tx, uint8_t>()) {
            valueX = float(valuesX[valIdx]) / 255.0f;
        } else if constexpr (std::is_same<Tx, uint16_t>()) {
            valueX = float(valuesX[valIdx]) / 65535.0f;
        }
        if constexpr (std::is_same<Ty, float>()) {
            valueY = valuesY[valIdx];
        } else if constexpr (std::is_same<Ty, uint8_t>()) {
            valueY = float(valuesY[valIdx]) / 255.0f;
        } else if constexpr (std::is_same<Ty, uint16_t>()) {
            valueY = float(valuesY[valIdx]) / 65535.0f;
        }
        if (std::isnan(valueX) || std::isnan(valueY)) {
            continue;
        }
        int histIdxX = std::clamp(
                static_cast<int>((valueX - minValX) / (maxValX - minValX) * static_cast<float>(histogramResolution)),
                0, histogramResolution - 1);
        int histIdxY = std::clamp(
                static_cast<int>((valueY - minValY) / (maxValY - minValY) * static_cast<float>(histogramResolution)),
                0, histogramResolution - 1);
        histogramAtomic.at(histIdxX + histIdxY * histogramResolution)++;
    }
#ifdef USE_TBB
    });
#endif

    histogram.resize(histogramResolution2d);
    for (int histIdx = 0; histIdx < histogramResolution2d; histIdx++) {
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
    #pragma omp parallel for shared(histogram, histogramResolution2d) reduction(max: histogramMax) default(none)
#endif
    for (int histIdx = 0; histIdx < histogramResolution2d; histIdx++) {
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
    #pragma omp parallel for shared(histogram, histogramResolution2d, histogramMax) default(none)
#endif
    for (int histIdx = 0; histIdx < histogramResolution2d; histIdx++) {
#endif
        histogram.at(histIdx) /= histogramMax;
    }
#ifdef USE_TBB
    });
#endif
}

void computeHistogram2d(
        std::vector<float>& histogram2d, int histogramResolution,
        ScalarDataFormat formatX, ScalarDataFormat formatY,
        const void* valuesX, const void* valuesY, size_t numValues,
        float minValX, float maxValX, float minValY, float maxValY) {
    if (formatX == ScalarDataFormat::FLOAT && formatY == ScalarDataFormat::FLOAT) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const float*>(valuesX), reinterpret_cast<const float*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::BYTE && formatY == ScalarDataFormat::FLOAT) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const uint8_t*>(valuesX), reinterpret_cast<const float*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::SHORT && formatY == ScalarDataFormat::FLOAT) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const uint16_t*>(valuesX), reinterpret_cast<const float*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::FLOAT && formatY == ScalarDataFormat::BYTE) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const float*>(valuesX), reinterpret_cast<const uint8_t*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::BYTE && formatY == ScalarDataFormat::BYTE) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const uint8_t*>(valuesX), reinterpret_cast<const uint8_t*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::SHORT && formatY == ScalarDataFormat::BYTE) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const uint16_t*>(valuesX), reinterpret_cast<const uint8_t*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::FLOAT && formatY == ScalarDataFormat::SHORT) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const float*>(valuesX), reinterpret_cast<const uint16_t*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::BYTE && formatY == ScalarDataFormat::SHORT) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const uint8_t*>(valuesX), reinterpret_cast<const uint16_t*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else if (formatX == ScalarDataFormat::SHORT && formatY == ScalarDataFormat::SHORT) {
        computeHistogram2dTemplated(
                histogram2d, histogramResolution,
                reinterpret_cast<const uint16_t*>(valuesX), reinterpret_cast<const uint16_t*>(valuesY),
                numValues, minValX, maxValX, minValY, maxValY);
    } else {
        sgl::Logfile::get()->writeError("Error in computeHistogram2d: Float16 is not yet supported.");
        histogram2d.resize(histogramResolution * histogramResolution);
    }
}

}
