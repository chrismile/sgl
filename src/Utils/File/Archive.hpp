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

#ifndef SGL_ARCHIVE_HPP
#define SGL_ARCHIVE_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <cstdint> // needed by GCC 15

namespace sgl {

enum ArchiveFileLoadReturnType {
    // Everything worked successfully.
    ARCHIVE_FILE_LOAD_SUCCESSFUL,
    // The archive format is not supported (this is true, e.g., for .gz, .bz2, .xz, .lzma - use .tar.<name>!).
    ARCHIVE_FILE_LOAD_FORMAT_UNSUPPORTED,
    // The archive format couldn't be determined.
    ARCHIVE_FILE_LOAD_FORMAT_NOT_FOUND,
    // The archive couldn't be found.
    ARCHIVE_FILE_LOAD_ARCHIVE_NOT_FOUND,
    // The file within the archive couldn't be found.
    ARCHIVE_FILE_LOAD_FILE_NOT_FOUND,
    // The archive contains invalid data.
    ARCHIVE_FILE_LOAD_INVALID_ARCHIVE_DATA
};

/**
 * Loads a file from an archive using the library libarchive.
 * IMPORTANT: The user must use "delete[]" to free the memory of "buffer" if ARCHIVE_FILE_LOAD_SUCCESSFUL is returned.
 * @param filename The concatenated archive and local archive file filename.
 * @param buffer A reference to where the newly allocated buffer should be stored.
 * @param bufferSize The size of the buffer in bytes.
 * @param verbose Whether to output information when, e.g., loading the archive fails.
 * @return Whether reading was successful, or what type of error occured.
 *
 * Example usage:
 * uint8_t* buffer = nullptr;
 * size_t bufferSize = 0;
 * ArchiveFileLoadReturnType returnCode = loadFileFromArchive("archive.zip/file1.txt", buffer, bufferSize, true);
 * if (returnCode == ARCHIVE_FILE_LOAD_SUCCESSFUL) {
 *     // Do something with the pointer.
 *     delete[] buffer;
 *     buffer = nullptr;
 * }
 */
DLL_OBJECT ArchiveFileLoadReturnType loadFileFromArchive(
        const std::string& filename,
        uint8_t*& buffer, size_t& bufferSize, bool verbose);
DLL_OBJECT ArchiveFileLoadReturnType loadFileFromArchiveBuffer(
        const uint8_t* archiveBuffer, size_t archiveBufferSize, bool isRaw, const std::string& filenameLocal,
        uint8_t*& buffer, size_t& bufferSize, bool verbose);

struct ArchiveEntry {
    std::shared_ptr<uint8_t[]> bufferData;
    size_t bufferSize;
};

/**
 * Loads all files from an archive using the library libarchive.
 * @param filename The concatenated archive and local archive file filename.
 * @param buffer A map of the file names to their content.
 * @param verbose Whether to output information when, e.g., loading the archive fails.
 * @return Whether reading was successful, or what type of error occured.
 */
DLL_OBJECT ArchiveFileLoadReturnType loadAllFilesFromArchive(
        const std::string& filename,
        std::unordered_map<std::string, ArchiveEntry>& files, bool verbose);
DLL_OBJECT ArchiveFileLoadReturnType loadAllFilesFromArchiveBuffer(
        const uint8_t* archiveBuffer, size_t archiveBufferSize,
        std::unordered_map<std::string, ArchiveEntry>& files, bool verbose);

}

#endif //SGL_ARCHIVE_HPP
