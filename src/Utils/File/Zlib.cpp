/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Christoph Neuhauser
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

#include <zlib.h>

#include "Logfile.hpp"
#include "Zlib.hpp"

namespace sgl {

DLL_OBJECT bool decompressZlibData(
        const uint8_t* compressedBuffer, size_t compressedBufferSize,
        uint8_t* decompressedBuffer, size_t decompressedBufferSize) {
    auto sourceLen = uLongf(compressedBufferSize);
    auto destLen = uLongf(decompressedBufferSize);
    if (uncompress(decompressedBuffer, &destLen, compressedBuffer, sourceLen) != Z_OK) {
        sgl::Logfile::get()->writeError("Error in decompressZlibDataStream: uncompress failed.");
        return false;
    }
    if (size_t(destLen) != decompressedBufferSize) {
        sgl::Logfile::get()->writeError("Error in decompressZlibDataStream: Decompressed size mismatch.");
        return false;
    }
    return true;
}

}
