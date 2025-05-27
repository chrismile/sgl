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

#ifndef SRC_UTILS_CONVERT_HPP_
#define SRC_UTILS_CONVERT_HPP_

#include <string>
#include <vector>
#include <sstream>
#include <cstdint>

#ifdef USE_GLM
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#else
#include <Math/Geometry/fallback/vec.hpp>
#endif

namespace sgl {

/// Conversion to and from string
template <class T>
std::string toString(T obj) {
    std::ostringstream ostr;
    ostr << obj;
    return ostr.str();
}
template <class T>
T fromString(const std::string &stringObject) {
    std::stringstream strstr;
    strstr << stringObject;
    T type;
    strstr >> type;
    return type;
}
inline std::string fromString(const std::string &stringObject) {
    return stringObject;
}

/// Conversion to and from string
template <class T>
std::string toString(
        T obj, int precision, bool fixed = true, bool noshowpoint = false, bool scientific = false) {
    std::ostringstream ostr;
    ostr.precision(precision);
    if (fixed) {
        ostr << std::fixed;
    }
    if (noshowpoint) {
        ostr << std::noshowpoint;
    }
    if (scientific) {
        ostr << std::scientific;
    }
    ostr << obj;
    return ostr.str();
}

template <class T>
std::string toHexString(T obj) {
    std::ostringstream ostr;
    ostr << std::hex << obj;
    return ostr.str();
}

/// Append vector2 to vector1
template<class T>
void appendVector(std::vector<T> &vector1, std::vector<T> &vector2) {
    vector1.insert(vector1.end(), vector2.begin(), vector2.end());
}

/// Special string conversion functions
DLL_OBJECT std::string floatToString(float f, int decimalPrecision = -1);
DLL_OBJECT uint32_t hexadecimalStringToUint32(const std::string &stringObject);
DLL_OBJECT int fromHexString(const std::string &stringObject);
DLL_OBJECT bool isInteger(const std::string &stringObject);
/// Integer or floating-point number?
DLL_OBJECT bool isNumeric(const std::string &stringObject);
/// Respects whether string contains decimal or hexadecimal number
DLL_OBJECT int stringToNumber(const char *str);
/// Converts e.g. 123456789 to "123,456,789"
DLL_OBJECT std::string numberToCommaString(int64_t number);

#ifndef USE_GLM
template <typename T>
std::string toString(const glm::tvec2<T>& obj) {
    std::ostringstream ostr;
    for (int i = 0; i < 2; i++) {
        ostr << obj[i];
        if (i != 1) {
            ostr << " ";
        }
    }
    return ostr.str();
}
template <typename T>
std::string toString(const glm::tvec3<T>& obj) {
    std::ostringstream ostr;
    for (int i = 0; i < 3; i++) {
        ostr << obj[i];
        if (i != 2) {
            ostr << " ";
        }
    }
    return ostr.str();
}
template <typename T>
std::string toString(const glm::tvec4<T>& obj) {
    std::ostringstream ostr;
    for (int i = 0; i < 4; i++) {
        ostr << obj[i];
        if (i != 3) {
            ostr << " ";
        }
    }
    return ostr.str();
}
#elif GLM_VERSION < 980
template <typename T, glm::precision P>
std::string toString(const glm::tvec2<T, P>& obj) {
    std::ostringstream ostr;
    for (int i = 0; i < 2; i++) {
        ostr << obj[i];
        if (i != 1) {
            ostr << " ";
        }
    }
    return ostr.str();
}
template <typename T, glm::precision P>
std::string toString(const glm::tvec3<T, P>& obj) {
    std::ostringstream ostr;
    for (int i = 0; i < 3; i++) {
        ostr << obj[i];
        if (i != 2) {
            ostr << " ";
        }
    }
    return ostr.str();
}
template <typename T, glm::precision P>
std::string toString(const glm::tvec4<T, P>& obj) {
    std::ostringstream ostr;
    for (int i = 0; i < 4; i++) {
        ostr << obj[i];
        if (i != 3) {
            ostr << " ";
        }
    }
    return ostr.str();
}
#else
template <int L, typename T, glm::qualifier Q>
std::string toString(const glm::vec<L, T, Q>& obj) {
    std::ostringstream ostr;
    for (int i = 0; i < L; i++) {
        ostr << obj[i];
        if (i != L - 1) {
            ostr << " ";
        }
    }
    return ostr.str();
}
// Partial specialization not allowed :(
/*template <int L, typename T, glm::qualifier Q>
glm::vec<L, T, Q> fromString(const std::string &stringObject) {
    std::stringstream strstr;
    strstr << stringObject;
    glm::vec<L, T, Q> type;
    for (int i = 0; i < L; i++) {
        strstr >> type[i];
    }
    return type;
}*/
#endif

template<>
glm::ivec2 fromString<glm::ivec2>(const std::string &stringObject);

}

/*! SRC_UTILS_CONVERT_HPP_ */
#endif
