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

/**
 * Note: https://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2DMultisample.xml
 * -> "glTexImage2DMultisample is available only if the GL version is 3.2 or greater."
 * You can't use multisampled textures on systems with GL < 3.2!
 */
class DLL_OBJECT FramebufferObjectGL : public FramebufferObject {
public:
    FramebufferObjectGL();
    ~FramebufferObjectGL() override;
    bool bindTexture(TexturePtr texture, FramebufferAttachment attachment = COLOR_ATTACHMENT) override;
    bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment = DEPTH_ATTACHMENT) override;
    int getWidth() override { return width; }
    int getHeight() override { return height; }
    unsigned int _bindInternal() override;
    unsigned int getID() override { return id; }

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
    ~FramebufferObjectGL2() override;
    bool bindTexture(TexturePtr texture, FramebufferAttachment attachment = COLOR_ATTACHMENT) override;
    bool bindRenderbuffer(RenderbufferObjectPtr renderbuffer, FramebufferAttachment attachment = DEPTH_ATTACHMENT) override;
    unsigned int _bindInternal() override;

protected:
    bool checkStatus() override;
};

}

/*! GRAPHICS_OPENGL_FBO_HPP_ */
#endif
