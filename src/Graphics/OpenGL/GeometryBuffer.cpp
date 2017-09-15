/*
 * GeometryBufferGL.cpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph
 */

#include <GL/glew.h>
#include "GeometryBuffer.hpp"
#include <Utils/File/Logfile.hpp>

namespace sgl {

GeometryBufferGL::GeometryBufferGL(size_t size, BufferType type /* = VERTEX_BUFFER */, BufferUse bufferUse /* = BUFFER_STATIC */)
	: GeometryBuffer(size, type, bufferUse)
{
	GLenum usage = GL_STATIC_DRAW;
	if (bufferUse == BUFFER_DYNAMIC) {
		usage = GL_DYNAMIC_DRAW;
	} else if (bufferUse == BUFFER_STREAM) {
		usage = GL_STREAM_DRAW;
	}

	oglBufferType = GL_ARRAY_BUFFER;
	if (type == INDEX_BUFFER) {
		oglBufferType = GL_ELEMENT_ARRAY_BUFFER;
	}

	glGenBuffers(1, &buffer);
	glBindBuffer(oglBufferType, buffer);
	glBufferData(oglBufferType, size, NULL, usage);
}

GeometryBufferGL::GeometryBufferGL(size_t size, void *data, BufferType type /* = VERTEX_BUFFER*/, BufferUse bufferUse /* = BUFFER_STATIC */)
	: GeometryBuffer(size, data, type, bufferUse)
{
	GLenum usage = GL_STATIC_DRAW;
	if (bufferUse == BUFFER_DYNAMIC) {
		usage = GL_DYNAMIC_DRAW;
	} else if (bufferUse == BUFFER_STREAM) {
		usage = GL_STREAM_DRAW;
	}

	oglBufferType = GL_ARRAY_BUFFER;
	if (type == INDEX_BUFFER) {
		oglBufferType = GL_ELEMENT_ARRAY_BUFFER;
	}

	glGenBuffers(1, &buffer);
	glBindBuffer(oglBufferType, buffer);
	glBufferData(oglBufferType, size, data, usage);
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

	glBindBuffer(oglBufferType, buffer);
	return glMapBufferRange(oglBufferType, offset, size, accessType);
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
