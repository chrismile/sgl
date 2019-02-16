/*!
 * AABB3.hpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_MATH_GEOMETRY_AABB3_HPP_
#define SRC_MATH_GEOMETRY_AABB3_HPP_

#include <glm/glm.hpp>
#include <cfloat>
#include <Defs.hpp>

namespace sgl {

class DLL_OBJECT AABB3 {
public:
    glm::vec3 min, max;

    AABB3() : min(FLT_MAX), max(-FLT_MAX) {}
    AABB3(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}

    inline glm::vec3 getDimensions() const { return max - min; }
    inline glm::vec3 getExtent() const { return (max - min) / 2.0f; }
    inline glm::vec3 getCenter() const { return (max + min) / 2.0f; }
    inline glm::vec3 getMinimum() const { return min; }
    inline glm::vec3 getMaximum() const { return max; }

    //! Merge the two AABBs
    void combine(const AABB3 &otherAABB);
    //! Merge AABB with a point
    void combine(const glm::vec3 &pt);
    //! Transform AABB
    AABB3 transformed(const glm::mat4 &matrix) const;
};

}

/*! SRC_MATH_GEOMETRY_AABB3_HPP_ */
#endif
