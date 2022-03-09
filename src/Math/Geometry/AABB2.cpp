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

#include "AABB2.hpp"
#include <Math/Geometry/MatrixUtil.hpp>

namespace sgl {

bool AABB2::intersects(const AABB2& otherAABB) const
{
    if (max.x < otherAABB.min.x || min.x > otherAABB.max.x
            || max.y < otherAABB.min.y || min.y > otherAABB.max.y) {
        return false;
    }
    return true;
}

void AABB2::combine(const AABB2& otherAABB) {
    if (otherAABB.min.x < min.x)
        min.x = otherAABB.min.x;
    if (otherAABB.min.y < min.y)
        min.y = otherAABB.min.y;
    if (otherAABB.max.x > max.x)
        max.x = otherAABB.max.x;
    if (otherAABB.max.y > max.y)
        max.y = otherAABB.max.y;
}

void AABB2::combine(const glm::vec2& pt) {
    if (pt.x < min.x)
        min.x = pt.x;
    if (pt.y < min.y)
        min.y = pt.y;
    if (pt.x > max.x)
        max.x = pt.x;
    if (pt.y > max.y)
        max.y = pt.y;
}

bool AABB2::contains(const glm::vec2& pt) const {
    return pt.x >= min.x && pt.y >= min.y && pt.x <= max.x && pt.y <= max.y;
}

AABB2 AABB2::transformed(const glm::mat4& matrix) const {
    glm::vec2 transformedCorners[4];
    transformedCorners[0] = transformPoint(matrix, min);
    transformedCorners[1] = transformPoint(matrix, max);
    transformedCorners[2] = transformPoint(matrix, glm::vec2(max.x, min.y));
    transformedCorners[3] = transformPoint(matrix, glm::vec2(min.x, max.y));

    AABB2 aabbTransformed;
    for (int i = 0; i < 4; ++i) {
        if (transformedCorners[i].x < aabbTransformed.min.x)
            aabbTransformed.min.x = transformedCorners[i].x;
        if (transformedCorners[i].x > aabbTransformed.max.x)
            aabbTransformed.max.x = transformedCorners[i].x;

        if (transformedCorners[i].y < aabbTransformed.min.y)
            aabbTransformed.min.y = transformedCorners[i].y;
        if (transformedCorners[i].y > aabbTransformed.max.y)
            aabbTransformed.max.y = transformedCorners[i].y;
    }

    return aabbTransformed;
}

}
