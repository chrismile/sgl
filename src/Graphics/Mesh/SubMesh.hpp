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

#ifndef GRAPHICS_MESH_SUBMESH_HPP_
#define GRAPHICS_MESH_SUBMESH_HPP_

#include "Vertex.hpp"
#include "Material.hpp"
#include <Graphics/Shader/ShaderAttributes.hpp>
#include <Graphics/Shader/Shader.hpp>
#include <Math/Geometry/AABB3.hpp>

namespace sgl {

class DLL_OBJECT SubMesh
{
    friend class Mesh;
public:
    SubMesh(ShaderProgramPtr &shader);
    //! Automatically retrieves the standard shaders
    SubMesh(bool textured);
    void render();
    inline MaterialPtr &getMaterial() { return material; }
    inline void setMaterial(const MaterialPtr &_material) { material = _material; }
    inline const AABB3 &getAABB() const { return aabb; }
    inline void setAABB(const AABB3 &_aabb) { aabb = _aabb; }

    //! Call these functions to create a mesh manually
    void createVertices(VertexPlain *vertices, size_t numVertices);
    void createVertices(VertexTextured *vertices, size_t numVertices);
    void createIndices(uint8_t *indices, size_t numIndices);
    void createIndices(uint16_t *indices, size_t numIndices);
    void createIndices(uint32_t *indices, size_t numIndices);
    inline void setVertexMode(VertexMode vertexMode) { renderData->setVertexMode(vertexMode); }

private:
    void computeAABB();
    ShaderAttributesPtr renderData;
    MaterialPtr material;
    AABB3 aabb;
};

typedef std::shared_ptr<SubMesh> SubMeshPtr;

}

/*! GRAPHICS_MESH_SUBMESH_HPP_ */
#endif
