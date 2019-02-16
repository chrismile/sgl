/*!
 * SubMesh.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
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

typedef boost::shared_ptr<SubMesh> SubMeshPtr;

}

/*! GRAPHICS_MESH_SUBMESH_HPP_ */
#endif
