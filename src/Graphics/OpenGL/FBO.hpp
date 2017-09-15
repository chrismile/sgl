/*!
 * FBO.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_OPENGL_FBO_HPP_
#define GRAPHICS_OPENGL_FBO_HPP_

#include <Graphics/Buffers/FBO.hpp>
#include <Graphics/Buffers/RBO.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <map>
#include <list>

namespace sgl {

/*! Note: https://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2DMultisample.xml
 *   -> "glTexImage2DMultisample is available only if the GL version is 3.2 or greater."
 * You can't use multisampled textures on systems with GL < 3.2! */
class DLL_OBJECT FramebufferObjectGL : public FramebufferObject
{
public:
	FramebufferObjectGL();
	~FramebufferObjectGL();
	virtual bool bind2DTexture(TexturePtr texture, FramebufferTexture fboTex = COLOR_ATTACHMENT);
	virtual bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, RenderbufferType rboType = DEPTH24_STENCIL8);
	virtual int getWidth() { return width; }
	virtual int getHeight() { return height; }
	virtual unsigned int _bindInternal();
	virtual unsigned int getID() { return id; }

private:
	virtual bool checkStatus();
	unsigned int id;
	std::map<FramebufferTexture, TexturePtr> textures;
	std::map<RenderbufferType, RenderbufferObjectPtr> rbos;
	int width, height;
};

class DLL_OBJECT FramebufferObjectGL2 : public FramebufferObjectGL
{
public:
	FramebufferObjectGL2();
	~FramebufferObjectGL2();
	virtual bool bind2DTexture(TexturePtr texture, FramebufferTexture rboTex = COLOR_ATTACHMENT);
	virtual bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, RenderbufferType rboType = DEPTH24_STENCIL8);
	virtual unsigned int _bindInternal();

private:
	bool checkStatus();
	unsigned int id;
	std::map<FramebufferTexture, TexturePtr> textures;
	std::map<RenderbufferType, RenderbufferObjectPtr> rbos;
	int width, height;
};

}

/*! GRAPHICS_OPENGL_FBO_HPP_ */
#endif
