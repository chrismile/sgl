/*
 * Ray3.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#include "Plane.hpp"
#include "Ray3.hpp"

namespace sgl {

RaycastResult Ray3::intersects(const Plane &plane) const {
    glm::vec3 planeNormal = plane.getNormal();
    float ln = glm::dot(planeNormal, this->direction);
    if (fabs(ln) < 10e-4) {
        // Plane and ray are parallel
        return RaycastResult(false, 0.0f);
    } else {
        float pos = glm::dot(planeNormal, this->origin) + plane.getOffset();
        float t = -pos/ln;
        bool positiveIntersection = t >= 0;
        return RaycastResult(positiveIntersection, t);
    }
}

}
