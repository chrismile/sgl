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
#define __USE_FILE_OFFSET64

#include <cassert>

#ifdef USE_LIBARCHIVE
#include "Archive.hpp"
#endif
#include "Logfile.hpp"
#include "FileLoader.hpp"

namespace sgl {

bool loadFileFromSource(
        const std::string& filename, uint8_t*& buffer, size_t& bufferSize, bool isBinaryFile) {
    buffer = nullptr;
    bufferSize = 0;

#ifdef USE_LIBARCHIVE
    ArchiveFileLoadReturnType returnCode = loadFileFromArchive(filename, buffer, bufferSize, false);
    if (returnCode == ARCHIVE_FILE_LOAD_SUCCESSFUL) {
        return true;
    }
#endif

#ifdef __MINGW32__
    // Carriage return was not counted in text mode when using MinGW.
    isBinaryFile = true;
#endif
#if defined(__linux__) || defined(__MINGW32__) // __GNUC__? Does GCC generally work on non-POSIX systems?
    FILE* file = fopen64(filename.c_str(), "rb");
#else
    FILE* file = fopen(filename.c_str(), "rb");
#endif
    if (!file) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in loadFileFromSource: File \"" + filename + "\" could not be opened.");
        return false;
    }
#if defined(_WIN32) && !defined(__MINGW32__)
    _fseeki64(file, 0, SEEK_END);
    bufferSize = _ftelli64(file);
    _fseeki64(file, 0, SEEK_SET);
#else
    fseeko(file, 0, SEEK_END);
    bufferSize = ftello(file);
    fseeko(file, 0, SEEK_SET);
#endif

    /**
     * Read the whole file at once. It might be a good improvement to use memory-mapped files or buffered reading, so
     * files don't need to fit into memory at once.
     */
    buffer = new uint8_t[bufferSize];
    size_t readBytes = fread(buffer, 1, bufferSize, file);
    fclose(file);

    if (readBytes != bufferSize) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in loadFileFromSource: File \"" + filename + "\" could not be read.");
        delete[] buffer;
        buffer = nullptr;
        bufferSize = 0;
        return false;
    }

    return true;
}

bool loadFileFromSourceRanged(
        const std::string& filename, uint8_t*& buffer, size_t& bufferSize,
        size_t numBytesToRead, size_t& fileLength, bool isBinaryFile) {
    buffer = nullptr;
    bufferSize = 0;

#ifdef __MINGW32__
    // Carriage return was not counted in text mode when using MinGW.
    isBinaryFile = true;
#endif
#if defined(__linux__) || defined(__MINGW32__) // __GNUC__? Does GCC generally work on non-POSIX systems?
    FILE* file = fopen64(filename.c_str(), "rb");
#else
    FILE* file = fopen(filename.c_str(), "rb");
#endif
    if (!file) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in loadFileFromSource: File \"" + filename + "\" could not be opened.");
        return false;
    }
#if defined(_WIN32) && !defined(__MINGW32__)
    _fseeki64(file, 0, SEEK_END);
    fileLength = _ftelli64(file);
    _fseeki64(file, 0, SEEK_SET);
#else
    fseeko(file, 0, SEEK_END);
    fileLength = ftello(file);
    fseeko(file, 0, SEEK_SET);
#endif

    /**
     * Read the 'numBytesToRead' bytes.
     */
    buffer = new uint8_t[numBytesToRead];
    size_t readBytes = fread(buffer, 1, numBytesToRead, file);
    fclose(file);

    if (readBytes != numBytesToRead) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in loadFileFromSource: File \"" + filename + "\" could not be read.");
        delete[] buffer;
        buffer = nullptr;
        return false;
    }
    bufferSize = numBytesToRead;

    return true;
}

}
