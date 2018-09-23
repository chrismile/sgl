/*
 * TextureManager.cpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph Neuhauser
 */

#include "TextureManager.hpp"

namespace sgl {

TexturePtr TextureManagerInterface::getAsset(const char *filename, const TextureSettings &settings)
{
	TextureInfo info;
	info.filename = filename;
	info.minificationFilter = settings.textureMinFilter;
	info.magnificationFilter = settings.textureMagFilter;
	info.textureWrapS = settings.textureWrapS;
	info.textureWrapT = settings.textureWrapT;
	info.anisotropicFilter = settings.anisotropicFilter;
	return FileManager<Texture, TextureInfo>::getAsset(info);
}

}
