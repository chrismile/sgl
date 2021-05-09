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

struct DLL_OBJECT TextureInfo {
    TextureInfo() : minificationFilter(0), magnificationFilter(0),
            textureWrapS(0), textureWrapT(0), anisotropicFilter(false), sRGB(false) {}
    std::string filename;
    int minificationFilter, magnificationFilter, textureWrapS, textureWrapT;
    bool anisotropicFilter;
    bool sRGB;
    bool operator <(const TextureInfo &rhs) const {
        return filename < rhs.filename;
    }
};

enum DepthTextureFormat {
    DEPTH_COMPONENT16 = 0x81A5, DEPTH_COMPONENT24 = 0x81A6,
    DEPTH_COMPONENT32 = 0x81A7, DEPTH_COMPONENT32F = 0x8CAC
};

enum DepthStencilTextureFormat {
    DEPTH24_STENCIL8 = 0x88F0, DEPTH32F_STENCIL8 = 0x8CAD
};

/*! Use TextureManager the following ways:
 * - Load texture files from your hard-disk using "getAsset"
 * - Create an 32-bit RGBA texture using createTexture
 * - Create an empty texture (e.g. for offscreen rendering) with "createEmptyTexture"
 * - Create an multisampled texture for offscreen rendering with "createMultisampledTexture" */
class DLL_OBJECT TextureManagerInterface : public FileManager<Texture, TextureInfo>
{
public:
    TexturePtr getAsset(const char *filename, const TextureSettings &settings = TextureSettings(), bool sRGB = false);
    virtual TexturePtr createEmptyTexture(int w, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTexture(void *data, int w, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createEmptyTexture(int w, int h, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTexture(void *data, int w, int h, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createEmptyTexture(int w, int h, int d, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTexture(void *data, int w, int h, int d, const TextureSettings &settings = TextureSettings())=0;

    /**
     * Uses glTexStorage<x>D for creating an immutable texture.
     */
    virtual TexturePtr createTextureStorage(int width, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTextureStorage(int width, int height, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTextureStorage(int width, int height, int depth, const TextureSettings &settings = TextureSettings())=0;

    //! Only for FBOs!
    virtual TexturePtr createMultisampledTexture(
            int width, int height, int numSamples,
            int internalFormat = 0x8058 /*GL_RGBA8*/, bool fixedSampleLocations = false)=0;
    virtual TexturePtr createDepthTexture(int w, int h, DepthTextureFormat format = DEPTH_COMPONENT16,
            int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR)=0;
    virtual TexturePtr createDepthStencilTexture(
            int width, int height, DepthStencilTextureFormat format = DEPTH24_STENCIL8,
            int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR)=0;

protected:
    virtual TexturePtr loadAsset(TextureInfo &textureInfo)=0;
};

DLL_OBJECT extern TextureManagerInterface* TextureManager;

}

/*! GRAPHICS_TEXTURE_TEXTUREMANAGER_HPP_ */
#endif
