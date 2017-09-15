/*
 * GeometryBuffer.hpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_OPENGL_GEOMETRYBUFFER_HPP_
#define GRAPHICS_OPENGL_GEOMETRYBUFFER_HPP_

#include <Graphics/Buffers/GeometryBuffer.hpp>

namespace sgl {

class GeometryBufferGL : public GeometryBuffer
{
public:
	GeometryBufferGL(size_t size, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC);
	GeometryBufferGL(size_t size, void *data, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC);
	~GeometryBufferGL();
	void subData(int offset, size_t size, void *data);
	void *mapBuffer(BufferMapping accessType);
	void *mapBufferRange(int offset, size_t size, BufferMapping accessType);
	void unmapBuffer();
	void bind();
	void unbind();
	inline unsigned int getBuffer() { return buffer; }
	inline unsigned int getGLBufferType() { return oglBufferType; }

private:
	unsigned int buffer;
	unsigned int oglBufferType;
};

}

#endif /* GRAPHICS_OPENGL_GEOMETRYBUFFER_HPP_ */
