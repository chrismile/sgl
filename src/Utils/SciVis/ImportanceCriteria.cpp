/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Christoph Neuhauser
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

#include <algorithm>
#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/fallback/vec3.hpp>
#endif

#ifdef USE_TBB
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#endif

#include <Math/Math.hpp>
#include <Utils/Parallel/Reduction.hpp>
#include "ImportanceCriteria.hpp"

namespace sgl {

/// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/packUnorm.xhtml
void packUnorm16Array(const std::vector<float>& floatVector, std::vector<uint16_t>& unormVector) {
    // LLVM raises the error "capturing a structured binding is not yet supported in OpenMP" with the following line...
    //auto [minValue, maxValue] = reduceFloatArrayMinMax(floatVector);
    auto [minValueBinding, maxValueBinding] = reduceFloatArrayMinMax(floatVector);
    const float minValue = minValueBinding;
    const float maxValue = maxValueBinding;
    unormVector.resize(floatVector.size());
#ifdef USE_TBB
    tbb::parallel_for(tbb::blocked_range<size_t>(0, unormVector.size()), [&](auto const& r) {
        for (size_t i = r.begin(); i != r.end(); i++) {
#else
#if _OPENMP >= 200805
    #pragma omp parallel for shared(floatVector, unormVector, minValue, maxValue) default(none)
#endif
    for (size_t i = 0; i < unormVector.size(); i++) {
#endif
        unormVector.at(i) = uint16_t(glm::clamp(glm::round(
                (floatVector.at(i) - minValue) / (maxValue - minValue) * 65535.0f), 0.0f, 65535.0f));
    }
#ifdef USE_TBB
    });
#endif
}

/// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/packUnorm.xhtml
void packUnorm16ArrayOfArrays(
        const std::vector<std::vector<float>> &floatVector,
        std::vector<std::vector<uint16_t>> &unormVector) {
    unormVector.resize(floatVector.size());
    for (size_t i = 0; i < unormVector.size(); i++) {
        packUnorm16Array(floatVector.at(i), unormVector.at(i));
    }
}


/// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/unpackUnorm.xhtml
void unpackUnorm16Array(const uint16_t* unormVector, size_t vectorSize, std::vector<float>& floatVector) {
    floatVector.resize(vectorSize);
#ifdef USE_TBB
    tbb::parallel_for(tbb::blocked_range<size_t>(0, vectorSize), [&](auto const& r) {
        for (size_t i = r.begin(); i != r.end(); i++) {
#else
#if _OPENMP >= 200805
    #pragma omp parallel for shared(floatVector, unormVector, vectorSize) default(none)
#endif
    for (size_t i = 0; i < vectorSize; i++) {
#endif
        floatVector.at(i) = float(unormVector[i]) / 65535.0f;
    }
#ifdef USE_TBB
    });
#endif
}


std::vector<float> computeSegmentLengths(std::vector<glm::vec3>& vertexPositions) {
    int n = (int)vertexPositions.size();
    std::vector<float> segmentLengths;
    segmentLengths.reserve(n);

    for (int i = 0; i < n; i++) {
        glm::vec3 tangent;
        if (i == 0) {
            // First node
            tangent = vertexPositions.at(i+1) - vertexPositions.at(i);
        } else if (i == n-1) {
            // Last node
            tangent = vertexPositions.at(i) - vertexPositions.at(i-1);
        } else {
            // Node with two neighbors - use both normals
            tangent = vertexPositions.at(i+1) - vertexPositions.at(i);
        }

        float lineSegmentLength = glm::length(tangent);
        segmentLengths.push_back(lineSegmentLength);
    }

    return segmentLengths;
}

std::vector<float> computeCurvature(std::vector<glm::vec3>& vertexPositions) {
    int n = (int)vertexPositions.size();
    std::vector<float> curvatures;
    curvatures.reserve(n);

    glm::vec3 tangent, lastTangent = glm::vec3(1.0f, 0.0f, 0.0f);

    for (int i = 0; i < n; i++) {
        float curvatureAngle = 0.0f;

        if (i == 0) {
            // First node
            tangent = vertexPositions.at(i+1) - vertexPositions.at(i);
        } else if (i == n-1) {
            // Last node
            tangent = vertexPositions.at(i) - vertexPositions.at(i-1);
        } else {
            // Node with two neighbors
            tangent = vertexPositions.at(i+1) - vertexPositions.at(i-1);
        }
        if (glm::length(tangent) < 1E-08f) {
            // In case the two vertices are almost identical, just skip this path line segment
            curvatures.push_back(0.0f);
            continue;
        }

        tangent = glm::normalize(tangent);

        // Compute curvature, i.e. angle between neighboring line segment tangents.
        // Fallback for first and last line point: Assume zero curvature.
        if (i != 0 && i != n-1) {
            float cosAngle = glm::clamp(glm::dot(tangent, lastTangent), 0.0f, 1.0f);
            curvatureAngle = glm::acos(cosAngle) / sgl::PI;
        }

        lastTangent = tangent;
        curvatures.push_back(curvatureAngle);
    }

    return curvatures;
}

std::vector<float> computeSegmentAttributeDifference(
        std::vector<glm::vec3>& vertexPositions,
        std::vector<float>& vertexAttributes) {
    int n = (int)vertexPositions.size();
    std::vector<float> segmentAttributeDifferences;
    segmentAttributeDifferences.reserve(n);

    for (int i = 0; i < n; i++) {
        float segmentAttributeDifference = 0.0f;

        if (i == 0) {
            // First node
            segmentAttributeDifference = vertexAttributes.at(i+1) - vertexAttributes.at(i);
        } else if (i == n-1) {
            // Last node
            segmentAttributeDifference = vertexAttributes.at(i) - vertexAttributes.at(i-1);
        } else {
            // Node with two neighbors
            segmentAttributeDifference = vertexAttributes.at(i+1) - vertexAttributes.at(i);
        }

        segmentAttributeDifferences.push_back(std::abs(segmentAttributeDifference));
    }

    return segmentAttributeDifferences;
}

std::vector<float> computeTotalAttributeDifference(
        std::vector<glm::vec3>& vertexPositions,
        std::vector<float>& vertexAttributes) {
    int n = (int)vertexPositions.size();
    std::vector<float> segmentAttributeDifferences;
    segmentAttributeDifferences.reserve(n);

    auto [minAttribute, maxAttribute] = reduceFloatArrayMinMax(vertexAttributes);
    float totalPressureDifference = maxAttribute - minAttribute;
    for (int i = 0; i < n; i++) {
        segmentAttributeDifferences.push_back(totalPressureDifference);
    }

    return segmentAttributeDifferences;
}

std::vector<float> computeAngleOfAscent(std::vector<glm::vec3>& vertexPositions) {
    int n = (int)vertexPositions.size();
    std::vector<float> angleOfAscentArray;
    angleOfAscentArray.reserve(n);
    glm::vec3 tangent;
    const glm::vec3 angleUp(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < n; i++) {
        // Angle between line segment and xz-plane
        float angleOfAscent = 0.0f;

        if (i == 0) {
            // First node
            tangent = vertexPositions.at(i+1) - vertexPositions.at(i);
        } else if (i == n-1) {
            // Last node
            tangent = vertexPositions.at(i) - vertexPositions.at(i-1);
        } else {
            // Node with two neighbors
            tangent = vertexPositions.at(i+1) - vertexPositions.at(i);
        }
        if (glm::length(tangent) < 0.0001f) {
            // In case the two vertices are almost identical, just skip this path line segment
            angleOfAscentArray.push_back(0.0f);
            continue;
        }

        tangent = glm::normalize(tangent);
        float cosAngle = glm::clamp(glm::dot(tangent, angleUp), 0.0f, 1.0f);
        angleOfAscent = 1.0f - glm::acos(cosAngle) / sgl::PI;

        angleOfAscentArray.push_back(angleOfAscent);
    }

    return angleOfAscentArray;
}

std::vector<float> computeSegmentHeightDifference(std::vector<glm::vec3>& vertexPositions) {
    int n = (int)vertexPositions.size();
    std::vector<float> computeSegmentHeightDifferences;
    computeSegmentHeightDifferences.reserve(n);

    for (int i = 0; i < n; i++) {
        float segmentHeightDifference = 0.0f;

        if (i == 0) {
            // First node
            segmentHeightDifference = vertexPositions.at(i+1).y - vertexPositions.at(i).y;
        } else if (i == n-1) {
            // Last node
            segmentHeightDifference = vertexPositions.at(i).y - vertexPositions.at(i-1).y;
        } else {
            // Node with two neighbors
            segmentHeightDifference = vertexPositions.at(i+1).y - vertexPositions.at(i).y;
        }

        computeSegmentHeightDifferences.push_back(segmentHeightDifference);
    }

    return computeSegmentHeightDifferences;
}

}
