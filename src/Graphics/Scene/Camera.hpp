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

#ifndef SRC_GRAPHICS_SCENE_CAMERA_HPP_
#define SRC_GRAPHICS_SCENE_CAMERA_HPP_

#include "SceneNode.hpp"
#include <Math/Math.hpp>
#include <Math/Geometry/AABB3.hpp>
#include <Math/Geometry/Plane.hpp>
#include <Math/Geometry/AABB2.hpp>
#include <Math/Geometry/Sphere.hpp>

namespace sgl {

class Ray3;
class AppSettings;
class RenderTarget;
typedef std::shared_ptr<RenderTarget> RenderTargetPtr;
class Camera;
typedef std::shared_ptr<Camera> CameraPtr;
class Event;
typedef std::shared_ptr<Event> EventPtr;

class DLL_OBJECT Camera : public SceneNode {
    friend class AppSettings;
public:
    Camera();
    virtual ~Camera() {}
    void onResolutionChanged(EventPtr event);

    enum ProjectionType {
        ORTHOGRAPHIC_PROJECTION, PERSPECTIVE_PROJECTION
    };
    enum DepthRange {
        DEPTH_RANGE_MINUS_ONE_ONE, // OpenGL
        DEPTH_RANGE_ZERO_ONE // Vulkan/DirectX
    };

    //! Render target & viewport area
    void setViewport(const AABB2 &_viewport) { viewport = _viewport; }
    //! Relative coordinates
    AABB2 getViewport() const { return viewport; }
    //! Viewport left, top, width, height for OpenGL/DirectX
    glm::ivec4 getViewportLTWH();
    void setRenderTarget(RenderTargetPtr target);
    inline RenderTargetPtr getRenderTarget() { return renderTarget; }

    //! Frustum data
    inline float getNearClipDistance() const { return nearDist; }
    inline float getFarClipDistance()  const { return farDist; }
    inline float getFOVy()             const { return fovy; }
    inline float getFOVx()             const { return 2.0f * atanf(tanf(fovy * 0.5f) * aspect); }
    inline float getAspectRatio()      const { return aspect; }
    void setNearClipDistance(float dist) { nearDist = dist; invalidateFrustum(); }
    void setFarClipDistance(float dist)  { farDist = dist; invalidateFrustum(); }
    void setFOVy(float fov)              { fovy = fov; invalidateFrustum(); }

    // View data
    float getYaw()   const { return yaw; }
    float getPitch() const { return pitch; }
    void rotateYaw(float offset)   { recalcModelMat = true; yaw += offset; }
    void setYaw(float newYaw)      { recalcModelMat = true; yaw = newYaw; }
    void rotatePitch(float offset) { recalcModelMat = true; pitch += offset; }
    void setPitch(float newPitch)  { recalcModelMat = true; pitch = newPitch; }
    const glm::vec3 &getCameraFront() const { return cameraFront; }
    const glm::vec3 &getCameraRight() const { return cameraRight; }
    const glm::vec3 &getCameraUp() const { return cameraUp; }

    //! Set projection type: Orthogonal or perspective
    //virtual void setProjectionType(ProjectionType pt);
    //virtual void setOrthoWindow(float w, float h);

    //! View & projection matrices
    inline const glm::mat4& getViewMatrix()            { updateCamera(); return modelMatrix; }
    inline const glm::mat4& getProjectionMatrix()      { updateCamera(); return projMat; }
    inline const glm::mat4& getViewProjMatrix()        { updateCamera(); return viewProjMat; }
    inline const glm::mat4& getInverseViewProjMatrix() { updateCamera(); return inverseViewProjMat; }
    inline DepthRange getDepthRange()            const { return depthRange; }
    glm::mat4 getRotationMatrix();
    void overwriteViewMatrix(const glm::mat4 &viewMatrix);

    //! For frustum culling
    virtual bool isVisible(const AABB3& bound) const;
    virtual bool isVisible(const Sphere& bound) const;
    virtual bool isVisible(const glm::vec2 &vert) const;
    virtual bool isVisible(const glm::vec3 &vert) const;

    //! AABB of a slice of the view frustum in distance planeDistance
    AABB2 getAABB2(float planeDistance = -1.0f);
    //! Position of the Mouse in the plane with the given distance
    glm::vec2 mousePositionInPlane(float planeDistance = -1.0f);

protected:
    //! screenPos has to be in relative window coordinates [0,1]x[0,1]
    Ray3 getCameraToViewportRay(const glm::vec2 &screenPos);
    //! Calls updateFrustumPlanes
    void updateCamera();
    void updateFrustumPlanes();
    void invalidateFrustum() { recalcFrustum = true; }
    void invalidateView() { recalcModelMat = true; }

    RenderTargetPtr renderTarget;

    // View matrix data
    float yaw = -sgl::PI/2.0f;   //< around y axis
    float pitch = 0.0f; //< around x axis
    glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraFront, cameraRight, cameraUp;

    // The depth range is set by AppSettings::initializeSubsystems depending on what renderer is used.
    static DepthRange depthRange;
    ProjectionType projType;
    float fovy;
    float nearDist;
    float farDist;
    float aspect;

    AABB2 viewport;

    //! modelMatrix of Camera is view matrix
    glm::mat4 projMat, viewProjMat, inverseViewProjMat;
    AABB3 boundingBox;
    glm::vec3 worldSpaceCorners[8];
    Plane frustumPlanes[6];
    bool recalcFrustum;

};

typedef std::shared_ptr<Camera> CameraPtr;

}

/*! SRC_GRAPHICS_SCENE_CAMERA_HPP_ */
#endif
