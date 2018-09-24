/*!
 * FBO.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_FBO_HPP_
#define GRAPHICS_OPENGL_FBO_HPP_

#include <GL/glew.h>

#include <Graphics/Buffers/FBO.hpp>
#include <Graphics/Buffers/RBO.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <map>
#include <list>
#include <vector>

namespace sgl {

/*! Note: https://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2DMultisample.xml
 *   -> "glTexImage2DMultisample is available only if the GL version is 3.2 or greater."
 * You can't use multisampled textures on systems with GL < 3.2! */
class DLL_OBJECT FramebufferObjectGL : public FramebufferObject
{
public:
	FramebufferObjectGL();
	~FramebufferObjectGL();
	virtual bool bindTexture(TexturePtr texture, FramebufferAttachment attachment = COLOR_ATTACHMENT);
	virtual bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment = DEPTH_ATTACHMENT);
	virtual int getWidth() { return width; }
	virtual int getHeight() { return height; }
	virtual unsigned int _bindInternal();
	virtual unsigned int getID() { return id; }

protected:
	virtual bool checkStatus();
	unsigned int id;
	std::map<FramebufferAttachment, TexturePtr> textures;
	std::map<FramebufferAttachment, RenderbufferObjectPtr> rbos;
	std::vector<GLuint> colorAttachments;
	int width, height;
	bool hasColorAttachment;
};

class DLL_OBJECT FramebufferObjectGL2 : public FramebufferObjectGL
{
public:
	FramebufferObjectGL2();
	~FramebufferObjectGL2();
	virtual bool bindTexture(TexturePtr texture, FramebufferAttachment attachment = COLOR_ATTACHMENT);
	virtual bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment = DEPTH_ATTACHMENT);
	virtual unsigned int _bindInternal();

protected:
	bool checkStatus();
};

}

/*! GRAPHICS_OPENGL_FBO_HPP_ */
#endif
