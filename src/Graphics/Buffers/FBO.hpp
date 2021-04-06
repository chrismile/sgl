/*!
 * FBO.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_BUFFERS_FBO_HPP_
#define GRAPHICS_BUFFERS_FBO_HPP_

#include <memory>

#include <Defs.hpp>
#include <Graphics/Buffers/RBO.hpp>
#include <Graphics/Texture/Texture.hpp>

namespace sgl {

enum FramebufferAttachment {
    DEPTH_ATTACHMENT = 0x8D00, STENCIL_ATTACHMENT = 0x8D20,
    DEPTH_STENCIL_ATTACHMENT = 0x821A, COLOR_ATTACHMENT = 0x8CE0,
    COLOR_ATTACHMENT0 = 0x8CE0, COLOR_ATTACHMENT1 = 0x8CE1,
    COLOR_ATTACHMENT2 = 0x8CE2, COLOR_ATTACHMENT3 = 0x8CE3,
    COLOR_ATTACHMENT4 = 0x8CE4, COLOR_ATTACHMENT5 = 0x8CE5,
    COLOR_ATTACHMENT6 = 0x8CE6, COLOR_ATTACHMENT7 = 0x8CE7,
    COLOR_ATTACHMENT8 = 0x8CE8, COLOR_ATTACHMENT9 = 0x8CE9,
    COLOR_ATTACHMENT10 = 0x8CEA, COLOR_ATTACHMENT11 = 0x8CEB,
    COLOR_ATTACHMENT12 = 0x8CEC, COLOR_ATTACHMENT13 = 0x8CED,
    COLOR_ATTACHMENT14 = 0x8CEE, COLOR_ATTACHMENT15 = 0x8CEF
};

/*! A framebuffer object (often called render target in DirectX) is used for offscreen rendering.
 *  You can attach either textures or renderbuffer objects to it. For more infos see
 *     https://www.khronos.org/opengl/wiki/Framebuffer_Object
 *  - A texture can be sampled after rendering. Use it when you want to use post-processing.
 *  - A renderbuffer object is often more optimized for being used as a render target and supports native MSAA.
 */

/*! Note: https://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2DMultisample.xml
 *   -> "glTexImage2DMultisample is available only if the GL version is 3.2 or greater."
 * You can't use multisampled textures on systems with GL < 3.2 */
class DLL_OBJECT FramebufferObject
{
public:
    FramebufferObject() {}
    virtual ~FramebufferObject() {}
    //virtual bool bind2DTexture(TexturePtr texture, FramebufferAttachment attachment = COLOR_ATTACHMENT)=0;
    virtual bool bindTexture(TexturePtr texture, FramebufferAttachment attachment = COLOR_ATTACHMENT)=0;
    virtual bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment = DEPTH_ATTACHMENT)=0;
    //! Width of framebuffer in pixels
    virtual int getWidth()=0;
    //! Height of framebuffer in pixels
    virtual int getHeight()=0;
    //! Only for use in the class Renderer!
    virtual unsigned int _bindInternal()=0;
    //! Only for use in the class Renderer!
    virtual unsigned int getID()=0;
};

typedef std::shared_ptr<FramebufferObject> FramebufferObjectPtr;

}

/*! GRAPHICS_BUFFERS_FBO_HPP_ */
#endif
