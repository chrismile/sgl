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

namespace sgl {

//! Conversion to and from string
template <class T>
std::string toString(T obj) {
    std::ostringstream ostr;
    ostr << obj;
    return ostr.str();
}
template <class T>
T fromString (const std::string &stringObject) {
    std::stringstream strstr;
    strstr << stringObject;
    T type;
    strstr >> type;
    return type;
}
inline std::string fromString (const std::string &stringObject) {
    return stringObject;
}

//! Conversion to and from string
template <class T>
std::string toString(T obj, int precision, bool fixed = true) {
    std::ostringstream ostr;
    ostr.precision(precision);
    if (fixed) {
        ostr << std::fixed << obj;
    } else {
        ostr << std::noshowpoint << obj;
    }
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

}

/*! SRC_UTILS_CONVERT_HPP_ */
#endif
