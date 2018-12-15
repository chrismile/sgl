/*
 * Math.cpp
 *
 *  Created on: 11.09.2017
 *      Author: Christoph Neuhauser
 */

#include "Math.hpp"
#include <glm/glm.hpp>

namespace sgl {

float vectorAngle(const glm::vec2 &u, const glm::vec2& v) {
	glm::vec2 un = glm::normalize(u);
	glm::vec2 vn = glm::normalize(v);
	float r = glm::dot(un, vn);
	if (r < -1.0f) r = -1.0f;
	if (r > 1.0f) r = 1.0f;
	return ((u.x*v.y < u.y*v.x) ? -1.0f : 1.0f) * acosf(r);
}

}
