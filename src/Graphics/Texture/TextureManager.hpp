/*
 * TextureManager.hpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_TEXTURE_TEXTUREMANAGER_HPP_
#define GRAPHICS_TEXTURE_TEXTUREMANAGER_HPP_

#include <Utils/File/FileManager.hpp>
#include "Texture.hpp"

namespace sgl {

struct TextureInfo {
	TextureInfo() : minificationFilter(0), magnificationFilter(0),
			textureWrapS(0), textureWrapT(0), anisotropicFilter(false) {}
	std::string filename;
	int minificationFilter, magnificationFilter, textureWrapS, textureWrapT;
	bool anisotropicFilter;
	bool operator <(const TextureInfo &rhs) const {
		return filename < rhs.filename;
	}
};

/* How should non-power-of-two textures be handled by the Texture Manager?
 * a) NPOT_SUPPORTED: Just use the npot textures (OpenGL 2.0+, OpenGL ES 2.0 + NPOT Extension)
 * b) NPOT_ES_SUPPORTED: Downscale textures with texture wrap mode repeat or mirror
 * c) NPOT_UPSCALE: Upscale all NPOT textures to POT textures
 * c) NPOT_DOWNSCALE: Downscale all NPOT textures to POT textures
 */
enum NPOTHandling {
	NPOT_SUPPORTED, NPOT_ES_SUPPORTED, NPOT_UPSCALE, NPOT_DOWNSCALE
};


// Use TextureManager the following ways:
// - Load texture files from your hard-disk using "getAsset"
// - Create an 32-bit RGBA texture using createTexture
// - Create an empty texture (e.g. for offscreen rendering) with "createEmptyTexture"
// - Create an multisampled texture for offscreen rendering with "createMultisampledTexture"
class TextureManagerInterface : public FileManager<Texture, TextureInfo>
{
public:
	TexturePtr getAsset(const char *filename, int minificationFilter, int magnificationFilter,
			int textureWrapS, int textureWrapT, bool anisotropicFilter = false);
	virtual TexturePtr createEmptyTexture(int w, int h,
			int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR,
			int textureWrapS = GL_REPEAT, int textureWrapT = GL_REPEAT, bool anisotropicFilter = false)=0;
	virtual TexturePtr createTexture(void *data, int w, int h,
			int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR,
			int textureWrapS = GL_REPEAT, int textureWrapT = GL_REPEAT, bool anisotropicFilter = false)=0;
	virtual TexturePtr createMultisampledTexture(int w, int h, int numSamples)=0; // Only for FBOs!
	virtual void setNPOTHandling(NPOTHandling npot)=0;

protected:
	virtual TexturePtr loadAsset(TextureInfo &textureInfo)=0;
};

extern TextureManagerInterface* TextureManager;

}

#endif /* GRAPHICS_TEXTURE_TEXTUREMANAGER_HPP_ */
