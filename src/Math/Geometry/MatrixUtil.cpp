/*
 * MatrixUtil.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#include "MatrixUtil.hpp"

namespace sgl {

glm::vec3 transformPoint(const glm::mat4 &mat, const glm::vec3 &vec) {
    glm::vec4 transVec = mat * glm::vec4(vec.x, vec.y, vec.z, 1.0f);
    if (transVec.w != 1.0f)
        transVec /= transVec.w;
    return glm::vec3(transVec.x, transVec.y, transVec.z);
}

glm::vec3 transformDirection(const glm::mat4 &mat, const glm::vec3 &vec) {
    glm::vec4 transVec = mat * glm::vec4(vec.x, vec.y, vec.z, 0.0f);
    return glm::vec3(transVec.x, transVec.y, transVec.z);
}

glm::vec2 transformPoint(const glm::mat4 &mat, const glm::vec2 &vec) {
    glm::vec4 transVec = mat * glm::vec4(vec.x, vec.y, 0.0f, 1.0f);
    if (transVec.w != 1.0f)
        transVec /= transVec.w;
    return glm::vec2(transVec.x, transVec.y);
}

glm::vec2 transformDirection(const glm::mat4 &mat, const glm::vec2 &vec) {
    glm::vec4 transVec = mat * glm::vec4(vec.x, vec.y, 0.0f, 0.0f);
    return glm::vec2(transVec.x, transVec.y);
}


DLL_OBJECT glm::mat4 matrixTranslation(const glm::vec3 &v) {
    /*return glm::translate(glm::vec3(v.x, v.y, v.z));*/
    return glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  v.x,  v.y,  v.z,  1.0f);

}

DLL_OBJECT glm::mat4 matrixTranslation(const glm::vec2 &v) {
    //return glm::translate(glm::vec3(v.x, v.y, 0.0f));
    return glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  v.x,  v.y,  0.0f, 1.0f);
}

DLL_OBJECT glm::mat4 matrixScaling(const glm::vec3 &vec) {
    return glm::scale(glm::vec3(vec.x, vec.y, vec.z));
}

DLL_OBJECT glm::mat4 matrixScaling(const glm::vec2 &vec) {
    return glm::scale(glm::vec3(vec.x, vec.y, 1.0f));
}

DLL_OBJECT glm::mat4 matrixOrthogonalProjection(float left, float right, float bottom, float top, float near, float far)
{
    return glm::mat4(2.0f / (right - left), 0.0f, 0.0f, - (right + left) / (right - left),
                   0.0f, 2.0f / (top - bottom), 0.0f, - (top + bottom) / (top - bottom),
                   0.0f, 0.0f, -2.0f / (far - near), - (far + near) / (far - near),
                   0.0f, 0.0f, 0.0f, 1.0f);
}
DLL_OBJECT glm::mat4 matrixSkewX(float f)
{
    // Calculate scaling matrix
    return glm::mat4(1.0f, tan(f), 0.0f, 0.0f,
                    0.0f,  1.0f,   0.0f, 0.0f,
                    0.0f,  0.0f,   1.0f, 0.0f,
                    0.0f,  0.0f,   0.0f, 1.0f);
}

DLL_OBJECT glm::mat4 matrixSkewY(float f)
{
    // Calculate scaling matrix
    return glm::mat4(1.0f,  0.0f, 0.0f, 0.0f,
                    tan(f), 1.0f, 0.0f, 0.0f,
                    0.0f,   0.0f, 1.0f, 0.0f,
                    0.0f,   0.0f, 0.0f, 1.0f);
}

/// Creates a matrix in row-major order (i.e. opposite from normal glm).
DLL_OBJECT glm::mat4 matrixRowMajor(
        float m11, float m12, float m13, float m14,
        float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34,
        float m41, float m42, float m43, float m44)
{
    return glm::mat4(
            m11, m21, m31, m41,
            m12, m22, m32, m42,
            m13, m23, m33, m43,
            m14, m24, m34, m44);
}

/// Creates a matrix in row-major order (i.e. opposite from normal glm).
DLL_OBJECT glm::mat4 matrixColumnMajor(
        float m11, float m21, float m31, float m41,
        float m12, float m22, float m32, float m42,
        float m13, float m23, float m33, float m43,
        float m14, float m24, float m34, float m44)
{
    return glm::mat4(
            m11, m21, m31, m41,
            m12, m22, m32, m42,
            m13, m23, m33, m43,
            m14, m24, m34, m44);
}

}
