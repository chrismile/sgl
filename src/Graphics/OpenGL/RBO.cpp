/*
 * RBO.cpp
 *
 *  Created on: 31.03.2015
 *      Author: Christoph Neuhauser
 */

#include <GL/glew.h>
#include "RBO.hpp"

namespace sgl {

RenderbufferObjectGL::RenderbufferObjectGL(int _width, int _height, RenderbufferType rboType, int _samples /* = 0 */)
	: rbo(0), width(_width), height(_height), samples(_samples)
{
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	GLuint type = 0;
	switch (rboType) {
	case DEPTH16:
		type = GL_DEPTH_COMPONENT16;
		break;
	case DEPTH24_STENCIL8:
		type = GL_DEPTH24_STENCIL8;
		break;
	case RGBA8:
		type = GL_RGBA8;
		break;
	}

	if (samples > 0) {
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, type, width, height);
	} else {
		glRenderbufferStorage(GL_RENDERBUFFER, type, width, height);
	}
}

RenderbufferObjectGL::~RenderbufferObjectGL()
{
	glDeleteRenderbuffersEXT(1, &rbo);
}

}
