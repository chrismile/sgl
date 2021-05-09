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

#include "Mesh.hpp"
#include <Utils/Convert.hpp>
#include <Utils/XML.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/StringUtils.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Shader/ShaderManager.hpp>
#include <Graphics/Renderer.hpp>
#include <vector>
#include <string>
#include <tinyxml2.h>

using namespace tinyxml2;

namespace sgl {

void Mesh::render()
{
    for (auto it = submeshes.begin(); it != submeshes.end(); ++it) {
        (*it)->render();
    }
}

void Mesh::computeAABB()
{
    aabb = AABB3();
    for (SubMeshPtr &submesh : submeshes) {
        aabb.combine(submesh->getAABB());
    }
}

bool Mesh::loadFromXML(const char *filename)
{
    XMLDocument doc;
    if (doc.LoadFile(filename) != 0) {
        Logfile::get()->writeError(std::string() + "Mesh::loadFromXML: Couldn't open file \"" + filename + "\"!");
        return false;
    }
    XMLElement *meshNode = doc.FirstChildElement("MeshXML");
    if (meshNode == NULL) {
        Logfile::get()->writeError("Mesh::loadFromXML: No \"MeshXML\" node found!");
        return false;
    }

    // Traverse all Materials
    std::map<std::string, MaterialPtr> materialMap;
    if (meshNode->FirstChildElement("Materials") != NULL) {
        for (XMLIterator it(meshNode->FirstChildElement("Materials"), XMLNameFilter("Material")); it.isValid(); ++it) {
            XMLElement *materialElement = *it;
            materialMap.insert(std::make_pair(materialElement->Attribute("name"), MaterialManager->getMaterial(materialElement)));
        }
    }

    // Traverse all SubMeshes
    for (XMLIterator it(meshNode, XMLNameFilter("SubMesh")); it.isValid(); ++it) {
        XMLElement *subMeshElement = *it;
        VertexMode vertexMode = VERTEX_MODE_TRIANGLES;

        // Retrieve general rendering information about the mesh
        XMLElement *vertexDataElement = subMeshElement->FirstChildElement("VertexData");
        XMLElement *indexDataElement = subMeshElement->FirstChildElement("IndexData");
        bool useIndices = indexDataElement && indexDataElement->Attribute("data");
        bool textured = vertexDataElement->FirstChildElement("Vertex")->Attribute("u");
        if (vertexDataElement->Attribute("vertexmode")) {
            vertexMode = (VertexMode)vertexDataElement->IntAttribute("vertexmode");
        }

        // Set the estimated amount of vertices and indices. It will be updated to an exact value later.
        size_t numVertices = size_t(vertexDataElement->Attribute("numVertices")
                ? vertexDataElement->FloatAttribute("numVertices") : 64);
        size_t numIndices = size_t(useIndices && indexDataElement->Attribute("numIndices")
                ? indexDataElement->FloatAttribute("numIndices") : 64);

        // Create the SubMesh
        SubMeshPtr subMeshData(new SubMesh(textured));
        subMeshData->setVertexMode(vertexMode);

        // Set the vertices
        if (textured) {
            // Textured
            std::vector<VertexTextured> vertices;
            vertices.reserve(numVertices);
            for(XMLElement* childElement = vertexDataElement->FirstChildElement("Vertex");
                    childElement; childElement = childElement->NextSibling()->ToElement()) {
                vertices.push_back(VertexTextured(glm::vec3(
                        childElement->FloatAttribute("x"), childElement->FloatAttribute("y"),
                        childElement->Attribute("z") ? childElement->FloatAttribute("z") : 0.0f),
                        glm::vec2(childElement->FloatAttribute("u"), childElement->FloatAttribute("v"))));
                if (childElement->NextSibling() == 0)
                    break;
            }
            numVertices = vertices.size();
            subMeshData->createVertices(&vertices.front(), vertices.size());
        } else {
            // Not textured
            std::vector<VertexPlain> vertices;
            vertices.reserve(numVertices);
            for(XMLElement* childElement = vertexDataElement->FirstChildElement("Vertex");
                    childElement; childElement = childElement->NextSibling()->ToElement()) {
                vertices.push_back(VertexPlain(glm::vec3(
                        childElement->FloatAttribute("x"), childElement->FloatAttribute("y"),
                        childElement->Attribute("z") ? childElement->FloatAttribute("z") : 0.0f)));
                if (childElement->NextSibling() == 0)
                    break;
            }
            numVertices = vertices.size();
            subMeshData->createVertices(&vertices.front(), vertices.size());
        }

        if (useIndices) {
            std::vector<std::string> indexStringList;
            splitString(indexDataElement->Attribute("data"), ' ', indexStringList);
            numIndices = indexStringList.size();

            // Add 8-/16-/32-bit indices
            if (numVertices <= UINT8_MAX) {
                std::vector<uint8_t> indices;
                indices.reserve(numIndices);
                for (size_t i = 0; i < indexStringList.size(); ++i)
                    indices.push_back(fromString<uint32_t>(indexStringList.at(i)));
                numIndices = indices.size();
                subMeshData->createIndices(&indices.front(), indices.size());
            } else if (numVertices <= UINT16_MAX) {
                std::vector<uint16_t> indices;
                indices.reserve(numIndices);
                for (size_t i = 0; i < indexStringList.size(); ++i)
                    indices.push_back(fromString<uint32_t>(indexStringList.at(i)));
                numIndices = indices.size();
                subMeshData->createIndices(&indices.front(), indices.size());
            } else {
                std::vector<uint32_t> indices;
                indices.reserve(numIndices);
                for (size_t i = 0; i < indexStringList.size(); ++i)
                    indices.push_back(fromString<uint32_t>(indexStringList.at(i)));
                numIndices = indices.size();
                subMeshData->createIndices(&indices.front(), indices.size());
            }

        }

        // Read the material data
        MaterialPtr material;
        XMLElement *materialElement = subMeshElement->FirstChildElement("Material");
        XMLElement *materialNameElement = subMeshElement->FirstChildElement("MaterialName");
        if (materialElement) {
            material = MaterialManager->getMaterial(materialElement);
        } else if (materialNameElement) {
            material = materialMap.find(materialNameElement->GetText())->second;
        } else {
            material = MaterialPtr(new Material);
        }
        subMeshData->setMaterial(material);

        /*XMLElement *poseAnimationsElement = subMeshElement->FirstChildElement("PoseAnimations");
        if (poseAnimationsElement)
        {
            std::shared_ptr<PoseAnimation> animation(new PoseAnimation);
            subMeshData->setPoseAnimation(animation);

            for(XMLElement* animationElement = poseAnimationsElement->FirstChildElement(); animationElement;
                    animationElement = animationElement->NextSibling()->ToElement())
            {
                const char *animationName = animationElement->Attribute("name");
                vector<glm::vec2> vertexOffsetList;

                for(XMLElement* vertexOffsetElement = animationElement->FirstChildElement(); animationElement;
                        vertexOffsetElement = vertexOffsetElement->NextSibling()->ToElement())
                {
                    vertexOffsetList.push_back(glm::vec2(
                            vertexOffsetElement->FloatAttribute("x"),
                            vertexOffsetElement->FloatAttribute("y")));

                    if (vertexOffsetElement->NextSibling() == 0)
                        break;
                }

                animation->addAnimation(animationName, &vertexOffsetList.front(), vertexOffsetList.size());

                if (animationElement->NextSibling() == 0)
                    break;
            }
        }*/


        // Add the SubMesh to the list
        submeshes.push_back(subMeshData);
    }

    computeAABB();

    return true;
}


void Mesh::addSubMesh(SubMeshPtr &submesh)
{
    submeshes.push_back(submesh);
}

void Mesh::finalizeManualMesh()
{
    computeAABB();
}

}
