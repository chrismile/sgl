/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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

#ifndef GRAPHICS_MESH_VERTEX_HPP_
#define GRAPHICS_MESH_VERTEX_HPP_

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/fallback/vec.hpp>
#endif
#include <Graphics/Color.hpp>

namespace sgl {

struct DLL_OBJECT VertexPlain {
    explicit VertexPlain(glm::vec3 _position) : position(_position) {}
    glm::vec3 position;
};

struct DLL_OBJECT VertexTextured {
    VertexTextured(glm::vec2 _position, glm::vec2 _texcoord) : position(_position.x, _position.y, 0.0f), texcoord(_texcoord) {}
    VertexTextured(glm::vec3 _position, glm::vec2 _texcoord) : position(_position), texcoord(_texcoord) {}
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct DLL_OBJECT VertexColor {
    VertexColor(glm::vec3 _position, Color _color) : position(_position), color(_color) {}
    glm::vec3 position;
    Color color;
};

struct DLL_OBJECT VertexColorTextured {
    VertexColorTextured(glm::vec3 _position, glm::vec2 _texcoord) : position(_position), texcoord(_texcoord) {}
    glm::vec3 position;
    glm::vec2 texcoord;
    Color color;
};

}

/*! GRAPHICS_MESH_VERTEX_HPP_ */
#endif
