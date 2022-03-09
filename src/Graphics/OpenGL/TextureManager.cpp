/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <unordered_map>
#include <GL/glew.h>
#include <SDL2/SDL_image.h>

#include <Utils/File/ResourceManager.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/Convert.hpp>
#include <Math/Math.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Texture/Bitmap.hpp>

#include "SystemGL.hpp"
#include "Texture.hpp"
#include "TextureManager.hpp"

namespace sgl {

static const std::unordered_map<int, int> pixelFormatMap = {
        { GL_RED, GL_RED },
        { GL_R8, GL_RED },
        { GL_R8_SNORM, GL_RED },
        { GL_R8UI, GL_RED },
        { GL_R8I, GL_RED },
        { GL_R16, GL_RED },
        { GL_R16_SNORM, GL_RED },
        { GL_R16F, GL_RED },
        { GL_R16UI, GL_RED },
        { GL_R16I, GL_RED },
        { GL_R32F, GL_RED },
        { GL_R32UI, GL_RED },
        { GL_R32I, GL_RED },

        { GL_RG, GL_RG },
        { GL_RG8, GL_RG },
        { GL_RG8_SNORM, GL_RG },
        { GL_RG8UI, GL_RG },
        { GL_RG8I, GL_RG },
        { GL_RG16, GL_RG },
        { GL_RG16_SNORM, GL_RG },
        { GL_RG16F, GL_RG },
        { GL_RG16UI, GL_RG },
        { GL_RG16I, GL_RG },
        { GL_RG32F, GL_RG },
        { GL_RG32UI, GL_RG },
        { GL_RG32I, GL_RG },

        { GL_RGB, GL_RGB },
        { GL_RGB8, GL_RGB },
        { GL_RGB8_SNORM, GL_RGB },
        { GL_RGB8UI, GL_RGB },
        { GL_RGB8I, GL_RGB },
        { GL_RGB16, GL_RGB },
        { GL_RGB16_SNORM, GL_RGB },
        { GL_RGB16F, GL_RGB },
        { GL_RGB16UI, GL_RGB },
        { GL_RGB16I, GL_RGB },
        { GL_RGB32F, GL_RGB },
        { GL_RGB32UI, GL_RGB },
        { GL_RGB32I, GL_RGB },

        { GL_RGBA, GL_RGBA },
        { GL_RGBA8, GL_RGBA },
        { GL_RGBA8_SNORM, GL_RGBA },
        { GL_RGBA8UI, GL_RGBA },
        { GL_RGBA8I, GL_RGBA },
        { GL_RGBA16, GL_RGBA },
        { GL_RGBA16_SNORM, GL_RGBA },
        { GL_RGBA16F, GL_RGBA },
        { GL_RGBA16UI, GL_RGBA },
        { GL_RGBA16I, GL_RGBA },
        { GL_RGBA32F, GL_RGBA },
        { GL_RGBA32UI, GL_RGBA },
        { GL_RGBA32I, GL_RGBA },

        { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT },
        { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT },
        { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT },
        { GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT },
        { GL_DEPTH24_STENCIL8, GL_DEPTH_COMPONENT },
        { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT },
        { GL_DEPTH32F_STENCIL8, GL_DEPTH_COMPONENT },
};

// Some formats (e.g. GL_R8_SNORM -> GL_BYTE) have multiple valid types. This just maps the first suitable type.
static const std::unordered_map<int, int> pixelTypeMap = {
        { GL_RED, GL_UNSIGNED_BYTE },
        { GL_R8, GL_UNSIGNED_BYTE },
        { GL_R8_SNORM, GL_BYTE },
        { GL_R8UI, GL_UNSIGNED_BYTE },
        { GL_R8I, GL_BYTE },
        { GL_R16, GL_FLOAT },
        { GL_R16_SNORM, GL_FLOAT },
        { GL_R16F, GL_FLOAT },
        { GL_R16UI, GL_UNSIGNED_SHORT },
        { GL_R16I, GL_SHORT },
        { GL_R32F, GL_FLOAT },
        { GL_R32UI, GL_UNSIGNED_INT },
        { GL_R32I, GL_INT },

        { GL_RG, GL_UNSIGNED_BYTE },
        { GL_RG8, GL_UNSIGNED_BYTE },
        { GL_RG8_SNORM, GL_BYTE },
        { GL_RG8UI, GL_UNSIGNED_BYTE },
        { GL_RG8I, GL_BYTE },
        { GL_RG16, GL_FLOAT },
        { GL_RG16_SNORM, GL_FLOAT },
        { GL_RG16F, GL_FLOAT },
        { GL_RG16UI, GL_UNSIGNED_SHORT },
        { GL_RG16I, GL_SHORT },
        { GL_RG32F, GL_FLOAT },
        { GL_RG32UI, GL_UNSIGNED_INT },
        { GL_RG32I, GL_INT },

        { GL_RGB, GL_UNSIGNED_BYTE },
        { GL_RGB8, GL_UNSIGNED_BYTE },
        { GL_RGB8_SNORM, GL_BYTE },
        { GL_RGB8UI, GL_UNSIGNED_BYTE },
        { GL_RGB8I, GL_BYTE },
        { GL_RGB16, GL_FLOAT },
        { GL_RGB16_SNORM, GL_FLOAT },
        { GL_RGB16F, GL_FLOAT },
        { GL_RGB16UI, GL_UNSIGNED_SHORT },
        { GL_RGB16I, GL_SHORT },
        { GL_RGB32F, GL_FLOAT },
        { GL_RGB32UI, GL_UNSIGNED_INT },
        { GL_RGB32I, GL_INT },

        { GL_RGBA, GL_UNSIGNED_BYTE },
        { GL_RGBA8, GL_UNSIGNED_BYTE },
        { GL_RGBA8_SNORM, GL_BYTE },
        { GL_RGBA8UI, GL_UNSIGNED_BYTE },
        { GL_RGBA8I, GL_BYTE },
        { GL_RGBA16, GL_FLOAT },
        { GL_RGBA16_SNORM, GL_FLOAT },
        { GL_RGBA16F, GL_FLOAT },
        { GL_RGBA16UI, GL_UNSIGNED_SHORT },
        { GL_RGBA16I, GL_SHORT },
        { GL_RGBA32F, GL_FLOAT },
        { GL_RGBA32UI, GL_UNSIGNED_INT },
        { GL_RGBA32I, GL_INT },

        { GL_DEPTH_COMPONENT, GL_FLOAT },
        { GL_DEPTH_COMPONENT16, GL_FLOAT },
        { GL_DEPTH_COMPONENT24, GL_FLOAT },
        { GL_DEPTH_COMPONENT32, GL_FLOAT },
        { GL_DEPTH24_STENCIL8, GL_FLOAT },
        { GL_DEPTH_COMPONENT32F, GL_FLOAT },
        { GL_DEPTH32F_STENCIL8, GL_FLOAT },
};

TexturePtr TextureManagerGL::createEmptyTexture(int width, const TextureSettings& settings) {
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

    glTexImage1D(
            TEXTURE_TYPE, 0, settings.internalFormat, width, 0,
            pixelFormatMap.find(settings.internalFormat)->second,
            pixelTypeMap.find(settings.internalFormat)->second, nullptr);

    return TexturePtr(new TextureGL(oglTexture, width, settings, 0));
}

TexturePtr TextureManagerGL::createTexture(
        void* data, int width, const PixelFormat& pixelFormat, const TextureSettings& settings) {
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

    glTexImage1D(
            TEXTURE_TYPE, 0, settings.internalFormat, width, 0,
            pixelFormat.pixelFormat, pixelFormat.pixelType, data);

    return TexturePtr(new TextureGL(oglTexture, width, settings, 0));
}


TexturePtr TextureManagerGL::loadAsset(TextureInfo& textureInfo) {
    ResourceBufferPtr resource = ResourceManager::get()->getFileSync(textureInfo.filename.c_str());
    if (!resource) {
        Logfile::get()->writeError(std::string() + "TextureManagerGL::loadFromFile: Unable to load image file "
                + "\"" + textureInfo.filename + "\"!");
        return TexturePtr();
    }

    SDL_Surface* image = 0;
    SDL_RWops* rwops = SDL_RWFromMem(resource->getBuffer(), int(resource->getBufferSize()));
    if (rwops == nullptr) {
        Logfile::get()->writeError(std::string() + "TextureManagerGL::loadFromFile: SDL_RWFromMem failed (file: \""
                + textureInfo.filename + "\")! SDL Error: " + "\"" + SDL_GetError() + "\"");
        return TexturePtr();
    }

    image = IMG_Load_RW(rwops, 0);
    rwops->close(rwops);

    // Was loading the file with SDL_image successful?
    if (image == nullptr) {
        Logfile::get()->writeError(std::string() + "TextureManagerGL::loadFromFile: IMG_Load_RW failed (file: \""
                + textureInfo.filename + "\")! SDL Error: " + "\"" + SDL_GetError() + "\"");
        return TexturePtr();
    }

    GLuint oglTexture;
    SDL_Surface* sdlTexture;
    GLint format = GL_RGBA;
    int w = 0, h = 0;

    sdlTexture = image;

    switch (image->format->BitsPerPixel) {
    case 24:
        if (textureInfo.sRGB) {
            format = GL_SRGB;
        } else {
            format = GL_RGB;
        }
        break;
    case 32:
        if (textureInfo.sRGB) {
            format = GL_SRGB_ALPHA;
        } else {
            format = GL_RGBA;
        }
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
        uint8_t* pixels = (uint8_t*)sdlTexture->pixels;
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



TexturePtr TextureManagerGL::createEmptyTexture(int width, int height, const TextureSettings& settings) {
    return createEmptyTexture(width, height, 0, settings);
}

TexturePtr TextureManagerGL::createTexture(
        void* data, int width, int height, const PixelFormat& pixelFormat, const TextureSettings& settings) {
    return createTexture(data, width, height, 0, pixelFormat, settings);
}


TexturePtr TextureManagerGL::createEmptyTexture(int width, int height, int depth, const TextureSettings& settings) {
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
        glTexImage2D(
                TEXTURE_TYPE, 0, settings.internalFormat, width, height, 0,
                pixelFormatMap.find(settings.internalFormat)->second,
                pixelTypeMap.find(settings.internalFormat)->second, nullptr);
    } else {
        glTexImage3D(
                TEXTURE_TYPE, 0, settings.internalFormat, width, height, depth, 0,
                pixelFormatMap.find(settings.internalFormat)->second,
                pixelTypeMap.find(settings.internalFormat)->second, nullptr);
    }

    return TexturePtr(new TextureGL(oglTexture, width, height, settings, 0));
}

TexturePtr TextureManagerGL::createTexture(
        void* data, int width, int height, int depth, const PixelFormat& pixelFormat, const TextureSettings& settings) {
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
        glTexImage2D(
                TEXTURE_TYPE, 0, settings.internalFormat, width, height, 0,
                pixelFormat.pixelFormat, pixelFormat.pixelType, data);
    } else {
        glTexImage3D(
                TEXTURE_TYPE, 0, settings.internalFormat, width, height, depth, 0,
                pixelFormat.pixelFormat, pixelFormat.pixelType, data);
    }

    return TexturePtr(new TextureGL(oglTexture, width, height, settings, 0));
}


TexturePtr TextureManagerGL::createMultisampledTexture(
        int width, int height, int numSamples, int internalFormat, bool fixedSampleLocations) {
    // https://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2DMultisample.xml
    //   -> "glTexImage2DMultisample is available only if the GL version is 3.2 or greater."
    if (!SystemGL::get()->openglVersionMinimum(3, 2) || SystemGL::get()->getMaximumTextureSamples() <= 0) {
        std::string infoString = "INFO: TextureManagerGL::createMultisampledTexture: Multisampling not supported.";
        std::cout << infoString << std::endl;
        Logfile::get()->write(infoString, BLUE);

        // Create a normal texture as a fallback
        return createEmptyTexture(width, height);
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

    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, internalFormat, width, height, fixedSampleLocations);

    TextureSettings settings(TEXTURE_2D_MULTISAMPLE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    settings.internalFormat = internalFormat;
    return TexturePtr(new TextureGL(oglTexture, width, height, settings, numSamples));
}

TexturePtr TextureManagerGL::createDepthTexture(
        int width, int height, DepthTextureFormat format,
        int textureMinFilter /* = GL_LINEAR */, int textureMagFilter /* = GL_LINEAR */) {
    GLuint oglTexture = 0;
    glGenTextures(1, &oglTexture);
    glBindTexture(GL_TEXTURE_2D, oglTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    TextureSettings settings(GL_TEXTURE_2D, textureMinFilter, textureMagFilter, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    settings.internalFormat = int(format);
    return TexturePtr(new TextureGL(oglTexture, width, height, settings));
}

TexturePtr TextureManagerGL::createDepthStencilTexture(
        int width, int height, DepthStencilTextureFormat format, int textureMinFilter, int textureMagFilter) {
    GLuint oglTexture = 0;
    glGenTextures(1, &oglTexture);
    glBindTexture(GL_TEXTURE_2D, oglTexture);

    glTexStorage2D(GL_TEXTURE_2D, 1, GLenum(format), width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    TextureSettings settings(GL_TEXTURE_2D, textureMinFilter, textureMagFilter, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    settings.internalFormat = int(format);
    return TexturePtr(new TextureGL(oglTexture, width, height, settings));
}


TexturePtr TextureManagerGL::createTextureStorage(int width, const TextureSettings& settings) {
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

    glTexStorage1D(TEXTURE_TYPE, 1, settings.internalFormat, width);

    return TexturePtr(new TextureGL(oglTexture, width, settings, 0));

}

TexturePtr TextureManagerGL::createTextureStorage(int width, int height, const TextureSettings& settings) {
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

    glTexStorage2D(TEXTURE_TYPE, 1, settings.internalFormat, width, height);

    return TexturePtr(new TextureGL(oglTexture, width, height, settings, 0));
}

TexturePtr TextureManagerGL::createTextureStorage(int width, int height, int depth, const TextureSettings& settings) {
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

    glTexStorage3D(TEXTURE_TYPE, 1, settings.internalFormat, width, height, depth);

    return TexturePtr(new TextureGL(oglTexture, width, height, settings, 0));
}

}
