/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Christoph Neuhauser
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

#ifndef STRESSLINEVIS_LINEREADER_HPP
#define STRESSLINEVIS_LINEREADER_HPP

#include <vector>
#include <string>
#include <sstream>
#include <Utils/File/Logfile.hpp>
#include <Utils/Convert.hpp>

namespace sgl {

/**
 * For now, this reader uses no buffering and reads everything at once.
 * Performance-wise, this is better than std::ifstream, which causes an overhead, but using a medium-sized buffer might
 * be better for a future version.
 */
class DLL_OBJECT LineReader {
public:
    explicit LineReader(const std::string& filename);
    LineReader(const char* bufferData, size_t bufferSize);
    ~LineReader();

    inline bool isLineLeft() {
        if (!hasLineData) {
            fillLineBuffer();
        }
        return !lineBuffer.empty();
    }

    void fillLineBuffer();
    const std::string& readLine();

    template<typename T>
    T readScalarLine() {
        if (!hasLineData) {
            fillLineBuffer();
        }
        if (!isLineLeft()) {
            sgl::Logfile::get()->writeError("Error in LineReader::readVectorLine: No lines left.");
            return {};
        }
        hasLineData = false;

        T value = sgl::fromString<T>(lineBuffer);
        return value;
    }

    template<typename T>
    std::vector<T> readVectorLine() {
        if (!hasLineData) {
            fillLineBuffer();
        }
        if (!isLineLeft()) {
            sgl::Logfile::get()->writeError("Error in LineReader::readVectorLine: No lines left.");
            return {};
        }
        hasLineData = false;

        std::string tokenString;
        std::vector<T> vec;

        for (size_t linePtr = 0; linePtr < lineBuffer.size(); linePtr++) {
            char currentChar = lineBuffer.at(linePtr);
            bool isWhitespace = currentChar == ' ' || currentChar == '\t';
            if (isWhitespace && !tokenString.empty()) {
                vec.push_back(sgl::fromString<T>(tokenString.c_str()));
                tokenString.clear();
            } else if (!isWhitespace) {
                tokenString.push_back(currentChar);
            }
        }
        if (!tokenString.empty()) {
            vec.push_back(sgl::fromString<T>(tokenString.c_str()));
            tokenString.clear();
        }

        return vec;
    }

    template<typename T>
    std::vector<T> readVectorLine(size_t knownVectorSize) {
        if (!hasLineData) {
            fillLineBuffer();
        }
        if (!isLineLeft()) {
            sgl::Logfile::get()->writeError("Error in LineReader::readVectorLine: No lines left.");
            return {};
        }
        hasLineData = false;

        std::string tokenString;
        std::vector<T> vec;
        vec.reserve(knownVectorSize);

        for (size_t linePtr = 0; linePtr < lineBuffer.size(); linePtr++) {
            char currentChar = lineBuffer.at(linePtr);
            bool isWhitespace = currentChar == ' ' || currentChar == '\t';
            if (isWhitespace && !tokenString.empty()) {
                vec.push_back(sgl::fromString<T>(tokenString.c_str()));
                tokenString.clear();
            } else if (!isWhitespace) {
                tokenString.push_back(currentChar);
            }
        }
        if (!tokenString.empty()) {
            vec.push_back(sgl::fromString<T>(tokenString.c_str()));
            tokenString.clear();
        }

        if (vec.size() != knownVectorSize) {
            sgl::Logfile::get()->writeError(
                    "WARNING in LineReader::readVectorLine: Expected and real size don't match.");
        }

        return vec;
    }

