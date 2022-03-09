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
#include "RBO.hpp"

namespace sgl {

RenderbufferObjectGL::RenderbufferObjectGL(int _width, int _height, RenderbufferType rboType, int _samples /* = 0 */)
    : rbo(0), width(_width), height(_height), samples(_samples) {
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    GLuint type = 0;
    switch (rboType) {
    case RBO_DEPTH16:
        type = GL_DEPTH_COMPONENT16;
        break;
    case RBO_DEPTH24_STENCIL8:
        type = GL_DEPTH24_STENCIL8;
        break;
    case RBO_DEPTH32F_STENCIL8:
        type = GL_DEPTH32F_STENCIL8;
        break;
    case RBO_RGBA8:
        type = GL_RGBA8;
        break;
    }

    if (samples > 0) {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, type, width, height);
    } else {
        glRenderbufferStorage(GL_RENDERBUFFER, type, width, height);
    }
}

RenderbufferObjectGL::~RenderbufferObjectGL() {
    glDeleteRenderbuffers(1, &rbo);
}

}
