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
	TexturePtr createEmptyTexture(int w, int h, TextureSettings settings = TextureSettings());
	TexturePtr createTexture(void *data, int w, int h, TextureSettings settings = TextureSettings());
	TexturePtr createEmptyTexture3D(int w, int h, int d, TextureSettings settings = TextureSettings());
	TexturePtr createTexture3D(void *data, int w, int h, int d, TextureSettings settings = TextureSettings());

	TexturePtr createTextureArray(void *data, int w, int h, int d, TextureSettings settings = TextureSettings());

	//! Only for FBOs!
	TexturePtr createMultisampledTexture(int w, int h, int numSamples);
	//! bitsPerPixel must be 16, 24 or 32
	TexturePtr createDepthTexture(int w, int h, DepthTextureFormat format = DEPTH_COMPONENT16,
			int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR);

	void setNPOTHandling(NPOTHandling npot) { npotHandling = npot; }

protected:
	virtual TexturePtr loadAsset(TextureInfo &textureInfo);
	NPOTHandling npotHandling;
};

}

/*! GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_ */
#endif
