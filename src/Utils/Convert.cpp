/*
 * Convert.cpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#include "Convert.hpp"
#include <boost/algorithm/string/predicate.hpp>

namespace sgl
{

uint32_t hexadecimalStringToUint32(const std::string &stringObject)
{
    std::stringstream strstr;
    strstr << std::hex << stringObject;
    uint32_t type;
    strstr >> type;
    return type;
}

int fromHexString(const std::string &stringObject)
{
    std::stringstream strstr;
    strstr << std::hex << stringObject;
    int type;
    strstr >> type;
    return type;
}

// Respects whether string contains decimal or hexadecimal number
inline int stringToNumber(const char *str) {
    if (boost::starts_with(str, "0x")) {
        return fromHexString(str);
    } else {
        return fromString<int>(str);
    }
}

bool isInteger(const std::string &stringObject)
{
    for (size_t i = 0; i < stringObject.size(); ++i) {
        if (stringObject.at(i) < '0' || stringObject.at(i) > '9') {
            return false;
        }
    }
    return true;
}

// Integer or floating-point number?
bool isNumeric(const std::string &stringObject)
{
    for (size_t i = 0; i < stringObject.size(); ++i) {
        if ((stringObject.at(i) < '0' || stringObject.at(i) > '9') && (stringObject.at(i) != '.')) {
            return false;
        }
    }
    return true;
}

}
