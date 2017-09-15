/*!
 * AABB2.hpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_MATH_GEOMETRY_AABB2_HPP_
#define SRC_MATH_GEOMETRY_AABB2_HPP_

#include <glm/glm.hpp>
#include <cfloat>
#include <Defs.hpp>

namespace sgl {

class DLL_OBJECT AABB2 {
public:
	glm::vec2 min, max;

	AABB2() {}
	AABB2(glm::vec2 min, glm::vec2 max) : min(min), max(max) {}

	inline glm::vec2 getDimensions() const { return max - min; }
	inline glm::vec2 getExtent() const { return (max - min) / 2.0f; }
	inline glm::vec2 getCenter() const { return (max + min) / 2.0f; }
	inline glm::vec2 getMinimum() const { return min; }
	inline glm::vec2 getMaximum() const { return max; }
	inline float getWidth() const { return max.x - min.x; }
	inline float getHeight() const { return max.y - min.y; }

	//! Merge the two AABBs
	void combine(const AABB2 &otherAABB);
	//! Transform AABB
	AABB2 transformed(const glm::mat4 &matrix);
};

}

/*! SRC_MATH_GEOMETRY_AABB2_HPP_ */
#endif
