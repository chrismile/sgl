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
	TexturePtr createEmptyTexture(int w, int h,
			int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR,
			int textureWrapS = GL_REPEAT, int textureWrapT = GL_REPEAT, bool anisotropicFilter = false);
	TexturePtr createTexture(void *data, int w, int h,
			int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR,
			int textureWrapS = GL_REPEAT, int textureWrapT = GL_REPEAT, bool anisotropicFilter = false);

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