/*
 * Sphere.hpp
 *
 *  Created on: 10.09.2017
 *      Author: christoph
 */

#ifndef SRC_MATH_GEOMETRY_SPHERE_HPP_
#define SRC_MATH_GEOMETRY_SPHERE_HPP_

#include <glm/glm.hpp>
#include <cfloat>
#include <Defs.hpp>

namespace sgl {

class DLL_OBJECT Sphere {
public:
	glm::vec3 center;
	float radius;
};

}

#endif /* SRC_MATH_GEOMETRY_SPHERE_HPP_ */
