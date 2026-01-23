/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#include <stdexcept>

#include "Format.hpp"

// Fallback for C++20 function std::format (or when the format string is not a constexpr).
std::string formatStringList(const std::string_view& formatString, std::initializer_list<std::string> argsList) {
    std::string outputString;
    auto argsListIt = argsList.begin();
    size_t formatStringIdx = 0;
    while (formatStringIdx < formatString.size()) {
        if (formatString[formatStringIdx] == '{') {
            if (formatStringIdx + 1 < formatString.size()) {
                if (formatString[formatStringIdx + 1] == '{') {
                    outputString += "{";
                    formatStringIdx += 2;
                    continue;
                }
                if (formatString[formatStringIdx + 1] == '}') {
                    // Add argument to output string.
                    if (argsListIt == argsList.end()) {
                        throw std::runtime_error("Insufficient number of arguments.");
                    }
                    outputString += *argsListIt;
                    argsListIt++;
                    formatStringIdx += 2;
                    continue;
                }
            }
            throw std::runtime_error("No closing bracket in format string.");
        }
        if (formatString[formatStringIdx] == '}') {
            if (formatStringIdx + 1 < formatString.size() && formatString[formatStringIdx + 1] == '}') {
                outputString += "}";
                formatStringIdx += 2;
                continue;
            }
            throw std::runtime_error("No opening bracket in format string.");
        }
        outputString += formatString[formatStringIdx];
        formatStringIdx += 1;
    }

    return outputString;
}
