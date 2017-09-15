/*
 * FBO.cpp
 *
 *  Created on: 31.03.2015
 *      Author: Christoph
 */

#include <GL/glew.h>
#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Renderer.hpp>
#include "FBO.hpp"
#include "RBO.hpp"
#include "Texture.hpp"

namespace sgl {

FramebufferObjectGL::FramebufferObjectGL() : id(0), width(0), height(0)
{
	glGenFramebuffers(1, &id);
}

FramebufferObjectGL::~FramebufferObjectGL()
{
	textures.clear();
	rbos.clear();
	glDeleteFramebuffers(1, &id);
}

bool FramebufferObjectGL::bind2DTexture(TexturePtr texture, FramebufferTexture fboTex /* = COLOR_ATTACHMENT */)
{
	textures[fboTex] = texture;
	TextureGL *textureGL = (TextureGL*)texture.get();

	int glTexture = textureGL->getTexture();
	width = textureGL->getW();
	height = textureGL->getH();
	int samples = texture->getNumSamples();
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			samples == 0 ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE, glTexture, 0);
	Renderer->bindFBO(Renderer->getFBO(), true);
	bool status = checkStatus();
	return status;
}

bool FramebufferObjectGL::bindRenderbuffer(RenderbufferObjectPtr renderbuffer, RenderbufferType rboType /* = DEPTH24_STENCIL8 */)
{
	rbos[rboType] = renderbuffer;
	RenderbufferObjectGL *rbo = (RenderbufferObjectGL*)renderbuffer.get();
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo->getID());
	Renderer->bindFBO(Renderer->getFBO(), true);
	bool status = checkStatus();
	return status;
}

bool FramebufferObjectGL::checkStatus()
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status) {
		case GL_FRAMEBUFFER_COMPLETE:
			break;
		default:
			Logfile::get()->writeError(std::string() + "Error: FramebufferObject::internBind2DTexture(): "
					+ "Cannot bind texture to FBO! status = " + toString(status));
			return false;
	}
	return true;
}

unsigned int FramebufferObjectGL::_bindInternal()
{
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	return id;
}



// OpenGL 2

FramebufferObjectGL2::FramebufferObjectGL2() : id(0), width(0), height(0)
{
	glGenFramebuffersEXT(1, &id);
}

FramebufferObjectGL2::~FramebufferObjectGL2()
{
	if (Renderer->getFBO().get() == this) {
		Renderer->unbindFBO();
	}

	textures.clear();
	rbos.clear();
	glDeleteFramebuffersEXT(1, &id);
}

bool FramebufferObjectGL2::bind2DTexture(TexturePtr texture, FramebufferTexture rboTex /* = COLOR_ATTACHMENT */)
{
	textures[rboTex] = texture;
	TextureGL *textureGL = (TextureGL*)texture.get();

	int glTexture = textureGL->getTexture();
	width = textureGL->getW();
	height = textureGL->getH();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, glTexture, 0);
	Renderer->bindFBO(Renderer->getFBO(), true);
	bool status = checkStatus();
	return status;
}

bool FramebufferObjectGL2::bindRenderbuffer(RenderbufferObjectPtr renderbuffer, RenderbufferType rboType /* = DEPTH24_STENCIL8 */)
{
	rbos[rboType] = renderbuffer;
	RenderbufferObjectGL *rbo = (RenderbufferObjectGL*)renderbuffer.get();
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo->getID());
	Renderer->bindFBO(Renderer->getFBO(), true);
	bool status = checkStatus();
	return status;
}

bool FramebufferObjectGL2::checkStatus()
{
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		default:
			Logfile::get()->writeError(std::string() + "Error: FramebufferObject::internBind2DTexture(): "
					+ "Cannot bind texture to FBO! status = " + toString(status));
			return false;
	}
	return true;
}

unsigned int FramebufferObjectGL2::_bindInternal()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
	return id;
}

}
