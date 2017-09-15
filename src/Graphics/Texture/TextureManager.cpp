/*
 * TextureManager.cpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph Neuhauser
 */

#include "TextureManager.hpp"

namespace sgl {

TexturePtr TextureManagerInterface::getAsset(const char *filename, int minificationFilter, int magnificationFilter,
		int textureWrapS, int textureWrapT, bool anisotropicFilter /* = false */)
{
	TextureInfo info;
	info.filename = filename;
	info.minificationFilter = minificationFilter;
	info.magnificationFilter = magnificationFilter;
	info.textureWrapS = textureWrapS;
	info.textureWrapT = textureWrapT;
	info.anisotropicFilter = anisotropicFilter;
	return FileManager<Texture, TextureInfo>::getAsset(info);
}

}
