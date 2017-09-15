/*
 * TextureManager.cpp
 *
 *  Created on: 31.03.2015
 *      Author: Christoph Neuhauser
 */

#include <GL/glew.h>
#include "TextureManager.hpp"
#include "SystemGL.hpp"
#include "Texture.hpp"
#include <Utils/File/ResourceManager.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/Convert.hpp>
#include <Math/Math.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Texture/Bitmap.hpp>
#include <SDL2/SDL_image.h>

namespace sgl {

TexturePtr TextureManagerGL::loadAsset(TextureInfo &textureInfo)
{
	ResourceBufferPtr resource = ResourceManager::get()->getFileSync(textureInfo.filename.c_str());
	if (!resource) {
		Logfile::get()->writeError(std::string() + "TextureManagerGL::loadFromFile: Unable to load image file "
				+ "\"" + textureInfo.filename + "\"!");
		return TexturePtr();
	}

	SDL_Surface *image = 0;
	SDL_RWops *rwops = SDL_RWFromMem(resource->getBuffer(), resource->getBufferSize());
	if (rwops == NULL) {
		Logfile::get()->writeError(std::string() + "TextureManagerGL::loadFromFile: SDL_RWFromMem failed (file: \""
				+ textureInfo.filename + "\")! SDL Error: " + "\"" + SDL_GetError() + "\"");
		return TexturePtr();
	}

	image = IMG_Load_RW(rwops, 0);
	rwops->close(rwops);

	// Was loading the file with SDL_image successful?
	if (image == NULL) {
		Logfile::get()->writeError(std::string() + "TextureManagerGL::loadFromFile: IMG_Load_RW failed (file: \""
				+ textureInfo.filename + "\")! SDL Error: " + "\"" + SDL_GetError() + "\"");
		return TexturePtr();
	}

	GLuint oglTexture;
	SDL_Surface *sdlTexture;
	GLint format = GL_RGBA;
	int w = 0, h = 0, bpp = 32;

	sdlTexture = image;

	switch (image->format->BitsPerPixel) {
	case 24:
		bpp = 24;
		format  = GL_RGB;
		break;
	case 32:
		bpp = 32;
		format  = GL_RGBA;
		break;
	default:
		Uint32 video_flags = SDL_SWSURFACE;
		sdlTexture = SDL_CreateRGBSurface(
				video_flags,
				image->w, image->h,
				32,
	#if SDL_BYTEORDER == SDL_LIL_ENDIAN // OpenGL RGBA masks
				0x000000FF,
				0x0000FF00,
				0x00FF0000,
				0xFF000000
	#else
				0xFF000000,
				0x00FF0000,
				0x0000FF00,
				0x000000FF
	#endif
		);
		if (sdlTexture == NULL) {
			Logfile::get()->writeError("ERROR: TextureManagerGL::loadFromFile: Couldn't allocate texture!");
		}

		SDL_FillRect(sdlTexture, NULL, SDL_MapRGBA(image->format, 0, 0, 0, 0));

		// Copy the surface into the GL texture image
		SDL_BlitSurface(image, NULL, sdlTexture, NULL);
	}


	// Premultiplied alpha
	if (SystemGL::get()->isPremulAphaEnabled() && sdlTexture->format->BitsPerPixel == 32) {
		uint8_t *pixels = (uint8_t*)sdlTexture->pixels;
		for (int y = 0; y < sdlTexture->h; ++y) {
			for (int x = 0; x < sdlTexture->w; ++x) {
				int alpha = pixels[(x+y*sdlTexture->w)*4+3];
				pixels[(x+y*sdlTexture->w)*4] = (uint8_t)((int)pixels[(x+y*sdlTexture->w)*4] * alpha / 255);
				pixels[(x+y*sdlTexture->w)*4+1] = (uint8_t)((int)pixels[(x+y*sdlTexture->w)*4+1] * alpha / 255);
				pixels[(x+y*sdlTexture->w)*4+2] = (uint8_t)((int)pixels[(x+y*sdlTexture->w)*4+2] * alpha / 255);
			}
		}
	}


	// Create an OpenGL texture for the image
	glGenTextures(1, &oglTexture);
	glBindTexture(GL_TEXTURE_2D, oglTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureInfo.magnificationFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureInfo.minificationFilter);
	if (textureInfo.minificationFilter == GL_LINEAR_MIPMAP_LINEAR
			|| textureInfo.minificationFilter == GL_NEAREST_MIPMAP_NEAREST
			|| textureInfo.minificationFilter == GL_NEAREST_MIPMAP_LINEAR
			|| textureInfo.minificationFilter == GL_LINEAR_MIPMAP_NEAREST) {
		if (!SystemGL::get()->openglVersionMinimum(3)) {
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		}
	} else if (textureInfo.anisotropicFilter) {
		float maxAnisotropy = SystemGL::get()->getMaximumAnisotropy();
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	if (npotHandling == NPOT_SUPPORTED
			|| (npotHandling == NPOT_ES_SUPPORTED && textureInfo.textureWrapS == GL_CLAMP_TO_EDGE
					 && textureInfo.textureWrapT == GL_CLAMP_TO_EDGE)
			|| (isPowerOfTwo(image->w) && isPowerOfTwo(image->h))) {
		w = image->w;
		h = image->h;
	} else if (npotHandling == NPOT_UPSCALE) {
		w = nextPowerOfTwo(image->w);
		h = nextPowerOfTwo(image->h);
		BitmapPtr bitmap(new Bitmap);
		bitmap->fromMemory(0, image->w, image->h, 32);
		bitmap = bitmap->resizeBiCubic(w, h);
	} else {
		w = lastPowerOfTwo(image->w);
		h = lastPowerOfTwo(image->h);
		BitmapPtr bitmap(new Bitmap);
		bitmap->fromMemory(0, image->w, image->h, 32);
		bitmap = bitmap->resizeBiCubic(w, h);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureInfo.textureWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureInfo.textureWrapT);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, sdlTexture->pixels);

	if (SystemGL::get()->openglVersionMinimum(3)) {
		std::cout << "IMG: " << w << ", " << h << std::endl;
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	TexturePtr texture = TexturePtr(new TextureGL(oglTexture, w, h, bpp, textureInfo.minificationFilter,
			textureInfo.magnificationFilter, textureInfo.textureWrapS, textureInfo.textureWrapT, 0));

	if (sdlTexture != image) {
		SDL_FreeSurface(sdlTexture);
	}
	SDL_FreeSurface(image);

	return texture;

}

TexturePtr TextureManagerGL::createEmptyTexture(int width, int height,
		int textureMinFilter /* = GL_LINEAR */, int textureMagFilter /* = GL_LINEAR */,
		int textureWrapS /* = GL_REPEAT */, int textureWrapT /* = GL_REPEAT */,
		bool anisotropicFilter /* = false */)
{
	GLuint oglTexture;
	// Create an OpenGL texture for the image
	glGenTextures(1, &oglTexture);
	glBindTexture(GL_TEXTURE_2D, oglTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMagFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMinFilter);
	if (textureMinFilter == GL_LINEAR_MIPMAP_LINEAR
			|| textureMinFilter == GL_NEAREST_MIPMAP_NEAREST
			|| textureMinFilter == GL_NEAREST_MIPMAP_LINEAR
			|| textureMinFilter == GL_LINEAR_MIPMAP_NEAREST) {
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	} else if (anisotropicFilter) {
		float maxAnisotropy = SystemGL::get()->getMaximumAnisotropy();
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapT);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA,
			width, height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			NULL);

	return TexturePtr(new TextureGL(oglTexture, width, height, 32, textureMinFilter,
			textureMagFilter, textureWrapS, textureWrapT, 0));
}

TexturePtr TextureManagerGL::createTexture(void *data, int width, int height,
		int textureMinFilter /* = GL_LINEAR */, int textureMagFilter /* = GL_LINEAR */,
		int textureWrapS /* = GL_REPEAT */, int textureWrapT /* = GL_REPEAT */,
		bool anisotropicFilter /* = false */)
{
	GLuint oglTexture;
	// Create an OpenGL texture for the image
	glGenTextures(1, &oglTexture);
	glBindTexture(GL_TEXTURE_2D, oglTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMagFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMinFilter);
	if (textureMinFilter == GL_LINEAR_MIPMAP_LINEAR
			|| textureMinFilter == GL_NEAREST_MIPMAP_NEAREST
			|| textureMinFilter == GL_NEAREST_MIPMAP_LINEAR
			|| textureMinFilter == GL_LINEAR_MIPMAP_NEAREST) {
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	} else if (anisotropicFilter) {
		float maxAnisotropy = SystemGL::get()->getMaximumAnisotropy();
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapT);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA,
			width, height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			data);

