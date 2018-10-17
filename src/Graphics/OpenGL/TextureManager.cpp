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

TexturePtr TextureManagerGL::createEmptyTexture(int width, const TextureSettings &settings)
{
    GLuint TEXTURE_TYPE = GL_TEXTURE_1D;

    GLuint oglTexture;
    // Create an OpenGL texture for the image
    glGenTextures(1, &oglTexture);
    glBindTexture(TEXTURE_TYPE, oglTexture);
    if (width % 4 != 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    }

    glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, settings.textureMagFilter);
    glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, settings.textureMinFilter);
    if (settings.textureMinFilter == GL_LINEAR_MIPMAP_LINEAR
        || settings.textureMinFilter == GL_NEAREST_MIPMAP_NEAREST
        || settings.textureMinFilter == GL_NEAREST_MIPMAP_LINEAR
        || settings.textureMinFilter == GL_LINEAR_MIPMAP_NEAREST) {
        glTexParameteri(TEXTURE_TYPE, GL_GENERATE_MIPMAP, GL_TRUE);
    } else if (settings.anisotropicFilter) {
        float maxAnisotropy = SystemGL::get()->getMaximumAnisotropy();
        glTexParameterf(TEXTURE_TYPE, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }

    glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_WRAP_S, settings.textureWrapS);

    glTexImage1D(TEXTURE_TYPE,
                 0,
                 settings.internalFormat,
                 width,
                 0,
                 settings.pixelFormat,
                 settings.pixelType,
                 NULL);

    return TexturePtr(new TextureGL(oglTexture, width, settings, 0));
}

TexturePtr TextureManagerGL::createTexture(void *data, int width, const TextureSettings &settings)
{
	GLuint TEXTURE_TYPE = GL_TEXTURE_1D;

    GLuint oglTexture;
    // Create an OpenGL texture for the image
    glGenTextures(1, &oglTexture);
    glBindTexture(TEXTURE_TYPE, oglTexture);
    if (width % 4 != 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    }

    glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, settings.textureMagFilter);
    glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, settings.textureMinFilter);
    if (settings.textureMinFilter == GL_LINEAR_MIPMAP_LINEAR
        || settings.textureMinFilter == GL_NEAREST_MIPMAP_NEAREST
        || settings.textureMinFilter == GL_NEAREST_MIPMAP_LINEAR
        || settings.textureMinFilter == GL_LINEAR_MIPMAP_NEAREST) {
        glTexParameteri(TEXTURE_TYPE, GL_GENERATE_MIPMAP, GL_TRUE);
    } else if (settings.anisotropicFilter) {
        float maxAnisotropy = SystemGL::get()->getMaximumAnisotropy();
        glTexParameterf(TEXTURE_TYPE, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }

    glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_WRAP_S, settings.textureWrapS);

    glTexImage1D(TEXTURE_TYPE,
                 0,
                 settings.internalFormat,
                 width,
                 0,
                 settings.pixelFormat,
                 settings.pixelType,
                 data);

    return TexturePtr(new TextureGL(oglTexture, width, settings, 0));
}


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

    w = image->w;
    h = image->h;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureInfo.textureWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureInfo.textureWrapT);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, sdlTexture->pixels);

	if (SystemGL::get()->openglVersionMinimum(3)) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	TextureSettings settings(GL_TEXTURE_2D, textureInfo.minificationFilter, textureInfo.magnificationFilter,
			textureInfo.textureWrapS, textureInfo.textureWrapT);
	TexturePtr texture = TexturePtr(new TextureGL(oglTexture, w, h, settings, 0));

	if (sdlTexture != image) {
		SDL_FreeSurface(sdlTexture);
	}
	SDL_FreeSurface(image);

	return texture;

}



TexturePtr TextureManagerGL::createEmptyTexture(int width, int height, const TextureSettings &settings)
{
	return createEmptyTexture(width, height, 0, settings);
}

TexturePtr TextureManagerGL::createTexture(void *data, int width, int height, const TextureSettings &settings)
{
	return createTexture(data, width, height, 0, settings);
}


