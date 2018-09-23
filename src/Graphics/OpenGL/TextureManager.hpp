/*!
 * TextureManager.hpp
 *
 *  Created on: 31.03.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_
#define GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_

#include <Graphics/Texture/TextureManager.hpp>

namespace sgl {

class TextureManagerGL : public TextureManagerInterface
{
public:
    TexturePtr createEmptyTexture(int w, int h, const TextureSettings &settings = TextureSettings());
    TexturePtr createTexture(void *data, int w, int h, const TextureSettings &settings = TextureSettings());
    TexturePtr createEmptyTexture(int w, int h, int d, const TextureSettings &settings = TextureSettings());
    TexturePtr createTexture(void *data, int w, int h, int d, const TextureSettings &settings = TextureSettings());

	//! Only for FBOs!
	TexturePtr createMultisampledTexture(int w, int h, int numSamples);
	//! bitsPerPixel must be 16, 24 or 32
	TexturePtr createDepthTexture(int w, int h, DepthTextureFormat format = DEPTH_COMPONENT16,
			int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR);

protected:
	virtual TexturePtr loadAsset(TextureInfo &textureInfo);
	void createTextureInternal();
};

}

/*! GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_ */
#endif
