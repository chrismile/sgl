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

#ifndef SGL_HISTOGRAM_HPP
#define SGL_HISTOGRAM_HPP

#include <vector>
#include <Utils/SciVis/ScalarDataFormat.hpp>

namespace sgl {

DLL_OBJECT void computeHistogram(
        std::vector<float>& histogram, int histogramResolution,
        const float* values, size_t numValues, float minVal, float maxVal);
DLL_OBJECT void computeHistogram(
        std::vector<float>& histogram, int histogramResolution, const float* values, size_t numValues);

// For 8-bit and 16-bit UNORM data (i.e., integer values normalized to [0, 1]).
DLL_OBJECT void computeHistogramUnormByte(
        std::vector<float>& histogram, int histogramResolution,
        const uint8_t* values, size_t numValues, float minVal, float maxVal);
DLL_OBJECT void computeHistogramUnormByte(
        std::vector<float>& histogram, int histogramResolution, const uint8_t* values, size_t numValues);
DLL_OBJECT void computeHistogramUnormShort(
        std::vector<float>& histogram, int histogramResolution,
        const uint16_t* values, size_t numValues, float minVal, float maxVal);
DLL_OBJECT void computeHistogramUnormShort(
        std::vector<float>& histogram, int histogramResolution, const uint16_t* values, size_t numValues);

// For 2D histograms.
DLL_OBJECT void computeHistogram2d(
        std::vector<float>& histogram2d, int histogramResolution,
        ScalarDataFormat formatX, ScalarDataFormat formatY,
        const void* valuesX, const void* valuesY, size_t numValues,
        float minValX, float maxValX, float minValY, float maxValY);

}

#endif //SGL_HISTOGRAM_HPP