TexturePtr TextureManagerGL::createEmptyTexture(int width, int height, int depth, const TextureSettings &settings)
{
	GLuint TEXTURE_TYPE = (GLuint)settings.type;

	GLuint oglTexture;
	// Create an OpenGL texture for the image
	glGenTextures(1, &oglTexture);
	glBindTexture(TEXTURE_TYPE, oglTexture);
	if (width % 2 != 0 || height % 2 != 0) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
	}

	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, settings.textureMagFilter);
	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, settings.textureMinFilter);
	if (settings.textureMinFilter == GL_LINEAR_MIPMAP_LINEAR
			|| settings.textureMinFilter == GL_NEAREST_MIPMAP_NEAREST
			|| settings.textureMinFilter == GL_NEAREST_MIPMAP_LINEAR
			|| settings.textureMinFilter == GL_LINEAR_MIPMAP_NEAREST) {
		glTexParameteri(TEXTURE_TYPE, GL_GENERATE_MIPMAP, GL_TRUE);
	} else if (settings.anisotropicFilter) {
		float maxAnisotropy = SystemGL::get()->getMaximumAnisotropy();
		glTexParameterf(TEXTURE_TYPE, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_WRAP_S, settings.textureWrapS);
	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_WRAP_T, settings.textureWrapT);
	if (settings.type == TEXTURE_3D || settings.type == TEXTURE_2D_ARRAY) {
		glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_WRAP_R, settings.textureWrapR);
	}

    if (depth < 1) {
        glTexImage2D(TEXTURE_TYPE,
                     0,
                     settings.internalFormat,
                     width, height,
                     0,
                     settings.pixelFormat,
                     settings.pixelType,
                     NULL);
    } else {
        glTexImage3D(TEXTURE_TYPE,
                     0,
                     settings.internalFormat,
                     width, height, depth,
                     0,
                     settings.pixelFormat,
                     settings.pixelType,
                     NULL);
    }

	return TexturePtr(new TextureGL(oglTexture, width, height, settings, 0));
}

TexturePtr TextureManagerGL::createTexture(void *data, int width, int height, int depth, const TextureSettings &settings)
{
	GLuint TEXTURE_TYPE = (GLuint)settings.type;

	GLuint oglTexture;
	// Create an OpenGL texture for the image
	glGenTextures(1, &oglTexture);
	glBindTexture(TEXTURE_TYPE, oglTexture);
	if (width % 2 != 0 || height % 2 != 0) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
	}

	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, settings.textureMagFilter);
	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, settings.textureMinFilter);
	if (settings.textureMinFilter == GL_LINEAR_MIPMAP_LINEAR
			|| settings.textureMinFilter == GL_NEAREST_MIPMAP_NEAREST
			|| settings.textureMinFilter == GL_NEAREST_MIPMAP_LINEAR
			|| settings.textureMinFilter == GL_LINEAR_MIPMAP_NEAREST) {
		glTexParameteri(TEXTURE_TYPE, GL_GENERATE_MIPMAP, GL_TRUE);
	} else if (settings.anisotropicFilter) {
		float maxAnisotropy = SystemGL::get()->getMaximumAnisotropy();
		glTexParameterf(TEXTURE_TYPE, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_WRAP_S, settings.textureWrapS);
	glTexParameteri(TEXTURE_TYPE, GL_TEXTURE_WRAP_T, settings.textureWrapT);
	if (settings.type == TEXTURE_3D || settings.type == TEXTURE_2D_ARRAY) {
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, settings.textureWrapR);
	}

	if (depth < 1) {
        glTexImage2D(TEXTURE_TYPE,
                     0,
                     settings.internalFormat,
                     width, height,
                     0,
                     settings.pixelFormat,
                     settings.pixelType,
                     data);
	} else {
        glTexImage3D(TEXTURE_TYPE,
                     0,
                     settings.internalFormat,
                     width, height, depth,
                     0,
                     settings.pixelFormat,
                     settings.pixelType,
                     data);
	}

	return TexturePtr(new TextureGL(oglTexture, width, height, settings, 0));
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

	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGBA8, w, h, false);

	TextureSettings settings(TEXTURE_2D_MULTISAMPLE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	settings.internalFormat = GL_RGBA8;
	settings.pixelFormat = GL_RGBA8;
	settings.pixelType = GL_BYTE;
	return TexturePtr(new TextureGL(oglTexture, w, h, settings, numSamples));
}

TexturePtr TextureManagerGL::createDepthTexture(int w, int h, DepthTextureFormat format,
		int textureMinFilter /* = GL_LINEAR */, int textureMagFilter /* = GL_LINEAR */)
{
	GLuint oglTexture = 0;
	glGenTextures(1, &oglTexture);
	glBindTexture(GL_TEXTURE_2D, oglTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, format, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMagFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMinFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	TextureSettings settings(GL_TEXTURE_2D, textureMinFilter, textureMagFilter, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	settings.internalFormat = format;
	settings.pixelFormat = GL_DEPTH_COMPONENT;
	settings.pixelType = GL_FLOAT;
	return TexturePtr(new TextureGL(oglTexture, w, h, settings));
}

}
