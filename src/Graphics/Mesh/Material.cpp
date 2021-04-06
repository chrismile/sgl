/*
 * Material.cpp
 *
 *  Created on: 08.04.2015
 *      Author: Christoph Neuhauser
 */

#include "Material.hpp"
#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <cstring>
#include <tinyxml2.h>

using namespace tinyxml2;

namespace sgl {

int minificationFilterFromString(const char *filter)
{
    int textureMinFilter = GL_LINEAR;

    if (strcmp(filter, "Linear") == 0)
        textureMinFilter = GL_LINEAR;
    else if (strcmp(filter, "Nearest") == 0)
        textureMinFilter = GL_NEAREST;
    else if (strcmp(filter, "NearestMipmapNearest") == 0)
        textureMinFilter = GL_NEAREST_MIPMAP_NEAREST;
    else if (strcmp(filter, "NearestMipmapLinear") == 0)
        textureMinFilter = GL_NEAREST_MIPMAP_LINEAR;
    else if (strcmp(filter, "LinearMipmapNearest") == 0)
        textureMinFilter = GL_LINEAR_MIPMAP_NEAREST;
    else if (strcmp(filter, "LinearMipmapLinear") == 0)
        textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
    else if (isInteger(filter))
        textureMinFilter = fromString<int>(filter);

    return textureMinFilter;
}

int magnificationFilterFromString(const char *filter)
{
    int textureMagFilter = GL_LINEAR;

    if (strcmp(filter, "Linear") == 0)
        textureMagFilter = GL_LINEAR;
    else if (strcmp(filter, "Nearest") == 0)
        textureMagFilter = GL_NEAREST;
    else if (isInteger(filter))
        textureMagFilter = fromString<int>(filter);

    return textureMagFilter;
}

int textureWrapFromString(const char *filter)
{
    int textureWrap = GL_REPEAT;

    if (strcmp(filter, "Repeat") == 0)
        textureWrap = GL_REPEAT;
    else if (strcmp(filter, "MirroredRepeat") == 0)
        textureWrap = GL_MIRRORED_REPEAT;
    else if (strcmp(filter, "ClampToEdge") == 0)
        textureWrap = GL_CLAMP_TO_EDGE;
    else if (strcmp(filter, "ClampToBorder") == 0)
        textureWrap = GL_CLAMP_TO_BORDER;
    else if (strcmp(filter, "Clamp") == 0)
        textureWrap = GL_CLAMP;
    else if (isInteger(filter))
        textureWrap = fromString<int>(filter);

    return textureWrap;
}

// Load the material with the name 'materialName' from the file 'filename'
MaterialPtr MaterialManagerInterface::getMaterial(const char *filename, const char *materialName)
{
    MaterialInfo info;
    info.filename = filename;
    info.materialName = materialName;
    return FileManager<Material, MaterialInfo>::getAsset(info);
}

// Get the material this element describes
MaterialPtr MaterialManagerInterface::getMaterial(tinyxml2::XMLElement *materialElement)
{
    // If this is element contains a reference to an external XML file, load the material in this file.
    if (materialElement->GetText()) {
        return getMaterial(materialElement->GetText(), materialElement->Attribute("name"));
    }

    // Otherwise, we have a node containing the material itself
    MaterialInfo info = loadMaterialInfo(materialElement);
    return createMaterial(info);

}

MaterialPtr MaterialManagerInterface::loadAsset(MaterialInfo &info)
{
    // Was the material data already parsed?
    if (info.loaded) {
        return createMaterial(info);
    }

    // We load a material of this file for the first time!
    // First, open the document and get the main node of the material list
    XMLDocument doc;
    if (doc.LoadFile(info.filename.c_str()) != 0) {
        Logfile::get()->writeError(std::string() + "loadMaterial: Couldn't open file \"" + info.filename + "\"!");
        return MaterialPtr();
    }
    XMLElement *masterNode = doc.FirstChildElement("MaterialList");
    if (masterNode == NULL) {
        Logfile::get()->writeError("loadMaterial: No \"MaterialList\" node found!");
        return MaterialPtr();
    }

    // Now traverse all materials in the list
    MaterialPtr material;
    for (XMLElement* materialElement = masterNode->FirstChildElement("Material");
            materialElement; materialElement = materialElement->NextSibling()->ToElement()) {
        // Get the material information and check if we found the material to be loaded
        MaterialInfo currMat = loadMaterialInfo(materialElement);
        if (info.materialName == currMat.materialName) {
            material = createMaterial(info);
            info = currMat;
        }
        assetMap[currMat] = std::weak_ptr<Material>();

        if (materialElement->NextSibling() == 0)
            break;
    }
    return getMaterial(masterNode);
}

MaterialInfo MaterialManagerInterface::loadMaterialInfo(tinyxml2::XMLElement *materialElement)
{
    // Read the material data
    MaterialInfo materialInfo;
    materialInfo.loaded = true;
    XMLElement *colorElement = materialElement->FirstChildElement("Color");
    if (colorElement) {
        uint8_t alpha = (colorElement->Attribute("a") ? colorElement->IntAttribute("a") : 255);
        Color color(colorElement->IntAttribute("r"), colorElement->IntAttribute("g"),
                colorElement->IntAttribute("b"), alpha);
        materialInfo.color = color;
    }

    XMLElement *textureFilename = materialElement->FirstChildElement("Texture");
    if (textureFilename) {
        int textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
        int textureMagFilter = GL_LINEAR;
        int textureWrapS = GL_REPEAT;
        int textureWrapT = GL_REPEAT;
        XMLElement *minificationFilterElement = materialElement->FirstChildElement("MinificationFilter");
        if (minificationFilterElement)
            textureMinFilter = minificationFilterFromString(minificationFilterElement->GetText());
        XMLElement *magnificationFilterElement = materialElement->FirstChildElement("MagnificationFilter");
        if (magnificationFilterElement)
            textureMagFilter = magnificationFilterFromString(magnificationFilterElement->GetText());
        XMLElement *wrapSElement = materialElement->FirstChildElement("WrapS");
        if (wrapSElement)
            textureWrapS = textureWrapFromString(wrapSElement->GetText());
        XMLElement *wrapTElement = materialElement->FirstChildElement("WrapT");
        if (wrapTElement)
            textureWrapT = textureWrapFromString(wrapTElement->GetText());

        // Set the data of the material info
        materialInfo.minificationFilter = textureMinFilter;
        materialInfo.magnificationFilter = textureMagFilter;
        materialInfo.textureWrapS = textureWrapS;
        materialInfo.textureWrapT = textureWrapT;

        if (!FileUtils::get()->exists(textureFilename->GetText())) {
            std::string errorString = "ERROR: MaterialManagerInterface::loadMaterialInfo: Could not load texture file!";
            if (textureFilename->GetText()) {
                errorString += std::string() + " File: \"" + textureFilename->GetText() + "\"";
            }
            Logfile::get()->writeError(errorString);
        }

        materialInfo.textureFilename = textureFilename->GetText();
    }

    return materialInfo;
}

MaterialPtr MaterialManagerInterface::createMaterial(const MaterialInfo &info)
{
    MaterialPtr material(new Material);
    material->color = info.color;
    if (!info.textureFilename.empty()) {
        TexturePtr texture(TextureManager->getAsset(info.textureFilename.c_str(),
                TextureSettings(info.minificationFilter, info.magnificationFilter, info.textureWrapS, info.textureWrapT)));
        material->texture = texture;
    }
    return material;
}

}
