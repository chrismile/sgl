/*
 * FBO.cpp
 *
 *  Created on: 31.03.2015
 *      Author: Christoph Neuhauser
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
    hasColorAttachment = false;
}

FramebufferObjectGL::~FramebufferObjectGL()
{
    textures.clear();
    rbos.clear();
    glDeleteFramebuffers(1, &id);
}

bool FramebufferObjectGL::bindTexture(TexturePtr texture, FramebufferAttachment attachment)
{
    if (attachment >= COLOR_ATTACHMENT0 && attachment <= COLOR_ATTACHMENT15) {
        hasColorAttachment = true;
    }

    textures[attachment] = texture;
    TextureGL *textureGL = (TextureGL*)texture.get();

    int oglTexture = textureGL->getTexture();
    width = textureGL->getW();
    height = textureGL->getH();
    //int samples = texture->getNumSamples();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, samples == 0 ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE, oglTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, attachment, oglTexture, 0);
    Renderer->bindFBO(Renderer->getFBO(), true);
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGL::bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment)
{
    rbos[attachment] = renderbuffer;
    RenderbufferObjectGL *rbo = (RenderbufferObjectGL*)renderbuffer.get();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo->getID());
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

    if (!hasColorAttachment) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        hasColorAttachment = true; // Only call once
    }

    // More than one color attachment
    if (textures.size() > 1) {
        if (colorAttachments.size() == 0) {
            colorAttachments.reserve(textures.size());
            for (auto it = textures.begin(); it != textures.end(); it++) {
                if (it->first >= COLOR_ATTACHMENT0 && it->first <= COLOR_ATTACHMENT15) {
                    colorAttachments.push_back(it->first);
                }
            }
        }
        if (colorAttachments.size() > 1) {
            glDrawBuffers(colorAttachments.size(), &colorAttachments.front());
        }
    }

    return id;
}



// OpenGL 2

FramebufferObjectGL2::FramebufferObjectGL2()
{
    glGenFramebuffersEXT(1, &id);
    hasColorAttachment = false;
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

bool FramebufferObjectGL2::bindTexture(TexturePtr texture, FramebufferAttachment attachment)
{
    if (attachment >= COLOR_ATTACHMENT0 && attachment <= COLOR_ATTACHMENT15) {
        hasColorAttachment = true;
    }

    textures[attachment] = texture;
    TextureGL *textureGL = (TextureGL*)texture.get();

    int oglTexture = textureGL->getTexture();
    width = textureGL->getW();
    height = textureGL->getH();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, oglTexture, 0);
    Renderer->bindFBO(Renderer->getFBO(), true);
    bool status = checkStatus();
    return status;
}

bool FramebufferObjectGL2::bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment)
{
    rbos[attachment] = renderbuffer;
    RenderbufferObjectGL *rbo = (RenderbufferObjectGL*)renderbuffer.get();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo->getID());
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
    if (!hasColorAttachment) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        hasColorAttachment = true; // Only call once
    }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
    return id;
}

}
