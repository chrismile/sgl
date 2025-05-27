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

#ifndef SGL_GEOMETRY_FALLBACK_FWD_HPP
#define SGL_GEOMETRY_FALLBACK_FWD_HPP

// Drop-in replacement for glm.

namespace glm {

template<typename T> class tvec2;
typedef tvec2<float> vec2;
typedef tvec2<double> dvec2;
typedef tvec2<int> ivec2;
typedef tvec2<unsigned int> uvec2;
typedef tvec2<bool> bvec2;

template<typename T> class tvec3;
typedef tvec3<float> vec3;
typedef tvec3<double> dvec3;
typedef tvec3<int> ivec3;
typedef tvec3<unsigned int> uvec3;
typedef tvec3<bool> bvec3;

template<typename T> class tvec4;
typedef tvec4<float> vec4;
typedef tvec4<double> dvec4;
typedef tvec4<int> ivec4;
typedef tvec4<unsigned int> uvec4;
typedef tvec4<bool> bvec4;

class mat3;
class mat4;
class quat;

}

#endif //SGL_GEOMETRY_FALLBACK_FWD_HPP
