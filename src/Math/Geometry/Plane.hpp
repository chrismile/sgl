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

#ifndef SRC_MATH_GEOMETRY_PLANE_HPP_
#define SRC_MATH_GEOMETRY_PLANE_HPP_

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

namespace sgl {

class AABB3;

/// Plane in 3D, ax + by + cz + d = 0
class DLL_OBJECT Plane {
public:
    Plane() : a(0.0f), b(0.0f), c(1.0f), d(0.0f) {}
    Plane(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) {}
    Plane(glm::vec3 normal, float offset) : a(normal.x), b(normal.y), c(normal.z), d(-offset) {}
    Plane(glm::vec3 normal, glm::vec3 point) : a(normal.x), b(normal.y), c(normal.z) { d = -glm::dot(normal, point); }
    [[nodiscard]] glm::vec3 getNormal() const { return glm::vec3(a,b,c); }
    [[nodiscard]] float getOffset() const { return d; }

    [[nodiscard]] float getDistance(const glm::vec3 &pt) const;
    [[nodiscard]] bool isOutside(const glm::vec3 &pt) const;
    [[nodiscard]] bool isOutside(const AABB3 &aabb) const;

    float a, b, c, d;
};

}

/*! SRC_MATH_GEOMETRY_PLANE_HPP_ */
#endif