	return TexturePtr(new TextureGL(oglTexture, width, height, 32, textureMinFilter,
			textureMagFilter, textureWrapS, textureWrapT, 0));
}

TexturePtr TextureManagerGL::createMultisampledTexture(int w, int h, int numSamples) // Only for FBOs!
{
	// https://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2DMultisample.xml
	//   -> "glTexImage2DMultisample is available only if the GL version is 3.2 or greater."
	if (!SystemGL::get()->openglVersionMinimum(3, 2) || SystemGL::get()->getMaximumTextureSamples() <= 0) {
		std::string infoString = "INFO: TextureManagerGL::createMultisampledTexture: Multisampling not supported.";
		std::cout << infoString << std::endl;
		Logfile::get()->write(infoString, BLUE);

		// Create a normal texture as a fallback
		return createEmptyTexture(w, h);
	}

	// Make sure that numSamples is supported.
	if (numSamples > SystemGL::get()->getMaximumTextureSamples()) {
		std::string infoString = std::string() + "INFO: TextureManagerGL::createMultisampledTexture: numSamples ("
				+ toString(numSamples) + ") > SystemSettings::get()->getMaximumTextureSamples() ("
				+ toString(SystemGL::get()->getMaximumTextureSamples()) + ")!";
		std::cout << infoString << std::endl;
		Logfile::get()->write(infoString, BLUE);

		numSamples = SystemGL::get()->getMaximumTextureSamples();
	}

	GLuint oglTexture = 0;
	glGenTextures(1, &oglTexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, oglTexture);

	/*glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, textureMagFilter);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, textureMinFilter);
	if (textureMinFilter == GL_LINEAR_MIPMAP_LINEAR
			|| textureMinFilter == GL_NEAREST_MIPMAP_NEAREST
			|| textureMinFilter == GL_NEAREST_MIPMAP_LINEAR
			|| textureMinFilter == GL_LINEAR_MIPMAP_NEAREST)
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_GENERATE_MIPMAP, GL_TRUE);

	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, textureWrapS);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, textureWrapT);*/

	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGBA8, w, h, false);

	return TexturePtr(new TextureGL(oglTexture, w, h, 32, GL_NEAREST,
			GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, numSamples));
}

}
