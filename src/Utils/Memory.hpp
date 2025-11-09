/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#ifndef SGL_MEMORY_HPP
#define SGL_MEMORY_HPP

#include <cstdlib>

namespace sgl {

/**
 * https://en.cppreference.com/w/cpp/memory/c/aligned_alloc says regarding std::aligned_alloc:
 * "This function is not supported in Microsoft C Runtime library because its implementation of std::free is unable to
 * handle aligned allocations of any kind. Instead, MS CRT provides _aligned_malloc (to be freed with _aligned_free)."
 */

#ifdef _WIN32
inline void* aligned_alloc(std::size_t alignment, std::size_t size) {
    return _aligned_malloc(alignment, size);
}
inline void aligned_free(void* ptr) {
    _aligned_free(ptr);
}
#else
inline void* aligned_alloc(std::size_t alignment, std::size_t size) {
    return std::aligned_alloc(alignment, size);
}
inline void aligned_free(void* ptr) {
    std::free(ptr);
}
#endif

}

#endif //SGL_MEMORY_HPP