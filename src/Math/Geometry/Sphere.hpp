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

#ifndef SRC_MATH_GEOMETRY_SPHERE_HPP_
#define SRC_MATH_GEOMETRY_SPHERE_HPP_

#include <cfloat>

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

#include <Defs.hpp>

namespace sgl {

class AABB3;

class DLL_OBJECT Sphere {
public:
    glm::vec3 center;
    float radius;

    Sphere() : center(0.0f), radius(0.0f) {}
    Sphere(glm::vec3 center, float radius) : center(center), radius(radius) {}

    /// Returns whether the two spheres intersect.
    [[nodiscard]] bool intersects(const Sphere& otherSphere) const;
    /// Returns whether the sphere contains the passed sphere.
    [[nodiscard]] bool contains(const Sphere& otherSphere) const;
    /// Returns whether the sphere contain the point.
    [[nodiscard]] bool contains(const glm::vec3& pt) const;
    /// Returns whether the sphere intersects the passed AABB.
    [[nodiscard]] bool intersects(const AABB3& aabb) const;
    /// Returns whether the sphere contains the passed AABB.
    [[nodiscard]] bool contains(const AABB3& aabb) const;
    /// Merge the two bounding spheres.
    void combine(const Sphere& otherSphere);
    /// Merge the bounding sphere with a point.
    void combine(const glm::vec3& pt);
};

}

/*! SRC_MATH_GEOMETRY_SPHERE_HPP_ */
#endif
