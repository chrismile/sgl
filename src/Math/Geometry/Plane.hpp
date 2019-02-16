/*!
 * Plane.hpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_MATH_GEOMETRY_PLANE_HPP_
#define SRC_MATH_GEOMETRY_PLANE_HPP_

#include <glm/glm.hpp>

namespace sgl {

class AABB3;

//! Plane in 3D, ax + by + cz + d = 0
class Plane {
public:
    Plane() : a(0.0f), b(0.0f), c(1.0f), d(0.0f) {}
    Plane(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) {}
    Plane(glm::vec3 normal, float offset) : a(normal.x), b(normal.y), c(normal.z), d(-offset) {}
    Plane(glm::vec3 normal, glm::vec3 point) : a(normal.x), b(normal.y), c(normal.z) { d = glm::dot(normal, point); }
    glm::vec3 getNormal() const { return glm::vec3(a,b,c); }
    float getOffset() const { return d; }

    float getDistance(const glm::vec3 &pt) const;
    bool isOutside(const glm::vec3 &pt) const;
    bool isOutside(const AABB3 &aabb) const;

    float a, b, c, d;
};

}

/*! SRC_MATH_GEOMETRY_PLANE_HPP_ */
#endif
