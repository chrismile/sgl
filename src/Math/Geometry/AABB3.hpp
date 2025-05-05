/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

#ifndef SRC_MATH_GEOMETRY_AABB3_HPP_
#define SRC_MATH_GEOMETRY_AABB3_HPP_

#include <cfloat>

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

#include <Defs.hpp>

namespace sgl {

class DLL_OBJECT AABB3 {
public:
    glm::vec3 min, max;

    AABB3() : min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest()) {}
    AABB3(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}

    [[nodiscard]] inline glm::vec3 getDimensions() const { return max - min; }
    [[nodiscard]] inline glm::vec3 getExtent() const { return (max - min) / 2.0f; }
    [[nodiscard]] inline glm::vec3 getCenter() const { return (max + min) / 2.0f; }
    [[nodiscard]] inline glm::vec3 getMinimum() const { return min; }
    [[nodiscard]] inline glm::vec3 getMaximum() const { return max; }

    /// Returns whether the two AABBs intersect.
    [[nodiscard]] bool intersects(const AABB3& otherAABB) const;
    /// Merge the two AABBs.
    void combine(const AABB3& otherAABB);
    /// Merge AABB with a point.
    void combine(const glm::vec3& pt);
    /// Returns whether the AABB contain the point.
    [[nodiscard]] bool contains(const glm::vec3& pt) const;
    /// Transform AABB.
    [[nodiscard]] AABB3 transformed(const glm::mat4& matrix) const;
    /// Transform AABB (faster, but doesn't work as expected for projective transforms).
    [[nodiscard]] AABB3 transformedFast(const glm::mat4& matrix) const;
};

}

/*! SRC_MATH_GEOMETRY_AABB3_HPP_ */
#endif
