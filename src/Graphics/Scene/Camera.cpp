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

#include "Camera.hpp"
#include "RenderTarget.hpp"
#include <Math/Math.hpp>
#include <Math/Geometry/MatrixUtil.hpp>
#include <Math/Geometry/Ray3.hpp>
#include <Math/Geometry/Plane.hpp>
#include <Input/Mouse.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Window.hpp>
#include <Graphics/Renderer.hpp>

namespace sgl {

Camera::DepthRange Camera::depthRange = Camera::DepthRange::MINUS_ONE_ONE;
Camera::CoordinateOrigin Camera::coordinateOrigin = Camera::CoordinateOrigin::BOTTOM_LEFT;

Camera::Camera() :  projType(Camera::ProjectionType::PERSPECTIVE),
        fovy(PI/4.0f), nearDist(0.1f), farDist(1000.0f), aspect(4.0f/3.0f),
        recalcFrustum(true) {
    renderTarget = RenderTargetPtr(new RenderTarget());
    viewport = AABB2(glm::vec2(0, 0), glm::vec2(1, 1));
    updateCamera();
}

void Camera::setRenderTarget(RenderTargetPtr target, bool bindFramebuffer) {
    renderTarget = target;
    if (Renderer->getCamera().get() == this) {
        Renderer->setCamera(Renderer->getCamera(), true);
    }
}

// Viewport left, top, width, height for OpenGL/DirectX
glm::ivec4 Camera::getViewportLTWH() {
    int targetW = renderTarget->getWidth();
    int targetH = renderTarget->getHeight();
    glm::vec2 absMin(viewport.min.x * targetW, viewport.min.y * targetH);
    glm::vec2 absMax(viewport.max.x * targetW, viewport.max.y * targetH);
    return glm::ivec4(
            roundf(absMin.x), // left
            roundf(targetH - absMax.y), // top
            roundf(absMax.x - absMin.x), // width
            roundf(absMax.y - absMin.y)); // height
}

glm::mat4 Camera::getRotationMatrix() {
    updateCamera();
    return glm::lookAt(glm::vec3(0.0f), cameraFront, cameraUp);
}


void Camera::overwriteViewMatrix(const glm::mat4 &viewMatrix) {
    modelMatrix = viewMatrix;
    viewProjMat = projMat * modelMatrix;
    inverseViewProjMat = glm::inverse(viewProjMat);
    recalcModelMat = false;

    glm::mat3 rotationMatrix(viewMatrix);
    glm::mat3 inverseRotationMatrix = glm::transpose(rotationMatrix);
    this->transform.position = inverseRotationMatrix * glm::vec3(
            -viewMatrix[3][0], -viewMatrix[3][1], -viewMatrix[3][2]);

    cameraRight =  glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    cameraUp    =  glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    cameraFront = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

    //cameraFront.x = cos(yaw) * cos(pitch);
    //cameraFront.y = sin(pitch);
    //cameraFront.z = sin(yaw) * cos(pitch);
    yaw = std::atan2(cameraFront.z, cameraFront.x);
    pitch = std::asin(cameraFront.y);
}

void Camera::setLookAtViewMatrix(const glm::vec3 &cameraPos, const glm::vec3 &lookAtPos, const glm::vec3 &upDir) {
    lookAtLocation = lookAtPos;
    isPitchMode = false;
    overwriteViewMatrix(glm::lookAt(cameraPos, lookAtPos, upDir));
}

void Camera::copyState(const CameraPtr& otherCamera) {
    this->transform = otherCamera->transform;
    this->cameraRight = otherCamera->cameraRight;
    this->cameraUp = otherCamera->cameraUp;
    this->cameraFront = otherCamera->cameraFront;
    this->yaw = otherCamera->yaw;
    this->pitch = otherCamera->pitch;
    this->isPitchMode = otherCamera->isPitchMode;
    this->lookAtLocation = otherCamera->lookAtLocation;

    this->projType = otherCamera->projType;
    this->fovy = otherCamera->fovy;
    this->nearDist = otherCamera->nearDist;
    this->farDist = otherCamera->farDist;

    this->modelMatrix = otherCamera->modelMatrix;
    this->projMat = otherCamera->projMat;
    this->viewProjMat = otherCamera->viewProjMat;
    this->inverseViewProjMat = otherCamera->inverseViewProjMat;
    this->recalcModelMat = otherCamera->recalcModelMat;

    recalcFrustum = true;
}


glm::mat4 Camera::getProjectionMatrix(DepthRange customDepthRange, CoordinateOrigin customCoordinateOrigin) {
    if (depthRange == customDepthRange && coordinateOrigin == customCoordinateOrigin) {
        return getProjectionMatrix();
    }

#if GLM_VERSION < 980
    sgl::Logfile::get()->throwError(
            "Error in Camera::updateCamera: Clip control not supported in GLM versions prior to 0.9.8.0.");
    return getProjectionMatrix();
#else
    glm::mat4 customProjectionMatrix;

    if (customDepthRange == Camera::DepthRange::MINUS_ONE_ONE) {
        customProjectionMatrix = glm::perspectiveRH_NO(fovy, aspect, nearDist, farDist);
    } else {
        customProjectionMatrix = glm::perspectiveRH_ZO(fovy, aspect, nearDist, farDist);
    }

    if (customCoordinateOrigin == CoordinateOrigin::TOP_LEFT) {
        customProjectionMatrix[1][1] = -customProjectionMatrix[1][1];
    }

    return customProjectionMatrix;
#endif
}


void Camera::onResolutionChanged(const EventPtr& event) {
    float w = viewport.getWidth() * float(renderTarget->getWidth());
    float h = viewport.getHeight() * float(renderTarget->getHeight());
    aspect = w / h;
    invalidateFrustum();
}


AABB2 Camera::getAABB2(float planeDistance) {
    updateCamera();
    Ray3 ray1 = getCameraToViewportRay(glm::vec2(0.0f, 1.0f));
    Ray3 ray2 = getCameraToViewportRay(glm::vec2(1.0f, 0.0f));
    Plane projPlane(glm::vec3(0.0f, 0.0f, 1.0f), -fabs(planeDistance));
    RaycastResult intersection1 = ray1.intersects(projPlane);
    RaycastResult intersection2 = ray2.intersects(projPlane);
    AABB2 aabb2;
    aabb2.min = ray1.getPoint2D(intersection1.t);
    aabb2.max = ray2.getPoint2D(intersection2.t);
    return aabb2;
}

glm::vec2 Camera::mousePositionInPlane(float planeDistance) {
    updateCamera();
    Window *window = AppSettings::get()->getMainWindow();
    Ray3 ray = getCameraToViewportRay(glm::vec2(
            Mouse->getX()/float(window->getWidth()),
            Mouse->getY()/float(window->getHeight())));
    Plane physicsPlane(glm::vec3(0.0f, 0.0f, 1.0f), -fabs(planeDistance));
    RaycastResult intersection = ray.intersects(physicsPlane);
    glm::vec2 planePosition = ray.getPoint2D(intersection.t);
    return planePosition;
}

void Camera::updateCamera() {
    if (recalcFrustum) {
        //projMat = glm::perspective(fovy, aspect, nearDist, farDist);
#if GLM_VERSION < 980
        if (depthRange != Camera::DepthRange::MINUS_ONE_ONE) {
            sgl::Logfile::get()->writeError(
                    "Error in Camera::updateCamera: Clip control not supported in GLM versions prior to 0.9.8.0.");
        }
        projMat = glm::perspective(fovy, aspect, nearDist, farDist);
#else
        if (depthRange == Camera::DepthRange::MINUS_ONE_ONE) {
            projMat = glm::perspectiveRH_NO(fovy, aspect, nearDist, farDist);
        } else {
            projMat = glm::perspectiveRH_ZO(fovy, aspect, nearDist, farDist);
        }
#endif
        if (coordinateOrigin == CoordinateOrigin::TOP_LEFT) {
            projMat[1][1] = -projMat[1][1];
        }
    }
    if (recalcModelMat) {
        // We don't want a flip-over at the poles of the unit sphere.
        const float EPSILON = 0.001f;
        if (isPitchMode) {
            pitch = sgl::clamp(pitch, -sgl::HALF_PI + EPSILON, sgl::HALF_PI - EPSILON);
        }

        cameraFront.x = cos(yaw) * cos(pitch);
        cameraFront.y = sin(pitch);
        cameraFront.z = sin(yaw) * cos(pitch);

        cameraFront = glm::normalize(cameraFront);
        cameraRight = glm::normalize(glm::cross(cameraFront, globalUp));
        cameraUp    = glm::normalize(glm::cross(cameraRight, cameraFront));

        modelMatrix = //glm::mat4(this->transform.orientation) *
                glm::lookAt(glm::vec3(0.0f), cameraFront, cameraUp) *
                matrixTranslation(-this->transform.position);
    }
    if (recalcFrustum || recalcModelMat) {
        viewProjMat = projMat * modelMatrix;
        inverseViewProjMat = glm::inverse(viewProjMat);
    }
    if (recalcFrustum) {
        updateFrustumPlanes();
    }
    recalcFrustum = recalcModelMat = false;
}

void Camera::updateFrustumPlanes() {
    // The underlying idea of the following code comes from
    // http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-implementation-details/

    // Near plane
    frustumPlanes[0] = Plane(
            viewProjMat[0][3] + viewProjMat[0][2],
            viewProjMat[1][3] + viewProjMat[1][2],
            viewProjMat[2][3] + viewProjMat[2][2],
            viewProjMat[3][3] + viewProjMat[3][2]);

    // Far plane
    frustumPlanes[1] = Plane(
            viewProjMat[0][3] - viewProjMat[0][2],
            viewProjMat[1][3] - viewProjMat[1][2],
            viewProjMat[2][3] - viewProjMat[2][2],
            viewProjMat[3][3] - viewProjMat[3][2]);

    // Left plane
    frustumPlanes[2] = Plane(
            viewProjMat[0][3] + viewProjMat[0][0],
            viewProjMat[1][3] + viewProjMat[1][0],
            viewProjMat[2][3] + viewProjMat[2][0],
            viewProjMat[3][3] + viewProjMat[3][0]);

    // Right plane
    frustumPlanes[3] = Plane(
            viewProjMat[0][3] - viewProjMat[0][0],
            viewProjMat[1][3] - viewProjMat[1][0],
            viewProjMat[2][3] - viewProjMat[2][0],
            viewProjMat[3][3] - viewProjMat[3][0]);

    // Bottom plane
    frustumPlanes[4] = Plane(
            viewProjMat[0][3] + viewProjMat[0][1],
            viewProjMat[1][3] + viewProjMat[1][1],
            viewProjMat[2][3] + viewProjMat[2][1],
            viewProjMat[3][3] + viewProjMat[3][1]);

    // Top plane
    frustumPlanes[5] = Plane(
            viewProjMat[0][3] - viewProjMat[0][1],
            viewProjMat[1][3] - viewProjMat[1][1],
            viewProjMat[2][3] - viewProjMat[2][1],
            viewProjMat[3][3] - viewProjMat[3][1]);

    // Normalize parameters
    for (int i = 0; i < 6; ++i) {
        float normalLength = glm::length(frustumPlanes[i].getNormal());
        frustumPlanes[i].a /= normalLength;
        frustumPlanes[i].b /= normalLength;
        frustumPlanes[i].c /= normalLength;
        frustumPlanes[i].d /= normalLength;
    }

}

Ray3 Camera::getCameraToViewportRay(const glm::vec2 &screenPos)
{
    // Normalized coordinates
    glm::vec2 nc(2.0f * screenPos.x - 1.0f, 1.0f - 2.0f * screenPos.y);
    glm::vec3 nearPoint(nc.x, nc.y, -1.0f), midPoint (nc.x, nc.y, 0.0f);

    // Compute ray origin and target on near plane
    glm::vec3 rayOrigin = transformPoint(inverseViewProjMat, nearPoint);
    glm::vec3 rayTarget = transformPoint(inverseViewProjMat, midPoint);
    glm::vec3 rayDirection = glm::normalize(rayTarget - rayOrigin);

    return Ray3(rayOrigin, rayDirection);
}


bool Camera::isVisible(const AABB3 &aabb) const {
    // Not visible if all points on negative side of one plane
    for (int i = 0; i < 6; ++i) {
        if (frustumPlanes[i].isOutside(aabb)) {
            return false;
        }
    }
    return true;
}

bool Camera::isVisible(const Sphere &sphere) const {
    // Not visible if distance plane -> sphere center less than negative radius
    for (int i = 0; i < 6; ++i) {
        if (frustumPlanes[i].getDistance(sphere.center) < -sphere.radius) {
            return false;
        }
    }
    return true;
}

bool Camera::isVisible(const glm::vec2 &vert) const {
    return isVisible(glm::vec3(vert.x, vert.y, 1.0f));
}

bool Camera::isVisible(const glm::vec3 &vert) const {
    // Not visible if point is on negative side of at least one plane
    for (int i = 0; i < 6; ++i) {
        if (frustumPlanes[i].isOutside(vert)) {
            return false;
        }
    }
    return true;
}

}
