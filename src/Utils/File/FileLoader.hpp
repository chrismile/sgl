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

#ifndef SGL_FILELOADER_HPP
#define SGL_FILELOADER_HPP

#include <string>

namespace sgl {

/**
 * Supports loading files from disk or from an archive (if built with libarchive support).
 * IMPORTANT: The user must use "delete[]" to free the memory of "buffer" if true is returned.
 * @param filename The concatenated archive and local archive file filename.
 * @param buffer A reference to where the newly allocated buffer should be stored.
 * @param bufferSize The size of the buffer in bytes.
 * @param isBinaryFile Whether the file is a binary file or a text file. This might be important e.g. for normalizing
 * line endings.
 * @return Whether reading was successful, or what type of error occurred.
 *
 * Example usage:
 * uint8_t* buffer = nullptr;
 * size_t bufferSize = 0;
 * bool loaded = loadFileFromSource("archive.zip/file1.txt", buffer, bufferSize, false);
 * if (loaded) {
 *     // Do something with the pointer.
 *     delete[] buffer;
 *     buffer = nullptr;
 * }
 */
DLL_OBJECT bool loadFileFromSource(
        const std::string& filename, uint8_t*& buffer, size_t& bufferSize, bool isBinaryFile);

}

#endif //SGL_FILELOADER_HPP
