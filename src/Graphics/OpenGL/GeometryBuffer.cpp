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
#include "GeometryBuffer.hpp"
#include <Utils/File/Logfile.hpp>

namespace sgl {

GeometryBufferGL::GeometryBufferGL(size_t size, BufferType type /* = VERTEX_BUFFER */, BufferUse bufferUse /* = BUFFER_STATIC */)
    : GeometryBuffer(size, type, bufferUse)
{
    initialize(type, bufferUse);

    glGenBuffers(1, &buffer);
    glBindBuffer(oglBufferType, buffer);
    glBufferData(oglBufferType, size, NULL, oglBufferUsage);
}

GeometryBufferGL::GeometryBufferGL(size_t size, void *data, BufferType type /* = VERTEX_BUFFER*/, BufferUse bufferUse /* = BUFFER_STATIC */)
    : GeometryBuffer(size, data, type, bufferUse)
{
    initialize(type, bufferUse);

    glGenBuffers(1, &buffer);
    glBindBuffer(oglBufferType, buffer);
    glBufferData(oglBufferType, size, data, oglBufferUsage);
}

void GeometryBufferGL::initialize(BufferType type, BufferUse bufferUse)
{
    oglBufferUsage = GL_STATIC_DRAW;
    if (bufferUse == BUFFER_DYNAMIC) {
        oglBufferUsage = GL_DYNAMIC_DRAW;
    } else if (bufferUse == BUFFER_STREAM) {
        oglBufferUsage = GL_STREAM_DRAW;
    }

    oglBufferType = GL_ARRAY_BUFFER;
    if (type == INDEX_BUFFER) {
        oglBufferType = GL_ELEMENT_ARRAY_BUFFER;
    } else if (type == SHADER_STORAGE_BUFFER) {
        oglBufferType = GL_SHADER_STORAGE_BUFFER;
    } else if (type == UNIFORM_BUFFER) {
        oglBufferType = GL_UNIFORM_BUFFER;
    }
}

GeometryBufferGL::~GeometryBufferGL()
{
    glDeleteBuffers(1, &buffer);
}

void GeometryBufferGL::subData(int offset, size_t size, void *data)
{
    if (offset + size > bufferSize) {
        Logfile::get()->writeError("GeometryBufferGL::subData: offset + size > bufferSize.");
    }

    glBindBuffer(oglBufferType, buffer);
    glBufferSubData(oglBufferType, offset, size, data);
}

void *GeometryBufferGL::mapBuffer(BufferMapping accessType)
{
    glBindBuffer(oglBufferType, buffer);
    return glMapBuffer(oglBufferType, accessType);
}

void *GeometryBufferGL::mapBufferRange(int offset, size_t size, BufferMapping accessType)
{
    if (offset + size > bufferSize) {
        Logfile::get()->writeError("GeometryBufferGL::subData: offset + size > bufferSize.");
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml
    GLbitfield access = 0;
    if (accessType == BUFFER_MAP_READ_ONLY || accessType == BUFFER_MAP_READ_WRITE) {
        access |= GL_MAP_READ_BIT;
    } else if (accessType == BUFFER_MAP_WRITE_ONLY || accessType == BUFFER_MAP_READ_WRITE) {
        access |= GL_MAP_WRITE_BIT;
    }

    glBindBuffer(oglBufferType, buffer);
    return glMapBufferRange(oglBufferType, offset, size, access);
}

void GeometryBufferGL::unmapBuffer()
{
    glBindBuffer(oglBufferType, buffer);
    bool success = glUnmapBuffer(oglBufferType);
    if (!success) {
        Logfile::get()->writeError("GeometryBufferGL::unmapBuffer: glUnmapBuffer returned GL_FALSE.");
    }
}

void GeometryBufferGL::bind()
{
    glBindBuffer(oglBufferType, buffer);
}

void GeometryBufferGL::unbind()
{
    glBindBuffer(oglBufferType, 0);
}

}
