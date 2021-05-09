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

#ifndef STRINGSTREAM_HPP_
#define STRINGSTREAM_HPP_

#include <Utils/Convert.hpp>
#include <cassert>
#include <string>

namespace sgl {

class StringReadStream;

class DLL_OBJECT StringWriteStream
{
friend class StringReadStream;
public:
    StringWriteStream(size_t size = STD_BUFFER_SIZE);
    ~StringWriteStream();
    inline size_t getSize() const { return bufferSize; }
    inline const char *getBuffer() const { return buffer; }
    void reserve(size_t size = STD_BUFFER_SIZE);

    //! Serialization
    void write(const void *data, size_t size);
    template<typename T>
    void write(const T &val) { write(toString(val)); }
    void write(const char *str);
    void write(const std::string &str);

    //! Serialization with pipe operator
    template<typename T>
    StringWriteStream& operator<<(const T &val) { write(val); return *this; }
    StringWriteStream& operator<<(const char *str) { write(str); return *this; }
    StringWriteStream& operator<<(const std::string &str) { write(str); return *this; }

protected:
    void resize();
    //! The current buffer size (only the used part of the buffer counts)
    size_t bufferSize;
    //! The maximum buffer size before it needs to be increased/reallocated
    size_t capacity;
    char *buffer;
};

class DLL_OBJECT StringReadStream
{
public:
    StringReadStream(StringWriteStream &stream);
    StringReadStream(void *_buffer, size_t _bufferSize);
    ~StringReadStream();
    inline size_t getSize() const { return bufferSize; }

    //! Deserialization
    void read(void *data, size_t size);
    template<typename T>
    void read(T &val) { read((void*)&val, sizeof(T)); }
    void read(std::string &str);

    //! Deserialization with pipe operator
    template<typename T>
    StringReadStream& operator>>(T &val) { read(val); return *this; }
    StringReadStream& operator>>(std::string &str) { read(str); return *this; }

protected:
    void resize();
    //! The total buffer size
    size_t bufferSize;
    //! The current point in the buffer where the code reads from
    size_t bufferStart;
    char *buffer;
};

}

/*! STRINGSTREAM_HPP_ */
#endif
