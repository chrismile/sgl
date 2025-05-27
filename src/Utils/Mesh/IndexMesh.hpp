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

#ifndef SGL_INDEXMESH_HPP
#define SGL_INDEXMESH_HPP

#include <vector>

#ifdef USE_GLM
#include <glm/vec3.hpp>
#else
#include <Math/Geometry/fallback/vec3.hpp>
#endif

namespace sgl {

/**
 * Computes a shared index representation for the passed list of vertices.
 * @param vertexPositions The vertex positions.
 * @param vertexNormals The output vertex normals.
 * @param vertexPositionsShared The shared vertex positions (output).
 * @param vertexNormalsShared The shared vertex normals (output).
 * @param triangleIndices A list of triangle indices. Three consecutive entries form one triangle.
 */
DLL_OBJECT void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions, const std::vector<glm::vec3>& vertexNormals,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared, std::vector<glm::vec3>& vertexNormalsShared,
        float EPSILON);
DLL_OBJECT void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions, const std::vector<glm::vec3>& vertexNormals,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared, std::vector<glm::vec3>& vertexNormalsShared);

/**
 * Computes a shared index representation for the passed list of vertices.
 * @param vertexPositions The vertex positions.
 * @param vertexPositionsShared The shared vertex positions (output).
 * @param triangleIndices A list of triangle indices. Three consecutive entries form one triangle.
 */
DLL_OBJECT void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared,
        float EPSILON);
DLL_OBJECT void computeSharedIndexRepresentation(
        const std::vector<glm::vec3>& vertexPositions,
        std::vector<uint32_t>& triangleIndices,
        std::vector<glm::vec3>& vertexPositionsShared);

}

#endif //SGL_INDEXMESH_HPP
