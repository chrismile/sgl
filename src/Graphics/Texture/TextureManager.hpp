/*!
 * TextureManager.hpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph Neuhauser
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

/*! How should non-power-of-two textures be handled by the Texture Manager?
 * a) NPOT_SUPPORTED: Just use the npot textures (OpenGL 2.0+, OpenGL ES 2.0 + NPOT Extension)
 * b) NPOT_ES_SUPPORTED: Downscale textures with texture wrap mode repeat or mirror
 * c) NPOT_UPSCALE: Upscale all NPOT textures to POT textures
 * c) NPOT_DOWNSCALE: Downscale all NPOT textures to POT textures
 */
enum NPOTHandling {
	NPOT_SUPPORTED, NPOT_ES_SUPPORTED, NPOT_UPSCALE, NPOT_DOWNSCALE
};

enum DepthTextureFormat {
	DEPTH_COMPONENT16 = 0x81A5, DEPTH_COMPONENT24 = 0x81A6,
	DEPTH_COMPONENT32 = 0x81A7, DEPTH_COMPONENT32F = 0x8CAC
};

struct TextureSettings {
	TextureSettings() {
		textureMinFilter = GL_LINEAR;
		textureMagFilter = GL_LINEAR;
		textureWrapS = GL_CLAMP_TO_EDGE;
		textureWrapT = GL_CLAMP_TO_EDGE;
		textureWrapR = GL_CLAMP_TO_EDGE;
		anisotropicFilter = false;

		internalFormat = 0x1908; // GL_RGBA
		pixelFormat = 0x1908; // GL_RGBA
		pixelType = 0x1401; // GL_UNSIGNED_BYTE
	}
	TextureSettings(int _textureMinFilter, int _textureMagFilter,
			int _textureWrapS, int _textureWrapT, int _textureWrapR = GL_CLAMP_TO_EDGE) : TextureSettings() {
		textureMinFilter = _textureMinFilter;
		textureMagFilter = _textureMagFilter;
		textureWrapS = _textureWrapS;
		textureWrapT = _textureWrapT;
		textureWrapR = _textureWrapR;
	}

	int textureMinFilter;
	int textureMagFilter;
	int textureWrapS;
	int textureWrapT;
	int textureWrapR;
	bool anisotropicFilter;

	int internalFormat; // OpenGL: Format of data on GPU
	int pixelFormat; // OpenGL: Format of pixel data, e.g. RGB, RGBA, BGRA, Depth, Stencil, ...
	int pixelType; // OpenGL: Type of one pixel data, e.g. Unsigned Byte, Float, ...
};

/*! Use TextureManager the following ways:
 * - Load texture files from your hard-disk using "getAsset"
 * - Create an 32-bit RGBA texture using createTexture
 * - Create an empty texture (e.g. for offscreen rendering) with "createEmptyTexture"
 * - Create an multisampled texture for offscreen rendering with "createMultisampledTexture" */
class TextureManagerInterface : public FileManager<Texture, TextureInfo>
{
public:
	TexturePtr getAsset(const char *filename, TextureSettings settings = TextureSettings());
	virtual TexturePtr createEmptyTexture(int w, int h, TextureSettings settings = TextureSettings())=0;
	virtual TexturePtr createTexture(void *data, int w, int h, TextureSettings settings = TextureSettings())=0;
	virtual TexturePtr createEmptyTexture3D(int w, int h, int d, TextureSettings settings = TextureSettings())=0;
	virtual TexturePtr createTexture3D(void *data, int w, int h, int d, TextureSettings settings = TextureSettings())=0;

	//! Only for FBOs!
	virtual TexturePtr createMultisampledTexture(int w, int h, int numSamples)=0;
	//! bitsPerPixel must be 16, 24 or 32
	virtual TexturePtr createDepthTexture(int w, int h, DepthTextureFormat format = DEPTH_COMPONENT16,
			int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR)=0;

	virtual void setNPOTHandling(NPOTHandling npot)=0;

protected:
	virtual TexturePtr loadAsset(TextureInfo &textureInfo)=0;
};

extern TextureManagerInterface* TextureManager;

}

/*! GRAPHICS_TEXTURE_TEXTUREMANAGER_HPP_ */
#endif
