/*!
 * GeometryBuffer.hpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_BUFFERS_GEOMETRYBUFFER_HPP_
#define GRAPHICS_BUFFERS_GEOMETRYBUFFER_HPP_

#include <boost/shared_ptr.hpp>

namespace sgl {

/*! VERTEX_BUFFER: GL_ARRAY_BUFFER (vertex data)
 * INDEX_BUFFER: GL_ELEMENT_ARRAY_BUFFER (indices) */
enum BufferType {
	VERTEX_BUFFER = 0x8892, INDEX_BUFFER = 0x8893, SHADER_STORAGE_BUFFER = 0x90D2, UNIFORM_BUFFER = 0x8A11
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

class GeometryBuffer
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

typedef boost::shared_ptr<GeometryBuffer> GeometryBufferPtr;

}

/*! GRAPHICS_BUFFERS_GEOMETRYBUFFER_HPP_ */
#endif
