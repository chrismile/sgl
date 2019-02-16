/*
 * SubMesh.cpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#include "SubMesh.hpp"
#include <Graphics/Renderer.hpp>
#include <Graphics/Shader/ShaderManager.hpp>

namespace sgl {

SubMesh::SubMesh(ShaderProgramPtr &shader)
{
    renderData = ShaderManager->createShaderAttributes(shader);
    material = MaterialPtr(new Material());
}

SubMesh::SubMesh(bool textured)
{
    ShaderProgramPtr shader;
    if (!textured) {
        shader = ShaderManager->getShaderProgram({"Mesh.Vertex.Plain", "Mesh.Fragment.Plain"});
    } else {
        shader = ShaderManager->getShaderProgram({"Mesh.Vertex.Textured", "Mesh.Fragment.Textured"});
    }
    renderData = ShaderManager->createShaderAttributes(shader);
    material = MaterialPtr(new Material());
}

void SubMesh::render()
{
    renderData->getShaderProgram()->setUniform("color", material->color);
    if (material->texture) {
        renderData->getShaderProgram()->setUniform("texture", material->texture);
    }
    Renderer->render(renderData);
}

void SubMesh::createVertices(VertexPlain *vertices, size_t numVertices)
{
    GeometryBufferPtr geometryBuffer = Renderer->createGeometryBuffer(sizeof(VertexPlain)*numVertices, vertices);
    renderData->addGeometryBuffer(geometryBuffer, "position", ATTRIB_FLOAT, 3);

    glm::vec2 minValues, maxValues;
    minValues.x = minValues.y = 1000000000.0f;
    maxValues.x = maxValues.y = -1000000000.0f;
    for (size_t i = 0; i < numVertices; ++i) {
        if (vertices[i].position.x < minValues.x)
            minValues.x = vertices[i].position.x;
        if (vertices[i].position.y < minValues.y)
            minValues.y = vertices[i].position.y;
        if (vertices[i].position.x > maxValues.x)
            maxValues.x = vertices[i].position.x;
        if (vertices[i].position.y > maxValues.y)
            maxValues.y = vertices[i].position.y;
    }

    aabb.min = glm::vec3(minValues.x, minValues.y, 0.0f);
    aabb.max = glm::vec3(maxValues.x, maxValues.y, 0.0f);
}

void SubMesh::createVertices(VertexTextured *vertices, size_t numVertices)
{
    int stride = sizeof(glm::vec3) + sizeof(glm::vec2);
    GeometryBufferPtr geometryBuffer = Renderer->createGeometryBuffer(sizeof(VertexTextured)*numVertices, vertices);
    renderData->addGeometryBuffer(geometryBuffer, "position", ATTRIB_FLOAT, 3, 0, stride);
    renderData->addGeometryBuffer(geometryBuffer, "texcoord", ATTRIB_FLOAT, 2, sizeof(glm::vec3), stride);

    glm::vec2 minValues, maxValues;
    minValues.x = minValues.y = 1000000000.0f;
    maxValues.x = maxValues.y = -1000000000.0f;
    for (size_t i = 0; i < numVertices; ++i) {
        if (vertices[i].position.x < minValues.x)
            minValues.x = vertices[i].position.x;
        if (vertices[i].position.y < minValues.y)
            minValues.y = vertices[i].position.y;
        if (vertices[i].position.x > maxValues.x)
            maxValues.x = vertices[i].position.x;
        if (vertices[i].position.y > maxValues.y)
            maxValues.y = vertices[i].position.y;
    }

    aabb.min = glm::vec3(minValues.x, minValues.y, 0.0f);
    aabb.max = glm::vec3(maxValues.x, maxValues.y, 0.0f);
}


void SubMesh::createIndices(uint8_t *indices, size_t numIndices)
{
    GeometryBufferPtr geometryBuffer = Renderer->createGeometryBuffer(sizeof(uint8_t)*numIndices, (void*)indices, INDEX_BUFFER);
    renderData->setIndexGeometryBuffer(geometryBuffer, ATTRIB_UNSIGNED_BYTE);
}

void SubMesh::createIndices(uint16_t *indices, size_t numIndices)
{
    GeometryBufferPtr geometryBuffer = Renderer->createGeometryBuffer(sizeof(uint16_t)*numIndices, (void*)indices, INDEX_BUFFER);
    renderData->setIndexGeometryBuffer(geometryBuffer, ATTRIB_UNSIGNED_SHORT);
}

void SubMesh::createIndices(uint32_t *indices, size_t numIndices)
{
    GeometryBufferPtr geometryBuffer = Renderer->createGeometryBuffer(sizeof(uint32_t)*numIndices, (void*)indices, INDEX_BUFFER);
    renderData->setIndexGeometryBuffer(geometryBuffer, ATTRIB_UNSIGNED_INT);
}

}
