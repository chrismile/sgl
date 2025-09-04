/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#include <limits>
#include <algorithm>
#include <Utils/Convert.hpp>
#include "NumberFormatting.hpp"

namespace sgl {

/// Removes trailing zeros and unnecessary decimal points.
std::string removeTrailingZeros(const std::string &numberString) {
    size_t lastPos = numberString.size();
    for (int i = int(numberString.size()) - 1; i > 0; i--) {
        char c = numberString.at(i);
        if (c == '.') {
            lastPos--;
            break;
        }
        if (c != '0') {
            break;
        }
        lastPos--;
    }
    return numberString.substr(0, lastPos);
}

/// Removes decimal points if more than maxDigits digits are used.
std::string getNiceNumberString(float number, int digits) {
    int maxDigits = digits + 2; // Add 2 digits for '.' and one digit afterwards.
    std::string outString = removeTrailingZeros(sgl::toString(number, digits, true));

    // Can we remove digits after the decimal point?
    size_t dotPos = outString.find('.');
    if (int(outString.size()) > maxDigits && dotPos != std::string::npos) {
        size_t substrSize = dotPos;
        if (int(dotPos) < maxDigits - 1) {
            substrSize = maxDigits;
        }
        outString = outString.substr(0, substrSize);
    }

    // Still too large?
    if (int(outString.size()) > maxDigits || (outString == "0" && number > std::numeric_limits<float>::epsilon())) {
        outString = sgl::toString(number, std::max(digits - 2, 1), false, false, true);
    }
    return outString;
}

std::string getNiceMemoryString(uint64_t numBytes, int digits) {
    const char* UNIT_NAME_MAP[5] = {
            "B", "KiB", "MiB", "GiB", "TiB"
    };
    constexpr uint64_t oneMaxUnit = 1024ull * 1024ull * 1024ull * 1024ull;
    int unit = 4;
    uint64_t unitSize = oneMaxUnit;
    while (numBytes * 10 < unitSize && unit != 0) {
        unitSize /= 1024ull;
        unit--;
    }
    auto memoryInUnits = float(double(numBytes) / double(unitSize));
    return getNiceNumberString(memoryInUnits, digits) + UNIT_NAME_MAP[unit];
}

}
