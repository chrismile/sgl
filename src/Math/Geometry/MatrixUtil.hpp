/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_MATH_GEOMETRY_MATRIXUTIL_HPP_
#define SRC_MATH_GEOMETRY_MATRIXUTIL_HPP_

#include <Defs.hpp>
#ifdef USE_GLM
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#else
#include <Math/Geometry/fallback/linalg.hpp>
#endif

namespace sgl {

DLL_OBJECT glm::vec3 transformPoint(const glm::mat4 &mat, const glm::vec3 &vec);
DLL_OBJECT glm::vec3 transformDirection(const glm::mat4 &mat, const glm::vec3 &vec);
DLL_OBJECT glm::vec2 transformPoint(const glm::mat4 &mat, const glm::vec2 &vec);
DLL_OBJECT glm::vec2 transformDirection(const glm::mat4 &mat, const glm::vec2 &vec);

/// Special types of matrices
inline     glm::mat4 matrixIdentity() {return glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);}
inline     glm::mat4 matrixZero() {return glm::mat4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);}
DLL_OBJECT glm::mat4 matrixTranslation(const glm::vec3 &vec);
DLL_OBJECT glm::mat4 matrixTranslation(const glm::vec2 &vec);
DLL_OBJECT glm::mat4 matrixScaling(const glm::vec3 &vec);
DLL_OBJECT glm::mat4 matrixScaling(const glm::vec2 &vec);
DLL_OBJECT glm::mat4 matrixOrthogonalProjection(float left, float right, float bottom, float top, float near, float far);
DLL_OBJECT glm::mat4 matrixSkewX(float f);
DLL_OBJECT glm::mat4 matrixSkewY(float f);

/// Creates a matrix in row-major order (i.e. opposite from normal glm).
DLL_OBJECT glm::mat4 matrixRowMajor(
        float m11, float m12, float m13, float m14,
        float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34,
        float m41, float m42, float m43, float m44);

/// Creates a matrix in row-major order (i.e. opposite from normal glm).
DLL_OBJECT glm::mat4 matrixColumnMajor(
        float m11, float m21, float m31, float m41,
        float m12, float m22, float m32, float m42,
        float m13, float m23, float m33, float m43,
        float m14, float m24, float m34, float m44);

}

/*! SRC_MATH_GEOMETRY_MATRIXUTIL_HPP_ */
#endif
