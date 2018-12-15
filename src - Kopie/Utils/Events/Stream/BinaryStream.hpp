/*!
 * BinaryStream.hpp
 *
 *  Created on: 03.08.2015
 *      Author: Christoph Neuhauser
 */

#ifndef BINARYSTREAM_HPP_
#define BINARYSTREAM_HPP_

#include <Defs.hpp>
#include <cassert>
#include <string>
#include <vector>

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

	//! Serialization
	void write(const void *data, size_t size);
	template<typename T>
	void write(const T &val) { write((const void*)&val, sizeof(T)); }
	void write(const char *str);
	void write(const std::string &str);

	template<typename T>
	void writeArray(const std::vector<T> &v)
	{
		uint32_t size = v.size();
		write(size);
		if (size > 0) {
			write((const void*)&v.front(), sizeof(T)*v.size());
		}
	}

	//! Serialization with pipe operator
	template<typename T>
	BinaryWriteStream& operator<<(const T &val) { write(val); return *this; }
	BinaryWriteStream& operator<<(const char *str) { write(str); return *this; }
	BinaryWriteStream& operator<<(const std::string &str) { write(str); return *this; }

protected:
	void resize();
	//! The current buffer size (only the used part of the buffer counts)
	size_t bufferSize;
	//! The maximum buffer size before it needs to be increased/reallocated
	size_t capacity;
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

	//! Deserialization
	void read(void *data, size_t size);
	template<typename T>
	void read(T &val) { read((void*)&val, sizeof(T)); }
	void read(std::string &str);

	template<typename T>
	void readArray(std::vector<T> &v)
	{
		uint32_t size;
		read(size);
		if (size > 0) {
			v.resize(size);
			read((void*)&v.front(), sizeof(T)*v.size());
		}
	}

	//! Deserialization with pipe operator
	template<typename T>
	BinaryReadStream& operator>>(T &val) { read(val); return *this; }
	BinaryReadStream& operator>>(std::string &str) { read(str); return *this; }

protected:
	void resize();
	//! The total buffer size
	size_t bufferSize;
	//! The current point in the buffer where the code reads from
	size_t bufferStart;
	uint8_t *buffer;
};

}

/*! BINARYSTREAM_HPP_ */
#endif
