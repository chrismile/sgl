/*!
 * SceneNode.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
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
