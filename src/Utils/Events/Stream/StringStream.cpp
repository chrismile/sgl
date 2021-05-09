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

#include "Stream.hpp"
#include "StringStream.hpp"
#include <algorithm>
#include <cstring>

namespace sgl {

StringWriteStream::StringWriteStream(size_t size /* = STD_BUFFER_SIZE */)
{
    bufferSize = 0;
    capacity = 0;
    buffer = NULL;
    reserve(size);
}

StringWriteStream::~StringWriteStream()
{
    if (buffer) {
        delete[] buffer;
        buffer = NULL;
        capacity = 0;
        bufferSize = 0;
    }
}

void StringWriteStream::reserve(size_t size /* = STD_BUFFER_SIZE */)
{
    size = std::max((size_t)4, size); // Minimum buffer size: 32 bits
    if (size > capacity) {
        char *_buffer = new char[size];
        if (buffer) {
            memcpy(_buffer, buffer, bufferSize);
            delete[] buffer;
        }
        buffer = _buffer;
        capacity = size;
    }
}

void StringWriteStream::write(const void *data, size_t size)
{
    // Check if we need to increase the buffer size
    if (bufferSize + size > capacity) {
        reserve(bufferSize*2);
    }

    assert(bufferSize + size <= capacity);
    memcpy(buffer + bufferSize, data, size);
    bufferSize += size;
}

void StringWriteStream::write(const char *str)
{
    size_t strSize = strlen(str) + 1;
    write((void*)str, strSize);
}

void StringWriteStream::write(const std::string &str)
{
    size_t strSize = str.size() + 1;
    write((void*)str.c_str(), strSize);
}



StringReadStream::StringReadStream(StringWriteStream &stream)
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

StringReadStream::StringReadStream(void *_buffer, size_t _bufferSize)
{
    buffer = (char*)_buffer;
    bufferSize = _bufferSize;
    bufferStart = 0;
}

StringReadStream::~StringReadStream()
{
    if (buffer) {
        delete[] buffer;
        buffer = NULL;
        bufferStart = 0;
        bufferSize = 0;
    }
}

void StringReadStream::read(void *data, size_t size)
{
    assert(bufferStart + size <= bufferSize);
    memcpy(data, buffer + bufferStart, size);
    bufferStart += size;
}

void StringReadStream::read(std::string &str)
{
    str = buffer + bufferStart;
    bufferStart += strlen(buffer) + 1;
}

}
