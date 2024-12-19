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

#define _FILE_OFFSET_BITS 64

#include <cstdio>

#include <Utils/File/Logfile.hpp>

#include "FileLoader.hpp"
#include "LineReader.hpp"

namespace sgl {

LineReader::LineReader(const std::string& filename)
        : userManagedBuffer(false), bufferData(nullptr), bufferSize(0) {
    uint8_t* fileBuffer = nullptr;
    bool loaded = loadFileFromSource(filename, fileBuffer, bufferSize, false);
    if (!loaded) {
        sgl::Logfile::get()->writeError("ERROR in LineReader::LineReader: Couldn't load file.");
        return;
    }

    bufferData = reinterpret_cast<const char*>(fileBuffer);
}

LineReader::LineReader(const char* bufferData, size_t bufferSize)
        : userManagedBuffer(true), bufferData(bufferData), bufferSize(bufferSize) {
}

LineReader::~LineReader() {
    if (!userManagedBuffer && bufferData) {
        delete[] bufferData;
    }
    bufferData = nullptr;
    bufferSize = 0;
}


void LineReader::fillLineBuffer() {
    lineBuffer.clear();
    while (bufferOffset < bufferSize) {
        lineBuffer.clear();

        while (bufferOffset < bufferSize) {
            char currentChar = bufferData[bufferOffset];
            if (currentChar == '\n' || currentChar == '\r') {
                bufferOffset++;
                break;
            }
            lineBuffer.push_back(currentChar);
            bufferOffset++;
        }

        if (lineBuffer.empty()) {
            continue;
        } else {
            break;
        }
    }
    hasLineData = true;
}

const std::string& LineReader::readLine() {
    if (!hasLineData) {
        fillLineBuffer();
    }
    if (!isLineLeft()) {
        sgl::Logfile::get()->writeError("ERROR in LineReader::readVectorLine: No lines left.");
    }
    hasLineData = false;

    return lineBuffer;
}

}
