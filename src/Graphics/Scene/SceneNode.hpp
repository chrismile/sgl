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

#ifndef SRC_GRAPHICS_SCENE_SCENENODE_HPP_
#define SRC_GRAPHICS_SCENE_SCENENODE_HPP_

#include "Transform.hpp"
#include "Renderable.hpp"

namespace sgl {

class DLL_OBJECT SceneNode {
public:
    SceneNode() : recalcModelMat(true) {}
    void setPosition(const glm::vec2 &pos) { recalcModelMat = true; transform.position = glm::vec3(pos.x, pos.y, 0.0f); }
    void setPosition(const glm::vec3 &pos) { recalcModelMat = true; transform.position = pos; }
    void setScale(const glm::vec3 &scale) { recalcModelMat = true; transform.scale = scale; }
    void setOrientation(const glm::quat &ort) { recalcModelMat = true; transform.orientation = ort; }
    void translate(const glm::vec3 &pos) { recalcModelMat = true; transform.position += pos; }
    void scale(const glm::vec3 &scale) { recalcModelMat = true; transform.scale.x *= scale.x; transform.scale.y *= scale.y; transform.scale.z *= scale.z; }
    void rotate(const glm::quat &ort) { recalcModelMat = true; transform.orientation *= ort; }

    glm::vec3 &getPosition() { return transform.position; }
    glm::vec3 &getScale() { return transform.scale; }
    glm::quat &getOrientation() { return transform.orientation; }

    void attachRenderable(RenderablePtr r) { renderable = r; }

protected:
    Transform transform;
    glm::mat4 modelMatrix;
    bool recalcModelMat;

    RenderablePtr renderable;
};


}

/*! SRC_GRAPHICS_SCENE_SCENENODE_HPP_ */
#endif
