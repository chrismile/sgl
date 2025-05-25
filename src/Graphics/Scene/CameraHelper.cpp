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

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

#include <Math/Math.hpp>

#include "CameraHelper.hpp"

CamVectors convertYawPitchToCamVectors(float yaw, float pitch) {
    const glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight, cameraUp, cameraFront;

    cameraFront.x = std::cos(yaw) * std::cos(pitch);
    cameraFront.y = std::sin(pitch);
    cameraFront.z = std::sin(yaw) * std::cos(pitch);

    cameraFront = glm::normalize(cameraFront);
    cameraRight = glm::normalize(glm::cross(cameraFront, globalUp));
    cameraUp    = glm::normalize(glm::cross(cameraRight, cameraFront));

    return { cameraRight, cameraUp, cameraFront };
}

glm::quat convertCamVectorsToQuat(const CamVectors& camVectors) {
    const glm::vec3& cameraRight = camVectors.cameraRight;
    const glm::vec3& cameraUp = camVectors.cameraUp;
    const glm::vec3& cameraFront = camVectors.cameraFront;

    glm::mat3 viewMatrix;
    viewMatrix[0][0] = cameraRight.x;
    viewMatrix[1][0] = cameraRight.y;
    viewMatrix[2][0] = cameraRight.z;
    viewMatrix[0][1] = cameraUp.x;
    viewMatrix[1][1] = cameraUp.y;
    viewMatrix[2][1] = cameraUp.z;
    viewMatrix[0][2] = -cameraFront.x;
    viewMatrix[1][2] = -cameraFront.y;
    viewMatrix[2][2] = -cameraFront.z;
    return glm::quat_cast(viewMatrix);
}

glm::quat convertYawPitchToQuat(float yaw, float pitch) {
    glm::quat rot =
            glm::angleAxis(-pitch, glm::vec3(1, 0, 0))
            * glm::angleAxis(yaw + sgl::PI / 2.0f, glm::vec3(0, 1, 0));
    return rot;
    // This has been confirmed to be equivalent to:
    //return convertCamVectorsToQuat(convertYawPitchToCamVectors(yaw, pitch));
}

std::pair<float, float> convertCamVectorsToYawPitch(const CamVectors& camVectors) {
    //const glm::vec3& cameraRight = camVectors.cameraRight;
    const glm::vec3& cameraUp = camVectors.cameraUp;
    const glm::vec3& cameraFront = camVectors.cameraFront;

    float yaw = std::atan2(cameraFront.z, cameraFront.x);
    //pitch = std::asin(cameraFront.y);
    float pitch = std::atan2(cameraFront.y, cameraUp.y);
    return std::make_pair(yaw, pitch);
}

CamVectors convertQuatToCamVectors(const glm::quat& ort) {
    glm::mat3 viewMatrix = glm::mat3_cast(ort);
    glm::vec3 cameraRight =  glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    glm::vec3 cameraUp    =  glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    glm::vec3 cameraFront = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
    return { cameraRight, cameraUp, cameraFront };
}

// Legacy code (for unsupported roll):
/*
 * float r_11 = viewMatrix[0][0];
 * float r_21 = viewMatrix[1][0];
 * float r_32 = viewMatrix[2][1];
 * float r_33 = viewMatrix[2][2];
 * yaw = std::atan2(r_21, r_11);
 * pitch = std::atan2(-r_32, std::sqrt(r_32 * r_32 + r_33 * r_33));
 * roll = std::atan2(r_32, r_33);
 */
/*
 * cameraFront.x = -std::sin(yaw) * std::cos(pitch) * std::cos(roll) - std::sin(pitch) * std::sin(roll);
 * cameraFront.y = -std::sin(yaw) * std::cos(pitch) * std::sin(roll) + std::sin(pitch) * std::cos(roll);
 * cameraFront.z = -std::cos(yaw) * std::cos(pitch);
 * cameraFront = glm::normalize(cameraFront);
 *
 * cameraRight.x = std::cos(yaw) * std::cos(roll);
 * cameraRight.y = std::cos(yaw) * std::sin(roll);
 * cameraRight.z = -std::sin(yaw);
 * cameraRight = glm::normalize(cameraRight);
 *
 * cameraUp.x = std::sin(yaw) * std::sin(pitch) * std::cos(roll) - std::cos(pitch) * std::sin(roll);
 * cameraUp.y = std::sin(yaw) * std::sin(pitch) * std::sin(roll) + std::cos(pitch) * std::cos(roll);
 * cameraUp.z = std::cos(yaw) * std::sin(pitch);
 * cameraUp = glm::normalize(cameraUp);
 */
