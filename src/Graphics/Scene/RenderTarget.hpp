/*!
 * RenderTarget.hpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_GRAPHICS_SCENE_RENDERTARGET_HPP_
#define SRC_GRAPHICS_SCENE_RENDERTARGET_HPP_

#include <memory>
#include <Graphics/Buffers/FBO.hpp>

namespace sgl {

class FramebufferObject;
typedef std::shared_ptr<FramebufferObject> FramebufferObjectPtr;
class RenderTarget;
typedef std::shared_ptr<RenderTarget> RenderTargetPtr;

class RenderTarget
{
public:
    void bindFramebufferObject(FramebufferObjectPtr _framebuffer);
    void bindWindowFramebuffer();
    FramebufferObjectPtr getFramebufferObject();
    void bindRenderTarget();
    int getWidth();
    int getHeight();

private:
    FramebufferObjectPtr framebuffer;
};

}

/*! SRC_GRAPHICS_SCENE_RENDERTARGET_HPP_ */
#endif
