/*!
 * FBO.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_BUFFERS_FBO_HPP_
#define GRAPHICS_BUFFERS_FBO_HPP_

#include <boost/shared_ptr.hpp>
#include <Defs.hpp>
#include <Graphics/Buffers/RBO.hpp>
#include <Graphics/Texture/Texture.hpp>

namespace sgl {

enum FramebufferTexture {
	COLOR_ATTACHMENT, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT
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
	virtual bool bind2DTexture(TexturePtr texture, FramebufferTexture rboTex = COLOR_ATTACHMENT)=0;
	virtual bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, RenderbufferType rboType = DEPTH24_STENCIL8)=0;
	//! Width of framebuffer in pixels
	virtual int getWidth()=0;
	//! Height of framebuffer in pixels
	virtual int getHeight()=0;
	//! Only for use in the class Renderer!
	virtual unsigned int _bindInternal()=0;
	//! Only for use in the class Renderer!
	virtual unsigned int getID()=0;
};

typedef boost::shared_ptr<FramebufferObject> FramebufferObjectPtr;

}

/*! GRAPHICS_BUFFERS_FBO_HPP_ */
#endif
