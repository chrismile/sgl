/*!
 * Defs.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
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

//! The MinGW version of GDB doesn't break on the standard assert.
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
