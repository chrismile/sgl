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

#include <cmath>

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

#include "Math.hpp"

#if __cplusplus >= 202002L
#if __has_cpp_attribute(__cpp_lib_int_pow2)
#include <bit>
#define SUPPORTS_STD_BIT_WIDTH
#endif
#endif

#if !defined(SUPPORTS_STD_BIT_WIDTH) && defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#endif

namespace sgl {

// Fast integer square root, i.e., floor(sqrt(s)), see https://en.wikipedia.org/wiki/Integer_square_root
uint32_t uisqrt(uint32_t s) {
    if (s <= 1) {
        return s;
    }

    /*
     * Initial estimate should be pow2(floor(log2(n)/2)+1) if this can be estimated cheaply.
     * Otherwise, n/2 is used as the initial estimate. The estimate MUST always be larger than the result.
     * std::bit_width(s) == floor(log2(n)) + 1
     * NOTE: pow2(floor(log2(n)/2)+1) == pow2(floor(log2(n))/2+1)
     */
#ifdef SUPPORTS_STD_BIT_WIDTH
    uint32_t x0 = 1 << (((std::bit_width(s) - 1) >> 1) + 1);
#elif defined(__GNUC__)
    uint32_t x0 = 1 << (((31 - __builtin_clz(s)) >> 1) + 1);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse(&index, s);
    uint32_t x0 = 1 << ((index >> 1) + 1);
#else
    uint32_t x0 = s / 2;
    // uint32_t x0 = 1 << (uint32_t(std::log2(s) / 2.0f) + 1);
    // uint32_t x0 = 1 << ((uint32_t(std::log2(s)) >> 1) + 1);
#endif
    // For GLSL use 1 << ((findMSB(s) >> 1) + 1).

    uint32_t x1 = (x0 + s / x0) / 2;
    while (x1 < x0) {
        x0 = x1;
        x1 = (x0 + s / x0) / 2;
    }
    return x0;
}

union FloatUint32Union {
    float valFloat;
    uint32_t valUint32;
};

uint32_t convertBitRepresentationFloatToUint32(float val) {
    FloatUint32Union u;
    u.valFloat = val;
    return u.valUint32;
}

float vectorAngle(const glm::vec2 &u, const glm::vec2& v) {
    glm::vec2 un = glm::normalize(u);
    glm::vec2 vn = glm::normalize(v);
    float r = glm::dot(un, vn);
    if (r < -1.0f) r = -1.0f;
    if (r > 1.0f) r = 1.0f;
    return ((u.x*v.y < u.y*v.x) ? -1.0f : 1.0f) * acosf(r);
}

}
