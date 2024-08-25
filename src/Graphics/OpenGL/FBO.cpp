/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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

#include <GL/glew.h>
#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Renderer.hpp>
#include "FBO.hpp"
#include "RBO.hpp"
#include "Texture.hpp"

namespace sgl {

FramebufferObjectGL::FramebufferObjectGL() : id(0), width(0), height(0) {
    glGenFramebuffers(1, &id);
    hasColorAttachment = false;
}

FramebufferObjectGL::FramebufferObjectGL(bool hasColorAttachment)
        : id(0), width(0), height(0), hasColorAttachment(hasColorAttachment) {
}

FramebufferObjectGL::~FramebufferObjectGL() {
    textures.clear();
    rbos.clear();
    glDeleteFramebuffers(1, &id);
}

bool FramebufferObjectGL::bindTexture(TexturePtr texture, FramebufferAttachment attachment) {
    if (attachment >= COLOR_ATTACHMENT0 && attachment <= COLOR_ATTACHMENT15) {
        hasColorAttachment = true;
    }

    textures[attachment] = texture;
    auto* textureGL = (TextureGL*)texture.get();

    int oglTexture = textureGL->getTexture();
    width = textureGL->getW();
    height = textureGL->getH();
    //int samples = texture->getNumSamples();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, samples == 0 ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE, oglTexture, 0);
#ifndef __EMSCRIPTEN__
    glFramebufferTexture(GL_FRAMEBUFFER, attachment, oglTexture, 0);
#else
    glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, oglTexture, 0, 0);
#endif
    Renderer->bindFBO(Renderer->getFBO(), true);
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGL::bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment) {
    rbos[attachment] = renderbuffer;
    RenderbufferObjectGL* rbo = (RenderbufferObjectGL*)renderbuffer.get();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo->getID());
    Renderer->bindFBO(Renderer->getFBO(), true);
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGL::checkStatus() {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        default:
            Logfile::get()->writeError(
                    std::string() + "Error: FramebufferObject::checkStatus(): "
                    + "Invalid FBO status " + toString(status) + "!");
            return false;
    }
    return true;
}

unsigned int FramebufferObjectGL::_bindInternal() {
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    if (!hasColorAttachment) {
#ifndef __EMSCRIPTEN__
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
#else
        GLenum noneBuffer = GL_NONE;
        glDrawBuffers(1, &noneBuffer);
        sgl::Logfile::get()->writeWarning(
                "Warning in FramebufferObjectGL::_bindInternal: !hasColorAttachment is not supported with Emscripten.");
#endif
        hasColorAttachment = true; // Only call once
    }

    // More than one color attachment
    if (textures.size() > 1) {
        if (colorAttachments.empty()) {
            colorAttachments.reserve(textures.size());
            for (auto& texture : textures) {
                if (texture.first >= COLOR_ATTACHMENT0 && texture.first <= COLOR_ATTACHMENT15) {
                    colorAttachments.push_back(texture.first);
                }
            }
        }
        if (colorAttachments.size() > 1) {
            glDrawBuffers(GLsizei(colorAttachments.size()), &colorAttachments.front());
        }
    }

    return id;
}


#ifndef __EMSCRIPTEN__

// OpenGL 4.5

FramebufferObjectGLNamed::FramebufferObjectGLNamed() : FramebufferObjectGL(false) {
    glCreateFramebuffers(1, &id);
}

FramebufferObjectGLNamed::~FramebufferObjectGLNamed() {
}

