/*!
 * Material.hpp
 *
 *  Created on: 08.04.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_MESH_MATERIAL_HPP_
#define GRAPHICS_MESH_MATERIAL_HPP_

#include <string>
#include <Graphics/Color.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <Utils/File/FileManager.hpp>

namespace tinyxml2 {
class XMLElement;
}

namespace sgl {

class Material
{
public:
	Color color;
	TexturePtr texture;
};

typedef boost::shared_ptr<Material> MaterialPtr;

int minificationFilterFromString(const char *filter);
int magnificationFilterFromString(const char *filter);
int textureWrapFromString(const char *filter);

struct MaterialInfo {
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
extern MaterialManagerInterface *MaterialManager;

}

/*! GRAPHICS_MESH_MATERIAL_HPP_ */
#endif
