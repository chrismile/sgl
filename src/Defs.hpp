/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

#ifndef SRC_DEFS_HPP_
#define SRC_DEFS_HPP_

#include <cstddef>

#ifndef SDL2
#define SDL2
#endif

#ifdef _WIN32
#ifndef WIN32
#define WIN32
#endif
#endif

//#if defined(_WIN32 ) && !defined(__MINGW32__)
//#ifdef DLL_BUILD
//#define DLL_OBJECT __declspec(dllexport)
//#else
//#define DLL_OBJECT __declspec(dllimport)
//#endif
//#else
//#define DLL_OBJECT
//#endif
#ifndef DLL_OBJECT
#define DLL_OBJECT
#endif

/// The MinGW version of GDB doesn't break on the standard assert.
#include <cassert>
#define CUSTOM_ASSERT

#ifdef CUSTOM_ASSERT
#ifndef NDEBUG
#include <iostream>
inline void MY_ASSERT(bool expression) {
    if (!expression) {
        std::cerr << "PUT YOUR BREAKPOINT HERE";
    }
    assert(expression);
}
#else
#define MY_ASSERT(expr) do { (void)sizeof(expr); } while(0)
#endif
#else
#define MY_ASSERT assert
#endif

#define UNUSED(x) ((void)(x))

/*! SRC_DEFS_HPP_ */
#endif
