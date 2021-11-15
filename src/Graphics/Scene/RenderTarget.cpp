/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "RenderTarget.hpp"
#include <Graphics/Renderer.hpp>
#include <Graphics/Buffers/FBO.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Window.hpp>
#include <utility>

namespace sgl {

RenderTarget::RenderTarget(FramebufferObjectPtr _framebuffer)
{
    framebuffer = std::move(_framebuffer);
}

void RenderTarget::bindFramebufferObject(FramebufferObjectPtr _framebuffer)
{
    framebuffer = std::move(_framebuffer);
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
