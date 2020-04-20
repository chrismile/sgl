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
    TexturePtr createEmptyTexture(int width, const TextureSettings &settings = TextureSettings());
    TexturePtr createTexture(void *data, int width, const TextureSettings &settings = TextureSettings());
    TexturePtr createEmptyTexture(int width, int height, const TextureSettings &settings = TextureSettings());
    TexturePtr createTexture(void *data, int width, int height, const TextureSettings &settings = TextureSettings());
    TexturePtr createEmptyTexture(int width, int height, int depth, const TextureSettings &settings = TextureSettings());
    TexturePtr createTexture(void *data, int width, int height, int depth, const TextureSettings &settings = TextureSettings());

    /**
     * Uses glTexStorage<x>D for creating an immutable texture.
     */
    TexturePtr createTextureStorage(int width, const TextureSettings &settings = TextureSettings());
    TexturePtr createTextureStorage(int width, int height, const TextureSettings &settings = TextureSettings());
    TexturePtr createTextureStorage(int width, int height, int depth, const TextureSettings &settings = TextureSettings());

    //! Only for FBOs!
    TexturePtr createMultisampledTexture(int width, int height, int numSamples);
    //! bitsPerPixel must be 16, 24 or 32
    TexturePtr createDepthTexture(
            int width, int height, DepthTextureFormat format = DEPTH_COMPONENT16,
            int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR);
    TexturePtr createDepthStencilTexture(
            int width, int height, DepthStencilTextureFormat format = DEPTH24_STENCIL8,
            int textureMinFilter = GL_LINEAR, int textureMagFilter = GL_LINEAR);

protected:
    virtual TexturePtr loadAsset(TextureInfo &textureInfo);
    void createTextureInternal();
};

}

/*! GRAPHICS_OPENGL_TEXTUREMANAGER_HPP_ */
#endif
