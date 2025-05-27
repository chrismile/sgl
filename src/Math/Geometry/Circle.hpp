/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Christoph Neuhauser
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

#ifndef SGL_CIRCLE_HPP
#define SGL_CIRCLE_HPP

#ifdef USE_GLM
#include <glm/vec2.hpp>
#else
#include <Math/Geometry/fallback/vec2.hpp>
#endif

namespace sgl {

class AABB2;

class DLL_OBJECT Circle {
public:
    glm::vec2 center;
    float radius;

    Circle() : center(0.0f), radius(0.0f) {}
    Circle(glm::vec2 center, float radius) : center(center), radius(radius) {}

    /// Returns whether the two circles intersect.
    [[nodiscard]] bool intersects(const Circle& otherCircle) const;
    /// Returns whether the circle contains the passed circle.
    [[nodiscard]] bool contains(const Circle& otherCircle) const;
    /// Returns whether the circle contain the point.
    [[nodiscard]] bool contains(const glm::vec2& pt) const;
    /// Returns whether the circle intersects the passed AABB.
    [[nodiscard]] bool intersects(const AABB2& aabb) const;
    /// Returns whether the circle contains the passed AABB.
    [[nodiscard]] bool contains(const AABB2& aabb) const;
    /// Merge the two bounding circles.
    void combine(const Circle& otherCircle);
    /// Merge the bounding circle with a point.
    void combine(const glm::vec2& pt);
};

}

#endif //SGL_CIRCLE_HPP
