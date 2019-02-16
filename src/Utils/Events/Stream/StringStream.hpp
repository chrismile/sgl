/*!
 * StringStream.hpp
 *
 *  Created on: 03.08.2015
 *      Author: Christoph Neuhauser
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
