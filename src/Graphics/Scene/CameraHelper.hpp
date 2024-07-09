/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_CAMERAHELPER_HPP
#define SGL_CAMERAHELPER_HPP

#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/**
 * Helpers for camera orientation mode conversions.
 *
 * Documentation regarding yaw, pitch, roll:
 * - The internal order is Roll * Yaw * Pitch.
 * - For legacy reasons, yaw' = 3pi/2 - yaw is used in the interface.
 * - Support for roll was removed, as the yaw-pitch mode is only used with the first-person camera navigator,
 *   which doesn't support roll anyways.
 *
 * Yaw * Pitch = \f[\begin{pmatrix}
 *  cos(y) & sin(y)sin(p) &  sin(y)cos(p) \\
 * 0       & cos(p)       & -sin(p)       \\
 * -sin(y) & cos(y)sin(p) &  cos(y)cos(p) \\
 * \end{pmatrix}\f]
 *
 * Roll * Yaw * Pitch = \f[\begin{pmatrix}
 *  cos(y)cos(r) & sin(y)sin(p)cos(r) - cos(p)sin(r) & sin(y)cos(p)cos(r) + sin(p)sin(r) \\
 *  cos(y)sin(r) & sin(y)sin(p)sin(r) + cos(p)cos(r) & sin(y)cos(p)sin(r) - sin(p)cos(r) \\
 * -sin(y)       & cos(y)sin(p)                      & cos(y)cos(p) \\
 * \end{pmatrix}\f]
 *
 * Conversion matrix -> roll pitch yw:
 * https://web.archive.org/web/20220428033032/http://planning.cs.uiuc.edu/node103.html
 * https://web.archive.org/web/20220428033039/http://planning.cs.uiuc.edu/node102.html#eqn:yprmat
 * alpha = roll
 * beta = yaw
 * gamma = pitch
 * Roll * Yaw * Pitch = \f[\begin{pmatrix}
 * r_{11} & r_{12} & {13} \\
 * r_{21} & r_{22} & {23} \\
 * r_{31} & r_{32} & {33}
 * \end{pmatrix}\f]
 *
 * alpha = roll = atan2(r_21, r_11) = atan2(cos(y)sin(r), cos(y)cos(r))
 * beta = yaw = atan2(-r_32, sqrt(r_32^2 + r_33^2)) =
 *  = atan2(-cos(y)sin(p), sqrt(cos(y)^2 sin(p)^2 + cos(y)^2 cos(p)^2))
 * gamma = pitch = atan2(r_32, r_33) = atan2(cos(y)sin(p), cos(y)cos(p))
 *
 * Due to y' = 3pi/2 - y:
 * - sin(3pi/2 - y) = -cos(y')
 * - cos(3pi/2 - y) = -sin(y')
 * Finally, we have front = Yaw * Pitch * (0, 0, -1)^T = (cos(y')cos(p), sin(p), sin(y')cos(p))^T
 */

const int ORT_YAW_PITCH = 1, ORT_QUAT = 2, ORT_CAM_VECTORS = 4;

struct CamVectors {
    glm::vec3 cameraRight; //< x
    glm::vec3 cameraUp; //< y
    glm::vec3 cameraFront; //< -z
};

DLL_OBJECT CamVectors convertYawPitchToCamVectors(float yaw, float pitch);
DLL_OBJECT glm::quat convertCamVectorsToQuat(const CamVectors& camVectors);
DLL_OBJECT glm::quat convertYawPitchToQuat(float yaw, float pitch);
DLL_OBJECT std::pair<float, float> convertCamVectorsToYawPitch(const CamVectors& camVectors);
DLL_OBJECT CamVectors convertQuatToCamVectors(const glm::quat& ort);

#endif //SGL_CAMERAHELPER_HPP
