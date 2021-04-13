/*!
 * Convert.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_UTILS_CONVERT_HPP_
#define SRC_UTILS_CONVERT_HPP_

#include <string>
#include <vector>
#include <sstream>

#ifdef USE_GLM
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#endif

namespace sgl {

//! Conversion to and from string
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

//! Conversion to and from string
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

//! Append vector2 to vector1
template<class T>
void appendVector(std::vector<T> &vector1, std::vector<T> &vector2) {
    vector1.insert(vector1.end(), vector2.begin(), vector2.end());
}

//! Special string conversion functions
std::string floatToString(float f, int decimalPrecision = -1);
uint32_t hexadecimalStringToUint32(const std::string &stringObject);
int fromHexString(const std::string &stringObject);
bool isInteger(const std::string &stringObject);
//! Integer or floating-point number?
bool isNumeric(const std::string &stringObject);
//! Respects whether string contains decimal or hexadecimal number
int stringToNumber(const char *str);

#ifdef USE_GLM

#if GLM_VERSION < 980
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
#endif

}

/*! SRC_UTILS_CONVERT_HPP_ */
#endif
