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

#include "AABB3.hpp"
#include "Sphere.hpp"

namespace sgl {

bool Sphere::intersects(const Sphere& otherSphere) const {
    return glm::distance(this->center, otherSphere.center) <= this->radius + otherSphere.radius;
}

bool Sphere::contains(const Sphere& otherSphere) const {
    return glm::distance(this->center, otherSphere.center) + otherSphere.radius <= this->radius;
}

bool Sphere::contains(const glm::vec3& pt) const {
    return glm::distance(this->center, pt) <= this->radius;
}

static inline float SQR(float value) {
    return value * value;
}

bool Sphere::intersects(const AABB3& aabb) const {
    /**
     * https://stackoverflow.com/questions/28343716/sphere-intersection-test-of-aabb
     * Algorithm by Jim Arvo in "Graphics Gems".
     */
    float dmin = 0;
    for(int i = 0; i < 3; i++) {
        if (this->center[i] < aabb.getMinimum()[i]) {
            dmin += SQR(this->center[i] - aabb.getMinimum()[i]);
        } else if (this->center[i] > aabb.getMaximum()[i]) {
            dmin += SQR(this->center[i] - aabb.getMaximum()[i]);
        }
    }
    return dmin <= this->radius * this->radius;
}

bool Sphere::contains(const AABB3& aabb) const {
    // Check whether the sphere contains all corners of the AABB.
    if (!contains(aabb.getMinimum())) {
        return false;
    }
    if (!contains(aabb.getMaximum())) {
        return false;
    }
    if (!contains(glm::vec3(aabb.getMaximum().x, aabb.getMinimum().y, aabb.getMinimum().z))) {
        return false;
    }
    if (!contains(glm::vec3(aabb.getMinimum().x, aabb.getMaximum().y, aabb.getMinimum().z))) {
        return false;
    }
    if (!contains(glm::vec3(aabb.getMaximum().x, aabb.getMaximum().y, aabb.getMinimum().z))) {
        return false;
    }
    if (!contains(glm::vec3(aabb.getMinimum().x, aabb.getMinimum().y, aabb.getMaximum().z))) {
        return false;
    }
    if (!contains(glm::vec3(aabb.getMaximum().x, aabb.getMinimum().y, aabb.getMaximum().z))) {
        return false;
    }
    if (!contains(glm::vec3(aabb.getMinimum().x, aabb.getMaximum().y, aabb.getMaximum().z))) {
        return false;
    }
    return true;
}

void Sphere::combine(const Sphere& otherSphere) {
    float dist = glm::distance(this->center, otherSphere.center);

    // Check for enclosure.
    if (dist + otherSphere.radius <= this->radius) {
        return;
    }
    if (dist + this->radius <= otherSphere.radius) {
        this->center = otherSphere.center;
        this->radius = otherSphere.radius;
    }

    float newRadius = (this->radius + otherSphere.radius + dist) / 2.0f;
    glm::vec3 newCenter = this->center + (otherSphere.center - this->center) * (newRadius - this->radius) / dist;
    this->radius = newRadius;
    this->center = newCenter;
}

void Sphere::combine(const glm::vec3& pt) {
    float dist = glm::distance(this->center, pt);

    // Check for enclosure.
    if (dist <= this->radius) {
        return;
    }

    float newRadius = (this->radius + dist) / 2.0f;
    glm::vec3 newCenter = this->center + (pt - this->center) * (newRadius - this->radius) / dist;
    this->radius = newRadius;
    this->center = newCenter;
}

}
