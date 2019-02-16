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

enum DepthTextureFormat {
    DEPTH_COMPONENT16 = 0x81A5, DEPTH_COMPONENT24 = 0x81A6,
    DEPTH_COMPONENT32 = 0x81A7, DEPTH_COMPONENT32F = 0x8CAC
};

/*! Use TextureManager the following ways:
 * - Load texture files from your hard-disk using "getAsset"
 * - Create an 32-bit RGBA texture using createTexture
 * - Create an empty texture (e.g. for offscreen rendering) with "createEmptyTexture"
 * - Create an multisampled texture for offscreen rendering with "createMultisampledTexture" */
class TextureManagerInterface : public FileManager<Texture, TextureInfo>
{
public:
    TexturePtr getAsset(const char *filename, const TextureSettings &settings = TextureSettings());
    virtual TexturePtr createEmptyTexture(int w, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTexture(void *data, int w, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createEmptyTexture(int w, int h, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTexture(void *data, int w, int h, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createEmptyTexture(int w, int h, int d, const TextureSettings &settings = TextureSettings())=0;
    virtual TexturePtr createTexture(void *data, int w, int h, int d, const TextureSettings &settings = TextureSettings())=0;

    //! Only for FBOs!
    virtual TexturePtr createMultisampledTexture(int w, int h, int numSamples)=0;
    virtual TexturePtr createDepthTexture(int w, int h, DepthTextureFormat format = DEPTH_COMPONENT16,
            int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR)=0;

protected:
    virtual TexturePtr loadAsset(TextureInfo &textureInfo)=0;
};

extern TextureManagerInterface* TextureManager;

}

/*! GRAPHICS_TEXTURE_TEXTUREMANAGER_HPP_ */
#endif
