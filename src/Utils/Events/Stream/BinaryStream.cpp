/*
 * BinaryStream.cpp
 *
 *  Created on: 03.08.2015
 *      Author: Christoph Neuhauser
 */

#include "Stream.hpp"
#include "BinaryStream.hpp"
#include <algorithm>
#include <cstring>
#include <Utils/File/Logfile.hpp>

namespace sgl {

BinaryWriteStream::BinaryWriteStream(size_t size /* = STD_BUFFER_SIZE */)
{
	bufferSize = 0;
	capacity = 0;
	buffer = NULL;
	reserve(size);
}

BinaryWriteStream::~BinaryWriteStream()
{
	if (buffer) {
		delete[] buffer;
		buffer = NULL;
		capacity = 0;
		bufferSize = 0;
	}
}

void BinaryWriteStream::reserve(size_t size /* = STD_BUFFER_SIZE */)
{
	size = std::max((size_t)4, size); // Minimum buffer size: 32 bits
	if (size > capacity) {
		uint8_t *_buffer = new uint8_t[size];
		if (buffer) {
			memcpy(_buffer, buffer, bufferSize);
			delete[] buffer;
		}
		buffer = _buffer;
		capacity = size;
	}
}

void BinaryWriteStream::write(const void *data, size_t size)
{
	// Check if we need to increase the buffer size
	if (bufferSize + size > capacity) {
		reserve(bufferSize*2);
	}

	assert(bufferSize + size <= capacity);
	memcpy(buffer + bufferSize, data, size);
	bufferSize += size;
}

void BinaryWriteStream::write(const char *str)
{
	size_t strSize = strlen(str);
	write((uint16_t)strSize);
	write((void*)str, strSize);
}

void BinaryWriteStream::write(const std::string &str)
{
	size_t strSize = str.size();
	write((uint16_t)strSize);
	write((void*)str.c_str(), strSize);
}



BinaryReadStream::BinaryReadStream(BinaryWriteStream &stream)
{
	// Copy the buffer address to this stream
	buffer = stream.buffer;
	bufferSize = stream.bufferSize;
	bufferStart = 0;

	// Delete the buffer from the old stream
	stream.buffer = NULL;
	stream.bufferSize = 0;
	stream.capacity = 0;
}

BinaryReadStream::BinaryReadStream(void *_buffer, size_t _bufferSize)
{
	buffer = (uint8_t*)_buffer;
	bufferSize = _bufferSize;
	bufferStart = 0;
}

BinaryReadStream::BinaryReadStream(const void *_buffer, size_t _bufferSize)
{
	buffer = new uint8_t[_bufferSize];
	memcpy(buffer, _buffer, _bufferSize);
	bufferSize = _bufferSize;
	bufferStart = 0;
}

BinaryReadStream::~BinaryReadStream()
{
	if (buffer) {
		delete[] buffer;
		buffer = NULL;
		bufferStart = 0;
		bufferSize = 0;
	}
}

void BinaryReadStream::read(void *data, size_t size)
{
	if (bufferStart + size > bufferSize) {
		Logfile::get()->writeError("FATAL ERROR: BinaryReadStream::read(void*, size_t)");
		return;
	}
	memcpy(data, buffer + bufferStart, size);
	bufferStart += size;
}

void BinaryReadStream::read(std::string &str)
{
	uint16_t strSize;
	read(strSize);
	if ((size_t)strSize + 1 < bufferSize - bufferStart) {
		Logfile::get()->writeError("FATAL ERROR: BinaryReadStream::read(string)");
		return;
	}
	char *cstr = new char[strSize+1];
	cstr[strSize] = '\0';
	read((void*)cstr, (size_t)strSize);
	str = cstr;
	delete[] cstr;
}


/*class BodyMovedEvent : public EventData
{
public:
	void serialize(StreamPtr &stream)
	{
		stream << getID() << object.getID() << object.getTransform();
	}
};*/

}
