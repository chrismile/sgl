/*
 * AABB2.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#include "AABB2.hpp"
#include <Math/Geometry/MatrixUtil.hpp>

namespace sgl {

void AABB2::combine(const AABB2 &otherAABB)
{
	if (otherAABB.min.x < min.x)
		min.x = otherAABB.min.x;
	if (otherAABB.min.y < min.y)
		min.y = otherAABB.min.y;
	if (otherAABB.max.x > max.x)
		max.x = otherAABB.max.x;
	if (otherAABB.max.y > max.y)
		max.y = otherAABB.max.y;
}

void AABB2::combine(const glm::vec2 &pt)
{
	if (pt.x < min.x)
		min.x = pt.x;
	if (pt.y < min.y)
		min.y = pt.y;
	if (pt.x > max.x)
		max.x = pt.x;
	if (pt.y > max.y)
		max.y = pt.y;
}

AABB2 AABB2::transformed(const glm::mat4 &matrix)
{
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
