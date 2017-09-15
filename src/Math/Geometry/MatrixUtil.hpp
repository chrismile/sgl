/*
 * MatrixUtil.hpp
 *
 *  Created on: 10.09.2017
 *      Author: christoph
 */

#ifndef SRC_MATH_GEOMETRY_MATRIXUTIL_HPP_
#define SRC_MATH_GEOMETRY_MATRIXUTIL_HPP_

#include <Defs.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace sgl {

glm::vec3 transformPoint(const glm::mat4 &mat, const glm::vec3 &vec);
glm::vec3 transformDirection(const glm::mat4 &mat, const glm::vec3 &vec);
glm::vec2 transformPoint(const glm::mat4 &mat, const glm::vec2 &vec);
glm::vec2 transformDirection(const glm::mat4 &mat, const glm::vec2 &vec);

// Special types of matrices
inline     glm::mat4 matrixIdentity() {return glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);}
inline     glm::mat4 matrixZero() {return glm::mat4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);}
DLL_OBJECT glm::mat4 matrixTranslation(const glm::vec3 &vec);
DLL_OBJECT glm::mat4 matrixTranslation(const glm::vec2 &vec);
DLL_OBJECT glm::mat4 matrixScaling(const glm::vec3 &vec);
DLL_OBJECT glm::mat4 matrixScaling(const glm::vec2 &vec);
DLL_OBJECT glm::mat4 matrixOrthogonalProjection(float left, float right, float bottom, float top, float near, float far);
DLL_OBJECT glm::mat4 matrixSkewX(float f);
DLL_OBJECT glm::mat4 matrixSkewY(float f);

}

#endif /* SRC_MATH_GEOMETRY_MATRIXUTIL_HPP_ */