bool FramebufferObjectGLNamed::bindTexture(TexturePtr texture, FramebufferAttachment attachment) {
    if (attachment >= COLOR_ATTACHMENT0 && attachment <= COLOR_ATTACHMENT15) {
        hasColorAttachment = true;
    }

    textures[attachment] = texture;
    TextureGL* textureGL = (TextureGL*)texture.get();

    int oglTexture = textureGL->getTexture();
    width = textureGL->getW();
    height = textureGL->getH();
    glNamedFramebufferTexture(id, attachment, oglTexture, 0);
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGLNamed::bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment) {
    rbos[attachment] = renderbuffer;
    RenderbufferObjectGL* rbo = (RenderbufferObjectGL*)renderbuffer.get();
    glNamedFramebufferRenderbuffer(id, attachment, GL_RENDERBUFFER, rbo->getID());
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGLNamed::checkStatus() {
    GLenum status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        default:
            Logfile::get()->writeError(
                    std::string() + "Error: FramebufferObjectGLNamed::checkStatus(): "
                    + "Invalid FBO status " + toString(status) + "!");
            return false;
    }
    return true;
}

unsigned int FramebufferObjectGLNamed::_bindInternal() {
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    if (!hasColorAttachment) {
        glNamedFramebufferDrawBuffer(id, GL_NONE);
        glNamedFramebufferReadBuffer(id, GL_NONE);
        hasColorAttachment = true; // Only call once
    }

    // More than one color attachment
    if (textures.size() > 1) {
        if (colorAttachments.empty()) {
            colorAttachments.reserve(textures.size());
            for (auto& texture : textures) {
                if (texture.first >= COLOR_ATTACHMENT0 && texture.first <= COLOR_ATTACHMENT15) {
                    colorAttachments.push_back(texture.first);
                }
            }
        }
        if (colorAttachments.size() > 1) {
            glNamedFramebufferDrawBuffers(id, GLsizei(colorAttachments.size()), &colorAttachments.front());
        }
    }

    return id;
}

#endif


// OpenGL 2

FramebufferObjectGL2::FramebufferObjectGL2() {
#ifndef __EMSCRIPTEN__
    glGenFramebuffersEXT(1, &id);
#else
    glGenFramebuffers(1, &id);
#endif
    hasColorAttachment = false;
}

FramebufferObjectGL2::~FramebufferObjectGL2() {
    if (Renderer->getFBO().get() == this) {
        Renderer->unbindFBO();
    }

    textures.clear();
    rbos.clear();
#ifndef __EMSCRIPTEN__
    glDeleteFramebuffersEXT(1, &id);
#else
    glDeleteFramebuffers(1, &id);
#endif
}

bool FramebufferObjectGL2::bindTexture(TexturePtr texture, FramebufferAttachment attachment) {
    if (attachment >= COLOR_ATTACHMENT0 && attachment <= COLOR_ATTACHMENT15) {
        hasColorAttachment = true;
    }

    textures[attachment] = texture;
    TextureGL* textureGL = (TextureGL*)texture.get();

    int oglTexture = textureGL->getTexture();
    width = textureGL->getW();
    height = textureGL->getH();
#ifndef __EMSCRIPTEN__
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, oglTexture, 0);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, oglTexture, 0);
#endif
    Renderer->bindFBO(Renderer->getFBO(), true);
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGL2::bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment) {
    rbos[attachment] = renderbuffer;
    RenderbufferObjectGL* rbo = (RenderbufferObjectGL*)renderbuffer.get();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo->getID());
    Renderer->bindFBO(Renderer->getFBO(), true);
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGL2::checkStatus() {
#ifndef __EMSCRIPTEN__
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
#else
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#endif
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            break;
        default:
            Logfile::get()->writeError(
                    std::string() + "Error: FramebufferObject::checkStatus(): "
                    + "Invalid FBO status " + toString(status) + "!");
            return false;
    }
    return true;
}

unsigned int FramebufferObjectGL2::_bindInternal() {
    if (!hasColorAttachment) {
#ifndef __EMSCRIPTEN__
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
#else
        GLenum noneBuffer = GL_NONE;
        glDrawBuffers(1, &noneBuffer);
        sgl::Logfile::get()->writeWarning(
                "Warning in FramebufferObjectGL::_bindInternal: !hasColorAttachment is not supported with Emscripten.");
#endif
        hasColorAttachment = true; // Only call once
    }

#ifndef __EMSCRIPTEN__
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, id);
#endif
    return id;
}

}
