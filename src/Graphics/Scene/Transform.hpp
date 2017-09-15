/*!
 * Transform.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_GRAPHICS_SCENE_TRANSFORM_HPP_
#define SRC_GRAPHICS_SCENE_TRANSFORM_HPP_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sgl {

class Transform {
public:
	Transform() : position(0.0f, 0.0f, 0.0f), scale(1.0f, 1.0f, 1.0f) {}
	glm::vec3 position;
	glm::vec3 scale;
	glm::quat orientation;
};

}

/*! SRC_GRAPHICS_SCENE_TRANSFORM_HPP_ */
#endif