    template<typename T>
    void readVectorLine(std::vector<T>& vec) {
        if (!hasLineData) {
            fillLineBuffer();
        }
        if (!isLineLeft()) {
            sgl::Logfile::get()->writeError("Error in LineReader::readVectorLine: No lines left.");
            return;
        }
        hasLineData = false;

        std::string tokenString;
        vec.clear();

        for (size_t linePtr = 0; linePtr < lineBuffer.size(); linePtr++) {
            char currentChar = lineBuffer.at(linePtr);
            bool isWhitespace = currentChar == ' ' || currentChar == '\t';
            if (isWhitespace && !tokenString.empty()) {
                vec.push_back(sgl::fromString<T>(tokenString.c_str()));
                tokenString.clear();
            } else if (!isWhitespace) {
                tokenString.push_back(currentChar);
            }
        }
        if (!tokenString.empty()) {
            vec.push_back(sgl::fromString<T>(tokenString.c_str()));
            tokenString.clear();
        }
    }

    template<typename... T>
    void readStructLine(T&... args) {
        if (!hasLineData) {
            fillLineBuffer();
        }
        if (!isLineLeft()) {
            sgl::Logfile::get()->writeError("Error in LineReader::readVectorLine: No lines left.");
            return;
        }
        hasLineData = false;

        std::string tokenString;
        size_t linePtr = 0;
        _readStructLine(tokenString, linePtr, args...);
    }

    // For binary file reading interop.
    template<typename T>
    T readBinaryValue() {
        if (hasLineData) {
            sgl::Logfile::get()->throwError(
                    "Error in LineReader::readBinaryValue: isLineLeft must not be called before readBinaryValue.");
        }
        if (bufferOffset + sizeof(T) > bufferSize) {
            sgl::Logfile::get()->throwErrorVar(
                    "Error in LineReader::readBinaryValue: Not enough space left for reading ", sizeof(T), " bytes.");
            return {};
        }
        const auto* typedPtr = reinterpret_cast<const T*>(bufferData + bufferOffset);
        bufferOffset += sizeof(T);
        return *typedPtr;
    }
    template<typename T>
    const T* getTypedPointerAndAdvance(size_t numEntries) {
        if (hasLineData) {
            sgl::Logfile::get()->throwError(
                    "Error in LineReader::readBinaryValue: isLineLeft must not be called before "
                    "getTypedPointerAndAdvance.");
        }
        const size_t numBytes = numEntries * sizeof(T);
        if (bufferOffset + numBytes > bufferSize) {
            sgl::Logfile::get()->throwErrorVar(
                    "Error in LineReader::readBinaryValue: Not enough space left for reading ", sizeof(T), " bytes.");
            return {};
        }
        const auto* typedPtr = reinterpret_cast<const T*>(bufferData + bufferOffset);
        bufferOffset += numBytes;
        return typedPtr;
    }

private:
    bool userManagedBuffer;
    const char* bufferData = nullptr;
    size_t bufferSize = 0;
    size_t bufferOffset = 0;

    // For buffering read lines.
    bool hasLineData = false;
    std::string lineBuffer;

    // Internal functions for @see readStructLine.
    template<typename T>
    void _tokenStringParse(std::string& tokenString, size_t& linePtr, T& t) {
        for (; linePtr < lineBuffer.size(); linePtr++) {
            char currentChar = lineBuffer.at(linePtr);
            bool isWhitespace = currentChar == ' ' || currentChar == '\t';
            if (isWhitespace && !tokenString.empty()) {
                t = sgl::fromString<T>(tokenString.c_str());
                tokenString.clear();
                return;
            } else if (!isWhitespace) {
                tokenString.push_back(currentChar);
            }
        }
        if (!tokenString.empty()) {
            t = sgl::fromString<T>(tokenString.c_str());
            tokenString.clear();
        }
    }
    template<typename T>
    void _readStructLine(std::string& tokenString, size_t& linePtr, T& t) {
        _tokenStringParse(tokenString, linePtr, t);
    }
    template<typename T, typename... Args>
    void _readStructLine(std::string& tokenString, size_t& linePtr, T& t, Args&... args) {
        _tokenStringParse(tokenString, linePtr, t);
        _readStructLine(tokenString, linePtr, args...);
    }
};

}

#endif //STRESSLINEVIS_LINEREADER_HPP
