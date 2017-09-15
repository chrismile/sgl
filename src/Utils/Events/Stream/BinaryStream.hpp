/*
 * BinaryStream.hpp
 *
 *  Created on: 03.08.2015
 *      Author: Christoph
 */

#ifndef BINARYSTREAM_HPP_
#define BINARYSTREAM_HPP_

#include <Defs.hpp>
#include <cassert>
#include <string>

namespace sgl {

class BinaryReadStream;

class DLL_OBJECT BinaryWriteStream
{
friend class BinaryReadStream;
public:
	BinaryWriteStream(size_t size = STD_BUFFER_SIZE);
	~BinaryWriteStream();
	inline size_t getSize() const { return bufferSize; }
	inline const uint8_t *getBuffer() const { return buffer; }
	void reserve(size_t size = STD_BUFFER_SIZE);

	// Serialization
	void write(const void *data, size_t size);
	template<typename T>
	void write(const T &val) { write((const void*)&val, sizeof(T)); }
	void write(const char *str);
	void write(const std::string &str);

	// Serialization with pipe operator
	template<typename T>
	BinaryWriteStream& operator<<(const T &val) { write(val); return *this; }
	BinaryWriteStream& operator<<(const char *str) { write(str); return *this; }
	BinaryWriteStream& operator<<(const std::string &str) { write(str); return *this; }

protected:
	void resize();
	size_t bufferSize; // The current buffer size (only the used part of the buffer counts)
	size_t capacity; // The maximum buffer size before it needs to be increased/reallocated
	uint8_t *buffer;
};

class DLL_OBJECT BinaryReadStream
{
public:
	BinaryReadStream(BinaryWriteStream &stream);
	BinaryReadStream(void *_buffer, size_t _bufferSize);
	BinaryReadStream(const void *_buffer, size_t _bufferSize);
	~BinaryReadStream();
	inline size_t getSize() const { return bufferSize; }

	// Deserialization
	void read(void *data, size_t size);
	template<typename T>
	void read(T &val) { read((void*)&val, sizeof(T)); }
	void read(std::string &str);

	// Deserialization with pipe operator
	template<typename T>
	BinaryReadStream& operator>>(T &val) { read(val); return *this; }
	BinaryReadStream& operator>>(std::string &str) { read(str); return *this; }

protected:
	void resize();
	size_t bufferSize; // The total buffer size
	size_t bufferStart; // The current point in the buffer where the code reads from
	uint8_t *buffer;
};

}

#endif /* BINARYSTREAM_HPP_ */
