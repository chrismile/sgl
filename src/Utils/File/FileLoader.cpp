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

    FILE* file = fopen64(filename.c_str(), isBinaryFile ? "rb" : "r");
    if (!file) {
        sgl::Logfile::get()->writeError(
                std::string() + "ERROR in loadFileFromSource: File \"" + filename + "\" could not be opened.");
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
                std::string() + "ERROR in loadFileFromSource: File \"" + filename + "\" could not be opened.");
        delete[] buffer;
        buffer = nullptr;
        bufferSize = 0;
        return false;
    }

    return true;
}

}
