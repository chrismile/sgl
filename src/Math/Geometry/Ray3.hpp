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

#ifndef SRC_MATH_GEOMETRY_RAY3_HPP_
#define SRC_MATH_GEOMETRY_RAY3_HPP_

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/fallback/vec2.hpp>
#include <Math/Geometry/fallback/vec3.hpp>
#endif

namespace sgl {

class AABB3;
class Plane;

struct DLL_OBJECT RaycastResult {
    RaycastResult(bool hit, float t) : hit(hit), t(t) {}
    bool hit;
    float t;
};

/// Plane in 3D, ax + by + cz + d = 0
class DLL_OBJECT Ray3 {
public:
    Ray3(const glm::vec3 &origin, const glm::vec3 &direction) : origin(origin), direction(direction) {}

    [[nodiscard]] RaycastResult intersects(const Plane &plane) const;
    [[nodiscard]] inline glm::vec3 getPoint(float t) const { return origin + direction * t; }
    [[nodiscard]] inline glm::vec2 getPoint2D(float t) const { glm::vec3 pt3d = getPoint(t); return glm::vec2(pt3d.x, pt3d.y); }

private:
    glm::vec3 origin, direction;
};

}

/*! SRC_MATH_GEOMETRY_RAY3_HPP_ */
#endif
