/*!
 * Vertex.hpp
 *
 *  Created on: 09.04.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_MESH_VERTEX_HPP_
#define GRAPHICS_MESH_VERTEX_HPP_

#include <glm/glm.hpp>
#include <Graphics/Color.hpp>

namespace sgl {

struct DLL_OBJECT VertexPlain
{
    VertexPlain(glm::vec3 _position) : position(_position) {}
    glm::vec3 position;
};

struct DLL_OBJECT VertexTextured
{
    VertexTextured(glm::vec2 _position, glm::vec2 _texcoord) : position(_position.x, _position.y, 0.0f), texcoord(_texcoord) {}
    VertexTextured(glm::vec3 _position, glm::vec2 _texcoord) : position(_position), texcoord(_texcoord) {}
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct DLL_OBJECT VertexColor
{
    VertexColor(glm::vec3 _position, Color _color) : position(_position), color(_color) {}
    glm::vec3 position;
    Color color;
};

struct DLL_OBJECT VertexColorTextured
{
    VertexColorTextured(glm::vec3 _position, glm::vec2 _texcoord) : position(_position), texcoord(_texcoord) {}
    glm::vec3 position;
    glm::vec2 texcoord;
    Color color;
};

}

/*! GRAPHICS_MESH_VERTEX_HPP_ */
#endif
