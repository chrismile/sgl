/*
 * RenderTarget.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#include "RenderTarget.hpp"
#include <Graphics/Renderer.hpp>
#include <Graphics/Buffers/FBO.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Window.hpp>

namespace sgl {

void RenderTarget::bindFramebufferObject(FramebufferObjectPtr _framebuffer)
{
	framebuffer = _framebuffer;
}

void RenderTarget::bindWindowFramebuffer()
{
	framebuffer = FramebufferObjectPtr();
}

FramebufferObjectPtr RenderTarget::getFramebufferObject()
{
	return framebuffer;
}

void RenderTarget::bindRenderTarget()
{
	if (framebuffer) {
		Renderer->bindFBO(framebuffer);
	} else {
		Renderer->unbindFBO();
	}
}

int RenderTarget::getWidth()
{
	if (framebuffer) {
		return framebuffer->getWidth();
	} else {
		return AppSettings::get()->getMainWindow()->getWidth();
	}
}

int RenderTarget::getHeight()
{
	if (framebuffer) {
		return framebuffer->getHeight();
	} else {
		return AppSettings::get()->getMainWindow()->getHeight();
	}
}

}
