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

#ifndef GRAPHICS_BUFFERS_GEOMETRYBUFFER_HPP_
#define GRAPHICS_BUFFERS_GEOMETRYBUFFER_HPP_

#include <memory>

namespace sgl {

/*! VERTEX_BUFFER: GL_ARRAY_BUFFER (vertex data)
 * INDEX_BUFFER: GL_ELEMENT_ARRAY_BUFFER (indices) */
enum BufferType {
    VERTEX_BUFFER = 0x8892, INDEX_BUFFER = 0x8893, SHADER_STORAGE_BUFFER = 0x90D2, UNIFORM_BUFFER = 0x8A11,
    ATOMIC_COUNTER_BUFFER = 0x92C0
};

/*! BUFFER_STATIC: Data is uploaded to the buffer only once and won't be updated afterwards (static meshes)
 * BUFFER_DYNAMIC: Buffer is updated more or less frequently
 * BUFFER_STREAM: Buffer will be updated almost every frame */
enum BufferUse {
    BUFFER_STATIC, BUFFER_DYNAMIC, BUFFER_STREAM
};

enum BufferMapping {
    BUFFER_MAP_READ_ONLY = 0x88B8, BUFFER_MAP_WRITE_ONLY = 0x88B9, BUFFER_MAP_READ_WRITE = 0x88BA
};

class DLL_OBJECT GeometryBuffer
{
public:
    GeometryBuffer(size_t size, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC)
        : bufferSize(size), bufferType(type) {}
    GeometryBuffer(size_t size, void *data, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC)
        : bufferSize(size), bufferType(type) {}
    virtual ~GeometryBuffer() {}

    /*! Upload data to sub-region of buffer */
    virtual void subData(int offset, size_t size, void *data)=0;
    /*! Map entire buffer into main memory */
    virtual void *mapBuffer(BufferMapping accessType)=0;
    /*! Map section of buffer into main memory */
    virtual void *mapBufferRange(int offset, size_t size, BufferMapping accessType)=0;
    virtual void unmapBuffer()=0;
    //! Mainly for internal use
    virtual void bind()=0;
    virtual void unbind()=0;
    inline size_t getSize() { return bufferSize; }
    inline BufferType getBufferType() { return bufferType; }

protected:
    size_t bufferSize;
    BufferType bufferType;
};

typedef std::shared_ptr<GeometryBuffer> GeometryBufferPtr;

}

/*! GRAPHICS_BUFFERS_GEOMETRYBUFFER_HPP_ */
#endif
