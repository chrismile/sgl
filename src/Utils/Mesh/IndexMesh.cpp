/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#include <chrono>
#include <iostream>

#include <Math/Math.hpp>
#include <Math/Geometry/AABB3.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/Parallel/Reduction.hpp>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif
#include "Utils/SearchStructures/KdTree.hpp"
#include "Utils/SearchStructures/HashedGrid.hpp"
#include "IndexMesh.hpp"

namespace sgl {

void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions, const std::vector<glm::vec3>& vertexNormals,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared, std::vector<glm::vec3>& vertexNormalsShared,
        float EPSILON) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

    sgl::AABB3 aabb = sgl::reduceVec3ArrayAabb(vertexPositions);
    size_t numEntries = std::max(vertexPositions.size() / 4, size_t(1));
    float cellSize = glm::length(aabb.getExtent()) / std::cbrt(float(numEntries)) * 1.0f / sgl::PI;

    SearchStructure<uint32_t>* searchStructure = new HashedGrid<uint32_t>(numEntries, cellSize);
    searchStructure->reserveDynamic(vertexPositions.size());

    uint32_t uniqueVertexCounter = 0;
    std::vector<std::pair<glm::vec3, uint32_t>> searchCache;
    for (size_t i = 0; i < vertexPositions.size(); i++) {
        const glm::vec3& vertexPosition = vertexPositions.at(i);
        const glm::vec3& vertexNormal = vertexNormals.at(i);
        searchCache.clear();
        auto closestPointIndex = searchStructure->findDataClosest(vertexPosition, EPSILON, searchCache);
        if (closestPointIndex) {
            triangleIndices.push_back(closestPointIndex.value());
        } else {
            searchStructure->add(vertexPositions.at(i), uniqueVertexCounter);
            vertexPositionsShared.push_back(vertexPosition);
            vertexNormalsShared.push_back(vertexNormal);
            triangleIndices.push_back(uniqueVertexCounter);
            uniqueVertexCounter++;
        }
    }

    delete searchStructure;
}

void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions, const std::vector<glm::vec3>& vertexNormals,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared, std::vector<glm::vec3>& vertexNormalsShared) {
    computeSharedIndexRepresentation(
            vertexPositions, vertexNormals, triangleIndices, vertexPositionsShared, vertexNormalsShared, 1e-5f);
}

void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared,
        float EPSILON) {
    SearchStructure<uint32_t>* searchStructure = new HashedGrid<uint32_t>(
            std::max(vertexPositions.size() / 4, size_t(1)), 1.0f / sgl::PI);
    searchStructure->reserveDynamic(vertexPositions.size());

    uint32_t uniqueVertexCounter = 0;
    std::vector<std::pair<glm::vec3, uint32_t>> searchCache;
    for (size_t i = 0; i < vertexPositions.size(); i++) {
        const glm::vec3& vertexPosition = vertexPositions.at(i);
        searchCache.clear();
        auto closestPointIndex = searchStructure->findDataClosest(vertexPosition, EPSILON, searchCache);
        if (closestPointIndex) {
            triangleIndices.push_back(closestPointIndex.value());
        } else {
            searchStructure->add(vertexPositions.at(i), uniqueVertexCounter);
            vertexPositionsShared.push_back(vertexPosition);
            triangleIndices.push_back(uniqueVertexCounter);
            uniqueVertexCounter++;
        }
    }

    delete searchStructure;
}

void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared) {
    computeSharedIndexRepresentation(vertexPositions, triangleIndices, vertexPositionsShared, 1e-5f);
}

}
