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

#ifndef UTILS_FILE_RESOURCEBUFFER_HPP_
#define UTILS_FILE_RESOURCEBUFFER_HPP_

#include <memory>
#include <atomic>

namespace sgl {

class DLL_OBJECT ResourceBuffer {
public:
    explicit ResourceBuffer(size_t size) : bufferSize(size), loaded(false) { data = new char[bufferSize]; }
    ~ResourceBuffer() { if (data) { delete[] data; data = nullptr; } }
    inline char *getBuffer() { return data; }
    [[nodiscard]] inline const char *getBuffer() const { return data; }
    [[nodiscard]] inline size_t getBufferSize() const { return bufferSize; }
    inline bool getIsLoaded() { return loaded; }
    inline void setIsLoaded() { loaded = true; }

private:
    char *data;
    size_t bufferSize;
    /// For asynchronously loaded resources
    std::atomic<bool> loaded;
    /// optional!
    std::shared_ptr<ResourceBuffer> parentZipFileResource;
};

typedef std::shared_ptr<ResourceBuffer> ResourceBufferPtr;

}

/*! UTILS_FILE_RESOURCEBUFFER_HPP_ */
#endif
