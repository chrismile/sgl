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

#ifndef GRAPHICS_BUFFERS_RBO_HPP_
#define GRAPHICS_BUFFERS_RBO_HPP_

#include <memory>

#include <Defs.hpp>

namespace sgl {

enum RenderbufferType {
    RBO_DEPTH16 = 0x81A5, RBO_DEPTH24_STENCIL8 = 0x88F0, RBO_DEPTH32F_STENCIL8 = 0x8CAD,
    RBO_RGBA8 = 0x8058
};

/**
 * RBOs can be attached to framebuffer objects (see FBO.hpp).
 * For more information on RBOs see https://www.khronos.org/opengl/wiki/Renderbuffer_Object
 */
class DLL_OBJECT RenderbufferObject {
public:
    RenderbufferObject() = default;
    virtual ~RenderbufferObject() = default;
    virtual int getWidth()=0;
    virtual int getHeight()=0;
    virtual int getSamples()=0;
};

typedef std::shared_ptr<RenderbufferObject> RenderbufferObjectPtr;

}

/*! GRAPHICS_BUFFERS_RBO_HPP_ */
#endif
