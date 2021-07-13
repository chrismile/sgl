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

#include <unordered_map>
#include <GL/glew.h>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Renderer.hpp>
#include "Texture.hpp"

namespace sgl {

TextureGL::TextureGL(unsigned int _texture, int _w, TextureSettings settings, int _samples /* = 0 */)
        : Texture(_w, settings, _samples)
{
    texture = _texture;
}

TextureGL::TextureGL(unsigned int _texture, int _w, int _h, TextureSettings settings, int _samples /* = 0 */)
        : Texture(_w, _h, settings, _samples)
{
    texture = _texture;
}

TextureGL::TextureGL(unsigned int _texture, int _w, int _h, int _d, TextureSettings settings, int _samples /* = 0 */)
        : Texture(_w, _h, _d, settings, _samples)
{
    texture = _texture;
}

TextureGL::~TextureGL()
{
    glDeleteTextures(1, &texture);
}

void TextureGL::uploadPixelData(int width, void *pixelData, PixelFormat pixelFormat)
{
    TexturePtr texturePtr = shared_from_this();
    Renderer->bindTexture(texturePtr);
    glTexSubImage1D(settings.type, 0, 0, width, pixelFormat.pixelFormat, pixelFormat.pixelType, pixelData);
}

void TextureGL::uploadPixelData(int width, int height, void *pixelData, PixelFormat pixelFormat)
{
    TexturePtr texturePtr = shared_from_this();
    Renderer->bindTexture(texturePtr);
    glTexSubImage2D(settings.type, 0, 0, 0, width, height, pixelFormat.pixelFormat, pixelFormat.pixelType, pixelData);
}

void TextureGL::uploadPixelData(int width, int height, int depth, void *pixelData, PixelFormat pixelFormat)
{
    TexturePtr texturePtr = shared_from_this();
    Renderer->bindTexture(texturePtr);
    glTexSubImage3D(settings.type, 0, 0, 0, 0, width, height, depth, pixelFormat.pixelFormat, pixelFormat.pixelType,
            pixelData);
}

TexturePtr TextureGL::createTextureView() {
    GLuint textureViewGL;
    glGenTextures(1, &textureViewGL);
    glTextureView(textureViewGL, GL_TEXTURE_2D, this->texture, settings.internalFormat, 0, 1, 0, 1);
    return TexturePtr(new TextureGL(textureViewGL, w, h, d, settings, samples));
}

static std::unordered_map<VkFormat, GLenum> vulkanFormatToGlSizedFormatMap = {
        { VK_FORMAT_R8_UNORM, GL_R8 },
        { VK_FORMAT_R8_SNORM, GL_R8_SNORM },
        { VK_FORMAT_R8_UINT, GL_R8UI },
        { VK_FORMAT_R8_SINT, GL_R8I },
        { VK_FORMAT_R8_SRGB, GL_R8 },

        { VK_FORMAT_R8G8_UNORM, GL_RG8 },
        { VK_FORMAT_R8G8_SNORM, GL_RG8_SNORM },
        { VK_FORMAT_R8G8_UINT, GL_RG8UI },
        { VK_FORMAT_R8G8_SINT, GL_RG8I },
        { VK_FORMAT_R8G8_SRGB, GL_RG8 },

        { VK_FORMAT_R8G8B8_UNORM, GL_RGB8 },
        { VK_FORMAT_R8G8B8_SNORM, GL_RGB8_SNORM },
        { VK_FORMAT_R8G8B8_UINT, GL_RGB8UI },
        { VK_FORMAT_R8G8B8_SINT, GL_RGB8I },
        { VK_FORMAT_R8G8B8_SRGB, GL_SRGB8 },

        { VK_FORMAT_R8G8B8A8_UNORM, GL_RGBA8 },
        { VK_FORMAT_R8G8B8A8_SNORM, GL_RGBA8_SNORM },
        { VK_FORMAT_R8G8B8A8_UINT, GL_RGBA8UI },
        { VK_FORMAT_R8G8B8A8_SINT, GL_RGBA8I },
        { VK_FORMAT_R8G8B8A8_SRGB, GL_SRGB8_ALPHA8 },

        { VK_FORMAT_R16_UNORM, GL_R16 },
        { VK_FORMAT_R16_SNORM, GL_R16_SNORM },
        { VK_FORMAT_R16_UINT, GL_R16UI },
        { VK_FORMAT_R16_SINT, GL_R16I },
        { VK_FORMAT_R16_SFLOAT, GL_R16F },

        { VK_FORMAT_R16G16_UNORM, GL_RG16 },
        { VK_FORMAT_R16G16_SNORM, GL_RG16_SNORM },
        { VK_FORMAT_R16G16_UINT, GL_RG16UI },
        { VK_FORMAT_R16G16_SINT, GL_RG16I },
        { VK_FORMAT_R16G16_SFLOAT, GL_RG16F },

        { VK_FORMAT_R16G16B16_UNORM, GL_RGB16 },
        { VK_FORMAT_R16G16B16_SNORM, GL_RGB16_SNORM },
        { VK_FORMAT_R16G16B16_UINT, GL_RGB16UI },
        { VK_FORMAT_R16G16B16_SINT, GL_RGB16I },
        { VK_FORMAT_R16G16B16_SFLOAT, GL_RGB16F },

        { VK_FORMAT_R16G16B16A16_UNORM, GL_RGBA16 },
        { VK_FORMAT_R16G16B16A16_SNORM, GL_RGBA16_SNORM },
        { VK_FORMAT_R16G16B16A16_UINT, GL_RGBA16UI },
        { VK_FORMAT_R16G16B16A16_SINT, GL_RGBA16I },
        { VK_FORMAT_R16G16B16A16_SFLOAT, GL_RGBA16F },

        { VK_FORMAT_R32_UINT, GL_R32UI },
        { VK_FORMAT_R32_SINT, GL_R32I },
        { VK_FORMAT_R32_SFLOAT, GL_R32F },

        { VK_FORMAT_R32G32_UINT, GL_RG32UI },
        { VK_FORMAT_R32G32_SINT, GL_RG32I },
        { VK_FORMAT_R32G32_SFLOAT, GL_RG32F },

        { VK_FORMAT_R32G32B32_UINT, GL_RGB32UI },
        { VK_FORMAT_R32G32B32_SINT, GL_RGB32I },
        { VK_FORMAT_R32G32B32_SFLOAT, GL_RGB32F },

        { VK_FORMAT_R32G32B32A32_UINT, GL_RGBA32UI },
        { VK_FORMAT_R32G32B32A32_SINT, GL_RGBA32I },
        { VK_FORMAT_R32G32B32A32_SFLOAT, GL_RGBA32F },

        //{ VK_FORMAT_D16_UNORM, GL_R16 },
        //{ VK_FORMAT_X8_D24_UNORM_PACK32, GL_R32UI },
        //{ VK_FORMAT_D32_SFLOAT, GL_R32F },
        //{ VK_FORMAT_D24_UNORM_S8_UINT, GL_R32UI },

        { VK_FORMAT_D16_UNORM, GL_DEPTH_COMPONENT16 },
        { VK_FORMAT_X8_D24_UNORM_PACK32, GL_DEPTH24_STENCIL8 },
        { VK_FORMAT_D32_SFLOAT, GL_DEPTH_COMPONENT32F },
        //{ VK_FORMAT_D16_UNORM_S8_UINT, UNSUPPORTED },
        { VK_FORMAT_D24_UNORM_S8_UINT, GL_DEPTH24_STENCIL8 },
        { VK_FORMAT_D32_SFLOAT_S8_UINT, GL_DEPTH32F_STENCIL8 },
};

static std::unordered_map<VkImageViewType, GLenum> vulkanImageViewTypeToGlTargetMap = {
        { VK_IMAGE_VIEW_TYPE_1D, GL_TEXTURE_1D },
        { VK_IMAGE_VIEW_TYPE_2D, GL_TEXTURE_2D },
        { VK_IMAGE_VIEW_TYPE_3D, GL_TEXTURE_3D },
        { VK_IMAGE_VIEW_TYPE_CUBE, GL_TEXTURE_CUBE_MAP },
        { VK_IMAGE_VIEW_TYPE_1D_ARRAY, GL_TEXTURE_1D_ARRAY },
        { VK_IMAGE_VIEW_TYPE_2D_ARRAY, GL_TEXTURE_2D_ARRAY },
        { VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY },
};

static std::unordered_map<VkSamplerAddressMode, int> samplerAddressModeVkToTextureWrapGlMap = {
        { VK_SAMPLER_ADDRESS_MODE_REPEAT, GL_REPEAT },
        { VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, GL_MIRRORED_REPEAT },
        { VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE },
        { VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER },
        { VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE, GL_MIRROR_CLAMP_TO_EDGE },
};

GLenum convertFilterVkToFilterGl(VkFilter filterVk, uint32_t mipLevels, VkSamplerMipmapMode samplerMipmapModeVk) {
    if (mipLevels == 0) {
        if (filterVk == VK_FILTER_NEAREST) {
            return GL_NEAREST;
        } else if (filterVk == VK_FILTER_LINEAR) {
            return GL_LINEAR;
        } else {
            Logfile::get()->writeInfo("Warning in convertFilterVkToFilterGl: Unsupported filtering mode.");
            return GL_LINEAR;
        }
    } else {
        if (filterVk == VK_FILTER_NEAREST) {
            if (samplerMipmapModeVk == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
                return GL_NEAREST_MIPMAP_NEAREST;
            } else {
                return GL_NEAREST_MIPMAP_LINEAR;
            }
        } else if (filterVk == VK_FILTER_LINEAR) {
            if (samplerMipmapModeVk == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
                return GL_LINEAR_MIPMAP_NEAREST;
            } else {
                return GL_LINEAR_MIPMAP_LINEAR;
            }
        } else {
            Logfile::get()->writeInfo("Warning in convertFilterVkToFilterGl: Unsupported filtering mode.");
            return GL_LINEAR;
        }
    }
}

TextureGLExternalMemoryVk::TextureGLExternalMemoryVk(vk::TexturePtr& vulkanTexture)
        : TextureGL(0, 0, 0, 0, TextureSettings(), 0) {
    this->vulkanImage = vulkanTexture->getImage();
    if (!vulkanImage->createGlMemoryObject(memoryObject)) {
        Logfile::get()->throwError(
                "GeometryBufferVkExternalMemoryGL::GeometryBufferVkExternalMemoryGL: createGlMemoryObject failed.");
    }

    const vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
    const vk::ImageSamplerSettings& imageSamplerSettings =
            vulkanTexture->getImageSampler()->getImageSamplerSettings();

    this->samples = int(imageSettings.numSamples);
    this->w = int(imageSettings.width);
    if (imageSettings.imageType == VK_IMAGE_TYPE_2D || imageSettings.imageType == VK_IMAGE_TYPE_3D) {
        this->h = int(imageSettings.height);
    }
    if (imageSettings.imageType == VK_IMAGE_TYPE_3D) {
        this->d = int(imageSettings.depth);
    }

    auto itFormat = vulkanFormatToGlSizedFormatMap.find(imageSettings.format);
    if (itFormat == vulkanFormatToGlSizedFormatMap.end()) {
        Logfile::get()->throwError(
                "Error in TextureGLExternalMemoryVk::TextureGLExternalMemoryVk: Unsupported Vulkan image format.");
    }
    GLenum format = itFormat->second;

    auto itTarget = vulkanImageViewTypeToGlTargetMap.find(vulkanTexture->getImageView()->getVkImageViewType());
    if (itTarget == vulkanImageViewTypeToGlTargetMap.end()) {
        Logfile::get()->throwError(
                "Error in TextureGLExternalMemoryVk::TextureGLExternalMemoryVk: Unsupported Vulkan image view "
                "type.");
    }
    GLenum target = itTarget->second;
    if (imageSettings.numSamples != VK_SAMPLE_COUNT_1_BIT) {
        if (target != GL_TEXTURE_2D) {
            Logfile::get()->throwError(
                    "Error in TextureGLExternalMemoryVk::TextureGLExternalMemoryVk: Sample count > 1, but texture "
                    "type is not GL_TEXTURE_2D.");
        }
        target = GL_TEXTURE_2D_MULTISAMPLE;
    }

    settings.type = TextureType(target);
    settings.textureMinFilter = convertFilterVkToFilterGl(
            imageSamplerSettings.minFilter, imageSettings.mipLevels, imageSamplerSettings.mipmapMode);
    settings.textureMagFilter = convertFilterVkToFilterGl(
            imageSamplerSettings.magFilter, imageSettings.mipLevels, imageSamplerSettings.mipmapMode);
    settings.textureWrapS = samplerAddressModeVkToTextureWrapGlMap.find(imageSamplerSettings.addressModeU)->second;
    settings.textureWrapT = samplerAddressModeVkToTextureWrapGlMap.find(imageSamplerSettings.addressModeV)->second;
    settings.textureWrapR = samplerAddressModeVkToTextureWrapGlMap.find(imageSamplerSettings.addressModeW)->second;
    settings.anisotropicFilter = imageSamplerSettings.anisotropyEnable;
    settings.internalFormat = int(format);

    GLint textureTiling = imageSettings.tiling == VK_IMAGE_TILING_LINEAR ? GL_LINEAR_TILING_EXT : GL_OPTIMAL_TILING_EXT;

    glCreateTextures(target, 1, &texture);

    glTextureParameteri(texture, GL_TEXTURE_TILING_EXT, textureTiling);

    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, settings.textureMagFilter);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, settings.textureMinFilter);

    if (settings.anisotropicFilter) {
        glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY_EXT, imageSamplerSettings.maxAnisotropy);
    }

    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, settings.textureWrapS);
    if (imageSettings.imageType == VK_IMAGE_TYPE_2D || imageSettings.imageType == VK_IMAGE_TYPE_3D) {
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, settings.textureWrapT);
    }
    if (imageSettings.imageType == VK_IMAGE_TYPE_3D) {
        glTextureParameteri(texture, GL_TEXTURE_WRAP_R, settings.textureWrapR);
    }

    vulkanImage->createGlMemoryObject(memoryObject);
    if (imageSettings.imageType == VK_IMAGE_TYPE_1D && imageSettings.numSamples == VK_SAMPLE_COUNT_1_BIT) {
        glTextureStorageMem1DEXT(texture, GLsizei(imageSettings.mipLevels), format, w, memoryObject, 0);
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_2D && imageSettings.numSamples == VK_SAMPLE_COUNT_1_BIT) {
        glTextureStorageMem2DEXT(texture, GLsizei(imageSettings.mipLevels), format, w, h, memoryObject, 0);
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_2D && imageSettings.numSamples != VK_SAMPLE_COUNT_1_BIT) {
        glTextureStorageMem2DMultisampleEXT(texture, imageSettings.numSamples, format, w, h, true, memoryObject, 0);
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_3D && imageSettings.numSamples == VK_SAMPLE_COUNT_1_BIT) {
        glTextureStorageMem3DEXT(texture, GLsizei(imageSettings.mipLevels), format, w, h, d, memoryObject, 0);
    }
}

TextureGLExternalMemoryVk::~TextureGLExternalMemoryVk() {
    glDeleteMemoryObjectsEXT(1, &memoryObject);
}

}
