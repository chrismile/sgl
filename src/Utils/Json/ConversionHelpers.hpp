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

#ifndef CONVERSIONHELPERS_HPP
#define CONVERSIONHELPERS_HPP

#ifdef USE_GLM
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

#include "SimpleJson.hpp"

namespace sgl {

template<class T>
inline void getJsonOptional(const JsonValue& parentVal, const std::string& key, T& outValue) {
    if (parentVal.hasMember(key)) {
        parentVal[key].getTyped(outValue);
    }
}

#ifdef USE_GLM

inline void getJsonOptional(const JsonValue& parentVal, const std::string& key, glm::ivec2& outValue) {
    if (parentVal.hasMember(key)) {
        outValue[0] = parentVal[key][0].asInt32();
        outValue[1] = parentVal[key][1].asInt32();
    }
}

#if GLM_VERSION < 980
template <typename T, glm::precision P>
JsonValue glmVecToJsonValue(const glm::tvec2<T, P>& obj) {
    JsonValue val;
    for (int i = 0; i < 2; i++) {
        val[i] = obj[i];
    }
    return val;
}
template <typename T, glm::precision P>
JsonValue glmVecToJsonValue(const glm::tvec3<T, P>& obj) {
    JsonValue val;
    for (int i = 0; i < 3; i++) {
        val[i] = obj[i];
    }
    return val;
}
template <typename T, glm::precision P>
JsonValue glmVecToJsonValue(const glm::tvec4<T, P>& obj) {
    JsonValue val;
    for (int i = 0; i < 4; i++) {
        val[i] = obj[i];
    }
    return val;
}
#else
template <int L, typename T, glm::qualifier Q>
JsonValue glmVecToJsonValue(const glm::vec<L, T, Q>& obj) {
    JsonValue val;
    for (int i = 0; i < L; i++) {
        val[i] = obj[i];
    }
    return val;
}
#endif

#else

template <typename T>
JsonValue glmVecToJsonValue(const glm::tvec2<T>& obj) {
    JsonValue val;
    for (int i = 0; i < 2; i++) {
        val[i] = obj[i];
    }
    return val;
}
template <typename T>
JsonValue glmVecToJsonValue(const glm::tvec3<T>& obj) {
    JsonValue val;
    for (int i = 0; i < 3; i++) {
        val[i] = obj[i];
    }
    return val;
}
template <typename T>
JsonValue glmVecToJsonValue(const glm::tvec4<T>& obj) {
    JsonValue val;
    for (int i = 0; i < 4; i++) {
        val[i] = obj[i];
    }
    return val;
}

#endif

}

#endif //CONVERSIONHELPERS_HPP
