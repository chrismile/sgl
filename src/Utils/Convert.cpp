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

// --- Specializations of fromString ---

template<>
glm::ivec2 fromString<glm::ivec2>(const std::string &stringObject) {
    std::stringstream strstr;
    strstr << stringObject;
    glm::ivec2 type;
    for (int i = 0; i < 2; i++) {
        strstr >> type[i];
    }
    return type;
}

}
