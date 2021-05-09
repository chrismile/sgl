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

#ifndef GRAPHICS_MESH_MATERIAL_HPP_
#define GRAPHICS_MESH_MATERIAL_HPP_

#include <string>
#include <memory>
#include <Graphics/Color.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <Utils/File/FileManager.hpp>

namespace tinyxml2 {
class XMLElement;
}

namespace sgl {

class DLL_OBJECT Material
{
public:
    Color color;
    TexturePtr texture;
};

typedef std::shared_ptr<Material> MaterialPtr;

DLL_OBJECT int minificationFilterFromString(const char *filter);
DLL_OBJECT int magnificationFilterFromString(const char *filter);
DLL_OBJECT int textureWrapFromString(const char *filter);

struct DLL_OBJECT MaterialInfo {
    MaterialInfo() : loaded(false), minificationFilter(0), magnificationFilter(0),
            textureWrapS(0), textureWrapT(0), anisotropicFilter(false) {}

    //! File information
    std::string filename;
    std::string materialName;

    //! Material data
    bool loaded;
    Color color;
    std::string textureFilename;
    int minificationFilter, magnificationFilter, textureWrapS, textureWrapT;
    bool anisotropicFilter;

    bool operator <(const MaterialInfo &rhs) const {
        if (filename != rhs.filename) {
            return filename < rhs.filename;
        }
        return materialName < rhs.materialName;
    }
    bool operator ==(const MaterialInfo &rhs) const {
        return filename == rhs.filename && materialName == rhs.materialName;
    }
};

/*! Handles the loading of materials from XML files */
class DLL_OBJECT MaterialManagerInterface : public FileManager<Material, MaterialInfo>
{
public:
    /*! Reference-counted loading:
     * Load the material with the name 'materialName' from the file 'filename' */
    MaterialPtr getMaterial(const char *filename, const char *materialName);

    //! Get the material this element describes
    MaterialPtr getMaterial(tinyxml2::XMLElement *materialElement);

protected:
    /*! Create the material if the file was already parsed.
     * Otherwise parse the file, add all material information and create the material described by the info */
    virtual MaterialPtr loadAsset(MaterialInfo &info);

    //! Parse the XML element and create the material info from it
    MaterialInfo loadMaterialInfo(tinyxml2::XMLElement *materialElement);

    //! Create a material from the info
    MaterialPtr createMaterial(const MaterialInfo &info);
};
DLL_OBJECT extern MaterialManagerInterface *MaterialManager;

}

/*! GRAPHICS_MESH_MATERIAL_HPP_ */
#endif
