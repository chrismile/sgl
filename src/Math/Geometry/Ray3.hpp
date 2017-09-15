/*
 * Ray3.hpp
 *
 *  Created on: 10.09.2017
 *      Author: christoph
 */

#ifndef SRC_MATH_GEOMETRY_RAY3_HPP_
#define SRC_MATH_GEOMETRY_RAY3_HPP_

#include <glm/glm.hpp>

namespace sgl {

class AABB3;

struct RaycastResult {
	RaycastResult(bool hit, float t) : hit(hit), t(t) {}
	bool hit;
	float t;
};

// Plane in 3D, ax + by + cz + d = 0
class Ray3 {
public:
	Ray3(const glm::vec3 &origin, const glm::vec3 &direction) : origin(origin), direction(direction) {}

	RaycastResult intersects(const Plane &plane) const;
	inline glm::vec3 getPoint(float t) const { return origin + direction * t; }

private:
	glm::vec3 origin, direction;
};

}

#endif /* SRC_MATH_GEOMETRY_RAY3_HPP_ */
