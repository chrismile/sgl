/*
 * Plane.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#include "Plane.hpp"
#include "AABB3.hpp"

namespace sgl {

float Plane::getDistance(const glm::vec3 &pt) const {
    return a*pt.x + b*pt.y + c*pt.z + d;
}

bool Plane::isOutside(const glm::vec3 &pt) const {
    return getDistance(pt) < 0;
}

bool Plane::isOutside(const AABB3 &aabb) const {
    glm::vec3 extent = aabb.getExtent();
    float centerDist = getDistance(aabb.getCenter());
    float maxAbsDist = fabs(a*extent.x + b*extent.y + c*extent.z);
    return -centerDist > maxAbsDist;
}

}
