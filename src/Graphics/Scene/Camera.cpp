/*
 * Camera.cpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#include "Camera.hpp"
#include "RenderTarget.hpp"
#include <Math/Math.hpp>
#include <Math/Geometry/MatrixUtil.hpp>
#include <Math/Geometry/Ray3.hpp>
#include <Math/Geometry/Plane.hpp>
#include <Input/Mouse.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Window.hpp>
#include <Graphics/Renderer.hpp>

namespace sgl {

Camera::Camera() :  projType(Camera::PERSPECTIVE_PROJECTION),
        fovy(PI/4.0f), nearDist(0.1f), farDist(1000.0f), aspect(4.0f/3.0f),
        recalcFrustum(true) {
    renderTarget = RenderTargetPtr(new RenderTarget());
    viewport = AABB2(glm::vec2(0, 0), glm::vec2(1, 1));
    updateCamera();
}

void Camera::setRenderTarget(RenderTargetPtr target) {
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

glm::mat4 Camera::getRotationMatrix()
{
    return glm::lookAt(glm::vec3(0.0f), cameraFront, cameraUp);
}

void Camera::overwriteViewMatrix(const glm::mat4 &viewMatrix)
{
    modelMatrix = viewMatrix;
    viewProjMat = projMat * modelMatrix;
    inverseViewProjMat = glm::inverse(viewProjMat);
    recalcModelMat = false;

    glm::mat3 rotationMatrix(viewMatrix);
    glm::mat3 inverseRotationMatrix = glm::transpose(rotationMatrix);
    this->transform.position = inverseRotationMatrix * glm::vec3(-viewMatrix[3][0], -viewMatrix[3][1], -viewMatrix[3][2]);

    cameraRight = glm::vec3(viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2]);
    cameraUp =    glm::vec3(viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2]);
    cameraFront = -glm::vec3(viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2]);

    // TODO: This doesn't work...
    //cameraFront.x = cos(yaw) * cos(pitch);
    //cameraFront.y = sin(pitch);
    //cameraFront.z = sin(yaw) * cos(pitch);
    pitch = std::asin(cameraFront.y);
    yaw = std::acos(cameraFront.z / std::cos(pitch));

    /*modelMatrix = //glm::mat4(this->transform.orientation) *
            glm::lookAt(glm::vec3(0.0f), cameraFront, cameraUp) *
            matrixTranslation(-this->transform.position);*/
}

/*void Camera::setProjectionType(ProjectionType pt) {
    ;
}

void Camera::setOrthoWindow(float w, float h) {
    ;
}*/


void Camera::onResolutionChanged(EventPtr event) {
    float w = viewport.getWidth()*renderTarget->getWidth();
    float h = viewport.getHeight()*renderTarget->getHeight();
    aspect = w/h;
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
        projMat = glm::perspective(fovy, aspect, nearDist, farDist);
    }
    if (recalcModelMat) {
        // We don't want a flip-over at the poles of the unit sphere.
        const float EPSILON = 0.001f;
        pitch = sgl::clamp(pitch, -sgl::HALF_PI + EPSILON, sgl::HALF_PI - EPSILON);

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
